// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bbsc_party.h"

#include <algorithm>
#include <bit>
#include <cerrno>
#include <cmath>
#include <fcntl.h>
#include <span>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_set>
#include <utility>
#include <zlib.h>

namespace Core::Bbsc::Party {
namespace {
namespace fs = std::filesystem;
constexpr std::size_t MaxRecordBytes = 65536;
constexpr std::size_t MaxEvents = 1024;

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
        record.pending.size() + record.committed.size() > MaxEvents ||
        (record.admission == 0 && (!record.pending.empty() || !record.committed.empty()))) {
        return false;
    }
    std::unordered_set<EventId> unique;
    for (const auto* events : {&record.pending, &record.committed}) {
        for (const auto event : *events) {
            if (event == 0 || !unique.insert(event).second) return false;
        }
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
    for (const auto* events : {&record.pending, &record.committed}) {
        Put(body, events->size(), 2);
        for (const auto event : *events) Put(body, event, 4);
    }
    std::vector<std::uint8_t> bytes{'B', 'B', 'P', 'R'};
    Put(bytes, 1, 4);
    Put(bytes, body.size(), 4);
    Put(bytes, crc32(0, body.data(), static_cast<uInt>(body.size())), 4);
    bytes.insert(bytes.end(), body.begin(), body.end());
    return bytes;
}

std::optional<Journal> Decode(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 16 || bytes[0] != 'B' || bytes[1] != 'B' || bytes[2] != 'P' || bytes[3] != 'R')
        return std::nullopt;
    Reader header{bytes, 4};
    if (header.Get(4) != 1 || header.Get(4) != bytes.size() - 16) return std::nullopt;
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
    record.pending = reader.Events();
    record.committed = reader.Events();
    if (!reader.valid || reader.at != reader.bytes.size() || !Valid(record)) return std::nullopt;
    return record;
}

bool AllowedUpdate(const Journal& before, const Journal& after) {
    if (before.party != after.party) {
        if (before.phase != Phase::Complete || after.phase != Phase::Prepared) return false;
    } else {
        if (before.origin != after.origin || before.fallback != after.fallback ||
            (before.admission != 0 && before.admission != after.admission) ||
            std::to_underlying(after.phase) < std::to_underlying(before.phase) ||
            std::to_underlying(after.phase) > std::to_underlying(before.phase) + 1) return false;
    }
    const auto has = [](const auto& ids, EventId id) {
        return std::ranges::find(ids, id) != ids.end();
    };
    for (const auto id : before.committed) if (!has(after.committed, id)) return false;
    for (const auto id : before.pending)
        if (!has(after.pending, id) && !has(after.committed, id)) return false;
    return true;
}
} // namespace

CharacterLease::CharacterLease(std::filesystem::path path, int fd) : path_(std::move(path)), fd_(fd) {}
CharacterLease::~CharacterLease() { if (fd_ >= 0) ::close(fd_); }
CharacterLease::CharacterLease(CharacterLease&& other) noexcept
    : path_(std::move(other.path_)), fd_(std::exchange(other.fd_, -1)) {}
CharacterLease& CharacterLease::operator=(CharacterLease&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) ::close(fd_);
        path_ = std::move(other.path_);
        fd_ = std::exchange(other.fd_, -1);
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
        const int directory = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (directory < 0) { SystemError(ec); return {}; }
        const bool flushed = ::fsync(directory) == 0;
        if (!flushed) SystemError(ec);
        const bool closed = ::close(directory) == 0;
        if (!closed && flushed) SystemError(ec);
        if (!flushed || !closed) return {};
    }
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
}

std::optional<Journal> LoadJournal(const fs::path& path, const CharacterKey& expected,
                                  std::error_code& ec) {
    ec.clear();
    if (!ValidKey(expected)) { Fail(ec, std::errc::invalid_argument); return {}; }
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) { SystemError(ec); return {}; }
    struct stat info{};
    if (::fstat(fd, &info) != 0 || !S_ISREG(info.st_mode) || info.st_size < 16 ||
        info.st_size > static_cast<off_t>(MaxRecordBytes)) {
        ::close(fd);
        Fail(ec, std::errc::bad_message);
        return {};
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(info.st_size));
    std::size_t at = 0;
    while (at < bytes.size()) {
        const auto count = ::read(fd, bytes.data() + at, bytes.size() - at);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        at += static_cast<std::size_t>(count);
    }
    ::close(fd);
    auto record = at == bytes.size() ? Decode(bytes) : std::nullopt;
    if (!record || record->key != expected) {
        Fail(ec, std::errc::bad_message);
        return {};
    }
    return record;
}

bool StoreJournal(const CharacterLease& lease, const Journal& record, std::error_code& ec) {
    ec.clear();
    if (lease.fd_ < 0) return Fail(ec, std::errc::bad_file_descriptor);
    if (!Valid(record)) return Fail(ec, std::errc::invalid_argument);
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
    std::string pattern = lease.path_.string() + ".tmp-XXXXXX";
    const int fd = ::mkstemp(pattern.data());
    if (fd < 0) return SystemError(ec);
    std::size_t at = 0;
    bool ok = ::fcntl(fd, F_SETFD, FD_CLOEXEC) == 0;
    if (!ok) SystemError(ec);
    while (ok && at < bytes.size()) {
        const auto count = ::write(fd, bytes.data() + at, bytes.size() - at);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { if (count == 0) errno = EIO; ok = SystemError(ec); break; }
        at += static_cast<std::size_t>(count);
    }
    if (ok && ::fsync(fd) != 0) ok = SystemError(ec);
    if (::close(fd) != 0 && ok) ok = SystemError(ec);
    if (ok && ::rename(pattern.c_str(), lease.path_.c_str()) != 0) ok = SystemError(ec);
    if (!ok) { ::unlink(pattern.c_str()); return false; }
    const fs::path parent = lease.path_.parent_path().empty() ? fs::path{"."} : lease.path_.parent_path();
    const int directory = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory < 0) return SystemError(ec);
    if (::fsync(directory) != 0) ok = SystemError(ec);
    if (::close(directory) != 0 && ok) ok = SystemError(ec);
    return ok;
}
} // namespace Core::Bbsc::Party
