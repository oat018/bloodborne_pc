// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <tuple>

#include "core/net/p2p_transport.h"

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

namespace Core::Net::P2P {

struct DatagramBinding {
    struct Packet {
        std::vector<u8> payload;
        Endpoint from;
        u16 src_vport;
    };
    u16 vport = 0;
    std::shared_ptr<Readiness> readiness;
    std::optional<std::pair<Endpoint, u16>> peer; // set by SetPeer
    std::deque<Packet> queue;
    size_t bytes = 0;
    // bbport: packets from a peer this vport has not sent to yet, held for a moment (see
    // HandleDatagram), and the peers it has sent to.
    std::vector<std::pair<Clock::time_point, Packet>> held;
    std::vector<Endpoint> open_peers;
};

struct Connection {
    ConnectionKey key;
    Protection protection;
    std::unique_ptr<TcpConnection> tcp;
    std::shared_ptr<Readiness> readiness;
    std::weak_ptr<ListenerState> listener; // during handshake
    bool pending_accept = false;           // not in accept queue yet
    bool released = true;                  // no socket owns it
};

struct ListenerState {
    u16 vport = 0;
    int backlog = 1;
    Protection protection;
    std::shared_ptr<Readiness> readiness;
    std::deque<std::shared_ptr<Connection>> accept_queue;
    int half_open = 0;
    bool closed = false;
};

namespace {

// bbport: how long a datagram from a peer the vport has not talked to yet is held, and how long
// one for a vport the game has not opened yet is kept.
constexpr auto kHoldTime = std::chrono::milliseconds{1000};
constexpr auto kUnclaimedTime = std::chrono::milliseconds{2000};
constexpr auto kSeamlessTick = std::chrono::milliseconds{50};

auto Canonical(const Endpoint& e) {
    std::array<u8, 16> address{};
    u16 port = 0;
    u32 scope = 0;
    if (e.Family() == AF_INET) {
        const auto* in = reinterpret_cast<const sockaddr_in*>(&e.addr);
        std::memcpy(address.data(), &in->sin_addr, 4);
        port = in->sin_port;
    } else if (e.Family() == AF_INET6) {
        const auto* in6 = reinterpret_cast<const sockaddr_in6*>(&e.addr);
        std::memcpy(address.data(), &in6->sin6_addr, 16);
        port = in6->sin6_port;
        scope = in6->sin6_scope_id;
    }
    return std::tuple{e.Family(), address, port, scope};
}

} // namespace

Endpoint Endpoint::FromSockaddr(const sockaddr* sa, socklen_t len) {
    Endpoint e;
    std::memcpy(&e.addr, sa, std::min<size_t>(static_cast<size_t>(len), sizeof(e.addr)));
    return e;
}

Endpoint Endpoint::IPv4(const char* dotted, u16 port) {
    Endpoint e;
    auto* in = reinterpret_cast<sockaddr_in*>(&e.addr);
    in->sin_family = AF_INET;
    in->sin_port = htons(port);
    inet_pton(AF_INET, dotted, &in->sin_addr);
    return e;
}

socklen_t Endpoint::Length() const {
    return Family() == AF_INET6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
}

u16 Endpoint::Port() const {
    return ntohs(std::get<2>(Canonical(*this)));
}

bool Endpoint::operator<(const Endpoint& other) const {
    return Canonical(*this) < Canonical(other);
}

bool Endpoint::operator==(const Endpoint& other) const {
    return Canonical(*this) == Canonical(other);
}

bool ConnectionKey::operator<(const ConnectionKey& other) const {
    if (!(peer == other.peer)) {
        return peer < other.peer;
    }
    return std::tie(peer_vport, local_vport) < std::tie(other.peer_vport, other.local_vport);
}

void ReadinessFlag::Set(bool ready) {
    if (ready == ready_) {
        return;
    }
    ready_ = ready;
    if (ready) {
        wake_.Signal();
    } else {
        wake_.Drain();
    }
}

Transport::Transport(int family, std::unique_ptr<Codec> codec, const TransportConfig& config)
    : family_(family), codec_(std::move(codec)), config_(config),
      signaling_(std::max<size_t>(config.signaling_channels, 1)),
      iss_state_(static_cast<u64>(Clock::now().time_since_epoch().count()) ^
                 reinterpret_cast<uintptr_t>(this)),
      receive_buffer_(65536) {}

std::shared_ptr<Transport> Transport::Create(int family, const Endpoint& bind_address,
                                             std::unique_ptr<Codec> codec,
                                             const TransportConfig& config, Error* error) {
    std::shared_ptr<Transport> t(new Transport(family, std::move(codec), config));
    if (!t->kick_.Valid()) {
        *error = Error::NoBufs;
        return nullptr;
    }
    t->socket_ = Host::CreateSocket(family, SOCK_DGRAM, 0, error);
    if (t->socket_ == Host::InvalidSocket) {
        return nullptr;
    }
    // Exclusive port. Nobody else should get the port we advertise.
    if ((*error = Host::PrepareBind(t->socket_, false, false)) != Error::Ok ||
        (*error = Host::Bind(t->socket_, bind_address.Sockaddr(), bind_address.Length())) !=
            Error::Ok) {
        return nullptr;
    }
    const int buffer = 1 << 20; // absorb bursts
    setsockopt(t->socket_, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&buffer),
               sizeof(buffer));
    sockaddr_storage actual{};
    socklen_t actual_len = sizeof(actual);
    getsockname(t->socket_, reinterpret_cast<sockaddr*>(&actual), &actual_len);
    t->bound_port_ =
        Endpoint::FromSockaddr(reinterpret_cast<sockaddr*>(&actual), actual_len).Port();
    t->thread_ = std::thread([raw = t.get()] { raw->ThreadMain(); });
    *error = Error::Ok;
    return t;
}

