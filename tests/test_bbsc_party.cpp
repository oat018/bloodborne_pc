// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bbsc_party.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <limits>
#include <string_view>

using namespace Core::Bbsc::Party;

static void TestPasswordAndMapEligibility() {
    constexpr std::uint32_t story = 0x18010000;
    constexpr std::uint32_t dream = 0x15000000;
    assert(CanCrossMapMatch(story, dream, "friends", "friends", true));
    assert(CanCrossMapMatch(dream, story, "friends", "friends", true));
    assert(!CanCrossMapMatch(story, dream, "", "", true));
    assert(!CanCrossMapMatch(story, dream, "friends", "", true));
    assert(!CanCrossMapMatch(story, dream, "", "friends", true));
    assert(!CanCrossMapMatch(story, dream, "friends", "Friends", true));
    assert(!CanCrossMapMatch(story, dream, "friends", "other", true));
    assert(!CanCrossMapMatch(story, dream, "friends", "friends", false));
    assert(!CanCrossMapMatch(story, story, "friends", "friends", true));
    for (const auto unsupported : {0u, 0xffffffffu, 0x1d000000u, 0x1d020400u,
                                   0x18030000u, 0x18010001u}) {
        assert(!IsSupportedStoryMap(unsupported));
        assert(!CanCrossMapMatch(story, unsupported, "friends", "friends", true));
        assert(!CanCrossMapMatch(unsupported, story, "friends", "friends", true));
    }
    for (const auto supported : {0x15000000u, 0x15010000u, 0x16000000u, 0x17000000u,
                                 0x18000000u, 0x18010000u, 0x18020000u, 0x19000000u,
                                 0x1a000000u, 0x1b000000u, 0x1c000000u, 0x20000000u,
                                 0x21000000u, 0x22000000u, 0x23000000u, 0x24000000u}) {
        assert(IsSupportedStoryMap(supported));
    }
    std::puts("PASS: policy");
}

static void TestReturnDestination() {
    assert(ChooseReturn(true, true, true) == ReturnChoice::OriginalPoint);
    assert(ChooseReturn(true, false, false) == ReturnChoice::OriginalPoint);
    assert(ChooseReturn(false, true, true) == ReturnChoice::SavedLamp);
    assert(ChooseReturn(false, false, true) == ReturnChoice::NativeInitial);
    assert(ChooseReturn(false, false, false) == ReturnChoice::Unavailable);
    std::puts("PASS: return");
}

static void TestReturnSaveFence() {
    CharacterKey key{"/tmp/bbsc-save/user/SPRJ0005", "4127", 1, {}, 2};
    key.character[0] = 0x34;
    key.character[8] = 0x56;
    Journal record;
    record.key = key;
    record.party[0] = 9;
    record.admission = 8;
    record.phase = Phase::Visiting;
    record.origin = {0x18000000, 1, 2, 3, 0, -1};
    record.fallback = {0x18000000, 4, 5, 6, 0, 2402950};
    record.pending = {101};
    record.retained.push_back({202, PartyId{1}, 7, false});
    const std::string hash(64, 'a');
    const auto proof = [&](const CharacterKey& persisted_key, std::string_view path,
                           std::uint64_t generation, std::string_view receipt_hash,
                           std::string_view disk_hash, std::string_view serialized_hash,
                           bool serialized_after_restore, bool at_destination) {
        return ReturnSaveProof{persisted_key, path, receipt_hash, disk_hash, serialized_hash,
                              generation, 0x140000, serialized_after_restore, at_destination};
    };

    BeginReturnSaveFence(record, 40);
    assert(record.phase == Phase::Returning && record.save_generation == 40);
    auto foreign = key;
    foreign.character[8]++;
    const std::string_view selected = "/tmp/bbsc-save/user/SPRJ0005/userdata0001";
    assert(!ConfirmReturnedSave(record, proof(foreign, selected, 41, hash, hash, hash, true, true)));
    assert(!ConfirmReturnedSave(record, proof(key, "/tmp/bbsc-save/user/SPRJ0005/userdata0002",
                                               41, hash, hash, hash, true, true)));
    assert(!ConfirmReturnedSave(record, proof(key, selected, 40, hash, hash, hash, true, true)));
    auto invalid = proof(key, selected, 41, hash, hash, hash, true, true);
    invalid.bytes = 0;
    assert(!ConfirmReturnedSave(record, invalid));
    assert(!ConfirmReturnedSave(record, proof(key, selected, 41, hash, std::string(64, 'b'),
                                               hash, true, true)));
    // A pre-restore serialization can be newer than the fence and still contain the host world.
    assert(!ConfirmReturnedSave(record, proof(key, selected, 41, hash, hash, hash, false, true)));
    assert(!ConfirmReturnedSave(record, proof(key, selected, 41, hash, hash, hash, true, false)));
    assert(record.phase == Phase::Returning && record.save_generation == 40);
    assert(ConfirmReturnedSave(record, proof(key, selected, 41, hash, hash, hash, true, true)));
    assert(record.phase == Phase::Complete && record.save_generation == 41);
    assert(record.pending == std::vector<EventId>{101} && record.retained.size() == 1);
    std::puts("PASS: return completion requires a newer selected-character save receipt and preserves earned claims");
}

