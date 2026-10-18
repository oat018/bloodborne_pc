// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace Core::Bbsc::Party {

enum class ReturnChoice { OriginalPoint, SavedLamp, NativeInitial, Unavailable };
using PartyId = std::array<std::uint8_t, 16>;
using EventId = std::uint32_t;

struct CharacterKey {
    std::string save_dir, user;
    std::uint8_t slot{};
    PartyId character{};
    std::uint32_t cycle{};
    bool operator==(const CharacterKey&) const = default;
};

struct Destination {
    std::uint32_t map{};
    float x{}, y{}, z{}, heading{};
    std::int32_t lamp{-1};
    bool operator==(const Destination&) const = default;
};

enum class Phase : std::uint8_t { Prepared, Visiting, Returning, Complete };
struct Journal {
    CharacterKey key;
    PartyId party{};
    std::uint64_t admission{};
    Phase phase{Phase::Prepared};
    Destination origin, fallback;
    std::uint64_t save_generation{};
    std::vector<EventId> pending, committed;
    bool operator==(const Journal&) const = default;
};

class CharacterLease {
public:
    static std::optional<CharacterLease> Acquire(const std::filesystem::path& path,
                                                std::error_code& ec);
    ~CharacterLease();
    CharacterLease(CharacterLease&& other) noexcept;
    CharacterLease& operator=(CharacterLease&& other) noexcept;
    CharacterLease(const CharacterLease&) = delete;
    CharacterLease& operator=(const CharacterLease&) = delete;

private:
    CharacterLease(std::filesystem::path path, int fd);
    std::filesystem::path path_;
    int fd_{-1};
    friend bool StoreJournal(const CharacterLease&, const Journal&, std::error_code&);
};

// One game thread owns updates; the lease excludes other processes, not other threads.
bool StoreJournal(const CharacterLease& lease, const Journal& record, std::error_code& ec);
std::optional<Journal> LoadJournal(const std::filesystem::path& path, const CharacterKey& expected,
                                  std::error_code& ec);

bool IsSupportedStoryMap(std::uint32_t map);
bool CanCrossMapMatch(std::uint32_t from, std::uint32_t to, std::string_view ours,
                      std::string_view theirs, bool cooperative);
ReturnChoice ChooseReturn(bool original_safe, bool saved_lamp_valid, bool native_initial_valid);

} // namespace Core::Bbsc::Party