Transport::~Transport() {
    stop_.store(true);
    kick_.Signal();
    if (thread_.joinable()) {
        thread_.join();
    }
    Host::CloseSocket(socket_); // thread is stopped by now
}

void Transport::SetOutgoingLossForTesting(double probability) {
    std::scoped_lock lock{mutex_};
    loss_for_testing_ = probability;
}

std::shared_ptr<DatagramSocket> Transport::CreateDatagram() {
    return std::shared_ptr<DatagramSocket>(new DatagramSocket(shared_from_this()));
}

std::shared_ptr<StreamSocket> Transport::CreateStream() {
    return std::shared_ptr<StreamSocket>(new StreamSocket(shared_from_this(), nullptr));
}

Error Transport::SendSignaling(std::span<const u8> data, const Endpoint& to) {
    std::scoped_lock lock{mutex_};
    return SendPacket(codec_->EncodeSignaling(data, to), to);
}

bool Transport::RecvSignaling(size_t channel, std::vector<u8>* data, Endpoint* from) {
    std::scoped_lock lock{mutex_};
    if (channel >= signaling_.size() || signaling_[channel].empty()) {
        return false;
    }
    auto& queue = signaling_[channel];
    *data = std::move(queue.front().first);
    *from = queue.front().second;
    queue.pop_front();
    return true;
}

void Transport::ThreadMain() {
    while (!stop_.load()) {
        std::optional<Clock::time_point> deadline;
        {
            std::scoped_lock lock{mutex_};
            deadline = EarliestDeadline();
            sleeping_until_ = deadline.value_or(Clock::time_point::max());
        }
        const auto result = Host::WaitOne(socket_, Host::Readable, kick_, deadline);
        if (stop_.load()) {
            return;
        }
        if (result == Host::WaitResult::Woken) {
            kick_.Drain();
        }

        std::scoped_lock lock{mutex_};
        sleeping_until_ = Clock::time_point::min();
        const auto now = Clock::now();
        for (int i = 0; i < 1024; ++i) { // don't starve timers
            sockaddr_storage from{};
            socklen_t from_len = sizeof(from);
            const auto r = Host::RecvFrom(socket_, receive_buffer_.data(), receive_buffer_.size(),
                                          0, reinterpret_cast<sockaddr*>(&from), &from_len);
            if (r.error != Error::Ok) {
                break;
            }
            HandlePacket({receive_buffer_.data(), static_cast<size_t>(r.value)},
                         Endpoint::FromSockaddr(reinterpret_cast<sockaddr*>(&from), from_len), now);
        }
        RunTimers(now);
    }
}

std::optional<Clock::time_point> Transport::EarliestDeadline() const {
    std::optional<Clock::time_point> earliest;
    for (const auto& [_, connection] : connections_) {
        const auto d = connection->tcp->NextDeadline();
        if (d && (!earliest || *d < *earliest)) {
            earliest = d;
        }
    }
    bool holding = !unclaimed_datagrams_.empty();
    for (const auto& [_, binding] : datagram_bindings_) {
        holding = holding || !binding->held.empty();
    }
    if (holding) {
        const auto tick = Clock::now() + kSeamlessTick;
        if (!earliest || tick < *earliest) {
            earliest = tick;
        }
    }
    return earliest;
}