static void TestReturnSaveTracker() {
    CharacterKey key{"/tmp/bbsc-save/user/SPRJ0005", "4127", 1, {}, 2};
    key.character[0] = 0x34;
    key.character[8] = 0x56;
    auto foreign = key;
    foreign.character[8]++;
    const std::string hash(64, 'a');
    const std::string other_hash(64, 'b');
    const std::string path = "/tmp/bbsc-save/user/SPRJ0005/userdata0001";
    const Destination destination{0x010001, 0, 0, 0, 0, 314};
    const Destination resolved{0x010001, 10, 20, 30, 0.5f, 314};
    ReturnSaveTracker tracker;
    assert(!tracker.SerializeBegin(key, 1, 0x1234, 20));
    tracker.Arm(key, 20, destination, false);
    assert(tracker.Matches(key) && !tracker.Matches(foreign));
    assert(!tracker.ConfirmDestination(key, resolved, resolved, true, true));
    assert(!tracker.ProofFor(key, hash)); // Restart recovery alone is not a selected native return.
    tracker.Arm(key, 20, destination, true);
    assert(!tracker.SerializeBegin(key, 1, 0x1234, 20)); // A pre-destination save cannot earn proof.
    assert(!tracker.ObserveReceipt(key, path, 21, 0x140000, hash, hash));
    auto wrong_point = resolved;
    wrong_point.lamp++;
    assert(!tracker.ConfirmDestination(key, resolved, wrong_point, true, true));
    auto wrong_map = resolved;
    wrong_map.map++;
    assert(!tracker.ConfirmDestination(key, resolved, wrong_map, true, true));
    auto wrong_position = resolved;
    wrong_position.x += 0.01f;
    assert(!tracker.ConfirmDestination(key, wrong_position, resolved, true, true));
    assert(!tracker.ConfirmDestination(foreign, resolved, resolved, true, true));
    assert(!tracker.ConfirmDestination(key, resolved, resolved, false, true));
    assert(!tracker.ConfirmDestination(key, resolved, resolved, true, false));
    assert(tracker.ConfirmDestination(key, resolved, resolved, true, true));
    assert(tracker.ExpectedDestination() == destination);
    assert(!tracker.SerializeBegin(foreign, 1, 0x1234, 20));
    assert(!tracker.SerializeBegin(key, 2, 0x1234, 20));
    assert(!tracker.SerializeBegin(key, 1, 0, 20));
    assert(!tracker.SerializeBegin(key, 1, 0x1234, 19));
    assert(tracker.SerializeBegin(key, 1, 0x1234, 20));
    assert(tracker.Active() && !tracker.Saved());
    assert(!tracker.SerializeEnd(foreign, foreign, 1, 0x1234, true, hash));
    assert(!tracker.Active());
    assert(!tracker.ObserveReceipt(key, path, 21, 0x140000, hash, hash));
    assert(tracker.SerializeBegin(key, 1, 0x5678, 20));
    assert(!tracker.SerializeEnd(key, key, 1, 0x1234, true, hash));
    assert(tracker.SerializeBegin(key, 1, 0x9abc, 20));
    assert(!tracker.SerializeEnd(key, key, 1, 0x9abc, false, hash));
    assert(tracker.SerializeBegin(key, 1, 0xdef0, 20));
    assert(!tracker.SerializeEnd(key, key, 1, 0xdef0, true, "not-a-sha256"));
    assert(tracker.SerializeBegin(key, 1, 0x1111, 20));
    assert(tracker.SerializeEnd(key, key, 1, 0x1111, true, hash));
    tracker.AbortSerialization();
    assert(!tracker.ObserveReceipt(key, path, 21, 0x140000, hash, hash));
    assert(tracker.SerializeBegin(key, 1, 0x1111, 20));
    assert(tracker.SerializeEnd(key, key, 1, 0x1111, true, hash));
    assert(!tracker.ObserveReceipt(foreign, path, 21, 0x140000, hash, hash));
    assert(!tracker.ObserveReceipt(key, "/tmp/bbsc-save/user/SPRJ0005/userdata0002",
                                   21, 0x140000, hash, hash));
    assert(!tracker.ObserveReceipt(key, path, 20, 0x140000, hash, hash));
    assert(!tracker.ObserveReceipt(key, path, 21, 0, hash, hash));
    assert(!tracker.ObserveReceipt(key, path, 21, 0x140000, other_hash, hash));
    assert(!tracker.ObserveReceipt(key, path, 21, 0x140000, hash, other_hash));
    assert(tracker.ObserveReceipt(key, path, 21, 0x140000, hash, hash));
    assert(tracker.Saved() && !tracker.Active());
    // A later serialization invalidates the earlier receipt match until its own receipt arrives.
    assert(tracker.SerializeBegin(key, 1, 0x2222, 21));
    assert(!tracker.Saved());
    assert(tracker.SerializeEnd(key, key, 1, 0x2222, true, other_hash));
    assert(!tracker.ObserveReceipt(key, path, 21, 0x140000, other_hash, other_hash));
    assert(tracker.ObserveReceipt(key, path, 22, 0x140000, other_hash, other_hash));
    auto proof = tracker.ProofFor(key, other_hash);
    assert(proof && proof->generation == 22 && proof->bytes == 0x140000 &&
           proof->destination_confirmed && proof->serialization_began_after_restore);
    assert(!tracker.ProofFor(foreign, other_hash));
    tracker.Disarm();
    assert(!tracker.Armed() && !tracker.Matches(key) && !tracker.ProofFor(key, other_hash));
    std::puts("PASS: return save provenance requires a matching post-arm serialize hash and selected-character receipt");
}

