// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "common/types.h"

namespace Core::Net::P2P {
class Keyring;
}

namespace Libraries::Net {
void ConfigureP2P(u16 udp_port, u32 advertised_addr);
u16 StartP2P(u16 udp_port);
void StopP2P();
std::shared_ptr<Core::Net::P2P::Keyring> GetP2PKeyring();

// Used by NP signaling/matching.
bool EnsureP2PTransport();
bool P2PTransportIsReady();
u16 GetP2PConfiguredPort(); // host order
u16 GetP2PBoundPort();      // host order, 0 when not running
u32 GetP2PAdvertisedAddr(); // Network order, 0 if unknown.
u16 GetP2PAdvertisedPort();
int P2PSignalingSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
int P2PSignalingRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);
int P2PControlSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
int P2PControlRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);
int P2PMatching2SendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
int P2PMatching2RecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);
/// Seamless co-op: a small note to the peers game packets are exchanged with, and the fresh
/// notes of those peers (entries of entry_size bytes); messages (up to a datagram) to the same
/// peers, and the oldest one received (0 when none).
int P2PSeamlessBroadcast(const void* data, u32 len);
int P2PSeamlessCollect(void* out, u32 entry_size, u32 max_entries);
int P2PSeamlessSendMessage(const void* data, u32 len);
int P2PSeamlessReceiveMessage(void* out, u32 len);

} // namespace Libraries::Net
