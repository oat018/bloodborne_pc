// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bbsc_party.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <charconv>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <openssl/sha.h>
#include <span>
#include <unordered_set>
#include <utility>
#include <zlib.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace Core::Bbsc::Party {
namespace {
namespace fs = std::filesystem;
constexpr std::size_t MaxRecordBytes = 65536;
constexpr std::size_t MaxEvents = 1024;
constexpr std::size_t MaxWorldSnapshotBytes = 2 * 1024 * 1024;

bool Fail(std::error_code& ec, std::errc error) {
    ec = std::make_error_code(error);
    return false;
}

bool SystemError(std::error_code& ec) {
    ec = {errno, std::generic_category()};
    return false;
}

bool ValidKey(const CharacterKey& key) {
    const fs::path path{key.save_dir};
    return !key.save_dir.empty() && key.save_dir.size() <= 1023 &&
           key.save_dir.find('\0') == std::string::npos && path.is_absolute() &&
           path.lexically_normal() == path && !key.user.empty() && key.user.size() <= 31 &&
           key.user.find_first_of("/\\") == std::string::npos &&
           key.user.find('\0') == std::string::npos && key.slot < 10 &&
           std::ranges::any_of(key.character, [](auto byte) { return byte != 0; }) &&
           ((key.cycle <= 6) || ((key.cycle & 0x80000000u) != 0 &&
                                  (key.cycle & 0x7fffffffu) != 0));
}

bool ValidSha256(std::string_view hash) {
    return hash.size() == 64 && std::ranges::all_of(hash, [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

bool SameCharacter(const CharacterKey& a, const CharacterKey& b) {
    return a.save_dir == b.save_dir && a.user == b.user && a.slot == b.slot &&
           a.character == b.character;
}

bool ValidCycleRecord(const ManagedCycleEpoch& record) {
    if (!ValidKey(record.key) || record.native_cycle != 7 || record.epoch > 0x7fffffffULL ||
        std::to_underlying(record.stage) > std::to_underlying(ManagedCycleStage::SaveSerialized))
        return false;
    if (record.stage == ManagedCycleStage::Stable)
        return record.epoch == 0 ? record.key.cycle <= 6 && record.target_key_cycle == 0
                                 : record.key.cycle == (0x80000000u | record.epoch) &&
                                       record.target_key_cycle == record.key.cycle &&
                                       ValidSha256(record.serialized_sha256);
    if (!ValidSha256(record.baseline_sha256)) return false;
    if (record.stage == ManagedCycleStage::Intent)
        return record.epoch <= 0x7fffffffULL && record.target_key_cycle != 0 &&
               (record.target_key_cycle & 0x80000000u) != 0;
    if (record.stage == ManagedCycleStage::Applied)
        return record.target_key_cycle != 0 && (record.target_key_cycle & 0x80000000u) != 0;
    return record.target_key_cycle != 0 && (record.target_key_cycle & 0x80000000u) != 0 &&
           record.serialized_generation >= record.baseline_generation &&
           ValidSha256(record.serialized_sha256);
}

bool ValidDestination(const Destination& to) {
    return IsSupportedStoryMap(to.map) && std::isfinite(to.x) && std::isfinite(to.y) &&
           std::isfinite(to.z) && std::isfinite(to.heading) && to.lamp >= -1;
}

bool Valid(const Journal& record) {
    if (!ValidKey(record.key) || !ValidDestination(record.origin) ||
        !ValidDestination(record.fallback) || std::to_underlying(record.phase) > 3 ||
        !std::ranges::any_of(record.party, [](auto byte) { return byte != 0; }) ||
        record.pending.size() + record.committed.size() + record.retained.size() > MaxEvents ||
        (record.admission == 0 && (!record.pending.empty() || !record.committed.empty())) ||
        record.world_snapshot.bytes > MaxWorldSnapshotBytes ||
        (record.world_snapshot.bytes == 0 && record.world_snapshot.crc32 != 0)) {
        return false;
    }
    std::unordered_set<EventId> unique;
    for (const auto* events : {&record.pending, &record.committed}) {
        for (const auto event : *events) {
            if (event == 0 || !unique.insert(event).second) return false;
        }
    }
    for (const auto& claim : record.retained) {
        if (!claim.event || !claim.admission ||
            !std::ranges::any_of(claim.party, [](auto byte) { return byte != 0; }) ||
            !((claim.cycle <= 6) || ((claim.cycle & 0x80000000u) != 0 &&
                                     (claim.cycle & 0x7fffffffu) != 0)) ||
            !unique.insert(claim.event).second) return false;
    }
    return true;
}

void Put(std::vector<std::uint8_t>& bytes, std::uint64_t value, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) bytes.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
}

void PutString(std::vector<std::uint8_t>& bytes, const std::string& value) {
    Put(bytes, value.size(), 2);
    bytes.insert(bytes.end(), value.begin(), value.end());
}

void PutDestination(std::vector<std::uint8_t>& bytes, const Destination& to) {
    Put(bytes, to.map, 4);
    for (const float value : {to.x, to.y, to.z, to.heading})
        Put(bytes, std::bit_cast<std::uint32_t>(value), 4);
    Put(bytes, std::bit_cast<std::uint32_t>(to.lamp), 4);
}

struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at{};
    bool valid{true};

    std::uint64_t Get(std::size_t size) {
        if (size > bytes.size() - at) { valid = false; return 0; }
        std::uint64_t value = 0;
        for (std::size_t i = 0; i < size; ++i) value |= std::uint64_t{bytes[at++]} << (i * 8);
        return value;
    }
    std::string String(std::size_t limit) {
        const auto count = Get(2);
        if (!valid || count > limit || count > bytes.size() - at) { valid = false; return {}; }
        const std::string value(bytes.begin() + at, bytes.begin() + at + count);
        at += count;
        return value;
    }
    PartyId Id() {
        PartyId id{};
        for (auto& byte : id) byte = static_cast<std::uint8_t>(Get(1));
        return id;
    }
    Destination To() {
        Destination to;
        to.map = static_cast<std::uint32_t>(Get(4));
        to.x = std::bit_cast<float>(static_cast<std::uint32_t>(Get(4)));
        to.y = std::bit_cast<float>(static_cast<std::uint32_t>(Get(4)));
        to.z = std::bit_cast<float>(static_cast<std::uint32_t>(Get(4)));
        to.heading = std::bit_cast<float>(static_cast<std::uint32_t>(Get(4)));
        to.lamp = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Get(4)));
        return to;
    }
    std::vector<EventId> Events() {
        const auto count = Get(2);
        if (!valid || count > MaxEvents || count > (bytes.size() - at) / 4) {
            valid = false;
            return {};
        }
        std::vector<EventId> events;
        for (std::size_t i = 0; i < count; ++i) events.push_back(static_cast<EventId>(Get(4)));
        return events;
    }
};

std::vector<std::uint8_t> EncodeCycleRecord(const ManagedCycleEpoch& record) {
    std::vector<std::uint8_t> body;
    PutString(body, record.key.save_dir);
    PutString(body, record.key.user);
    Put(body, record.key.slot, 1);
    body.insert(body.end(), record.key.character.begin(), record.key.character.end());
    Put(body, record.key.cycle, 4);
    Put(body, record.epoch, 8);
    Put(body, record.target_key_cycle, 4);
    Put(body, record.native_cycle, 1);
    Put(body, std::to_underlying(record.stage), 1);
    Put(body, record.baseline_generation, 8);
    Put(body, record.serialized_generation, 8);
    PutString(body, record.baseline_sha256);
    PutString(body, record.serialized_sha256);
    std::vector<std::uint8_t> bytes{'B', 'B', 'C', 'E'};
    Put(bytes, 1, 4);
    Put(bytes, body.size(), 4);
    Put(bytes, crc32(0, body.data(), static_cast<uInt>(body.size())), 4);
    bytes.insert(bytes.end(), body.begin(), body.end());
    return bytes;
}

std::optional<ManagedCycleEpoch> DecodeCycleRecord(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 16 || std::memcmp(bytes.data(), "BBCE", 4) != 0) return {};
    Reader header{bytes, 4};
    if (header.Get(4) != 1 || header.Get(4) != bytes.size() - 16 ||
        header.Get(4) != crc32(0, bytes.data() + 16, static_cast<uInt>(bytes.size() - 16)))
        return {};
    Reader body{bytes.subspan(16)};
    ManagedCycleEpoch record;
    record.key.save_dir = body.String(1023);
    record.key.user = body.String(31);
    record.key.slot = static_cast<std::uint8_t>(body.Get(1));
    record.key.character = body.Id();
    record.key.cycle = static_cast<std::uint32_t>(body.Get(4));
    record.epoch = body.Get(8);
    record.target_key_cycle = static_cast<std::uint32_t>(body.Get(4));
    record.native_cycle = static_cast<std::uint8_t>(body.Get(1));
    record.stage = static_cast<ManagedCycleStage>(body.Get(1));
    record.baseline_generation = body.Get(8);
    record.serialized_generation = body.Get(8);
    record.baseline_sha256 = body.String(64);
    record.serialized_sha256 = body.String(64);
    if (!body.valid || body.at != body.bytes.size() || !ValidCycleRecord(record)) return {};
    return record;
}