static void TestPartyMessageCodec() {
    Message event;
    event.party[0] = 0xA1;
    event.admission = 0x0102030405060708ULL;
    event.sequence = 0x1112131415161718ULL;
    event.kind = Kind::Event;
    event.events = {0x10203040};
    const std::vector<std::uint8_t> golden{
        0x53, 0x48, 0x41, 0x44, 0x28, 1, 3, 0,
        0xA1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        8, 7, 6, 5, 4, 3, 2, 1, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11,
        0, 0, 1, 0, 1, 0, 0x40, 0x30, 0x20, 0x10};
    assert(EncodeMessage(event) == golden);
    assert(DecodeMessage(golden) == event);
    for (std::size_t n = 0; n < golden.size(); ++n)
        assert(!DecodeMessage(std::span(golden).first(n)));
    for (const auto& [offset, value] : {std::pair{0u, 0u}, {4u, 0x22u}, {4u, 0x26u},
                                      {5u, 2u}, {6u, 6u}, {7u, 1u}, {40u, 1u},
                                      {42u, 0u}, {42u, 6u}, {44u, 255u}, {45u, 255u}}) {
        auto bad = golden; bad[offset] = static_cast<std::uint8_t>(value);
        assert(!DecodeMessage(bad));
    }
    for (const auto& [first, last] : {std::pair{8u, 24u}, {24u, 32u}}) {
        auto zero_identity = golden;
        std::fill(zero_identity.begin() + first, zero_identity.begin() + last, 0);
        assert(!DecodeMessage(zero_identity));
    }
    auto trailing = golden; trailing.push_back(0);
    assert(!DecodeMessage(trailing));
    assert(!DecodeMessage(std::vector<std::uint8_t>(1025)));
    auto invalid = event; invalid.party = {}; assert(EncodeMessage(invalid).empty());
    invalid = event; invalid.admission = 0; assert(EncodeMessage(invalid).empty());
    invalid = event; invalid.sequence = 0; assert(EncodeMessage(invalid).empty());
    invalid = event; invalid.events = {}; assert(EncodeMessage(invalid).empty());
    invalid = event; invalid.events = {0}; assert(EncodeMessage(invalid).empty());
    invalid = event; invalid.events = {7, 7}; assert(EncodeMessage(invalid).empty());
    invalid = event; invalid.kind = static_cast<Kind>(255); assert(EncodeMessage(invalid).empty());
    for (const Kind kind : {Kind::Hello, Kind::Ready, Kind::Ack, Kind::Leave}) {
        auto control = event; control.kind = kind; control.events.clear(); control.sequence = 0;
        assert(DecodeMessage(EncodeMessage(control)) == control);
        control.events = {1}; assert(EncodeMessage(control).empty());
    }
    auto baseline = event; baseline.kind = Kind::Baseline; baseline.sequence = 0;
    baseline.events.clear(); assert(DecodeMessage(EncodeMessage(baseline)) == baseline);
    baseline.events = {1, 2}; baseline.chunks = 5; baseline.chunk = 4;
    assert(DecodeMessage(EncodeMessage(baseline)) == baseline);
    baseline.chunk = 5; assert(EncodeMessage(baseline).empty());
    baseline.chunk = 0; baseline.chunks = 6; assert(EncodeMessage(baseline).empty());
    baseline.chunks = 2; baseline.events.clear(); assert(EncodeMessage(baseline).empty());
    auto maximum = event;
    maximum.admission = maximum.sequence = std::numeric_limits<std::uint64_t>::max();
    maximum.events = {std::numeric_limits<EventId>::max()};
    assert(DecodeMessage(EncodeMessage(maximum)) == maximum);
    auto full = event; full.events.clear();
    for (EventId id = 1; id <= 244; ++id) full.events.push_back(id);
    const auto encoded = EncodeMessage(full);
    assert(encoded.size() == 1022 && DecodeMessage(encoded) == full);
    full.events.push_back(245); assert(EncodeMessage(full).empty());
    auto zero = golden; std::fill(zero.begin() + 46, zero.end(), 0);
    assert(!DecodeMessage(zero));
    auto duplicate = golden;
    duplicate[44] = 2;
    duplicate.insert(duplicate.end(), golden.begin() + 46, golden.end());
    assert(!DecodeMessage(duplicate));
    std::puts("PASS: versioned party codec validates fixed bytes, lengths, kinds, chunks and event bounds");
}

