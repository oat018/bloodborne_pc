// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bbsc_memscan.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>

namespace {
bool fail_vm_read = false, short_vm_read = false, fail_mem_read = false;
bool unmap_on_read = false, interrupt_mem_read = false, short_mem_read = false;
constexpr std::size_t Piece = 1u << 20;
constexpr std::array<std::uint8_t, 6> Needle{0x79, 0x31, 0x94, 0x62, 0x88, 0x2a};
}
extern "C" ssize_t __real_process_vm_readv(pid_t, const iovec*, unsigned long,
                                           const iovec*, unsigned long, unsigned long);
extern "C" ssize_t __wrap_process_vm_readv(pid_t pid, const iovec* local, unsigned long nl,
                                           const iovec* remote, unsigned long nr, unsigned long flags) {
    if (unmap_on_read) {
        unmap_on_read = false;
        assert(munmap(remote[0].iov_base, 4096) == 0);
        errno = EFAULT;
        return -1;
    }
    if (fail_vm_read) { errno = EFAULT; return -1; }
    if (short_vm_read) {
        iovec prefix = local[0];
        if (prefix.iov_len > 32) prefix.iov_len = 32;
        return __real_process_vm_readv(pid, &prefix, 1, remote, nr, flags);
    }
    return __real_process_vm_readv(pid, local, nl, remote, nr, flags);
}
extern "C" ssize_t __real_pread(int, void*, size_t, off_t);
extern "C" ssize_t __wrap_pread(int fd, void* data, size_t size, off_t offset) {
    if (fail_mem_read) { errno = EIO; return -1; }
    if (interrupt_mem_read) { interrupt_mem_read = false; errno = EINTR; return -1; }
    if (short_mem_read && size > 4096) size = 4096;
    return __real_pread(fd, data, size, offset);
}
int main() {
    const auto size = Piece * 2 + 4096;
    auto* p = static_cast<std::uint8_t*>(mmap(nullptr, size, PROT_READ | PROT_WRITE,
                                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    assert(p != MAP_FAILED);
    for (const auto offset : std::array<std::size_t, 4>{64, Piece - 2, Piece + 4096, Piece * 2})
        std::memcpy(p + offset, Needle.data(), Needle.size());
    assert(mprotect(p + Piece * 2, 4096, PROT_NONE) == 0);
    const auto lo = reinterpret_cast<std::uint64_t>(p);
    const std::vector<std::uint64_t> expected{lo + 64, lo + Piece - 2, lo + Piece + 4096};
    assert(Core::Bbsc::FindInMemory(Needle, lo, lo + size, 16) == expected);
    assert(Core::Bbsc::FindInMemory(Needle, lo + 65, lo + Piece, 16).empty());
    assert(Core::Bbsc::FindInMemory(Needle, lo, lo + size, 0).empty());
    assert(Core::Bbsc::FindInMemory(Needle, lo, lo + size, 1) ==
           std::vector<std::uint64_t>{lo + 64});
    fail_vm_read = true; // Device-backed VMAs reject get_user_pages with EFAULT.
    assert(Core::Bbsc::FindInMemory(Needle, lo, lo + size, 16) == expected);
    interrupt_mem_read = true;
    short_mem_read = true;
    assert(Core::Bbsc::FindInMemory(Needle, lo, lo + size, 16) == expected);
    short_mem_read = false;
    fail_mem_read = true;
    assert(Core::Bbsc::FindInMemory(Needle, lo, lo + size, 16).empty());
    fail_mem_read = false;
    fail_vm_read = false;
    short_vm_read = true;
    assert(Core::Bbsc::FindInMemory(Needle, lo, lo + size, 16) == expected);
    short_vm_read = false;
    assert(munmap(p, size) == 0);
    p = static_cast<std::uint8_t*>(mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    assert(p != MAP_FAILED);
    unmap_on_read = true; // The readable VMA disappeared after /proc/self/maps was read.
    const auto gone = reinterpret_cast<std::uint64_t>(p);
    assert(Core::Bbsc::FindInMemory(Needle, gone, gone + 4096, 16).empty());
    puts("PASS: BBSC scan bounds, chunk overlap, dma-buf read fallback, short reads and disappearing mappings");
}
