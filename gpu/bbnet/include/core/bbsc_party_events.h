// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/bbsc_party.h"

namespace Core::Bbsc::Party {

// This table is an allowlist of native EMEVD completion event IDs. Common event 50 explicitly
// maps the base-game subset 12101800..13301800 to global completion markers. It describes possible
// synchronization; it does not authorize applying a row without the runtime's admission,
// guest-progress, conflict, inventory, and durable-reward checks.
enum class PartyEventClass : std::uint8_t { Boss, Story, Key };
enum class PartyConflictPolicy : std::uint8_t { AdditiveOnly, DeferEndgameMutation };
enum class PartyUniqueItemPolicy : std::uint8_t { GrantWhenMissing, PreserveChoiceItemOnly };

struct PartyEventRule {
    EventId flag_id;
    std::uint32_t packed_map;
    PartyEventClass event_class;
    std::array<EventId, 2> prerequisites{};
    std::uint8_t prerequisite_count{};
    PartyConflictPolicy conflict_policy{PartyConflictPolicy::AdditiveOnly};
};

// Map values are packed areas accepted by IsSupportedStoryMap in bbsc_party.cpp. 12700710 is a non-boss
// action marker; 13501802/13601803 are action/aftermath markers and are intentionally absent.
inline constexpr std::array<PartyEventRule, 22> kPartyBossCompletionCatalog{{
    {12101800, 0x15000000, PartyEventClass::Boss, {}, 0, PartyConflictPolicy::DeferEndgameMutation},
    {12101850, 0x15000000, PartyEventClass::Boss, {}, 0, PartyConflictPolicy::DeferEndgameMutation},
    {12201800, 0x16000000, PartyEventClass::Boss},
    {12301800, 0x17000000, PartyEventClass::Boss},
    {12301700, 0x17000000, PartyEventClass::Boss},
    {12401800, 0x18000000, PartyEventClass::Boss},
    {12411700, 0x18010000, PartyEventClass::Boss},
    {12411800, 0x18010000, PartyEventClass::Boss},
    {12421700, 0x18020000, PartyEventClass::Boss},
    {12421800, 0x18020000, PartyEventClass::Boss},
    {12501800, 0x19000000, PartyEventClass::Boss},
    {12601850, 0x1a000000, PartyEventClass::Boss},
    {12601800, 0x1a000000, PartyEventClass::Boss},
    {12701800, 0x1b000000, PartyEventClass::Boss},
    {12801800, 0x1c000000, PartyEventClass::Boss},
    {13201800, 0x20000000, PartyEventClass::Boss},
    {13301800, 0x21000000, PartyEventClass::Boss},
    {13401800, 0x22000000, PartyEventClass::Boss},
    {13401850, 0x22000000, PartyEventClass::Boss},
    {13501800, 0x23000000, PartyEventClass::Boss},
    {13501850, 0x23000000, PartyEventClass::Boss},
    {13601800, 0x24000000, PartyEventClass::Boss},
}};

inline constexpr EventId kNoItemAcquisitionFlag = 0;
struct PartyUniqueItemRule {
    EventId source_event_flag;
    EventId flag_id; // Native lot acquisition flag, or kNoItemAcquisitionFlag when none exists.
    std::uint32_t goods_id;
    std::uint8_t quantity;
    PartyUniqueItemPolicy unique_policy;
};

// Only unique story items found in the original lots are listed. Runtime still checks carried
// inventory, storage, native acquisition flags, the guest's state, and its durable event ledger.
// The Cord is an earned inventory item only: never consume it or mirror any ending/NG+ choice.
inline constexpr std::array<PartyUniqueItemRule, 3> kPartyUniqueItemCatalog{{
    {12411800, kNoItemAcquisitionFlag, 4000, 1, PartyUniqueItemPolicy::GrantWhenMissing},
    {13501850, 53501850, 4020, 1, PartyUniqueItemPolicy::GrantWhenMissing},
    {12601800, 50000305, 4323, 1, PartyUniqueItemPolicy::PreserveChoiceItemOnly},
}};

} // namespace Core::Bbsc::Party