static void TestCapabilityCodec() {
    CapabilityMessage message;
    message.kind = CapabilityKind::Confirm;
    message.room = 0x0102030405060708ULL;
    for (std::uint8_t i = 0; i < message.challenge.size(); ++i) message.challenge[i] = i + 1;
    for (std::uint8_t i = 0; i < message.own_proof.size(); ++i) message.own_proof[i] = 0x20 + i;
    const std::vector<std::uint8_t> golden{
        0x53,0x48,0x41,0x44,0x29,0x01,0x01,0x00,
        0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,
        0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    };
    assert(golden.size() == 96 && EncodeCapability(message) == golden);
    assert(DecodeCapability(golden) == message);
    for (std::size_t n = 0; n < golden.size(); ++n)
        assert(!DecodeCapability(std::span(golden).first(n)));
    auto oversized = golden; oversized.push_back(0);
    assert(!DecodeCapability(oversized));
    for (const auto& [offset, value] : {std::pair{0u, 0u}, {4u, 0x28u}, {5u, 2u},
                                      {6u, 2u}, {7u, 1u}}) {
        auto bad = golden; bad[offset] = static_cast<std::uint8_t>(value);
        assert(!DecodeCapability(bad));
    }
    for (const auto& [first, last] : {std::pair{8u, 16u}, {16u, 32u}, {32u, 64u}}) {
        auto zero_required = golden;
        std::fill(zero_required.begin() + first, zero_required.begin() + last, 0);
        assert(!DecodeCapability(zero_required));
    }
    auto invalid = message; invalid.room = 0; assert(EncodeCapability(invalid).empty());
    invalid = message; invalid.challenge = {}; assert(EncodeCapability(invalid).empty());
    invalid = message; invalid.own_proof = {}; assert(EncodeCapability(invalid).empty());
    invalid = message; invalid.kind = static_cast<CapabilityKind>(2);
    assert(EncodeCapability(invalid).empty());
    message.selected_proof[0] = 0xa5;
    assert(DecodeCapability(EncodeCapability(message)) == message);
    std::puts("PASS: capability codec uses strict fixed 96-byte v1 framing and optional selected proof");
}

