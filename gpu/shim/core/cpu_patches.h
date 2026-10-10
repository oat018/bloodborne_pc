// bbport co-op: the guest code hook from shadp2p's cpu_patches.h. bbport runs the game code
// as it is (no CPU patches), so only the hook is here; bbnet/guest_hook.cpp implements it
// with its own trampoline memory near the game image.
#pragma once
#include <array>
#include <span>
#include "common/types.h"

namespace Core {

struct alignas(16) GuestRegisterSnapshot {
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    u64 rbp;
    u64 rsp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
    u64 rflags;
    u32 mxcsr;
    alignas(16) std::array<std::array<u8, 32>, 16> ymm;
};

using GuestCodeHook = void PS4_SYSV_ABI (*)(u64 tag, const GuestRegisterSnapshot* registers);

/// Installs a byte-verified observer while preserving GPR, flags, MXCSR, and YMM state.
/// The copied instruction span must contain whole instructions and must not use relative
/// addressing. The code must be writable (bbport installs hooks before the image is protected).
bool InstallGuestCodeHook(void* address, std::span<const u8> expected_instructions, u64 tag,
                          GuestCodeHook hook);

} // namespace Core
