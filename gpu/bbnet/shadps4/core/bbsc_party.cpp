// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bbsc_party.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <unordered_set>

namespace Core::Bbsc::Party {

namespace {
using namespace Libraries::Np::NpSignaling;


bool ValidMessage(const Message& message) {
    if (static_cast<unsigned>(message.kind) > static_cast<unsigned>(Kind::Leave) ||
        !message.admission ||
        !std::ranges::any_of(message.party, [](auto byte) { return byte != 0; }) ||
        message.events.size() > kBbscPartyEventsPerPacket || message.chunks == 0 ||
        message.chunks > kBbscPartyMaxChunks || message.chunk >= message.chunks) return false;
    if (message.kind == Kind::Baseline) {
        if (message.chunks > 1 && message.events.empty()) return false;
    } else {
        if (message.chunk || message.chunks != 1) return false;
        if (message.kind == Kind::Event) {
            if (!message.sequence || message.events.empty()) return false;
        } else if (!message.events.empty()) return false;
    }
    std::unordered_set<EventId> unique;
    for (auto event : message.events) {
        if (!event || !unique.insert(event).second) return false;
    }
    return true;
}
} // namespace

std::vector<std::uint8_t> EncodeMessage(const Message& message) {
    if (!ValidMessage(message)) return {};
    std::vector<std::uint8_t> bytes{'S', 'H', 'A', 'D', kSignalingBbscPartyType,
        kBbscPartyVersion, static_cast<std::uint8_t>(message.kind), 0};
    bytes.reserve(kBbscPartyHeaderBytes + message.events.size() * sizeof(EventId));
    bytes.insert(bytes.end(), message.party.begin(), message.party.end());
    const auto put = [&](std::uint64_t value, unsigned count) {
        for (unsigned i = 0; i < count; ++i)
            bytes.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
    };
    put(message.admission, 8);
    put(message.sequence, 8);
    put(message.chunk, 2);
    put(message.chunks, 2);
    put(message.events.size(), 2);
    for (auto event : message.events) put(event, 4);
    return bytes;
}

std::optional<Message> DecodeMessage(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < kBbscPartyHeaderBytes || bytes.size() > kBbscPartyMaxPacketBytes ||
        std::memcmp(bytes.data(), "SHAD", 4) || bytes[4] != kSignalingBbscPartyType ||
        bytes[5] != kBbscPartyVersion || bytes[7] != 0) return std::nullopt;
    Message message;
    message.kind = static_cast<Kind>(bytes[6]);
    std::copy_n(bytes.begin() + 8, message.party.size(), message.party.begin());
    std::size_t at = 24;
    const auto get = [&](unsigned count) {
        std::uint64_t value = 0;
        for (unsigned i = 0; i < count; ++i) value |= std::uint64_t{bytes[at++]} << (i * 8);
        return value;
    };
    message.admission = get(8);
    message.sequence = get(8);
    message.chunk = static_cast<std::uint16_t>(get(2));
    message.chunks = static_cast<std::uint16_t>(get(2));
    const auto count = get(2);
    if (count > kBbscPartyEventsPerPacket ||
        bytes.size() != kBbscPartyHeaderBytes + count * sizeof(EventId)) return std::nullopt;
    message.events.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
        message.events.push_back(static_cast<EventId>(get(4)));
    if (!ValidMessage(message)) return std::nullopt;
    return message;
}

std::vector<std::uint8_t> EncodeCapability(const CapabilityMessage& message) {
    const auto nonzero = [](const auto& bytes) {
        return std::ranges::any_of(bytes, [](auto byte) { return byte != 0; });
    };
    if (static_cast<unsigned>(message.kind) > static_cast<unsigned>(CapabilityKind::Confirm) ||
        !message.room || !nonzero(message.challenge) || !nonzero(message.own_proof)) return {};
    std::vector<std::uint8_t> bytes{'S', 'H', 'A', 'D',
        Libraries::Np::NpSignaling::kSignalingBbscCapabilityType,
        Libraries::Np::NpSignaling::kBbscCapabilityVersion,
        static_cast<std::uint8_t>(message.kind), 0};
    bytes.reserve(Libraries::Np::NpSignaling::kBbscCapabilityPacketBytes);
    for (unsigned i = 0; i < 8; ++i)
        bytes.push_back(static_cast<std::uint8_t>(message.room >> (i * 8)));
    bytes.insert(bytes.end(), message.challenge.begin(), message.challenge.end());
    bytes.insert(bytes.end(), message.own_proof.begin(), message.own_proof.end());
    bytes.insert(bytes.end(), message.selected_proof.begin(), message.selected_proof.end());
    return bytes;
}