static void TestOrderedAdmission() {
    const Peer host{"host", 7};
    PartyId party{}; party[0] = 1;
    const std::array<EventId, 1> old{10};
    Admission admission;
    admission.Begin(host, party, 11, 100, old);
    const auto packet = [&](Kind kind, std::uint64_t seq, std::vector<EventId> ids = {}) {
        Message m; m.party = party; m.admission = 11; m.sequence = seq;
        m.kind = kind; m.events = std::move(ids); return m;
    };
    assert(admission.Earned().empty() && !admission.Active());
    assert(!admission.Accept(host, packet(Kind::Event, 101, {20})));
    assert(admission.Accept(host, packet(Kind::Hello, 100)));
    assert(!admission.Accept(host, packet(Kind::Ack, 100)));
    assert(admission.Accept(host, packet(Kind::Ready, 100)));
    assert(admission.Accept(host, packet(Kind::Event, 102, {30})));
    assert(admission.Accept(host, packet(Kind::Event, 101, {10, 20})));
    assert(admission.Earned().empty());
    auto chunk = packet(Kind::Baseline, 100, {10}); chunk.chunks = 2;
    assert(admission.Accept(host, chunk));
    assert(!admission.Accept(host, chunk));
    assert(!admission.Accept(host, packet(Kind::Ack, 100)));
    chunk.chunk = 1; chunk.events = {15};
    assert(admission.Accept(host, chunk));
    assert(admission.Accept(host, packet(Kind::Ack, 100)) && admission.Active());
    assert((std::vector<EventId>(admission.Earned().begin(), admission.Earned().end()) ==
            std::vector<EventId>{20, 30}));
    assert(admission.LastSequence() == 102);
    assert(!admission.Accept(host, packet(Kind::Event, 101, {20})));
    assert(!admission.Accept({"other", 7}, packet(Kind::Event, 103, {40})));
    assert(!admission.Accept({"host", 8}, packet(Kind::Event, 103, {40})));
    auto stale = packet(Kind::Event, 103, {40}); stale.admission = 10;
    assert(!admission.Accept(host, stale));
    stale.admission = 11; stale.party[0] = 2; assert(!admission.Accept(host, stale));
    assert(admission.Accept(host, packet(Kind::Event, 104, {50})));
    admission.Close();
    assert(!admission.Active());
    assert(!admission.Accept(host, packet(Kind::Event, 105, {60})));
    // Earlier sequenced events can finish delivery after leave, without widening its boundary.
    assert(admission.Accept(host, packet(Kind::Event, 103, {40})));
    assert(admission.LastSequence() == 104 && admission.Earned().size() == 4);
    std::puts("PASS: ordered admission binds host/party, excludes baseline, retains reordered pre-leave events");
}