void Transport::RunTimers(Clock::time_point now) {
    for (const auto& [_, binding] : datagram_bindings_) {
        if (!binding->held.empty()) {
            ReleaseHeldDatagrams(*binding, nullptr);
        }
    }
    std::erase_if(unclaimed_datagrams_, [now](const UnclaimedDatagram& datagram) {
        return now - datagram.arrived >= kUnclaimedTime;
    });

    std::vector<std::shared_ptr<Connection>> due;
    for (const auto& [_, connection] : connections_) {
        if (const auto d = connection->tcp->NextDeadline(); d && *d <= now) {
            due.push_back(connection);
        }
    }
    for (const auto& connection : due) {
        connection->tcp->OnTimer(now);
        Update(connection);
    }
}

void Transport::Reschedule(const Connection& connection) {
    if (const auto d = connection.tcp->NextDeadline(); d && *d < sleeping_until_) {
        sleeping_until_ = *d;
        kick_.Signal();
    }
}

Error Transport::SendPacket(std::span<const u8> packet, const Endpoint& to) {
    if (loss_for_testing_ > 0) {
        loss_rng_ ^= loss_rng_ << 13;
        loss_rng_ ^= loss_rng_ >> 7;
        loss_rng_ ^= loss_rng_ << 17;
        if (static_cast<double>(loss_rng_ >> 11) / static_cast<double>(1ull << 53) <
            loss_for_testing_) {
            return Error::Ok;
        }
    }
    const auto r =
        Host::SendTo(socket_, packet.data(), packet.size(), 0, to.Sockaddr(), to.Length());
    return r.error;
}

u32 Transport::NewIss() {
    iss_state_ += 0x9e3779b97f4a7c15ull;
    u64 z = iss_state_;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    z ^= z >> 31;
    const auto clock =
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now().time_since_epoch())
            .count() /
        4;
    return static_cast<u32>(z) + static_cast<u32>(clock);
}

u16 Transport::AllocateVport(bool stream) {
    const u16 first = stream ? 49152 : 32768;
    const u16 last = stream ? 65535 : 49999;
    u16& next = stream ? next_stream_port_ : next_datagram_vport_;
    for (int attempt = 0; attempt <= last - first; ++attempt) {
        const u16 vport = next;
        next = next == last ? first : static_cast<u16>(next + 1);
        const bool used =
            stream ? (stream_vports_in_use_.contains(vport) || listeners_.contains(vport))
                   : datagram_bindings_.contains(vport);
        if (!used) {
            return vport;
        }
    }
    return 0;
}

void Transport::HandlePacket(std::span<const u8> packet, const Endpoint& from,
                             Clock::time_point now) {
    Codec::Decoded decoded = codec_->Decode(packet, from);
    switch (decoded.kind) {
    case Codec::Kind::Stream:
        HandleStreamSegment(decoded.payload, from, now);
        break;
    case Codec::Kind::Datagram:
        HandleDatagram(decoded, from);
        break;
    case Codec::Kind::Signaling: {
        const size_t channel =
            config_.classify_signaling ? config_.classify_signaling(decoded.payload) : 0;
        if (channel < signaling_.size() &&
            signaling_[channel].size() < config_.signaling_queue_packets) {
            signaling_[channel].emplace_back(std::move(decoded.payload), from);
        }
        break;
    }
    case Codec::Kind::Invalid:
        break;
    }
}

void Transport::HandleDatagram(Codec::Decoded& decoded, const Endpoint& from) {
    if (decoded.dst_vport == SeamlessNoteVport) {
        NoteSeamlessPeer(from, decoded.payload);
        return;
    }
    if (decoded.dst_vport == SeamlessMessageVport) {
        if (seamless_messages_.size() >= 64) {
            seamless_messages_.pop_front();
        }
        seamless_messages_.push_back(std::move(decoded.payload));
        return;
    }
    const auto it = datagram_bindings_.find(decoded.dst_vport);
    if (it == datagram_bindings_.end()) {
        // bbport: the other game may already talk on a vport this game is about to open (both
        // open their session connection on the same event): kept for kUnclaimedTime.
        if (unclaimed_datagrams_.size() < 64) {
            unclaimed_datagrams_.push_back({Clock::now(), decoded.dst_vport,
                                            std::move(decoded.payload), from, decoded.src_vport});
        }
        return;
    }
    DatagramBinding& binding = *it->second;
    if (binding.peer &&
        !(binding.peer->first == from && binding.peer->second == decoded.src_vport)) {
        return; // not from the connected peer
    }
    NoteSeamlessPeer(from, {}); // a peer of the game
    // bbport: a packet from a peer this vport has not sent anything to yet waits until it has (or
    // for kHoldTime). Bloodborne's session connections (vports 30 and 40) are opened by both
    // games at once, each on its own signaling event; a game that received the other's opening
    // packet before it had sent its own never answered it, and the join ran into the room's
    // time-out. With this, every game sees its own opening first.
    if (std::ranges::find(binding.open_peers, from) == binding.open_peers.end()) {
        binding.held.push_back(
            {Clock::now(), {std::move(decoded.payload), from, decoded.src_vport}});
        return;
    }
    if (binding.bytes + decoded.payload.size() > config_.datagram_queue_bytes) {
        return; // queue full, drop like UDP
    }
    binding.bytes += decoded.payload.size();
    binding.queue.push_back({std::move(decoded.payload), from, decoded.src_vport});
    binding.readiness->readable.Set(true);
}

