// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// BBSC packets on the np_signaling socket that the Bloodborne code also builds and reads. Kept out
// of np_signaling_state.h, which pulls in the platform headers (their `near`/`far` macros break
// bloodborne_re.cpp).

#include <cstddef>

#include "common/types.h"

namespace Libraries::Np::NpSignaling {

inline constexpr u8 kSignalingBbscWorldFlagsType = 0x22;
inline constexpr u8 kSignalingBbscPartyType = 0x28;
inline constexpr u8 kSignalingBbscCapabilityType = 0x29;
inline constexpr u8 kBbscPartyVersion = 1;
inline constexpr size_t kBbscPartyHeaderBytes = 46;
inline constexpr size_t kBbscPartyMaxPacketBytes = 1024;
inline constexpr size_t kBbscPartyMaxCatalogEvents = 1024;
inline constexpr size_t kBbscPartyEventsPerPacket =
    (kBbscPartyMaxPacketBytes - kBbscPartyHeaderBytes) / sizeof(u32);
inline constexpr size_t kBbscPartyMaxChunks =
    (kBbscPartyMaxCatalogEvents + kBbscPartyEventsPerPacket - 1) / kBbscPartyEventsPerPacket;
enum class BbscPartyKind : u8 { Hello = 0, Ready = 1, Baseline = 2, Event = 3, Ack = 4, Leave = 5 };
// Little endian, no struct casts: magic4/type1/version1/kind1/reserved0_1,
// party16/admission8/sequence8/chunk2/chunks2/event_count2, then event_count * u32.

inline constexpr u8 kBbscCapabilityVersion = 1;
inline constexpr size_t kBbscCapabilityPacketBytes = 96;
enum class BbscCapabilityKind : u8 { Probe = 0, Confirm = 1 };
// Little endian: magic4/type1/version1/kind1/reserved0_1, room8, challenge16,
// sender-own proof32, sender-selected-target proof32.


// One chunk of a host world-flag snapshot: up to 8 event-flag groups (1000 flags, 125 bytes each)
// of the host's current map block, in the fixed order of Core::Bloodborne's BbscWorldGroupKey.
// ~1 KB, below the 1280-byte tunnel MTU (Tailscale) with the UDP and v-port headers.
inline constexpr size_t kBbscWorldFlagGroupBytes = 125;
inline constexpr size_t kBbscWorldFlagGroupsPerChunk = 8;

#pragma pack(push, 1)
struct SignalingBbscWorldFlags {
    u8 magic[4] = {'S', 'H', 'A', 'D'};
    u8 type = kSignalingBbscWorldFlagsType;
    u8 version = 1;
    u32 packed_map = 0;
    u32 snapshot = 0;   // the host's snapshot counter: chunks of one snapshot belong together
    u8 chunk = 0;       // index of this chunk
    u8 chunk_count = 0; // chunks in the snapshot
    u8 present = 0;     // bit n: group n of this chunk exists in the host's flag table
    u8 group_count = 0; // groups carried (the last chunk may carry fewer than 8)
    u8 groups[kBbscWorldFlagGroupsPerChunk][kBbscWorldFlagGroupBytes]{};
};
static_assert(sizeof(SignalingBbscWorldFlags) == 18 + 8 * 125);
#pragma pack(pop)

} // namespace Libraries::Np::NpSignaling
