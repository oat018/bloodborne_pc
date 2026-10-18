// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bbsc_party.h"

#include <algorithm>
#include <array>

namespace Core::Bbsc::Party {

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