std::vector<std::uint8_t> Encode(const Journal& record) {
    std::vector<std::uint8_t> body;
    PutString(body, record.key.save_dir);
    PutString(body, record.key.user);
    Put(body, record.key.slot, 1);
    body.insert(body.end(), record.key.character.begin(), record.key.character.end());
    Put(body, record.key.cycle, 4);
    body.insert(body.end(), record.party.begin(), record.party.end());
    Put(body, record.admission, 8);
    Put(body, std::to_underlying(record.phase), 1);
    PutDestination(body, record.origin);
    PutDestination(body, record.fallback);
    Put(body, record.save_generation, 8);
    Put(body, record.world_snapshot.bytes, 4);
    Put(body, record.world_snapshot.crc32, 4);
    for (const auto* events : {&record.pending, &record.committed}) {
        Put(body, events->size(), 2);
        for (const auto event : *events) Put(body, event, 4);
    }
    Put(body, record.retained.size(), 2);
    for (const auto& claim : record.retained) {
        Put(body, claim.event, 4);
        body.insert(body.end(), claim.party.begin(), claim.party.end());
        Put(body, claim.admission, 8);
        Put(body, claim.committed, 1);
        Put(body, claim.cycle, 4);
    }
    Put(body, record.native_return_selected, 1);
    std::vector<std::uint8_t> bytes{'B', 'B', 'P', 'R'};
    Put(bytes, 5, 4);
    Put(bytes, body.size(), 4);
    Put(bytes, crc32(0, body.data(), static_cast<uInt>(body.size())), 4);
    bytes.insert(bytes.end(), body.begin(), body.end());
    return bytes;
}

std::optional<Journal> Decode(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 16 || bytes[0] != 'B' || bytes[1] != 'B' || bytes[2] != 'P' || bytes[3] != 'R')
        return std::nullopt;
    Reader header{bytes, 4};
    const auto version = header.Get(4);
    if ((version < 1 || version > 5) || header.Get(4) != bytes.size() - 16) return std::nullopt;
    if (header.Get(4) != crc32(0, bytes.data() + 16, static_cast<uInt>(bytes.size() - 16)))
        return std::nullopt;
    Reader reader{bytes.subspan(16)};
    Journal record;
    record.key.save_dir = reader.String(1023);
    record.key.user = reader.String(31);
    record.key.slot = static_cast<std::uint8_t>(reader.Get(1));
    record.key.character = reader.Id();
    record.key.cycle = static_cast<std::uint32_t>(reader.Get(4));
    record.party = reader.Id();
    record.admission = reader.Get(8);
    record.phase = static_cast<Phase>(reader.Get(1));
    record.origin = reader.To();
    record.fallback = reader.To();
    record.save_generation = reader.Get(8);
    if (version >= 3) {
        record.world_snapshot.bytes = static_cast<std::uint32_t>(reader.Get(4));
        record.world_snapshot.crc32 = static_cast<std::uint32_t>(reader.Get(4));
    }
    record.pending = reader.Events();
    record.committed = reader.Events();
    if (version >= 2) {
        const auto count = reader.Get(2);
        const std::size_t claim_size = version >= 5 ? 33 : 29;
        if (!reader.valid || count > MaxEvents || count > (reader.bytes.size() - reader.at) / claim_size)
            return std::nullopt;
        for (std::size_t i = 0; i < count; ++i) {
            EarnedClaim claim;
            claim.event = static_cast<EventId>(reader.Get(4));
            claim.party = reader.Id();
            claim.admission = reader.Get(8);
            const auto committed = reader.Get(1);
            if (committed > 1) return std::nullopt;
            claim.committed = committed != 0;
            claim.cycle = version >= 5 ? static_cast<std::uint32_t>(reader.Get(4))
                                       : record.key.cycle;
            record.retained.push_back(claim);
        }
    }
    if (version >= 4) {
        const auto selected = reader.Get(1);
        if (selected > 1) return std::nullopt;
        record.native_return_selected = selected != 0;
    }
    if (!reader.valid || reader.at != reader.bytes.size() || !Valid(record)) return std::nullopt;
    return record;
}

std::vector<EarnedClaim> Claims(const Journal& record) {
    auto claims = record.retained;
    for (const auto event : record.pending)
        claims.push_back({event, record.party, record.admission, false, record.key.cycle});
    for (const auto event : record.committed)
        claims.push_back({event, record.party, record.admission, true, record.key.cycle});
    return claims;
}

bool AllowedUpdate(const Journal& before, const Journal& after) {
    const bool rollover = before.phase == Phase::Complete && after.phase == Phase::Prepared &&
                          before.party != after.party;
    const bool bind = before.phase == Phase::Prepared && before.admission == 0 &&
                      before.pending.empty() && before.committed.empty() &&
                      (after.phase == Phase::Prepared || after.phase == Phase::Visiting);
    if (rollover) {
        if (after.admission || !after.pending.empty() || !after.committed.empty() ||
            after.native_return_selected) return false;
    } else {
        if ((!bind && before.party != after.party) ||
            before.origin != after.origin || before.fallback != after.fallback ||
            before.world_snapshot != after.world_snapshot ||
            (before.admission != 0 && before.admission != after.admission) ||
            std::to_underlying(after.phase) < std::to_underlying(before.phase) ||
            std::to_underlying(after.phase) > std::to_underlying(before.phase) + 1) return false;
        if ((before.native_return_selected && !after.native_return_selected) ||
            (!before.native_return_selected && after.native_return_selected &&
             after.phase != Phase::Returning)) return false;
    }
    const auto old_claims = Claims(before), new_claims = Claims(after);
    const auto same_proof = [](const EarnedClaim& a, const EarnedClaim& b) {
        return a.event == b.event && a.party == b.party && a.admission == b.admission;
    };
    for (const auto& old : old_claims) {
        if (!std::ranges::any_of(new_claims, [&](const auto& now) {
                return same_proof(old, now) && (!old.committed || now.committed);
            })) return false;
    }
    // Retained provenance can only come from an existing claim, never a fresh packet.
    for (const auto& claim : after.retained)
        if (!std::ranges::any_of(old_claims, [&](const auto& old) { return same_proof(old, claim); }))
            return false;
    return true;
}

using NativeHandle = std::intptr_t;
constexpr NativeHandle InvalidHandle = -1;

void CloseHandleValue(NativeHandle handle) {
    if (handle == InvalidHandle) return;
#ifdef _WIN32
    ::CloseHandle(reinterpret_cast<HANDLE>(handle));
#else
    ::close(static_cast<int>(handle));
#endif
}