static void TestLateGuestBaseline() {
    const Peer host{"host", 7};
    PartyId party{}; party[0] = 1;
    std::array<Admission, 4> guests;
    for (std::size_t i = 0; i < guests.size(); ++i) {
        const std::uint64_t boundary = 100 + i;
        guests[i].Begin(host, party, i + 1, boundary, {});
        Message m; m.party = party; m.admission = i + 1; m.sequence = boundary;
        for (Kind kind : {Kind::Hello, Kind::Ready, Kind::Baseline, Kind::Ack}) {
            m.kind = kind; m.events = kind == Kind::Baseline ? std::vector<EventId>{10, 20}
                                                          : std::vector<EventId>{};
            assert(guests[i].Accept(host, m));
        }
        m.kind = Kind::Event; m.sequence = boundary + 1; m.events = {30};
        assert(guests[i].Accept(host, m));
        assert(guests[i].Earned().size() == 1 && guests[i].Earned()[0] == 30);
        m.admission += 10; assert(!guests[i].Accept(host, m));
    }
    Admission invalid;
    invalid.Begin({"", 0}, party, 0, 0, {});
    Message m; m.party = party; m.admission = 1;
    assert(!invalid.Accept(host, m));
    std::puts("PASS: four admissions keep independent late-guest baselines and reject invalid setup");
}

static void TestRetriedDelivery() {
    const Peer host{"host", 7}, guest{"guest", 8};
    PartyId party{}; party[0] = 1;
    const std::array<EventId, 1> old{10};
    Delivery delivery;
    assert(delivery.Begin(guest, party, 11, 100, old));
    Admission admission;
    admission.Begin(host, party, 11, 100, {});
    assert(delivery.Pending().size() == 4);
    for (const auto& message : delivery.Pending()) {
        const auto decoded = DecodeMessage(EncodeMessage(message));
        assert(decoded && admission.Accept(host, *decoded));
    }
    assert(admission.Active() && admission.Earned().empty());
    assert(delivery.Push(101, 20) && delivery.Push(102, 30));
    assert(!delivery.Push(103, 20) && !delivery.Push(104, 40));
    const auto retry = delivery.Pending();
    assert(delivery.Pending() == retry && retry.size() == 6);
    Message ack; ack.party = party; ack.admission = 11; ack.kind = Kind::Ack; ack.sequence = 100;
    assert(!delivery.AcceptAck(host, ack));
    assert(delivery.AcceptAck(guest, ack));
    assert(delivery.Pending().size() == 2);
    ack.sequence = 103; assert(!delivery.AcceptAck(guest, ack));
    ack.sequence = 101; assert(delivery.AcceptAck(guest, ack));
    assert(delivery.Pending().size() == 1 && delivery.Pending()[0].sequence == 102);
    assert(!delivery.AcceptAck(guest, ack));
    ack.admission = 12; ack.sequence = 102; assert(!delivery.AcceptAck(guest, ack));
    delivery.Close();
    assert(delivery.Pending().empty() && !delivery.Push(103, 40));
    const std::array<EventId, 2> duplicate{1, 1};
    assert(!delivery.Begin(guest, party, 11, 100, duplicate));
    std::puts("PASS: delivery retries baseline/events until peer-bound cumulative Ack without widening admission");
}

int main(int argc, char** argv) {
    void TestJournal();
    const std::string_view section = argc == 2 ? argv[1] : "all";
    if (section != "all" && section != "policy" && section != "return" && section != "journal" && section != "protocol") {
        std::fputs("Unknown test section\n", stderr);
        return 2;
    }
    if (section == "all" || section == "policy") {
        TestPasswordAndMapEligibility();
    }
    if (section == "all" || section == "return") {
        TestReturnDestination();
        TestReturnSaveFence();
        TestReturnSaveTracker();
    }
    if (section == "all" || section == "journal") TestJournal();
    if (section == "all" || section == "protocol") {
        TestPartyMessageCodec();
        TestCapabilityCodec();
        TestOrderedAdmission();
        TestLateGuestBaseline();
        TestRetriedDelivery();
    }
}
