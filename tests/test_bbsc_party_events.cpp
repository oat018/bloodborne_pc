// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bbsc_party_events.h"

#include <algorithm>
#include <cassert>
#include <cstdio>

using namespace Core::Bbsc::Party;

static void TestBossCatalog() {
    static_assert(kPartyBossCompletionCatalog.size() == 22);
    std::size_t deferred = 0;
    for (std::size_t i = 0; i < kPartyBossCompletionCatalog.size(); ++i) {
        const auto& rule = kPartyBossCompletionCatalog[i];
        assert(rule.flag_id != 12700710 && rule.flag_id != 13501802 && rule.flag_id != 13601803);
        assert(rule.flag_id != 9468 && rule.flag_id != 9469);
        assert(rule.flag_id < 3510 || rule.flag_id > 3518);
        assert(IsSupportedStoryMap(rule.packed_map));
        assert(rule.event_class == PartyEventClass::Boss);
        assert(rule.prerequisite_count == 0);
        for (std::size_t j = i + 1; j < kPartyBossCompletionCatalog.size(); ++j)
            assert(rule.flag_id != kPartyBossCompletionCatalog[j].flag_id);
        if (rule.conflict_policy == PartyConflictPolicy::DeferEndgameMutation) {
            ++deferred;
            assert(rule.flag_id == 12101800 || rule.flag_id == 12101850);
        }
    }
    assert(deferred == 2);
}

static void TestUniqueItems() {
    static_assert(kPartyUniqueItemCatalog.size() == 3);
    for (std::size_t i = 0; i < kPartyUniqueItemCatalog.size(); ++i) {
        const auto& item = kPartyUniqueItemCatalog[i];
        assert(item.quantity == 1);
        assert(item.source_event_flag != 0);
        assert(std::any_of(kPartyBossCompletionCatalog.begin(), kPartyBossCompletionCatalog.end(),
                           [&](const auto& event) { return event.flag_id == item.source_event_flag; }));
        for (std::size_t j = i + 1; j < kPartyUniqueItemCatalog.size(); ++j)
            assert(item.goods_id != kPartyUniqueItemCatalog[j].goods_id);
    }
    const auto find_item = [](std::uint32_t goods) -> const PartyUniqueItemRule* {
        for (const auto& item : kPartyUniqueItemCatalog)
            if (item.goods_id == goods) return &item;
        return nullptr;
    };
    const auto* oedon_key = find_item(4000);
    assert(oedon_key && oedon_key->source_event_flag == 12411800 &&
           oedon_key->flag_id == kNoItemAcquisitionFlag &&
           oedon_key->unique_policy == PartyUniqueItemPolicy::GrantWhenMissing);
    const auto* clocktower_key = find_item(4020);
    assert(clocktower_key && clocktower_key->source_event_flag == 13501850 &&
           clocktower_key->flag_id == 53501850 &&
           clocktower_key->unique_policy == PartyUniqueItemPolicy::GrantWhenMissing);
    const auto* cord = find_item(4323);
    assert(cord && cord->source_event_flag == 12601800 && cord->flag_id == 50000305 &&
           cord->unique_policy == PartyUniqueItemPolicy::PreserveChoiceItemOnly);
}

int main() {
    TestBossCatalog();
    TestUniqueItems();
    std::puts("PASS: verified 22-event map/completion catalog and unique-item/endgame policies");
}