std::optional<CapabilityMessage> DecodeCapability(std::span<const std::uint8_t> bytes) {
    using namespace Libraries::Np::NpSignaling;
    if (bytes.size() != kBbscCapabilityPacketBytes || std::memcmp(bytes.data(), "SHAD", 4) ||
        bytes[4] != kSignalingBbscCapabilityType || bytes[5] != kBbscCapabilityVersion ||
        bytes[6] > static_cast<std::uint8_t>(CapabilityKind::Confirm) || bytes[7] != 0)
        return std::nullopt;
    CapabilityMessage message;
    message.kind = static_cast<CapabilityKind>(bytes[6]);
    for (unsigned i = 0; i < 8; ++i)
        message.room |= std::uint64_t{bytes[8 + i]} << (i * 8);
    std::copy_n(bytes.begin() + 16, message.challenge.size(), message.challenge.begin());
    std::copy_n(bytes.begin() + 32, message.own_proof.size(), message.own_proof.begin());
    std::copy_n(bytes.begin() + 64, message.selected_proof.size(), message.selected_proof.begin());
    const auto nonzero = [](const auto& value) {
        return std::ranges::any_of(value, [](auto byte) { return byte != 0; });
    };
    if (!message.room || !nonzero(message.challenge) || !nonzero(message.own_proof))
        return std::nullopt;
    return message;
}


bool ValidPeer(const Peer& peer) {
    return !peer.online_id.empty() && peer.online_id.size() <= 16 && peer.connection > 0 &&
           peer.online_id.find('\0') == std::string::npos;
}

void Admission::Begin(const Peer& host, PartyId party, std::uint64_t admission,
                      std::uint64_t boundary, std::span<const EventId> baseline) {
    *this = Admission{};
    if (!ValidPeer(host) || !admission ||
        baseline.size() > kBbscPartyMaxCatalogEvents ||
        !std::ranges::any_of(party, [](auto byte) { return byte != 0; })) return;
    std::unordered_set<EventId> unique;
    for (auto event : baseline) if (!event || !unique.insert(event).second) return;
    host_ = host;
    party_ = party;
    admission_ = admission;
    boundary_ = last_sequence_ = boundary;
    baseline_.assign(baseline.begin(), baseline.end());
    started_ = true;
}

void Admission::Earn(std::span<const EventId> events) {
    for (auto event : events) {
        if (std::ranges::find(baseline_, event) == baseline_.end() &&
            std::ranges::find(earned_, event) == earned_.end()) earned_.push_back(event);
    }
}

void Admission::Drain() {
    if (!admitted_) return;
    while (last_sequence_ != std::numeric_limits<std::uint64_t>::max()) {
        const auto it = pending_.find(last_sequence_ + 1);
        if (it == pending_.end()) break;
        Earn(it->second);
        ++last_sequence_;
        pending_.erase(it);
    }
}

void Admission::CloseAt(std::uint64_t sequence) {
    if (closed_) return;
    closed_ = true;
    close_sequence_ = sequence;
    for (auto it = pending_.begin(); it != pending_.end();) {
        if (it->first > sequence) it = pending_.erase(it);
        else {
            if (admitted_) Earn(it->second);
            ++it;
        }
    }
}

void Admission::Close() {
    if (!started_) return;
    // Only sequences already observed before local departure can finish delivery afterward.
    CloseAt(pending_.empty() ? last_sequence_ : pending_.rbegin()->first);
}

bool Admission::Accept(const Peer& from, const Message& message) {
    if (!started_ || from != host_ || message.party != party_ ||
        message.admission != admission_ || !ValidMessage(message)) return false;
    if (message.kind == Kind::Event) {
        if (!ready_ || message.sequence <= boundary_ || message.sequence <= last_sequence_ ||
            message.sequence - boundary_ > kBbscPartyMaxCatalogEvents ||
            (closed_ && (!admitted_ || message.sequence > close_sequence_)) ||
            pending_.contains(message.sequence) ||
            received_events_ + message.events.size() > kBbscPartyMaxCatalogEvents) return false;
        pending_.emplace(message.sequence, message.events);
        received_events_ += message.events.size();
        if (closed_) Earn(message.events);
        Drain();
        return true;
    }
    if (closed_) return false;
    if (message.kind == Kind::Leave) {
        if (!ready_ || message.sequence < last_sequence_ || message.sequence < boundary_ ||
            message.sequence - boundary_ > kBbscPartyMaxCatalogEvents) return false;
        CloseAt(message.sequence);
        return true;
    }
    if (message.sequence != boundary_) return false;
    switch (message.kind) {
    case Kind::Hello:
        if (hello_) return false;
        hello_ = true;
        return true;
    case Kind::Ready:
        if (!hello_ || ready_) return false;
        ready_ = true;
        return true;
    case Kind::Baseline: {
        if (!ready_ || admitted_ ||
            (!chunks_.empty() && (chunks_.size() != message.chunks || chunks_[message.chunk])) ||
            baseline_count_ + message.events.size() > kBbscPartyMaxCatalogEvents) return false;
        auto baseline = baseline_;
        for (auto event : message.events) {
            for (const auto& chunk : chunks_) {
                if (chunk && std::ranges::find(*chunk, event) != chunk->end()) return false;
            }
            if (std::ranges::find(baseline, event) == baseline.end()) baseline.push_back(event);
        }
        if (baseline.size() > kBbscPartyMaxCatalogEvents) return false;
        if (chunks_.empty()) chunks_.resize(message.chunks);
        chunks_[message.chunk] = message.events;
        baseline_count_ += message.events.size();
        baseline_ = std::move(baseline);
        baseline_complete_ = std::ranges::all_of(chunks_, [](const auto& c) { return c.has_value(); });
        return true;
    }
    case Kind::Ack:
        if (!baseline_complete_ || admitted_) return false;
        admitted_ = true;
        Drain();
        return true;
    default:
        return false;
    }
}