// peer == nullptr: the held packets that are kHoldTime old; otherwise those of that peer.
void Transport::ReleaseHeldDatagrams(DatagramBinding& binding, const Endpoint* peer) {
    const auto now = Clock::now();
    for (auto it = binding.held.begin(); it != binding.held.end();) {
        if (peer ? !(it->second.from == *peer) : now - it->first < kHoldTime) {
            ++it;
            continue;
        }
        if (std::ranges::find(binding.open_peers, it->second.from) == binding.open_peers.end()) {
            binding.open_peers.push_back(it->second.from);
        }
        if (binding.bytes + it->second.payload.size() <= config_.datagram_queue_bytes) {
            binding.bytes += it->second.payload.size();
            binding.queue.push_back(std::move(it->second));
            binding.readiness->readable.Set(true);
        }
        it = binding.held.erase(it);
    }
}

void Transport::NoteSeamlessPeer(const Endpoint& from, std::span<const u8> note) {
    const auto now = Clock::now();
    auto it = std::ranges::find_if(seamless_peers_,
                                   [&](const SeamlessPeer& peer) { return peer.endpoint == from; });
    if (it == seamless_peers_.end()) {
        if (seamless_peers_.size() >= 8) {
            seamless_peers_.erase(seamless_peers_.begin());
        }
        it = seamless_peers_.insert(seamless_peers_.end(), SeamlessPeer{from});
    }
    it->seen = now;
    if (!note.empty() && note.size() <= it->note.size()) {
        std::copy(note.begin(), note.end(), it->note.begin());
        it->note_size = static_cast<u32>(note.size());
        it->noted = now;
    }
}

int Transport::SeamlessSend(u16 vport, std::span<const u8> data) {
    std::scoped_lock lock{mutex_};
    const auto now = Clock::now();
    int count = 0;
    for (const auto& peer : seamless_peers_) {
        if (now - peer.seen < std::chrono::seconds{10}) {
            SendPacket(codec_->EncodeDatagram(vport, vport, data, peer.endpoint, {}),
                       peer.endpoint);
            ++count;
        }
    }
    return count;
}

int Transport::SeamlessCollectNotes(void* out, u32 entry_size, u32 max_entries) {
    std::scoped_lock lock{mutex_};
    const auto now = Clock::now();
    u32 count = 0;
    for (const auto& peer : seamless_peers_) {
        if (count < max_entries && peer.note_size == entry_size &&
            now - peer.noted < std::chrono::seconds{6}) {
            std::memcpy(static_cast<u8*>(out) + count * entry_size, peer.note.data(), entry_size);
            ++count;
        }
    }
    return static_cast<int>(count);
}

int Transport::SeamlessPopMessage(void* out, u32 len) {
    std::scoped_lock lock{mutex_};
    if (seamless_messages_.empty()) {
        return 0;
    }
    const auto& message = seamless_messages_.front();
    const u32 size = static_cast<u32>(std::min<size_t>(message.size(), len));
    std::memcpy(out, message.data(), size);
    seamless_messages_.pop_front();
    return static_cast<int>(size);
}

