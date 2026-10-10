// SPDX-License-Identifier: GPL-2.0-or-later
// bbport co-op: Core::InstallGuestCodeHook from shadp2p's cpu_patches.cpp. shadPS4 writes its
// trampolines into an area reserved beside each module; here one block is allocated within
// reach of a near jump from the game image.

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include <xbyak/xbyak.h>

#include "common/alignment.h"
#include "common/decoder.h"
#include "common/memory_patcher.h"
#include "core/cpu_patches.h"

using namespace Xbyak::util;

namespace Core {
namespace {

constexpr size_t TrampolineAreaSize = 1024 * 1024;
constexpr u64 NearReach = 0x70000000; // kept below 2 GiB: the image itself is a few hundred MiB

std::mutex hook_mutex;
std::unique_ptr<Xbyak::CodeGenerator> trampoline_gen;

u8* AllocateNear(u64 target) {
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    const u64 step = info.dwAllocationGranularity;
    const u64 lowest = std::max<u64>(target > NearReach ? target - NearReach : 0,
                                     reinterpret_cast<u64>(info.lpMinimumApplicationAddress));
    const u64 highest = target + NearReach;
    for (u64 at = Common::AlignUp(lowest, step); at < highest;) {
        MEMORY_BASIC_INFORMATION region;
        if (!VirtualQuery(reinterpret_cast<void*>(at), &region, sizeof(region))) {
            break;
        }
        const u64 region_end = reinterpret_cast<u64>(region.BaseAddress) + region.RegionSize;
        if (region.State == MEM_FREE && region_end - at >= TrampolineAreaSize) {
            if (void* memory = VirtualAlloc(reinterpret_cast<void*>(at), TrampolineAreaSize,
                                            MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE)) {
                return static_cast<u8*>(memory);
            }
        }
        at = Common::AlignUp(region_end, step);
    }
    return nullptr;
#else
    for (u64 at = Common::AlignUp(target + MemoryPatcher::g_eboot_image_size, u64{0x10000});
         at < target + NearReach; at += 0x1000000) {
        void* memory = mmap(reinterpret_cast<void*>(at), TrampolineAreaSize,
                            PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (memory == MAP_FAILED) {
            continue;
        }
        const u64 got = reinterpret_cast<u64>(memory);
        if (got >= target ? got - target < NearReach : target - got < NearReach) {
            return static_cast<u8*>(memory);
        }
        munmap(memory, TrampolineAreaSize);
    }
    return nullptr;
#endif
}

bool CanCopyGuestHookInstructions(void* address, std::span<const u8> instructions) {
    size_t offset = 0;
    while (offset < instructions.size()) {
        ZydisDecodedInstruction instruction{};
        std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
        const auto status = Common::Decoder::Instance()->decodeInstruction(
            instruction, operands.data(), static_cast<u8*>(address) + offset,
            instructions.size() - offset);
        if (!ZYAN_SUCCESS(status) || instruction.length == 0 ||
            instruction.length > instructions.size() - offset) {
            std::printf("Co-op: hook at %p does not cover whole instructions\n", address);
            return false;
        }
        for (u8 index = 0; index < instruction.operand_count; ++index) {
            const auto& operand = operands[index];
            const bool relative_immediate =
                operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE && operand.imm.is_relative;
            const bool instruction_pointer_memory =
                operand.type == ZYDIS_OPERAND_TYPE_MEMORY &&
                (operand.mem.base == ZYDIS_REGISTER_RIP || operand.mem.base == ZYDIS_REGISTER_EIP);
            if (relative_immediate || instruction_pointer_memory) {
                std::printf("Co-op: hook at %p contains relative addressing\n", address);
                return false;
            }
        }
        offset += instruction.length;
    }
    return true;
}

} // namespace

bool InstallGuestCodeHook(void* address, std::span<const u8> expected_instructions, u64 tag,
                          GuestCodeHook hook) {
    constexpr size_t NearJumpBytes = 5;
    constexpr size_t GuestRedZoneBytes = 128;
    constexpr size_t SnapshotOffset = 0;
    constexpr size_t ScratchR11Offset = sizeof(GuestRegisterSnapshot);
    constexpr size_t ScratchRflagsOffset = ScratchR11Offset + sizeof(u64);
    constexpr size_t FrameReserve =
        Common::AlignUp(ScratchRflagsOffset + sizeof(u64) + GuestRedZoneBytes, 16);
    static_assert(FrameReserve % 16 == 0);

    auto* code = static_cast<u8*>(address);
    const u64 image = MemoryPatcher::g_eboot_address;
    const u64 image_size = MemoryPatcher::g_eboot_image_size;
    const u64 at = reinterpret_cast<u64>(code);
    if (image == 0 || hook == nullptr || expected_instructions.size() < NearJumpBytes ||
        at < image || at - image > image_size ||
        expected_instructions.size() > image_size - (at - image)) {
        return false;
    }

    std::scoped_lock lock{hook_mutex};
    if (!std::ranges::equal(expected_instructions,
                            std::span<const u8>{code, expected_instructions.size()})) {
        std::printf("Co-op: hook signature mismatch at image offset %#llx\n",
                    static_cast<unsigned long long>(at - image));
        return false;
    }
    if (!CanCopyGuestHookInstructions(address, expected_instructions)) {
        return false;
    }
    if (!trampoline_gen) {
        u8* area = AllocateNear(image);
        if (area == nullptr) {
            std::printf("Co-op: no memory for hooks within reach of the game image\n");
            return false;
        }
        trampoline_gen = std::make_unique<Xbyak::CodeGenerator>(TrampolineAreaSize, area);
    }

    auto& trampoline = *trampoline_gen;
    const size_t trampoline_offset = trampoline.getSize();
    const auto* trampoline_start = trampoline.getCurr();
    const auto snapshot = [](size_t field_offset) {
        return qword[rsp + SnapshotOffset + field_offset];
    };

    try {
        trampoline.lea(rsp, ptr[rsp - FrameReserve]);
        trampoline.mov(qword[rsp + ScratchR11Offset], r11);
        trampoline.pushfq();
        trampoline.pop(r11);
        trampoline.mov(qword[rsp + ScratchRflagsOffset], r11);
        trampoline.mov(r11, rsp);
        trampoline.and_(rsp, -16);

        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, rax)), rax);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, rbx)), rbx);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, rcx)), rcx);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, rdx)), rdx);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, rsi)), rsi);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, rdi)), rdi);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, rbp)), rbp);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, r8)), r8);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, r9)), r9);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, r10)), r10);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, r12)), r12);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, r13)), r13);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, r14)), r14);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, r15)), r15);
        trampoline.mov(rax, qword[r11 + ScratchR11Offset]);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, r11)), rax);
        trampoline.mov(rax, qword[r11 + ScratchRflagsOffset]);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, rflags)), rax);
        trampoline.lea(rax, ptr[r11 + FrameReserve]);
        trampoline.mov(snapshot(offsetof(GuestRegisterSnapshot, rsp)), rax);
        trampoline.stmxcsr(dword[rsp + SnapshotOffset + offsetof(GuestRegisterSnapshot, mxcsr)]);

        for (size_t index = 0; index < 16; ++index) {
            const size_t offset = SnapshotOffset + offsetof(GuestRegisterSnapshot, ymm) +
                                  index * sizeof(std::array<u8, 32>);
            trampoline.vmovdqu(yword[rsp + offset], Xbyak::Ymm(static_cast<int>(index)));
        }

        // Keep the aligned snapshot base in a callee-saved register across the host call.
        trampoline.mov(r12, rsp);
        trampoline.mov(rdi, tag);
        trampoline.lea(rsi, ptr[r12 + SnapshotOffset]);
        trampoline.mov(rax, reinterpret_cast<u64>(hook));
        trampoline.call(rax);
        trampoline.mov(rsp, r12);

        for (size_t index = 0; index < 16; ++index) {
            const size_t offset = SnapshotOffset + offsetof(GuestRegisterSnapshot, ymm) +
                                  index * sizeof(std::array<u8, 32>);
            trampoline.vmovdqu(Xbyak::Ymm(static_cast<int>(index)), yword[rsp + offset]);
        }
        trampoline.ldmxcsr(dword[rsp + SnapshotOffset + offsetof(GuestRegisterSnapshot, mxcsr)]);
        trampoline.mov(rbx, snapshot(offsetof(GuestRegisterSnapshot, rbx)));
        trampoline.mov(rcx, snapshot(offsetof(GuestRegisterSnapshot, rcx)));
        trampoline.mov(rdx, snapshot(offsetof(GuestRegisterSnapshot, rdx)));
        trampoline.mov(rsi, snapshot(offsetof(GuestRegisterSnapshot, rsi)));
        trampoline.mov(rdi, snapshot(offsetof(GuestRegisterSnapshot, rdi)));
        trampoline.mov(rbp, snapshot(offsetof(GuestRegisterSnapshot, rbp)));
        trampoline.mov(r8, snapshot(offsetof(GuestRegisterSnapshot, r8)));
        trampoline.mov(r9, snapshot(offsetof(GuestRegisterSnapshot, r9)));
        trampoline.mov(r10, snapshot(offsetof(GuestRegisterSnapshot, r10)));
        trampoline.mov(r11, snapshot(offsetof(GuestRegisterSnapshot, r11)));
        trampoline.mov(r12, snapshot(offsetof(GuestRegisterSnapshot, r12)));
        trampoline.mov(r13, snapshot(offsetof(GuestRegisterSnapshot, r13)));
        trampoline.mov(r14, snapshot(offsetof(GuestRegisterSnapshot, r14)));
        trampoline.mov(r15, snapshot(offsetof(GuestRegisterSnapshot, r15)));
        trampoline.mov(rax, snapshot(offsetof(GuestRegisterSnapshot, rflags)));
        trampoline.push(rax);
        trampoline.popfq();
        trampoline.mov(rax, snapshot(offsetof(GuestRegisterSnapshot, rax)));
        trampoline.mov(rsp, snapshot(offsetof(GuestRegisterSnapshot, rsp)));
        trampoline.db(expected_instructions.data(), expected_instructions.size());
        trampoline.jmp(code + expected_instructions.size(),
                       Xbyak::CodeGenerator::LabelType::T_NEAR);
    } catch (const Xbyak::Error& error) {
        trampoline.setSize(trampoline_offset);
        std::printf("Co-op: could not generate the hook at %p: %s\n", address, error.what());
        return false;
    }

    const s64 distance = reinterpret_cast<s64>(trampoline_start) -
                         reinterpret_cast<s64>(code + NearJumpBytes);
    if (distance != static_cast<s32>(distance)) {
        trampoline.setSize(trampoline_offset);
        std::printf("Co-op: hook memory is out of reach of %p\n", address);
        return false;
    }
    const s32 displacement = static_cast<s32>(distance);
    code[0] = 0xE9;
    std::memcpy(code + 1, &displacement, sizeof(displacement));
    std::memset(code + NearJumpBytes, 0x90, expected_instructions.size() - NearJumpBytes);
    return true;
}

} // namespace Core