bool Delivery::Begin(const Peer& guest, PartyId party, std::uint64_t admission,
                     std::uint64_t boundary, std::span<const EventId> baseline) {
    *this = Delivery{};
    identity_.party = party;
    identity_.admission = admission;
    identity_.sequence = boundary;
    if (!ValidPeer(guest) || !ValidMessage(identity_) ||
        baseline.size() > kBbscPartyMaxCatalogEvents) return false;
    std::unordered_set<EventId> unique;
    for (auto event : baseline) if (!event || !unique.insert(event).second) return false;
    guest_ = guest;
    seen_.assign(baseline.begin(), baseline.end());
    last_sequence_ = last_ack_ = boundary;
    for (Kind kind : {Kind::Hello, Kind::Ready}) {
        auto message = identity_; message.kind = kind; baseline_.push_back(std::move(message));
    }
    const auto count = std::max<std::size_t>(1, (baseline.size() + kBbscPartyEventsPerPacket - 1) /
                                                kBbscPartyEventsPerPacket);
    for (std::size_t chunk = 0; chunk < count; ++chunk) {
        auto message = identity_;
        message.kind = Kind::Baseline;
        message.chunk = static_cast<std::uint16_t>(chunk);
        message.chunks = static_cast<std::uint16_t>(count);
        const auto first = chunk * kBbscPartyEventsPerPacket;
        if (first < baseline.size()) {
            const auto n = std::min(kBbscPartyEventsPerPacket, baseline.size() - first);
            message.events.assign(baseline.begin() + first, baseline.begin() + first + n);
        }
        baseline_.push_back(std::move(message));
    }
    auto ack = identity_; ack.kind = Kind::Ack;
    baseline_.push_back(std::move(ack));
    open_ = true;
    return true;
}

bool Delivery::Push(std::uint64_t sequence, EventId event) {
    if (!open_ || !event || last_sequence_ == std::numeric_limits<std::uint64_t>::max() ||
        sequence != last_sequence_ + 1 || seen_.size() >= kBbscPartyMaxCatalogEvents ||
        std::ranges::find(seen_, event) != seen_.end()) return false;
    auto message = identity_;
    message.kind = Kind::Event;
    message.sequence = sequence;
    message.events = {event};
    pending_.emplace(sequence, std::move(message));
    seen_.push_back(event);
    last_sequence_ = sequence;
    return true;
}

bool Delivery::AcceptAck(const Peer& from, const Message& ack) {
    if (!open_ || from != guest_ || !ValidMessage(ack) || ack.kind != Kind::Ack ||
        ack.party != identity_.party || ack.admission != identity_.admission ||
        ack.sequence < identity_.sequence || ack.sequence > last_sequence_ ||
        (acknowledged_ && ack.sequence <= last_ack_)) return false;
    acknowledged_ = true;
    last_ack_ = ack.sequence;
    baseline_.clear();
    std::erase_if(pending_, [&](const auto& item) { return item.first <= ack.sequence; });
    return true;
}

std::vector<Message> Delivery::Pending() const {
    if (!open_) return {};
    auto messages = baseline_;
    for (const auto& [sequence, message] : pending_) messages.push_back(message);
    return messages;
}

void Delivery::Close() {
    open_ = false;
    baseline_.clear();
    pending_.clear();
}

bool IsSupportedStoryMap(std::uint32_t map) {
    // Exact map directories in the 1.09 dump; m29 instances are Chalice Dungeons.
    static constexpr std::array maps{
        0x15000000u, 0x15010000u, 0x16000000u, 0x17000000u,
        0x18000000u, 0x18010000u, 0x18020000u, 0x19000000u,
        0x1a000000u, 0x1b000000u, 0x1c000000u, 0x20000000u,
        0x21000000u, 0x22000000u, 0x23000000u, 0x24000000u,
    };
    return std::ranges::find(maps, map) != maps.end();
}

bool CanCrossMapMatch(std::uint32_t from, std::uint32_t to, std::string_view ours,
                      std::string_view theirs, bool cooperative) {
    return cooperative && from != to && IsSupportedStoryMap(from) && IsSupportedStoryMap(to) &&
           !ours.empty() && ours == theirs;
}

ReturnChoice ChooseReturn(bool original_safe, bool saved_lamp_valid, bool native_initial_valid) {
    if (original_safe) return ReturnChoice::OriginalPoint;
    if (saved_lamp_valid) return ReturnChoice::SavedLamp;
    if (native_initial_valid) return ReturnChoice::NativeInitial;
    return ReturnChoice::Unavailable;
}

} // namespace Core::Bbsc::Party