void Transport::HandleStreamSegment(std::span<const u8> bytes, const Endpoint& from,
                                    Clock::time_point now) {
    std::optional<PseudoHeader> pseudo;
    if (config_.verify_stream_checksum) {
        pseudo = codec_->StreamPseudoHeader(from);
    }
    const auto seg = ParseTcpSegment(bytes, pseudo ? &*pseudo : nullptr);
    if (!seg) {
        return;
    }
    const ConnectionKey key{from, seg->src_port, seg->dst_port};
    if (const auto it = connections_.find(key); it != connections_.end()) {
        const auto connection = it->second;
        if (connection->tcp->State() != TcpState::Closed) {
            connection->tcp->OnSegment(*seg, now);
            Update(connection);
            return;
        }
        if (connection->released) {
            connections_.erase(it);
        }
    }

    const bool bare_syn =
        (seg->flags & (TcpFlag::Syn | TcpFlag::Ack | TcpFlag::Rst)) == TcpFlag::Syn;
    if (bare_syn && !connections_.contains(key)) {
        const auto lit = listeners_.find(seg->dst_port);
        if (lit != listeners_.end()) {
            const auto listener = lit->second;
            if (static_cast<int>(listener->accept_queue.size()) + listener->half_open >=
                listener->backlog) {
                return; // backlog full, peer will retry
            }
            const auto connection =
                NewConnection(key, listener->protection, std::make_shared<Readiness>());
            connection->listener = listener;
            connection->pending_accept = true;
            ++listener->half_open;
            connection->tcp->AcceptSyn(*seg, now);
            Update(connection);
            return;
        }
    }
    // Nobody listening, RST so the peer fails fast.
    if (!pseudo) {
        pseudo = codec_->StreamPseudoHeader(from);
    }
    if (const auto rst = BuildResetFor(*pseudo, *seg)) {
        SendPacket(codec_->EncodeStream(*rst, from, {}), from);
    }
}

std::shared_ptr<Connection> Transport::NewConnection(const ConnectionKey& key,
                                                     Protection protection,
                                                     std::shared_ptr<Readiness> readiness) {
    auto connection = std::make_shared<Connection>();
    connection->key = key;
    connection->protection = protection;
    connection->readiness = std::move(readiness);
    const Endpoint peer = key.peer;
    connection->tcp = std::make_unique<TcpConnection>(
        config_.tcp, codec_->StreamPseudoHeader(peer), key.local_vport, key.peer_vport, NewIss(),
        [this, peer, protection](std::span<const u8> segment) {
            SendPacket(codec_->EncodeStream(segment, peer, protection), peer);
        });
    connections_[key] = connection;
    return connection;
}

void Transport::Update(const std::shared_ptr<Connection>& connection) {
    TcpConnection& tcp = *connection->tcp;
    if (connection->pending_accept) {
        const auto listener = connection->listener.lock();
        const TcpState state = tcp.State();
        if (!listener || listener->closed) {
            connection->pending_accept = false;
            tcp.Abort(); // listener is gone
        } else if (state != TcpState::SynReceived) {
            connection->pending_accept = false;
            --listener->half_open;
            connection->listener.reset();
            if (state != TcpState::Closed) {
                listener->accept_queue.push_back(connection);
                listener->readiness->readable.Set(true);
            }
        }
    }
    connection->readiness->readable.Set(tcp.Readable());
    connection->readiness->writable.Set(tcp.Writable());
    if (tcp.State() == TcpState::Closed && connection->released) {
        const auto it = connections_.find(connection->key);
        if (it != connections_.end() && it->second == connection) {
            connections_.erase(it);
        }
    }
}

DatagramSocket::DatagramSocket(std::shared_ptr<Transport> transport)
    : transport_(std::move(transport)), readiness_(std::make_shared<Readiness>()) {
    readiness_->writable.Set(true); // UDP is always writable
}

DatagramSocket::~DatagramSocket() {
    Close();
}

Error DatagramSocket::Bind(u16 vport) {
    std::scoped_lock lock{transport_->mutex_};
    return BindLocked(vport);
}

Error DatagramSocket::BindLocked(u16 vport) {
    if (binding_) {
        return Error::Inval;
    }
    if (vport == 0 && (vport = transport_->AllocateVport(false)) == 0) {
        return Error::AddrNotAvail;
    }
    if (transport_->datagram_bindings_.contains(vport)) {
        return Error::AddrInUse;
    }
    binding_ = std::make_shared<DatagramBinding>();
    binding_->vport = vport;
    binding_->readiness = readiness_;
    binding_->peer = peer_;
    transport_->datagram_bindings_[vport] = binding_;
    // bbport: packets that came for this vport shortly before the game opened it go through
    // HandleDatagram again, so the hold for a peer this vport has not talked to applies.
    auto& unclaimed = transport_->unclaimed_datagrams_;
    std::vector<Transport::UnclaimedDatagram> mine;
    for (auto it = unclaimed.begin(); it != unclaimed.end();) {
        if (it->dst_vport == vport && Clock::now() - it->arrived < kUnclaimedTime) {
            mine.push_back(std::move(*it));
            it = unclaimed.erase(it);
        } else {
            ++it;
        }
    }
    for (auto& datagram : mine) {
        Codec::Decoded decoded;
        decoded.kind = Codec::Kind::Datagram;
        decoded.src_vport = datagram.src_vport;
        decoded.dst_vport = datagram.dst_vport;
        decoded.payload = std::move(datagram.payload);
        transport_->HandleDatagram(decoded, datagram.from);
    }
    if (!mine.empty()) {
        transport_->kick_.Signal(); // the held ones need the transport thread's timer
    }
    return Error::Ok;
}

