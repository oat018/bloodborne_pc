// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "core/libraries/np/np_signaling/np_signaling_bbsc.h"

namespace Core::Bbsc::Party {

enum class ReturnChoice { OriginalPoint, SavedLamp, NativeInitial, Unavailable };
using PartyId = std::array<std::uint8_t, 16>;
using EventId = std::uint32_t;

using Kind = Libraries::Np::NpSignaling::BbscPartyKind;
struct Message {
    PartyId party{};
    std::uint64_t admission{}, sequence{};
    Kind kind{Kind::Hello};
    std::uint16_t chunk{}, chunks{1};
    std::vector<EventId> events;
    bool operator==(const Message&) const = default;
};
std::optional<Message> DecodeMessage(std::span<const std::uint8_t> bytes);
std::vector<std::uint8_t> EncodeMessage(const Message& message);

using CapabilityKind = Libraries::Np::NpSignaling::BbscCapabilityKind;
struct CapabilityMessage {
    CapabilityKind kind{CapabilityKind::Probe};
    std::uint64_t room{};
    std::array<std::uint8_t, 16> challenge{};
    std::array<std::uint8_t, 32> own_proof{};
    std::array<std::uint8_t, 32> selected_proof{};
    bool operator==(const CapabilityMessage&) const = default;
};
std::optional<CapabilityMessage> DecodeCapability(std::span<const std::uint8_t> bytes);
std::vector<std::uint8_t> EncodeCapability(const CapabilityMessage& message);

struct Peer {
    std::string online_id;
    std::int32_t connection{};
    bool operator==(const Peer&) const = default;
};

bool ValidPeer(const Peer& peer);
struct Journal;
class CharacterLease;

// Game-thread receiver; event Ack is emitted by the caller only after StoreJournal succeeds.
class Admission {
public:
    void Begin(const Peer& host, PartyId party, std::uint64_t admission,
               std::uint64_t boundary, std::span<const EventId> baseline);
    bool Accept(const Peer& from, const Message& message);
    std::span<const EventId> Earned() const { return earned_; }
    bool Active() const { return admitted_ && !closed_; }
    std::uint64_t LastSequence() const { return last_sequence_; }
    void Close();

private:
    friend std::optional<Message> PersistEarnedEvents(const CharacterLease&, Journal&,
                                                       const Admission&, std::error_code&);
    void Drain();
    void Earn(std::span<const EventId> events);
    void CloseAt(std::uint64_t sequence);
    Peer host_;
    PartyId party_{};
    std::uint64_t admission_{}, boundary_{}, last_sequence_{}, close_sequence_{};
    bool started_{}, hello_{}, ready_{}, baseline_complete_{}, admitted_{}, closed_{};
    std::vector<EventId> baseline_, earned_;
    std::vector<std::optional<std::vector<EventId>>> chunks_;
    std::map<std::uint64_t, std::vector<EventId>> pending_;
    std::size_t baseline_count_{}, received_events_{};
};

class Delivery {
public:
    bool Begin(const Peer& guest, PartyId party, std::uint64_t admission,
               std::uint64_t boundary, std::span<const EventId> baseline);
    bool Push(std::uint64_t sequence, EventId event);
    bool AcceptAck(const Peer& from, const Message& ack);
    std::vector<Message> Pending() const;
    std::uint64_t LastSequence() const { return last_sequence_; }
    void Close();

private:
    Peer guest_;
    Message identity_;
    bool open_{}, acknowledged_{};
    std::uint64_t last_sequence_{}, last_ack_{};
    std::vector<EventId> seen_;
    std::vector<Message> baseline_;
    std::map<std::uint64_t, Message> pending_;
};

struct CharacterKey {
    std::string save_dir, user;
    std::uint8_t slot{};
    PartyId character{};
    // Native NG uses 0..6. 0x80000000 | ordinal is a separate managed namespace for
    // saturated native count 7; it never aliases native NG0.
    std::uint32_t cycle{};
    bool operator==(const CharacterKey&) const = default;
};

enum class ManagedCycleStage : std::uint8_t { Stable, Intent, Applied, TransitionSerialized, SaveSerialized };
struct ManagedCycleEpoch {
    CharacterKey key;
    std::uint64_t epoch{};
    std::uint32_t target_key_cycle{};
    std::uint8_t native_cycle{7};
    ManagedCycleStage stage{ManagedCycleStage::Stable};
    std::uint64_t baseline_generation{};
    std::uint64_t serialized_generation{};
    std::string baseline_sha256;
    std::string serialized_sha256;
    bool operator==(const ManagedCycleEpoch&) const = default;
};

struct Destination {
    std::uint32_t map{};
    float x{}, y{}, z{}, heading{};
    std::int32_t lamp{-1};
    bool operator==(const Destination&) const = default;
};

enum class Phase : std::uint8_t { Prepared, Visiting, Returning, Complete };
struct EarnedClaim {
    EventId event{};
    PartyId party{};
    std::uint64_t admission{};
    bool committed{};
    std::uint32_t cycle{};
    bool operator==(const EarnedClaim&) const = default;
};
struct SnapshotRef {
    std::uint32_t bytes{};
    std::uint32_t crc32{};
    bool operator==(const SnapshotRef&) const = default;
};
struct ReturnSaveProof {
    CharacterKey persisted_key;
    std::string_view receipt_path, receipt_sha256, persisted_sha256, serialize_end_sha256;
    std::uint64_t generation{}, bytes{};
    bool serialization_began_after_restore{}, destination_confirmed{};
};
class ReturnSaveTracker {
public:
    void Arm(const CharacterKey& key, std::uint64_t baseline_generation,
             const Destination& expected_destination, bool native_return_selected);
    bool SerializeBegin(const CharacterKey& key, std::uint8_t slot, std::uintptr_t buffer,
                       std::uint64_t current_generation);
    bool SerializeEnd(const CharacterKey& live_key, const CharacterKey& serialized_key,
                      std::uint8_t slot, std::uintptr_t buffer, bool success,
                      std::string_view sha256);
    void AbortSerialization();
    bool ObserveReceipt(const CharacterKey& persisted_key, std::string_view receipt_path,
                       std::uint64_t generation, std::uint64_t bytes,
                       std::string_view receipt_sha256, std::string_view persisted_sha256);
    bool ConfirmDestination(const CharacterKey& key, const Destination& observed,
                           const Destination& resolved_native_point,
                           bool own_world, bool player_ready);
    std::optional<Destination> ExpectedDestination() const;
    std::optional<ReturnSaveProof> ProofFor(const CharacterKey& persisted_key,
                                            std::string_view persisted_sha256) const;
    void Disarm();
    bool Armed() const { return armed_; }
    const CharacterKey& Key() const { return key_; }
    bool Matches(const CharacterKey& key) const { return armed_ && key_ == key; }
    bool Active() const { return active_; }
    bool Saved() const { return saved_; }
    bool DestinationConfirmed() const { return destination_confirmed_; }

private:
    CharacterKey key_;
    Destination expected_destination_;
    std::uint64_t baseline_generation_{};
    std::uint64_t serialization_generation_{};
    std::uintptr_t buffer_{};
    std::string serialized_sha256_;
    std::string receipt_path_, receipt_sha256_;
    std::uint64_t receipt_generation_{}, receipt_bytes_{};
    bool armed_{}, active_{}, saved_{}, destination_confirmed_{}, native_return_selected_{};
};
struct Journal {
    CharacterKey key;
    PartyId party{};
    std::uint64_t admission{};
    Phase phase{Phase::Prepared};
    Destination origin, fallback;
    std::uint64_t save_generation{};
    SnapshotRef world_snapshot;
    bool native_return_selected{};
    std::vector<EventId> pending, committed;
    std::vector<EarnedClaim> retained;
    bool operator==(const Journal&) const = default;
};

// Carries unclaimed rewards with their original admission into the next party.
std::optional<Journal> PrepareNextParty(const Journal& previous, PartyId party,
                                        Destination origin, Destination fallback);
std::optional<Journal> MigrateCompletedCycleHome(const Journal& previous,
                                                  const CharacterKey& new_key,
                                                  PartyId party, Destination origin,
                                                  Destination fallback);

class CharacterLease {
public:
    static std::optional<CharacterLease> Acquire(const std::filesystem::path& path,
                                                std::error_code& ec);
    ~CharacterLease();
    CharacterLease(CharacterLease&& other) noexcept;
    CharacterLease& operator=(CharacterLease&& other) noexcept;
    CharacterLease(const CharacterLease&) = delete;
    CharacterLease& operator=(const CharacterLease&) = delete;

private:
    CharacterLease(std::filesystem::path path, std::intptr_t fd);
    std::filesystem::path path_;
    std::intptr_t fd_{-1};
    friend bool StoreJournal(const CharacterLease&, const Journal&, std::error_code&);
    friend bool StoreWorldSnapshot(const CharacterLease&, const CharacterKey&, const PartyId&,
                                   std::span<const std::uint8_t>, SnapshotRef&, std::error_code&);
    friend bool StoreSelectedSaveBackup(const CharacterLease&, const CharacterKey&, std::string_view,
                                        std::uint64_t, std::uint64_t, std::uint64_t,
                                        std::string_view, std::span<const std::uint8_t>,
                                        std::error_code&);
    friend bool StoreManagedCycleEpoch(const CharacterLease&, const ManagedCycleEpoch&,
                                       std::error_code&);
};

// One game thread owns updates; the lease excludes other processes, not other threads.
bool StoreJournal(const CharacterLease& lease, const Journal& record, std::error_code& ec);
bool StoreSelectedSaveBackup(const CharacterLease& lease, const CharacterKey& key,
                             std::string_view selected_path, std::uint64_t server_user,
                             std::uint64_t server_character, std::uint64_t generation,
                             std::string_view sha256, std::span<const std::uint8_t> bytes,
                             std::error_code& ec);
std::optional<Journal> LoadJournal(const std::filesystem::path& path, const CharacterKey& expected,
                                  std::error_code& ec);
std::filesystem::path ManagedCycleEpochPath(const std::filesystem::path& journal);
// A tagged epoch may proceed only if every older same-character home is absent or
// durably complete. Corrupt/unreadable matching history fails closed with ec set.
bool HasUnresolvedPriorCycleHome(const std::filesystem::path& journal,
                                 const CharacterKey& expected, std::error_code& ec);
std::optional<ManagedCycleEpoch> LoadManagedCycleEpoch(
    const std::filesystem::path& journal, const CharacterKey& expected, std::error_code& ec);
bool StoreManagedCycleEpoch(const CharacterLease& lease, const ManagedCycleEpoch& record,
                            std::error_code& ec);
std::optional<ManagedCycleEpoch> BeginManagedCycleIntent(
    const ManagedCycleEpoch& stable, std::uint8_t previous_native_cycle,
    std::uint64_t generation, std::string_view preincrement_sha256);
std::optional<ManagedCycleEpoch> ObserveManagedCycleApplied(
    const ManagedCycleEpoch& intent, std::uint8_t previous_native_cycle,
    std::uint8_t live_native_cycle);
std::optional<ManagedCycleEpoch> ObserveManagedCycleSerializeEnd(
    const ManagedCycleEpoch& applied, std::string_view serialized_sha256,
    std::uint64_t generation);
std::optional<ManagedCycleEpoch> RebaseManagedCycleForResume(
    const ManagedCycleEpoch& persisted, std::string_view selected_sha256,
    std::uint64_t current_process_generation);
std::optional<ManagedCycleEpoch> CommitManagedCycleEpoch(
    const ManagedCycleEpoch& serialized, std::uint64_t receipt_generation,
    std::string_view receipt_sha256, std::string_view persisted_sha256);
// The receipt generation is process-local: reset the fence before each restore attempt.
bool BeginReturnSaveFence(Journal& record, std::uint64_t current_generation);
bool ConfirmReturnedSave(Journal& record, const ReturnSaveProof& proof);
std::filesystem::path WorldSnapshotPath(const std::filesystem::path& journal, const PartyId& party);
bool StoreWorldSnapshot(const CharacterLease& lease, const CharacterKey& key, const PartyId& party,
                        std::span<const std::uint8_t> payload, SnapshotRef& ref,
                        std::error_code& ec);
std::optional<std::vector<std::uint8_t>> LoadWorldSnapshot(
    const std::filesystem::path& journal, const CharacterKey& expected, const PartyId& party,
    const SnapshotRef& ref, std::error_code& ec);

// Returns Ack only after the earned-event intent journal is durable, not after item delivery.
std::optional<Message> PersistEarnedEvents(const CharacterLease& lease, Journal& record,
                                         const Admission& admission, std::error_code& ec);

bool IsSupportedStoryMap(std::uint32_t map);
bool CanCrossMapMatch(std::uint32_t from, std::uint32_t to, std::string_view ours,
                      std::string_view theirs, bool cooperative);
ReturnChoice ChooseReturn(bool original_safe, bool saved_lamp_valid, bool native_initial_valid);

} // namespace Core::Bbsc::Party
