// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bbsc_party.h"

#include <cassert>
#include <cstdio>
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

int main(int argc, char** argv) {
    void TestJournal();
    const std::string_view section = argc == 2 ? argv[1] : "all";
    if (section != "all" && section != "policy" && section != "return" && section != "journal") {
        std::fputs("Unknown test section\n", stderr);
        return 2;
    }
    if (section == "all" || section == "policy") TestPasswordAndMapEligibility();
    if (section == "all" || section == "return") TestReturnDestination();
    if (section == "all" || section == "journal") TestJournal();
}