Error DatagramSocket::SetPeer(const Endpoint& peer, u16 vport) {
    std::scoped_lock lock{transport_->mutex_};
    if (vport == 0) {
        return Error::AddrNotAvail;
    }
    peer_ = std::pair{peer, vport};
    if (binding_) {
        binding_->peer = peer_;
    }
    return Error::Ok;
}

void DatagramSocket::SetProtection(Protection protection) {
    std::scoped_lock lock{transport_->mutex_};
    protection_ = protection;
}

IoResult DatagramSocket::SendTo(std::span<const u8> data, const Endpoint* to, u16 to_vport,
                                Protection extra) {
    std::scoped_lock lock{transport_->mutex_};
    if (!binding_ && BindLocked(0) != Error::Ok) { // implicit bind
        return {0, Error::AddrNotAvail};
    }
    if (to == nullptr) {
        if (!peer_) {
            return {0, Error::DestAddrReq};
        }
        to = &peer_->first;
        to_vport = peer_->second;
    }
    if (to_vport == 0) {
        return {0, Error::AddrNotAvail};
    }
    const Protection protection{protection_.crypto || extra.crypto,
                                protection_.signature || extra.signature};
    const auto packet =
        transport_->codec_->EncodeDatagram(binding_->vport, to_vport, data, *to, protection);
    const Error e = transport_->SendPacket(packet, *to);
    // bbport: what this peer sent before is handed over now, after this vport's own packet.
    if (std::ranges::find(binding_->open_peers, *to) == binding_->open_peers.end()) {
        binding_->open_peers.push_back(*to);
        transport_->ReleaseHeldDatagrams(*binding_, to);
    }
    if (e == Error::WouldBlock || e == Error::NoBufs) {
        // PS4 drops instead of returning ENOBUFS
        return {data.size(), Error::Ok};
    }
    return e == Error::Ok ? IoResult{data.size(), Error::Ok} : IoResult{0, e};
}

IoResult DatagramSocket::RecvFrom(std::span<u8> out, bool peek, Endpoint* from, u16* from_vport) {
    std::scoped_lock lock{transport_->mutex_};
    if (!binding_ || binding_->queue.empty()) {
        return {0, Error::WouldBlock};
    }
    auto& packet = binding_->queue.front();
    const size_t n = std::min(out.size(), packet.payload.size()); // rest is discarded
    std::copy_n(packet.payload.begin(), n, out.begin());
    if (from) {
        *from = packet.from;
    }
    if (from_vport) {
        *from_vport = packet.src_vport;
    }
    if (!peek) {
        binding_->bytes -= packet.payload.size();
        binding_->queue.pop_front();
        if (binding_->queue.empty()) {
            readiness_->readable.Set(false);
        }
    }
    return {n, Error::Ok};
}

u32 DatagramSocket::Events() const {
    std::scoped_lock lock{transport_->mutex_};
    return Host::EvOut | (binding_ && !binding_->queue.empty() ? static_cast<u32>(Host::EvIn) : 0u);
}

u16 DatagramSocket::BoundVport() const {
    std::scoped_lock lock{transport_->mutex_};
    return binding_ ? binding_->vport : 0;
}

u16 DatagramSocket::TransportPort() const {
    return transport_->BoundPort();
}

bool DatagramSocket::GetPeer(Endpoint* peer, u16* peer_vport) const {
    std::scoped_lock lock{transport_->mutex_};
    if (!peer_) {
        return false;
    }
    *peer = peer_->first;
    *peer_vport = peer_->second;
    return true;
}

Protection DatagramSocket::GetProtection() const {
    std::scoped_lock lock{transport_->mutex_};
    return protection_;
}

void DatagramSocket::Close() {
    std::scoped_lock lock{transport_->mutex_};
    if (!binding_) {
        return;
    }
    const auto it = transport_->datagram_bindings_.find(binding_->vport);
    if (it != transport_->datagram_bindings_.end() && it->second == binding_) {
        transport_->datagram_bindings_.erase(it);
    }
    binding_.reset();
}