#ifdef _WIN32
// shortcut: path components are re-resolved after checking, when hostile rename races matter use handle-relative I/O.
bool CheckWindowsPath(const fs::path& path, bool allow_missing_leaf, bool directory_leaf,
                      std::error_code& ec) {
    std::error_code path_error;
    const auto absolute = fs::absolute(path, path_error).lexically_normal();
    if (path_error) { ec = path_error; return false; }
    auto current = absolute.root_path();
    const auto relative = absolute.relative_path();
    for (auto part = relative.begin(); part != relative.end(); ++part) {
        current /= *part;
        const bool leaf = std::next(part) == relative.end();
        const auto handle = ::CreateFileW(current.c_str(), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (handle == INVALID_HANDLE_VALUE) {
            const auto error = ::GetLastError();
            if (leaf && allow_missing_leaf &&
                (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)) return true;
            ec = {static_cast<int>(error), std::system_category()};
            return false;
        }
        BY_HANDLE_FILE_INFORMATION info{};
        const bool read_info = ::GetFileInformationByHandle(handle, &info) != 0;
        const auto error = read_info ? ERROR_SUCCESS : ::GetLastError();
        ::CloseHandle(handle);
        if (!read_info) { ec = {static_cast<int>(error), std::system_category()}; return false; }
        if ((info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
            return Fail(ec, std::errc::too_many_symbolic_link_levels);
        if (!leaf && (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            return Fail(ec, std::errc::not_a_directory);
        if (leaf && ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) != directory_leaf)
            return Fail(ec, directory_leaf ? std::errc::not_a_directory : std::errc::is_a_directory);
    }
    return true;
}
#endif

bool ReadRegularFile(const fs::path& path, std::size_t minimum, std::size_t maximum,
                     std::vector<std::uint8_t>& bytes, std::error_code& ec) {
    ec.clear();
#ifdef _WIN32
    if (!CheckWindowsPath(path, false, false, ec)) return false;
    HANDLE file = ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        ec = {static_cast<int>(::GetLastError()), std::system_category()};
        return false;
    }
    BY_HANDLE_FILE_INFORMATION info{};
    LARGE_INTEGER size{};
    const bool regular = ::GetFileInformationByHandle(file, &info) &&
                         (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == 0 &&
                         ::GetFileSizeEx(file, &size);
    if (!regular || size.QuadPart < 0 || static_cast<std::uint64_t>(size.QuadPart) < minimum ||
        static_cast<std::uint64_t>(size.QuadPart) > maximum) {
        ::CloseHandle(file);
        ec = std::make_error_code(std::errc::bad_message);
        return false;
    }
    bytes.resize(static_cast<std::size_t>(size.QuadPart));
    std::size_t at = 0;
    while (at < bytes.size()) {
        DWORD count = 0;
        const DWORD want = static_cast<DWORD>(std::min<std::size_t>(bytes.size() - at, MAXDWORD));
        if (!::ReadFile(file, bytes.data() + at, want, &count, nullptr) || count == 0) break;
        at += count;
    }
    const DWORD error = ::GetLastError();
    const bool closed = ::CloseHandle(file) != 0;
    if (at != bytes.size() || !closed) {
        ec = at != bytes.size() && error != ERROR_SUCCESS
                 ? std::error_code{static_cast<int>(error), std::system_category()}
                 : std::make_error_code(std::errc::io_error);
        return false;
    }
    return true;
#else
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return SystemError(ec);
    struct stat info{};
    if (::fstat(fd, &info) != 0 || !S_ISREG(info.st_mode) || info.st_size < 0 ||
        static_cast<std::uint64_t>(info.st_size) < minimum ||
        static_cast<std::uint64_t>(info.st_size) > maximum) {
        ::close(fd);
        return Fail(ec, std::errc::bad_message);
    }
    bytes.resize(static_cast<std::size_t>(info.st_size));
    std::size_t at = 0;
    while (at < bytes.size()) {
        const auto count = ::read(fd, bytes.data() + at, bytes.size() - at);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        at += static_cast<std::size_t>(count);
    }
    const bool closed = ::close(fd) == 0;
    if (at != bytes.size() || !closed) return Fail(ec, std::errc::io_error);
    return true;
#endif
}

bool AtomicWrite(const fs::path& destination, std::span<const std::uint8_t> bytes, bool replace,
                 std::error_code& ec) {
    ec.clear();
#ifdef _WIN32
    if (!CheckWindowsPath(destination, true, false, ec)) return false;
    static std::atomic<std::uint64_t> serial{};
    fs::path temporary;
    HANDLE file = INVALID_HANDLE_VALUE;
    for (unsigned attempt = 0; attempt < 32 && file == INVALID_HANDLE_VALUE; ++attempt) {
        temporary = fs::path(destination.native() + L".tmp-" +
            std::to_wstring(::GetCurrentProcessId()) + L"-" +
            std::to_wstring(serial.fetch_add(1, std::memory_order_relaxed)));
        file = ::CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                             FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (file == INVALID_HANDLE_VALUE && ::GetLastError() != ERROR_FILE_EXISTS &&
            ::GetLastError() != ERROR_ALREADY_EXISTS) break;
    }
    if (file == INVALID_HANDLE_VALUE) {
        ec = {static_cast<int>(::GetLastError()), std::system_category()};
        return false;
    }
    bool ok = true;
    std::size_t at = 0;
    while (at < bytes.size()) {
        DWORD count = 0;
        const DWORD want = static_cast<DWORD>(std::min<std::size_t>(bytes.size() - at, MAXDWORD));
        if (!::WriteFile(file, bytes.data() + at, want, &count, nullptr) || count == 0) {
            ec = {static_cast<int>(::GetLastError()), std::system_category()};
            ok = false;
            break;
        }
        at += count;
    }
    if (ok && !::FlushFileBuffers(file)) {
        ec = {static_cast<int>(::GetLastError()), std::system_category()};
        ok = false;
    }
    if (!::CloseHandle(file) && ok) {
        ec = {static_cast<int>(::GetLastError()), std::system_category()};
        ok = false;
    }
    if (ok) {
        const DWORD flags = MOVEFILE_WRITE_THROUGH | (replace ? MOVEFILE_REPLACE_EXISTING : 0);
        if (!::MoveFileExW(temporary.c_str(), destination.c_str(), flags)) {
            ec = {static_cast<int>(::GetLastError()), std::system_category()};
            ok = false;
        }
    }
    if (!ok) ::DeleteFileW(temporary.c_str());
    return ok;
#else
    std::string pattern = destination.string() + ".tmp-XXXXXX";
    const int fd = ::mkstemp(pattern.data());
    if (fd < 0) return SystemError(ec);
    bool ok = ::fcntl(fd, F_SETFD, FD_CLOEXEC) == 0;
    if (!ok) SystemError(ec);
    std::size_t at = 0;
    while (ok && at < bytes.size()) {
        const auto count = ::write(fd, bytes.data() + at, bytes.size() - at);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { if (count == 0) errno = EIO; ok = SystemError(ec); break; }
        at += static_cast<std::size_t>(count);
    }
    if (ok && ::fsync(fd) != 0) ok = SystemError(ec);
    if (::close(fd) != 0 && ok) ok = SystemError(ec);
    if (ok) {
        if (replace) {
            if (::rename(pattern.c_str(), destination.c_str()) != 0) ok = SystemError(ec);
        } else if (::link(pattern.c_str(), destination.c_str()) != 0) {
            ok = SystemError(ec);
        } else {
            ::unlink(pattern.c_str());
        }
    }
    if (!ok) { ::unlink(pattern.c_str()); return false; }
    const auto parent = destination.parent_path().empty() ? fs::path{"."} : destination.parent_path();
    const int directory = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory < 0) return SystemError(ec);
    if (::fsync(directory) != 0) ok = SystemError(ec);
    if (::close(directory) != 0 && ok) ok = SystemError(ec);
    return ok;
#endif
}

bool HasAnyTaggedCycleHome(const fs::path& base_journal, const CharacterKey& expected,
                           std::error_code& ec) {
    ec.clear();
    static constexpr char hex[] = "0123456789abcdef";
    std::string character;
    for (const auto byte : expected.character) {
        character.push_back(hex[byte >> 4]);
        character.push_back(hex[byte & 15]);
    }
    const auto prefix = "home-" + std::to_string(expected.slot) + "-" + character + "-cycle-";
    const auto directory = base_journal.parent_path();
    std::error_code status_error;
    const auto directory_status = fs::symlink_status(directory, status_error);
    if (status_error == std::errc::no_such_file_or_directory) return false;
    if (status_error) { ec = status_error; return true; }
#ifdef _WIN32
    if (!CheckWindowsPath(directory, false, true, ec)) return true;
#endif
    if (directory_status.type() != fs::file_type::directory)
        return Fail(ec, std::errc::not_a_directory);
    fs::directory_iterator it(directory, status_error), end;
    if (status_error) { ec = status_error; return true; }
    for (; it != end; it.increment(status_error)) {
        if (status_error) { ec = status_error; return true; }
        if (it->path().filename().string().starts_with(prefix)) return true;
    }
    if (status_error) { ec = status_error; return true; }
    return false;
}

} // namespace

std::optional<Journal> PrepareNextParty(const Journal& previous, PartyId party,
                                        Destination origin, Destination fallback) {
    if (!Valid(previous) || previous.phase != Phase::Complete || previous.party == party)
        return std::nullopt;
    Journal next;
    next.key = previous.key;
    next.party = party;
    next.origin = origin;
    next.fallback = fallback;
    next.retained = Claims(previous);
    return Valid(next) ? std::optional{std::move(next)} : std::nullopt;
}

std::optional<Journal> MigrateCompletedCycleHome(const Journal& previous,
                                                  const CharacterKey& new_key,
                                                  PartyId party, Destination origin,
                                                  Destination fallback) {
    if (!Valid(previous) || previous.phase != Phase::Complete || !ValidKey(new_key) ||
        !SameCharacter(previous.key, new_key) || party == previous.party ||
        (new_key.cycle & 0x80000000u) == 0) return {};
    const auto next_ordinal = new_key.cycle & 0x7fffffffu;
    const auto previous_ordinal = previous.key.cycle & 0x7fffffffu;
    const bool native_boundary = previous.key.cycle == 6 && next_ordinal == 1;
    const bool managed_boundary = (previous.key.cycle & 0x80000000u) != 0 &&
                                  next_ordinal == previous_ordinal + 1;
    if (!native_boundary && !managed_boundary) return {};
    // A cycle boundary starts a new claim namespace. The completed prior journal is
    // retained byte-for-byte at its old path, but its event claims must not suppress
    // fresh boss/item delivery in this NG epoch.
    Journal migrated;
    migrated.key = new_key;
    migrated.party = party;
    migrated.origin = origin;
    migrated.fallback = fallback;
    return Valid(migrated) ? std::optional{std::move(migrated)} : std::nullopt;
}

void ReturnSaveTracker::Arm(const CharacterKey& key, std::uint64_t baseline_generation,
                            const Destination& expected_destination, bool native_return_selected) {
    key_ = {};
    expected_destination_ = {};
    baseline_generation_ = 0;
    serialization_generation_ = 0;
    buffer_ = 0;
    serialized_sha256_.clear();
    receipt_path_.clear();
    receipt_sha256_.clear();
    receipt_generation_ = 0;
    receipt_bytes_ = 0;
    active_ = false;
    saved_ = false;
    destination_confirmed_ = false;
    native_return_selected_ = false;
    armed_ = ValidKey(key);
    if (armed_) {
        key_ = key;
        expected_destination_ = expected_destination;
        baseline_generation_ = baseline_generation;
        serialization_generation_ = baseline_generation;
        native_return_selected_ = native_return_selected;
    }
}

bool ReturnSaveTracker::SerializeBegin(const CharacterKey& key, std::uint8_t slot,
                                       std::uintptr_t buffer,
                                       std::uint64_t current_generation) {
    active_ = false;
    buffer_ = 0;
    serialized_sha256_.clear();
    saved_ = false;
    receipt_path_.clear();
    receipt_sha256_.clear();
    receipt_generation_ = 0;
    receipt_bytes_ = 0;
    if (!armed_ || !native_return_selected_ || !destination_confirmed_ ||
        key != key_ || slot != key_.slot || buffer == 0 ||
        current_generation < baseline_generation_) return false;
    serialization_generation_ = current_generation;
    active_ = true;
    buffer_ = buffer;
    return true;
}

bool ReturnSaveTracker::SerializeEnd(const CharacterKey& live_key,
                                     const CharacterKey& serialized_key, std::uint8_t slot,
                                     std::uintptr_t buffer, bool success,
                                     std::string_view sha256) {
    const auto valid_sha = [](std::string_view hash) {
        return hash.size() == 64 && std::ranges::all_of(hash, [](char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        });
    };
    const bool matched = armed_ && active_ && live_key == key_ && serialized_key == key_ &&
                         slot == key_.slot && buffer == buffer_ && success && valid_sha(sha256);
    active_ = false;
    buffer_ = 0;
    serialized_sha256_.clear();
    saved_ = false;
    if (!matched) return false;
    serialized_sha256_.assign(sha256);
    return true;
}

void ReturnSaveTracker::AbortSerialization() {
    active_ = false;
    buffer_ = 0;
    serialized_sha256_.clear();
    saved_ = false;
}

bool ReturnSaveTracker::ObserveReceipt(const CharacterKey& persisted_key,
                                       std::string_view receipt_path,
                                       std::uint64_t generation, std::uint64_t bytes,
                                       std::string_view receipt_sha256,
                                       std::string_view persisted_sha256) {
    const auto valid_sha = [](std::string_view hash) {
        return hash.size() == 64 && std::ranges::all_of(hash, [](char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        });
    };
    const auto expected_path = fs::path{key_.save_dir} /
                               ("userdata000" + std::to_string(key_.slot));
    if (!armed_ || active_ || persisted_key != key_ || generation <= serialization_generation_ ||
        bytes == 0 || receipt_path != expected_path.string() || serialized_sha256_.empty() ||
        !valid_sha(receipt_sha256) || !valid_sha(persisted_sha256) ||
        receipt_sha256 != persisted_sha256 || receipt_sha256 != serialized_sha256_) return false;
    saved_ = true;
    receipt_path_.assign(receipt_path);
    receipt_sha256_.assign(receipt_sha256);
    receipt_generation_ = generation;
    receipt_bytes_ = bytes;
    return true;
}

bool ReturnSaveTracker::ConfirmDestination(const CharacterKey& key, const Destination& observed,
                                           const Destination& resolved_native_point,
                                           bool own_world, bool player_ready) {
    if (!armed_ || !native_return_selected_ || key != key_ || !own_world || !player_ready ||
        resolved_native_point.map == 0 || resolved_native_point.map != expected_destination_.map ||
        resolved_native_point.lamp < 0 || resolved_native_point.lamp != expected_destination_.lamp ||
        observed.map != resolved_native_point.map || !std::isfinite(observed.x) ||
        !std::isfinite(observed.y) || !std::isfinite(observed.z) ||
        !std::isfinite(resolved_native_point.x) || !std::isfinite(resolved_native_point.y) ||
        !std::isfinite(resolved_native_point.z) || observed.x != resolved_native_point.x ||
        observed.y != resolved_native_point.y || observed.z != resolved_native_point.z) return false;
    if (!destination_confirmed_) {
        active_ = false;
        buffer_ = 0;
        serialized_sha256_.clear();
        receipt_path_.clear();
        receipt_sha256_.clear();
        receipt_generation_ = 0;
        receipt_bytes_ = 0;
        saved_ = false;
    }
    destination_confirmed_ = true;
    return true;
}

std::optional<Destination> ReturnSaveTracker::ExpectedDestination() const {
    return armed_ ? std::optional{expected_destination_} : std::nullopt;
}

std::optional<ReturnSaveProof> ReturnSaveTracker::ProofFor(
    const CharacterKey& persisted_key, std::string_view persisted_sha256) const {
    if (!armed_ || !saved_ || !destination_confirmed_ || persisted_key != key_ ||
        persisted_sha256 != serialized_sha256_ || receipt_sha256_ != persisted_sha256) return {};
    return ReturnSaveProof{persisted_key, receipt_path_, receipt_sha256_, persisted_sha256,
                           serialized_sha256_, receipt_generation_, receipt_bytes_, true, true};
}

void ReturnSaveTracker::Disarm() {
    key_ = {};
    expected_destination_ = {};
    baseline_generation_ = serialization_generation_ = receipt_generation_ = receipt_bytes_ = 0;
    buffer_ = 0;
    serialized_sha256_.clear();
    receipt_path_.clear();
    receipt_sha256_.clear();
    armed_ = active_ = saved_ = destination_confirmed_ = native_return_selected_ = false;
}

CharacterLease::CharacterLease(std::filesystem::path path, std::intptr_t fd)
    : path_(std::move(path)), fd_(fd) {}
CharacterLease::~CharacterLease() { CloseHandleValue(fd_); }
CharacterLease::CharacterLease(CharacterLease&& other) noexcept
    : path_(std::move(other.path_)), fd_(std::exchange(other.fd_, InvalidHandle)) {}
CharacterLease& CharacterLease::operator=(CharacterLease&& other) noexcept {
    if (this != &other) {
        CloseHandleValue(fd_);
        path_ = std::move(other.path_);
        fd_ = std::exchange(other.fd_, InvalidHandle);
    }
    return *this;
}

std::optional<CharacterLease> CharacterLease::Acquire(const fs::path& path, std::error_code& ec) {
    ec.clear();
    if (path.empty() || path.filename().empty()) { Fail(ec, std::errc::invalid_argument); return {}; }
    if (!path.parent_path().empty()) {
#ifdef _WIN32
        if (!CheckWindowsPath(path.parent_path(), true, true, ec)) return {};
#endif
        // Create only one level so its parent is known to exist and can be flushed.
        fs::create_directory(path.parent_path(), ec);
        if (ec) return {};
        const auto parent = path.parent_path().parent_path().empty() ? fs::path{"."}
                                                                   : path.parent_path().parent_path();
#ifdef _WIN32
        (void)parent;
        if (!CheckWindowsPath(path, true, false, ec)) return {};
        const fs::path lock = path.wstring() + L".lock";
        if (!CheckWindowsPath(lock, true, false, ec)) return {};
        HANDLE handle = ::CreateFileW(lock.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                      OPEN_ALWAYS, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (handle == INVALID_HANDLE_VALUE) {
            ec = {static_cast<int>(::GetLastError()), std::system_category()};
            return {};
        }
        BY_HANDLE_FILE_INFORMATION info{};
        if (!::GetFileInformationByHandle(handle, &info) ||
            (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0) {
            ::CloseHandle(handle);
            Fail(ec, std::errc::invalid_argument);
            return {};
        }
        return CharacterLease{path, reinterpret_cast<std::intptr_t>(handle)};
#else
        const int directory = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (directory < 0) { SystemError(ec); return {}; }
        const bool flushed = ::fsync(directory) == 0;
        if (!flushed) SystemError(ec);
        const bool closed = ::close(directory) == 0;
        if (!closed && flushed) SystemError(ec);
        if (!flushed || !closed) return {};
#endif
    }
#ifdef _WIN32
    if (!CheckWindowsPath(path, true, false, ec)) return {};
    const fs::path lock = path.wstring() + L".lock";
    if (!CheckWindowsPath(lock, true, false, ec)) return {};
    HANDLE handle = ::CreateFileW(lock.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                  OPEN_ALWAYS, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        ec = {static_cast<int>(::GetLastError()), std::system_category()};
        return {};
    }
    BY_HANDLE_FILE_INFORMATION info{};
    if (!::GetFileInformationByHandle(handle, &info) ||
        (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0) {
        ::CloseHandle(handle);
        Fail(ec, std::errc::invalid_argument);
        return {};
    }
    return CharacterLease{path, reinterpret_cast<std::intptr_t>(handle)};
#else
    const auto lock = fs::path(path.string() + ".lock");
    const int fd = ::open(lock.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0600);
    if (fd < 0) { SystemError(ec); return {}; }
    struct stat info{};
    if (::fstat(fd, &info) != 0 || !S_ISREG(info.st_mode)) {
        Fail(ec, std::errc::invalid_argument);
        ::close(fd);
        return {};
    }
    if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
        SystemError(ec);
        ::close(fd);
        return {};
    }
    return CharacterLease{path, fd};
#endif
}

std::optional<Journal> LoadJournal(const fs::path& path, const CharacterKey& expected,
                                  std::error_code& ec) {
    ec.clear();
    if (!ValidKey(expected)) { Fail(ec, std::errc::invalid_argument); return {}; }
    std::vector<std::uint8_t> bytes;
    if (!ReadRegularFile(path, 16, MaxRecordBytes, bytes, ec)) return {};
    auto record = Decode(bytes);
    if (!record || record->key != expected) {
        Fail(ec, std::errc::bad_message);
        return {};
    }
    return record;
}

fs::path ManagedCycleEpochPath(const fs::path& journal) {
#ifdef _WIN32
    return fs::path(journal.native() + L".cycle");
#else
    return fs::path(journal.string() + ".cycle");
#endif
}

bool HasUnresolvedPriorCycleHome(const fs::path& journal, const CharacterKey& expected,
                                 std::error_code& ec) {
    ec.clear();
    if (!ValidKey(expected) || (expected.cycle & 0x80000000u) == 0)
        return Fail(ec, std::errc::invalid_argument);
    static constexpr char hex[] = "0123456789abcdef";
    std::string character;
    character.reserve(expected.character.size() * 2);
    for (const auto byte : expected.character) {
        character.push_back(hex[byte >> 4]);
        character.push_back(hex[byte & 15]);
    }
    std::string cycle_hex(8, '0');
    for (std::size_t i = 0; i < cycle_hex.size(); ++i)
        cycle_hex[cycle_hex.size() - 1 - i] = hex[(expected.cycle >> (i * 4)) & 15];
    const auto base_name = "home-" + std::to_string(expected.slot) + "-" + character + ".bin";
    const auto current_name = "home-" + std::to_string(expected.slot) + "-" + character +
                              "-cycle-" + cycle_hex + ".bin";
    if (journal.filename().string() != current_name)
        return Fail(ec, std::errc::invalid_argument);

    const auto directory = journal.parent_path();
    std::error_code status_error;
    const auto directory_status = fs::symlink_status(directory, status_error);
    if (status_error == std::errc::no_such_file_or_directory) return false;
    if (status_error) { ec = status_error; return true; }
#ifdef _WIN32
    if (!CheckWindowsPath(directory, false, true, ec)) return true;
#endif
    if (directory_status.type() != fs::file_type::directory)
        return Fail(ec, std::errc::not_a_directory);
    fs::directory_iterator it(directory, status_error), end;
    if (status_error) { ec = status_error; return true; }
    for (; it != end; it.increment(status_error)) {
        if (status_error) { ec = status_error; return true; }
        const auto name = it->path().filename().string();
        if (name.ends_with(".bin.lock")) continue; // CharacterLease coordination file, not a home record.
        std::uint32_t cycle = 0;
        if (name == base_name) {
            cycle = 6;
        } else {
            const auto prefix = "home-" + std::to_string(expected.slot) + "-" + character + "-cycle-";
            if (!name.starts_with(prefix)) continue;
            const auto suffix = std::string_view{name}.substr(prefix.size());
            if (suffix.size() != 12 || suffix.substr(8) != ".bin")
                return Fail(ec, std::errc::bad_message);
            for (std::size_t i = 0; i < 8; ++i) {
                const char c = suffix[i];
                const unsigned digit = c >= '0' && c <= '9' ? c - '0' :
                    (c >= 'a' && c <= 'f' ? c - 'a' + 10 : 16);
                if (digit == 16) return Fail(ec, std::errc::bad_message);
                cycle = (cycle << 4) | digit;
            }
            if ((cycle & 0x80000000u) == 0) return Fail(ec, std::errc::bad_message);
        }
        const auto candidate = it->path();
        if (candidate.filename().string() == current_name) continue;
        std::error_code candidate_error;
        const auto candidate_status = fs::symlink_status(candidate, candidate_error);
        if (candidate_error || candidate_status.type() != fs::file_type::regular) {
            ec = candidate_error ? candidate_error : std::make_error_code(std::errc::bad_message);
            return true;
        }
        auto prior_key = expected;
        prior_key.cycle = cycle;
        auto lease = CharacterLease::Acquire(candidate, candidate_error);
        if (!lease) { ec = candidate_error; return true; }
        const auto record = LoadJournal(candidate, prior_key, candidate_error);
        if (!record) { ec = candidate_error ? candidate_error : std::make_error_code(std::errc::bad_message); return true; }
        if (record->phase != Phase::Complete) {
            ec = std::make_error_code(std::errc::operation_in_progress);
            return true;
        }
    }
    if (status_error) { ec = status_error; return true; }
    return false;
}

std::optional<ManagedCycleEpoch> LoadManagedCycleEpoch(
    const fs::path& journal, const CharacterKey& expected, std::error_code& ec) {
    ec.clear();
    if (!ValidKey(expected)) { Fail(ec, std::errc::invalid_argument); return {}; }
    std::vector<std::uint8_t> bytes;
    if (!ReadRegularFile(ManagedCycleEpochPath(journal), 16, 2048, bytes, ec)) return {};
    auto record = DecodeCycleRecord(bytes);
    if (!record || !SameCharacter(record->key, expected)) {
        Fail(ec, std::errc::bad_message);
        return {};
    }
    return record;
}

bool StoreManagedCycleEpoch(const CharacterLease& lease, const ManagedCycleEpoch& record,
                            std::error_code& ec) {
    ec.clear();
    if (lease.fd_ < 0) return Fail(ec, std::errc::bad_file_descriptor);
    if (!ValidCycleRecord(record)) return Fail(ec, std::errc::invalid_argument);
    const auto path = ManagedCycleEpochPath(lease.path_);
    std::vector<std::uint8_t> prior_bytes;
    std::error_code read_error;
    if (ReadRegularFile(path, 16, 2048, prior_bytes, read_error)) {
        const auto prior = DecodeCycleRecord(prior_bytes);
        if (!prior || !SameCharacter(prior->key, record.key))
            return Fail(ec, std::errc::bad_message);
        bool allowed = false;
        if (prior->stage == ManagedCycleStage::Stable && record.stage == ManagedCycleStage::Intent)
            allowed = prior->key == record.key && prior->epoch == record.epoch &&
                      record.target_key_cycle > prior->key.cycle;
        else if (prior->stage == ManagedCycleStage::Intent && record.stage == ManagedCycleStage::Applied)
            allowed = prior->key == record.key && prior->target_key_cycle == record.target_key_cycle &&
                      prior->baseline_sha256 == record.baseline_sha256 &&
                      prior->baseline_generation == record.baseline_generation;
        else if (prior->stage == ManagedCycleStage::Applied &&
                 record.stage == ManagedCycleStage::TransitionSerialized)
            allowed = prior->key == record.key && prior->target_key_cycle == record.target_key_cycle &&
                      prior->baseline_sha256 == record.baseline_sha256 &&
                      prior->baseline_generation == record.baseline_generation;
        else if (prior->stage == ManagedCycleStage::Applied &&
                 record.stage == ManagedCycleStage::Applied)
            allowed = prior->key == record.key && prior->target_key_cycle == record.target_key_cycle &&
                      prior->baseline_sha256 == record.baseline_sha256;
        else if (prior->stage == ManagedCycleStage::TransitionSerialized &&
                 record.stage == ManagedCycleStage::Stable) {
            const bool committed_transition = record.key.cycle == prior->target_key_cycle &&
                record.epoch == (prior->target_key_cycle & 0x7fffffffu) &&
                record.serialized_sha256 == prior->serialized_sha256 &&
                record.serialized_generation > prior->serialized_generation;
            const bool canceled_unpersisted_transition = record.key == prior->key &&
                record.epoch == prior->epoch && record.serialized_sha256 == prior->baseline_sha256;
            allowed = committed_transition || canceled_unpersisted_transition;
        }
        else if (prior->stage == ManagedCycleStage::Applied && record.stage == ManagedCycleStage::Stable)
            allowed = record.key == prior->key && record.epoch == prior->epoch &&
                      record.serialized_sha256 == prior->baseline_sha256;
        else if (prior->stage == ManagedCycleStage::Intent && record.stage == ManagedCycleStage::Stable)
            allowed = record.key == prior->key && record.epoch == prior->epoch &&
                      record.serialized_sha256 == prior->baseline_sha256;
        else if (prior->stage == ManagedCycleStage::TransitionSerialized &&
                 record.stage == ManagedCycleStage::Applied)
            allowed = prior->key == record.key && prior->target_key_cycle == record.target_key_cycle &&
                      prior->serialized_sha256 != prior->baseline_sha256 &&
                      record.baseline_sha256 == prior->serialized_sha256;
        else if (prior->stage == ManagedCycleStage::Stable &&
                 (prior->key.cycle & 0x80000000u) && record.stage == ManagedCycleStage::Stable)
            allowed = prior->key == record.key && prior->epoch == record.epoch &&
                      prior->serialized_sha256 == record.serialized_sha256;
        else if (prior->stage == ManagedCycleStage::Stable &&
                 (prior->key.cycle & 0x80000000u) && record.stage == ManagedCycleStage::SaveSerialized)
            allowed = prior->key == record.key && prior->epoch == record.epoch &&
                      prior->serialized_sha256 == record.baseline_sha256 &&
                      record.target_key_cycle == prior->key.cycle;
        else if (prior->stage == ManagedCycleStage::SaveSerialized && record.stage == ManagedCycleStage::Stable)
            allowed = prior->key == record.key && prior->epoch == record.epoch &&
                      (record.serialized_sha256 == prior->baseline_sha256 ||
                       (record.serialized_sha256 == prior->serialized_sha256 &&
                        record.serialized_generation > prior->serialized_generation));
        else if (prior->stage == ManagedCycleStage::SaveSerialized &&
                 record.stage == ManagedCycleStage::SaveSerialized)
            allowed = prior->key == record.key && prior->epoch == record.epoch &&
                      record.target_key_cycle == prior->target_key_cycle &&
                      (record.baseline_sha256 == prior->baseline_sha256 ||
                       record.baseline_sha256 == prior->serialized_sha256);
        if (!allowed) return Fail(ec, std::errc::invalid_argument);
    } else if (read_error != std::errc::no_such_file_or_directory) {
        ec = read_error;
        return false;
    } else if (record.stage != ManagedCycleStage::Stable ||
               !((record.epoch == 0 && record.key.cycle == 6) ||
                 (record.epoch == 1 && record.key.cycle == 0x80000001u &&
                  ValidSha256(record.serialized_sha256)))) {
        return Fail(ec, std::errc::invalid_argument);
    } else {
        // A fresh cap-7 anchor is legal only when no prior home journal exists, including
        // a completed journal. A native 6->7 transition may seed its pre-increment baseline
        // alongside that same-cycle journal, but only when it is already Complete; this
        // sidecar never modifies or erases its claims.
        std::error_code status_error;
        const auto status = fs::symlink_status(lease.path_, status_error);
        const bool journal_exists = !status_error && status.type() != fs::file_type::not_found;
        if (record.epoch == 1 && journal_exists)
            return Fail(ec, std::errc::file_exists);
        if (status_error && status_error != std::errc::no_such_file_or_directory)
            return Fail(ec, std::errc::io_error);
        if (record.epoch == 1) {
            std::error_code history_error;
            if (HasAnyTaggedCycleHome(lease.path_, record.key, history_error))
                return Fail(ec, std::errc::file_exists);
            if (history_error) { ec = history_error; return false; }
        }
        if (record.epoch == 0 && journal_exists) {
            std::error_code journal_error;
            const auto previous = LoadJournal(lease.path_, record.key, journal_error);
            if (!previous || previous->phase != Phase::Complete)
                return Fail(ec, std::errc::file_exists);
        }
    }
    const auto bytes = EncodeCycleRecord(record);
    return AtomicWrite(path, bytes, true, ec);
}

std::optional<ManagedCycleEpoch> BeginManagedCycleIntent(
    const ManagedCycleEpoch& stable, std::uint8_t previous_native_cycle,
    std::uint64_t generation, std::string_view preincrement_sha256) {
    if (!ValidCycleRecord(stable) || stable.stage != ManagedCycleStage::Stable ||
        previous_native_cycle < 6 || previous_native_cycle > 7 ||
        (previous_native_cycle == 6 && stable.key.cycle != 6) ||
        (previous_native_cycle == 7 &&
         (stable.key.cycle & 0x80000000u) == 0) || !ValidSha256(preincrement_sha256)) return {};
    const auto ordinal = stable.epoch == 0 ? 1 : stable.epoch + 1;
    if (ordinal > 0x7fffffffULL) return {};
    auto intent = stable;
    intent.target_key_cycle = 0x80000000u | static_cast<std::uint32_t>(ordinal);
    intent.stage = ManagedCycleStage::Intent;
    intent.baseline_generation = generation;
    intent.serialized_generation = 0;
    intent.baseline_sha256 = preincrement_sha256;
    intent.serialized_sha256.clear();
    return intent;
}

std::optional<ManagedCycleEpoch> ObserveManagedCycleApplied(
    const ManagedCycleEpoch& intent, std::uint8_t previous_native_cycle,
    std::uint8_t live_native_cycle) {
    if (!ValidCycleRecord(intent) || intent.stage != ManagedCycleStage::Intent ||
        previous_native_cycle < 6 || previous_native_cycle > 7 ||
        live_native_cycle != std::min<unsigned>(previous_native_cycle + 1, 7)) return {};
    auto applied = intent;
    applied.stage = ManagedCycleStage::Applied;
    return applied;
}

std::optional<ManagedCycleEpoch> ObserveManagedCycleSerializeEnd(
    const ManagedCycleEpoch& source, std::string_view serialized_sha256,
    std::uint64_t generation) {
    if (!ValidCycleRecord(source) || !ValidSha256(serialized_sha256) ||
        generation < source.baseline_generation) return {};
    auto serialized = source;
    if (source.stage == ManagedCycleStage::Applied) {
        serialized.stage = ManagedCycleStage::TransitionSerialized;
    } else if ((source.stage == ManagedCycleStage::Stable ||
                source.stage == ManagedCycleStage::SaveSerialized) &&
               (source.key.cycle & 0x80000000u) != 0) {
        serialized.stage = ManagedCycleStage::SaveSerialized;
        serialized.baseline_sha256 = source.stage == ManagedCycleStage::Stable
                                         ? source.serialized_sha256
                                         : source.baseline_sha256;
        serialized.baseline_generation = generation;
        serialized.target_key_cycle = source.key.cycle;
    } else {
        return {};
    }
    serialized.serialized_sha256 = serialized_sha256;
    serialized.serialized_generation = generation;
    return ValidCycleRecord(serialized) ? std::optional{serialized} : std::nullopt;
}

std::optional<ManagedCycleEpoch> RebaseManagedCycleForResume(
    const ManagedCycleEpoch& persisted, std::string_view selected_sha256,
    std::uint64_t current_process_generation) {
    if (!ValidCycleRecord(persisted) || !ValidSha256(selected_sha256)) return {};
    auto rebased = persisted;
    const auto make_stable = [&] {
        auto stable = persisted;
        stable.stage = ManagedCycleStage::Stable;
        stable.target_key_cycle = stable.epoch == 0 ? 0 : stable.key.cycle;
        stable.baseline_generation = 0;
        stable.serialized_generation = current_process_generation;
        stable.baseline_sha256.clear();
        stable.serialized_sha256 = std::string(selected_sha256);
        return stable;
    };
    if (persisted.stage == ManagedCycleStage::Stable && persisted.epoch != 0) {
        if (selected_sha256 != persisted.serialized_sha256) return {};
        rebased.serialized_generation = current_process_generation;
    } else if (persisted.stage == ManagedCycleStage::Intent ||
               persisted.stage == ManagedCycleStage::Applied) {
        // The selected save is still the pre-mutation image, so the candidate epoch
        // was never durably established in this process. Resume the prior namespace.
        if (selected_sha256 != persisted.baseline_sha256) return {};
        rebased = make_stable();
    } else if (persisted.stage == ManagedCycleStage::TransitionSerialized) {
        if (persisted.serialized_sha256 == persisted.baseline_sha256) {
            // For native 7->7 the unchanged bytes cannot reveal whether the increment
            // was durably committed before shutdown. Do not infer either outcome.
            return {};
        } else if (selected_sha256 == persisted.baseline_sha256) {
            // A pre-hash after restart proves the native increment was not persisted.
            rebased = make_stable();
        } else if (selected_sha256 == persisted.serialized_sha256) {
            // The post-image is recognized, but a fresh SerializeEnd and receipt are
            // still required in this process before promoting the managed epoch.
            rebased.stage = ManagedCycleStage::Applied;
            rebased.baseline_sha256 = std::string(selected_sha256);
            rebased.baseline_generation = current_process_generation;
            rebased.serialized_generation = 0;
            rebased.serialized_sha256.clear();
        } else return {};
    } else if (persisted.stage == ManagedCycleStage::SaveSerialized) {
        if (selected_sha256 == persisted.baseline_sha256) {
            // Ordinary save did not become the selected image; retain the stable epoch.
            rebased = make_stable();
        } else if (selected_sha256 == persisted.serialized_sha256) {
            // Recognize the selected image but require fresh serialization provenance
            // and a receipt in this process before accepting it as the epoch baseline.
            rebased.baseline_sha256 = std::string(selected_sha256);
            rebased.baseline_generation = current_process_generation;
            rebased.serialized_generation = current_process_generation;
        } else {
            return {};
        }
    } else {
        return {};
    }
    return ValidCycleRecord(rebased) ? std::optional{rebased} : std::nullopt;
}

std::optional<ManagedCycleEpoch> CommitManagedCycleEpoch(
    const ManagedCycleEpoch& serialized, std::uint64_t receipt_generation,
    std::string_view receipt_sha256, std::string_view persisted_sha256) {
    if (!ValidCycleRecord(serialized) ||
        (serialized.stage != ManagedCycleStage::TransitionSerialized &&
         serialized.stage != ManagedCycleStage::SaveSerialized) ||
        receipt_generation <= serialized.serialized_generation ||
        !ValidSha256(receipt_sha256) || !ValidSha256(persisted_sha256) ||
        receipt_sha256 != serialized.serialized_sha256 ||
        persisted_sha256 != serialized.serialized_sha256) return {};
    auto committed = serialized;
    if (serialized.stage == ManagedCycleStage::TransitionSerialized) {
        committed.key.cycle = serialized.target_key_cycle;
        committed.epoch = serialized.target_key_cycle & 0x7fffffffu;
    }
    committed.stage = ManagedCycleStage::Stable;
    committed.target_key_cycle = committed.epoch == 0 ? 0 : committed.key.cycle;
    committed.baseline_generation = 0;
    committed.serialized_generation = receipt_generation;
    committed.baseline_sha256.clear();
    return committed;
}

fs::path WorldSnapshotPath(const fs::path& journal, const PartyId& party) {
#ifdef _WIN32
    static constexpr wchar_t hex[] = L"0123456789abcdef";
    std::wstring id;
    id.reserve(party.size() * 2);
    for (const auto byte : party) {
        id.push_back(hex[byte >> 4]);
        id.push_back(hex[byte & 15]);
    }
    return fs::path(journal.native() + L".world-" + id);
#else
    static constexpr char hex[] = "0123456789abcdef";
    std::string id;
    id.reserve(party.size() * 2);
    for (const auto byte : party) {
        id.push_back(hex[byte >> 4]);
        id.push_back(hex[byte & 15]);
    }
    return fs::path(journal.string() + ".world-" + id);
#endif
}

std::optional<std::vector<std::uint8_t>> LoadWorldSnapshot(
    const fs::path& journal, const CharacterKey& expected, const PartyId& party,
    const SnapshotRef& ref, std::error_code& ec) {
    ec.clear();
    if (!ValidKey(expected) || ref.bytes == 0 || ref.bytes > MaxWorldSnapshotBytes ||
        !std::ranges::any_of(party, [](auto byte) { return byte != 0; })) {
        Fail(ec, std::errc::invalid_argument);
        return {};
    }
    std::vector<std::uint8_t> bytes;
    if (!ReadRegularFile(WorldSnapshotPath(journal, party), 24,
                         MaxWorldSnapshotBytes + 2048, bytes, ec)) return {};
    if (std::memcmp(bytes.data(), "BBWS", 4)) {
        Fail(ec, std::errc::bad_message);
        return {};
    }
    Reader header{bytes, 4};
    const auto version = header.Get(4), body_size = header.Get(4), body_crc = header.Get(4);
    if (version != 1 || body_size != bytes.size() - 16 ||
        body_crc != crc32(0, bytes.data() + 16, static_cast<uInt>(bytes.size() - 16))) {
        Fail(ec, std::errc::bad_message);
        return {};
    }
    Reader body{std::span<const std::uint8_t>(bytes).subspan(16)};
    CharacterKey key;
    key.save_dir = body.String(1023);
    key.user = body.String(31);
    key.slot = static_cast<std::uint8_t>(body.Get(1));
    key.character = body.Id();
    key.cycle = static_cast<std::uint32_t>(body.Get(4));
    const auto saved_party = body.Id();
    const auto payload_size = body.Get(4), payload_crc = body.Get(4);
    if (!body.valid || key != expected || saved_party != party || payload_size != ref.bytes ||
        payload_size != body.bytes.size() - body.at || payload_crc != ref.crc32 ||
        payload_crc != crc32(0, body.bytes.data() + body.at, static_cast<uInt>(payload_size))) {
        Fail(ec, std::errc::bad_message);
        return {};
    }
    return std::vector<std::uint8_t>(body.bytes.begin() + body.at, body.bytes.end());
}

bool BeginReturnSaveFence(Journal& record, std::uint64_t current_generation) {
    if (!Valid(record) ||
        (record.phase != Phase::Visiting && record.phase != Phase::Returning)) return false;
    record.phase = Phase::Returning;
    record.save_generation = current_generation;
    return true;
}

bool ConfirmReturnedSave(Journal& record, const ReturnSaveProof& proof) {
    const auto valid_sha = [](std::string_view hash) {
        return hash.size() == 64 && std::ranges::all_of(hash, [](char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        });
    };
    if (!Valid(record) || record.phase != Phase::Returning || proof.persisted_key != record.key ||
        proof.generation <= record.save_generation || proof.bytes == 0 ||
        !proof.serialization_began_after_restore || !proof.destination_confirmed ||
        !valid_sha(proof.receipt_sha256) || !valid_sha(proof.persisted_sha256) ||
        !valid_sha(proof.serialize_end_sha256) ||
        proof.receipt_sha256 != proof.persisted_sha256 ||
        proof.receipt_sha256 != proof.serialize_end_sha256) return false;
    const auto expected = fs::path{record.key.save_dir} /
                          ("userdata000" + std::to_string(record.key.slot));
    if (proof.receipt_path != expected.string()) return false;
    record.phase = Phase::Complete;
    record.save_generation = proof.generation;
    return true;
}

bool StoreWorldSnapshot(const CharacterLease& lease, const CharacterKey& key, const PartyId& party,
                        std::span<const std::uint8_t> payload, SnapshotRef& ref,
                        std::error_code& ec) {
    ec.clear();
    if (lease.fd_ < 0 || !ValidKey(key) || payload.empty() || payload.size() > MaxWorldSnapshotBytes ||
        !std::ranges::any_of(party, [](auto byte) { return byte != 0; }))
        return Fail(ec, std::errc::invalid_argument);
    ref = {static_cast<std::uint32_t>(payload.size()), static_cast<std::uint32_t>(
           crc32(0, payload.data(), static_cast<uInt>(payload.size())))};
    const auto destination = WorldSnapshotPath(lease.path_, party);
    std::error_code exists_error;
    const auto status = fs::symlink_status(destination, exists_error);
    if (!exists_error && status.type() != fs::file_type::not_found) {
        const auto existing = LoadWorldSnapshot(lease.path_, key, party, ref, ec);
        if (existing && std::ranges::equal(*existing, payload)) return true;
        return Fail(ec, std::errc::file_exists);
    }
    if (exists_error != std::errc::no_such_file_or_directory && exists_error)
        return Fail(ec, std::errc::io_error);

    std::vector<std::uint8_t> body;
    PutString(body, key.save_dir);
    PutString(body, key.user);
    Put(body, key.slot, 1);
    body.insert(body.end(), key.character.begin(), key.character.end());
    Put(body, key.cycle, 4);
    body.insert(body.end(), party.begin(), party.end());
    Put(body, payload.size(), 4);
    Put(body, ref.crc32, 4);
    body.insert(body.end(), payload.begin(), payload.end());
    std::vector<std::uint8_t> bytes{'B', 'B', 'W', 'S'};
    Put(bytes, 1, 4);
    Put(bytes, body.size(), 4);
    Put(bytes, crc32(0, body.data(), static_cast<uInt>(body.size())), 4);
    bytes.insert(bytes.end(), body.begin(), body.end());
    return AtomicWrite(destination, bytes, false, ec);
}

bool StoreJournal(const CharacterLease& lease, const Journal& record, std::error_code& ec) {
    ec.clear();
    if (lease.fd_ < 0) return Fail(ec, std::errc::bad_file_descriptor);
    if (!Valid(record)) return Fail(ec, std::errc::invalid_argument);
    if (record.world_snapshot.bytes != 0 &&
        !LoadWorldSnapshot(lease.path_, record.key, record.party, record.world_snapshot, ec)) return false;
    const auto previous = LoadJournal(lease.path_, record.key, ec);
    if (previous) {
        if (!AllowedUpdate(*previous, record)) return Fail(ec, std::errc::invalid_argument);
    } else if (ec != std::errc::no_such_file_or_directory) {
        return false;
    } else if (record.phase != Phase::Prepared) {
        return Fail(ec, std::errc::invalid_argument);
    }
    ec.clear();
    const auto bytes = Encode(record);
    return AtomicWrite(lease.path_, bytes, true, ec);
}

bool RollbackUnadmittedJournal(const CharacterLease& lease, const Journal& expected_current,
                               const std::optional<Journal>& previous, std::error_code& ec) {
    ec.clear();
    const bool prepared_attempt = expected_current.phase == Phase::Prepared &&
                                  expected_current.admission == 0;
    const bool visiting_attempt = expected_current.phase == Phase::Visiting &&
                                  expected_current.admission != 0;
    if (lease.fd_ < 0 || !Valid(expected_current) ||
        (!prepared_attempt && !visiting_attempt) || !expected_current.pending.empty() ||
        !expected_current.committed.empty() || expected_current.native_return_selected) {
        return Fail(ec, std::errc::invalid_argument);
    }
    auto prepared_current = expected_current;
    prepared_current.admission = 0;
    prepared_current.phase = Phase::Prepared;
    if (expected_current.world_snapshot.bytes != 0 &&
        !LoadWorldSnapshot(lease.path_, expected_current.key, expected_current.party,
                           expected_current.world_snapshot, ec)) return false;
    const auto current = LoadJournal(lease.path_, expected_current.key, ec);
    if (!current || (*current != expected_current && *current != prepared_current)) {
        if (!ec) Fail(ec, std::errc::state_not_recoverable);
        return false;
    }
    if (previous) {
        if (!Valid(*previous) || previous->key != expected_current.key ||
            previous->phase != Phase::Complete) {
            return Fail(ec, std::errc::invalid_argument);
        }
        auto prepared = PrepareNextParty(*previous, expected_current.party,
                                         expected_current.origin, expected_current.fallback);
        if (!prepared) return Fail(ec, std::errc::invalid_argument);
        prepared->save_generation = expected_current.save_generation;
        prepared->world_snapshot = expected_current.world_snapshot;
        if (*prepared != prepared_current || !AllowedUpdate(*prepared, expected_current))
            return Fail(ec, std::errc::invalid_argument);
        const auto bytes = Encode(*previous);
        return AtomicWrite(lease.path_, bytes, true, ec);
    }
    auto prepared = expected_current;
    prepared.admission = 0;
    prepared.phase = Phase::Prepared;
    if (!expected_current.retained.empty() || !Valid(prepared_current) ||
        !AllowedUpdate(prepared_current, expected_current)) {
        return Fail(ec, std::errc::invalid_argument);
    }
#ifdef _WIN32
    return Fail(ec, std::errc::operation_not_supported);
#else
    if (!fs::remove(lease.path_, ec)) {
        if (!ec) ec = std::make_error_code(std::errc::no_such_file_or_directory);
        return false;
    }
    const auto parent = lease.path_.parent_path().empty() ? fs::path{"."} : lease.path_.parent_path();
    const int directory = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory < 0) return SystemError(ec);
    bool ok = ::fsync(directory) == 0;
    if (!ok) SystemError(ec);
    if (::close(directory) != 0 && ok) ok = SystemError(ec);
    return ok;
#endif
}

bool StoreSelectedSaveBackup(const CharacterLease& lease, const CharacterKey& key,
                             std::string_view selected_path, std::uint64_t server_user,
                             std::uint64_t server_character, std::uint64_t generation,
                             std::string_view sha256, std::span<const std::uint8_t> bytes,
                             std::error_code& ec) {
    ec.clear();
    std::uint64_t parsed_user = 0;
    const auto parsed = std::from_chars(key.user.data(), key.user.data() + key.user.size(), parsed_user);
    const auto expected_path = fs::path{key.save_dir} / ("userdata000" + std::to_string(key.slot));
    bool ids_match = parsed.ec == std::errc{} && parsed.ptr == key.user.data() + key.user.size() &&
                     parsed_user == server_user;
    for (std::size_t i = 0; i < 8; ++i) {
        ids_match = ids_match && key.character[i] == static_cast<std::uint8_t>(server_user >> (i * 8)) &&
                    key.character[i + 8] == static_cast<std::uint8_t>(server_character >> (i * 8));
    }
    if (lease.fd_ < 0 || !ValidKey(key) || selected_path.empty() || selected_path.size() > 4096 ||
        selected_path.find('\0') != std::string_view::npos || server_user == 0 ||
        server_character == 0 || generation == 0 || bytes.size() != 0x140000 ||
        !ValidSha256(sha256) || !ids_match || fs::path{selected_path} != expected_path)
        return Fail(ec, std::errc::invalid_argument);
    std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
    if (!SHA256(bytes.data(), bytes.size(), digest.data()))
        return Fail(ec, std::errc::io_error);
    constexpr char hex[] = "0123456789abcdef";
    std::string actual_sha256(SHA256_DIGEST_LENGTH * 2, '0');
    for (std::size_t i = 0; i < digest.size(); ++i) {
        actual_sha256[i * 2] = hex[digest[i] >> 4];
        actual_sha256[i * 2 + 1] = hex[digest[i] & 0x0f];
    }
    if (actual_sha256 != sha256) return Fail(ec, std::errc::invalid_argument);
    std::vector<std::uint8_t> body;
    PutString(body, key.save_dir);
    PutString(body, std::string{selected_path});
    Put(body, key.slot, 1);
    Put(body, server_user, 8);
    Put(body, server_character, 8);
    Put(body, key.cycle, 4);
    Put(body, generation, 8);
    PutString(body, std::string{sha256});
    Put(body, bytes.size(), 4);
    body.insert(body.end(), bytes.begin(), bytes.end());
    std::vector<std::uint8_t> encoded{'B', 'B', 'S', 'B'};
    Put(encoded, 1, 4);
    Put(encoded, body.size(), 4);
    Put(encoded, crc32(0, body.data(), static_cast<uInt>(body.size())), 4);
    encoded.insert(encoded.end(), body.begin(), body.end());
    const auto path = lease.path_.parent_path() /
        (lease.path_.stem().string() + ".selected-save-" + std::to_string(generation) +
         "-" + actual_sha256 + ".bin");
    std::error_code exists_error;
    const auto status = fs::symlink_status(path, exists_error);
    if (!exists_error && status.type() != fs::file_type::not_found) {
        if (status.type() != fs::file_type::regular) return Fail(ec, std::errc::file_exists);
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input || input.tellg() != static_cast<std::streamoff>(encoded.size()))
            return Fail(ec, std::errc::file_exists);
        std::vector<std::uint8_t> existing(encoded.size());
        input.seekg(0);
        if (!input.read(reinterpret_cast<char*>(existing.data()), existing.size()) || existing != encoded)
            return Fail(ec, std::errc::file_exists);
        return true;
    }
    if (exists_error != std::errc::no_such_file_or_directory && exists_error)
        return Fail(ec, std::errc::io_error);
    return AtomicWrite(path, encoded, false, ec);
}

std::optional<Message> PersistEarnedEvents(const CharacterLease& lease, Journal& record,
                                         const Admission& admission, std::error_code& ec) {
    ec.clear();
    if (!admission.started_ || !admission.admitted_ || record.phase == Phase::Prepared ||
        record.party != admission.party_ || record.admission != admission.admission_) {
        Fail(ec, std::errc::invalid_argument);
        return std::nullopt;
    }
    auto candidate = record;
    for (auto event : admission.Earned()) {
        if (std::ranges::find(candidate.pending, event) == candidate.pending.end() &&
            std::ranges::find(candidate.committed, event) == candidate.committed.end() &&
            !std::ranges::any_of(candidate.retained, [event](const auto& claim) { return claim.event == event; }))
            candidate.pending.push_back(event);
    }
    if (!StoreJournal(lease, candidate, ec)) return std::nullopt;
    record = std::move(candidate);
    Message ack;
    ack.party = admission.party_;
    ack.admission = admission.admission_;
    ack.sequence = admission.LastSequence();
    ack.kind = Kind::Ack;
    return ack;
}
} // namespace Core::Bbsc::Party
