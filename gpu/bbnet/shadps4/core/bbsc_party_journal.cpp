// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bbsc_party.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cerrno>
#include <cmath>
#include <cstring>
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
           std::ranges::any_of(key.character, [](auto byte) { return byte != 0; });
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
    }
    Put(body, record.native_return_selected, 1);
    std::vector<std::uint8_t> bytes{'B', 'B', 'P', 'R'};
    Put(bytes, 4, 4);
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
    if ((version < 1 || version > 4) || header.Get(4) != bytes.size() - 16) return std::nullopt;
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
        if (!reader.valid || count > MaxEvents || count > (reader.bytes.size() - reader.at) / 29)
            return std::nullopt;
        for (std::size_t i = 0; i < count; ++i) {
            EarnedClaim claim;
            claim.event = static_cast<EventId>(reader.Get(4));
            claim.party = reader.Id();
            claim.admission = reader.Get(8);
            const auto committed = reader.Get(1);
            if (committed > 1) return std::nullopt;
            claim.committed = committed != 0;
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
        claims.push_back({event, record.party, record.admission, false});
    for (const auto event : record.committed)
        claims.push_back({event, record.party, record.admission, true});
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

bool ReadRegularFile(const fs::path& path, std::size_t minimum, std::size_t maximum,
                     std::vector<std::uint8_t>& bytes, std::error_code& ec) {
    ec.clear();
#ifdef _WIN32
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
        // Create only one level so its parent is known to exist and can be flushed.
        fs::create_directory(path.parent_path(), ec);
        if (ec) return {};
        const auto parent = path.parent_path().parent_path().empty() ? fs::path{"."}
                                                                   : path.parent_path().parent_path();
#ifdef _WIN32
        (void)parent; // Atomic record publication below uses MOVEFILE_WRITE_THROUGH.
        const fs::path lock = path.wstring() + L".lock";
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
    const fs::path lock = path.wstring() + L".lock";
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