StreamSocket::StreamSocket(std::shared_ptr<Transport> transport,
                           std::shared_ptr<Readiness> readiness)
    : transport_(std::move(transport)),
      readiness_(readiness ? std::move(readiness) : std::make_shared<Readiness>()) {}

StreamSocket::~StreamSocket() {
    Close();
}

void StreamSocket::SetProtection(Protection protection) {
    std::scoped_lock lock{transport_->mutex_};
    protection_ = protection;
}

Error StreamSocket::Bind(u16 vport) {
    std::scoped_lock lock{transport_->mutex_};
    return BindLocked(vport);
}

Error StreamSocket::BindLocked(u16 vport) {
    if (bound_vport_ != 0 || closed_) {
        return Error::Inval;
    }
    if (vport == 0 && (vport = transport_->AllocateVport(true)) == 0) {
        return Error::AddrNotAvail;
    }
    if (transport_->stream_vports_in_use_.contains(vport)) {
        return Error::AddrInUse;
    }
    ++transport_->stream_vports_in_use_[vport];
    bound_vport_ = vport;
    owns_vport_ = true;
    return Error::Ok;
}

Error StreamSocket::Listen(int backlog) {
    std::scoped_lock lock{transport_->mutex_};
    if (connection_ || closed_) {
        return Error::Inval;
    }
    if (bound_vport_ == 0 && BindLocked(0) != Error::Ok) {
        return Error::AddrNotAvail;
    }
    const int clamped = std::clamp(backlog, 1, transport_->config_.max_backlog);
    if (listener_) {
        listener_->backlog = clamped;
        return Error::Ok;
    }
    listener_ = std::make_shared<ListenerState>();
    listener_->vport = bound_vport_;
    listener_->backlog = clamped;
    listener_->protection = protection_;
    listener_->readiness = readiness_;
    transport_->listeners_[bound_vport_] = listener_;
    return Error::Ok;
}

Error StreamSocket::Connect(const Endpoint& peer, u16 peer_vport) {
    std::scoped_lock lock{transport_->mutex_};
    if (listener_ || closed_) {
        return Error::Inval;
    }
    if (connection_) {
        const TcpState state = connection_->tcp->State();
        return state == TcpState::SynSent || state == TcpState::SynReceived ? Error::Already
                                                                            : Error::IsConn;
    }
    if (peer_vport == 0) {
        return Error::AddrNotAvail;
    }
    if (bound_vport_ == 0 && BindLocked(0) != Error::Ok) {
        return Error::AddrNotAvail;
    }
    const ConnectionKey key{peer, peer_vport, bound_vport_};
    if (transport_->connections_.contains(key)) {
        return Error::AddrInUse; // e.g. old connection in TIME-WAIT
    }
    connection_ = transport_->NewConnection(key, protection_, readiness_);
    connection_->released = false;
    connection_->tcp->Connect(Clock::now());
    transport_->Update(connection_);
    transport_->Reschedule(*connection_);
    return Error::InProgress;
}

Error StreamSocket::ConnectResult() const {
    std::scoped_lock lock{transport_->mutex_};
    if (!connection_) {
        return Error::NotConn;
    }
    const TcpConnection& tcp = *connection_->tcp;
    if (tcp.PendingError() != Error::Ok) {
        return tcp.PendingError();
    }
    switch (tcp.State()) {
    case TcpState::SynSent:
    case TcpState::SynReceived:
        return Error::InProgress;
    case TcpState::Closed:
        return Error::NotConn;
    default:
        return Error::Ok;
    }
}

std::shared_ptr<StreamSocket> StreamSocket::Accept(Endpoint* peer, u16* peer_vport, Error* error) {
    std::scoped_lock lock{transport_->mutex_};
    if (!listener_) {
        *error = Error::Inval;
        return nullptr;
    }
    if (listener_->accept_queue.empty()) {
        *error = Error::WouldBlock;
        return nullptr;
    }
    auto connection = std::move(listener_->accept_queue.front());
    listener_->accept_queue.pop_front();
    if (listener_->accept_queue.empty()) {
        readiness_->readable.Set(false);
    }
    connection->released = false;
    std::shared_ptr<StreamSocket> accepted(new StreamSocket(transport_, connection->readiness));
    accepted->protection_ = connection->protection;
    accepted->bound_vport_ = connection->key.local_vport; // listener owns the vport
    accepted->connection_ = connection;
    if (peer) {
        *peer = connection->key.peer;
    }
    if (peer_vport) {
        *peer_vport = connection->key.peer_vport;
    }
    transport_->Update(connection);
    *error = Error::Ok;
    return accepted;
}

