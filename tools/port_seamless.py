#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Turns shadp2p's src/core/bloodborne_re.cpp into gpu/bbnet/bloodborne_re.cpp.

    python tools/port_seamless.py <shadp2p checkout>/src/core/bloodborne_re.cpp

The seamless co-op code works on the game image itself, so it carries over as it is; only what
it asks of the emulator differs: the game identity comes from ElfInfo (bbnet_configure) instead
of the PSF singleton, and its messages go to the port's console (last_run.log) under "Co-op:".
"""
from pathlib import Path
import sys

PORT = Path(__file__).resolve().parent.parent

INCLUDES_OLD = '''#include "common/logging/log.h"
#include "common/memory_patcher.h"
#include "common/path_util.h"
#include "common/singleton.h"
#include "core/cpu_patches.h"
#include "core/debugger.h"
#include "core/file_format/psf.h"
#include "core/memory.h"
'''
INCLUDES_NEW = '''#include <cstdio>
#include <fmt/format.h>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/memory_patcher.h"
#include "common/path_util.h"
#include "core/cpu_patches.h"
#include "core/debugger.h"

#ifdef _WIN32
#include <windows.h>
#else
extern "C" int runtime_memory_is_mapped(uintptr_t address, uint64_t size);
#endif

// bbport: the port's log shows warnings and errors only; co-op messages always go to the
// console, which the launcher keeps in user/last_run.log.
#undef LOG_INFO
#undef LOG_WARNING
#undef LOG_ERROR
#define BB_COOP_LOG(...)                                                                           \\
    do {                                                                                           \\
        std::printf("Co-op: %s\\n", fmt::format(__VA_ARGS__).c_str());                              \\
        std::fflush(stdout);                                                                       \\
    } while (false)
#define LOG_INFO(log_class, ...) BB_COOP_LOG(__VA_ARGS__)
#define LOG_WARNING(log_class, ...) BB_COOP_LOG(__VA_ARGS__)
#define LOG_ERROR(log_class, ...) BB_COOP_LOG(__VA_ARGS__)
'''
MEMORY_OLD = '''bool HasMemoryAccess(u64 address, size_t size, MemoryProt required) {
    if (size == 0 || address > std::numeric_limits<u64>::max() - size) {
        return false;
    }

    auto* memory = Memory::Instance();
    if (memory == nullptr || !memory->IsValidMapping(address, size)) {
        return false;
    }

    void* mapping_start{};
    void* mapping_end{};
    u32 protection{};
    const u32 required_bits = static_cast<u32>(required);
    return memory->QueryProtection(address, &mapping_start, &mapping_end, &protection) == 0 &&
           (protection & required_bits) == required_bits &&
           address >= reinterpret_cast<u64>(mapping_start) &&
           address + size <= reinterpret_cast<u64>(mapping_end);
}
'''
MEMORY_NEW = '''// bbport: guest memory is host memory at the same address; the image, the game's heaps and
// its thread stacks come from different allocators, so the operating system is asked.
enum class MemoryProt : u32 { CpuRead = 1, CpuWrite = 2 };

bool HasMemoryAccess(u64 address, size_t size, MemoryProt required) {
    if (size == 0 || address > std::numeric_limits<u64>::max() - size) {
        return false;
    }
#ifdef _WIN32
    for (u64 at = address; at < address + size;) {
        MEMORY_BASIC_INFORMATION region;
        if (!VirtualQuery(reinterpret_cast<void*>(at), &region, sizeof(region)) ||
            region.State != MEM_COMMIT || (region.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
            return false;
        }
        const DWORD writable = PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE |
                               PAGE_EXECUTE_WRITECOPY;
        const DWORD readable = writable | PAGE_READONLY | PAGE_EXECUTE_READ;
        if (!(region.Protect & (required == MemoryProt::CpuWrite ? writable : readable))) {
            return false;
        }
        at = reinterpret_cast<u64>(region.BaseAddress) + region.RegionSize;
    }
    return true;
#else
    const u64 image = MemoryPatcher::g_eboot_address;
    if (address >= image && address - image < MemoryPatcher::g_eboot_image_size &&
        size <= MemoryPatcher::g_eboot_image_size - (address - image)) {
        return true;
    }
    return runtime_memory_is_mapped(address, size) != 0;
#endif
}
'''
PSF_OLD = '''    const auto* param_sfo = Common::Singleton<PSF>::Instance();
    const std::string_view app_version = param_sfo->GetString("APP_VER").value_or("Unknown");
'''
PSF_NEW = '''    const std::string_view app_version = Common::ElfInfo::Instance().AppVer();
'''


def main():
    source = Path(sys.argv[1]).read_text(encoding='utf-8').replace('\r\n', '\n')
    for old, new, count in ((INCLUDES_OLD, INCLUDES_NEW, 1), (PSF_OLD, PSF_NEW, 2),
                            (MEMORY_OLD, MEMORY_NEW, 1)):
        if source.count(old) != count:
            sys.exit(f'expected {count} occurrence(s) of:\n{old}')
        source = source.replace(old, new)
    source = source.replace(
        '// SPDX-License-Identifier: GPL-2.0-or-later\n',
        '// SPDX-License-Identifier: GPL-2.0-or-later\n'
        '// bbport: generated by tools/port_seamless.py from shadp2p; edit there and regenerate.\n', 1)
    target = PORT / 'gpu' / 'bbnet' / 'bloodborne_re.cpp'
    target.write_text(source, encoding='utf-8', newline='\n')
    print(f'wrote {target}')


if __name__ == '__main__':
    main()
