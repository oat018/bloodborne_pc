// SPDX-License-Identifier: GPL-2.0-or-later
#include "../gpu/bbnet/shadps4/core/libraries/np/np_signaling/np_signaling_helpers.cpp"
#include <cassert>
#include <cstdio>

using namespace Libraries::Np::NpSignaling;
using namespace Core::Bbsc::Party;
static unsigned sent{}, received{};
static bool send_fail{};
static Peer delivered;
namespace Libraries::Np::NpSignaling {
std::unordered_map<s32, ConnectionInfo> g_connections;
SignalingMutexGuard::SignalingMutexGuard() {}
SignalingMutexGuard::~SignalingMutexGuard() {}
std::string OnlineIdToString(const OrbisNpOnlineId& id) {
    return std::string(id.data, strnlen(id.data, 16));
}
namespace Stubs {
int SignalingSendTo(const void*, u32 size, u32 address, u16 port) {
    assert(address == 123 && port == 456); ++sent;
    return send_fail ? -1 : static_cast<int>(size);
}
}
}
namespace Core::Bloodborne {
void OnBbscPartyPacket(const Peer& peer, const void* bytes, std::uint32_t size) {
    assert(DecodeMessage({static_cast<const u8*>(bytes), size}));
    delivered = peer; ++received;
}
}
int main() {
    Message message; message.party[0] = 1; message.admission = 3;
    const auto bytes = EncodeMessage(message);
    const Peer peer{"host", 7};
    auto& connection = g_connections[7];
    connection.addr = 123; connection.port = 456; connection.state = ConnState::Established;
    std::memcpy(connection.online_id.data, "host", 5);
    assert(Helpers::FindBbscPartyPeer("host") == peer);
    assert(!Helpers::FindBbscPartyPeer("missing"));
    assert(Helpers::SendBbscPartyToPeer(peer, bytes.data(), bytes.size()) == 1 && sent == 1);
    assert(!Helpers::SendBbscPartyToPeer({"other", 7}, bytes.data(), bytes.size()) && sent == 1);
    assert(!Helpers::SendBbscPartyToPeer({"host", 8}, bytes.data(), bytes.size()) && sent == 1);
    assert(!Helpers::SendBbscPartyToPeer(peer, nullptr, bytes.size()));
    assert(Helpers::ReceiveBbscPartyPacket(bytes.data(), bytes.size(), 123, 456));
    assert(received == 1 && delivered == peer);
    assert(!Helpers::ReceiveBbscPartyPacket(bytes.data(), bytes.size(), 123, 457));
    connection.state = ConnState::Inactive;
    assert(!Helpers::FindBbscPartyPeer("host"));
    assert(!Helpers::SendBbscPartyToPeer(peer, bytes.data(), bytes.size()) && sent == 1);
    assert(!Helpers::ReceiveBbscPartyPacket(bytes.data(), bytes.size(), 123, 456));
    connection.state = ConnState::Established;
    g_connections[8] = connection;
    assert(!Helpers::FindBbscPartyPeer("host"));
    assert(!Helpers::ReceiveBbscPartyPacket(bytes.data(), bytes.size(), 123, 456));
    g_connections.erase(8);
    auto bad = bytes; bad[4] = 0x22;
    assert(!Helpers::SendBbscPartyToPeer(peer, bad.data(), bad.size()));
    assert(!Helpers::ReceiveBbscPartyPacket(bad.data(), bad.size(), 123, 456));
    send_fail = true;
    assert(!Helpers::SendBbscPartyToPeer(peer, bytes.data(), bytes.size()));
    assert(sent == 2 && received == 1);
    std::puts("PASS: targeted party transport pins account/connection and rejects unknown, inactive or ambiguous sources");
}