IoResult StreamSocket::Send(std::span<const u8> data) {
    std::scoped_lock lock{transport_->mutex_};
    if (!connection_) {
        return {0, Error::NotConn};
    }
    const auto r = connection_->tcp->Send(data, Clock::now());
    transport_->Update(connection_);
    transport_->Reschedule(*connection_);
    return {r.bytes, r.error};
}

IoResult StreamSocket::Recv(std::span<u8> out, bool peek) {
    std::scoped_lock lock{transport_->mutex_};
    if (!connection_) {
        return {0, Error::NotConn};
    }
    if (read_shutdown_) {
        return {0, Error::Ok};
    }
    const auto r = connection_->tcp->Recv(out, peek, Clock::now());
    transport_->Update(connection_);
    return {r.bytes, r.error};
}

Error StreamSocket::Shutdown(int how) {
    std::scoped_lock lock{transport_->mutex_};
    if (!connection_) {
        return Error::NotConn;
    }
    if (how < 0 || how > 2) {
        return Error::Inval;
    }
    if (how != 1) {
        read_shutdown_ = true;
        readiness_->readable.Set(true); // reads return EOF now
    }
    if (how != 0) {
        connection_->tcp->Shutdown(Clock::now());
        transport_->Update(connection_);
        transport_->Reschedule(*connection_);
    }
    return Error::Ok;
}

void StreamSocket::Close() {
    std::scoped_lock lock{transport_->mutex_};
    if (closed_) {
        return;
    }
    closed_ = true;
    if (listener_) {
        listener_->closed = true;
        const auto it = transport_->listeners_.find(listener_->vport);
        if (it != transport_->listeners_.end() && it->second == listener_) {
            transport_->listeners_.erase(it);
        }
        for (const auto& pending : listener_->accept_queue) {
            pending->tcp->Abort(); // reset unaccepted, like BSD
            transport_->Update(pending);
        }
        listener_->accept_queue.clear();
        listener_.reset();
    }
    if (connection_) {
        connection_->released = true;
        connection_->tcp->Close(Clock::now()); // FIN, or RST if data is unread
        transport_->Update(connection_);
        transport_->Reschedule(*connection_);
        connection_.reset();
    }
    if (owns_vport_) {
        auto& count = transport_->stream_vports_in_use_[bound_vport_];
        if (--count <= 0) {
            transport_->stream_vports_in_use_.erase(bound_vport_);
        }
        owns_vport_ = false;
    }
}

void StreamSocket::Abort() {
    std::scoped_lock lock{transport_->mutex_};
    if (connection_) {
        connection_->tcp->Abort();
        transport_->Update(connection_);
    }
}

u32 StreamSocket::Events() const {
    std::scoped_lock lock{transport_->mutex_};
    if (listener_) {
        return listener_->accept_queue.empty() ? 0u : static_cast<u32>(Host::EvIn);
    }
    if (!connection_) {
        return 0;
    }
    const TcpConnection& tcp = *connection_->tcp;
    u32 events = 0;
    if (tcp.Readable() || read_shutdown_) {
        events |= Host::EvIn;
    }
    if (tcp.Writable()) {
        events |= Host::EvOut;
    }
    if (tcp.PendingError() != Error::Ok) {
        events |= Host::EvErr;
    }
    if (tcp.State() == TcpState::Closed || tcp.State() == TcpState::TimeWait || tcp.ReceivedFin()) {
        events |= Host::EvHup; // TODO: peer FIN alone is RDHUP on Linux, not HUP
    }
    return events;
}

std::optional<TcpState> StreamSocket::State() const {
    std::scoped_lock lock{transport_->mutex_};
    return connection_ ? std::optional{connection_->tcp->State()} : std::nullopt;
}

bool StreamSocket::GetPeer(Endpoint* peer, u16* peer_vport) const {
    std::scoped_lock lock{transport_->mutex_};
    if (!connection_) {
        return false;
    }
    *peer = connection_->key.peer;
    *peer_vport = connection_->key.peer_vport;
    return true;
}

u16 StreamSocket::BoundVport() const {
    std::scoped_lock lock{transport_->mutex_};
    return bound_vport_;
}

u16 StreamSocket::TransportPort() const {
    return transport_->BoundPort();
}

Protection StreamSocket::GetProtection() const {
    std::scoped_lock lock{transport_->mutex_};
    return protection_;
}

} // namespace Core::Net::P2P
