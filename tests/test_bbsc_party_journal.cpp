// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bbsc_party.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <openssl/sha.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <zlib.h>

using namespace Core::Bbsc::Party;
namespace fs = std::filesystem;

static int flush_calls = 0;
static int fail_flush = 0;
static int interrupt_flush = 0;
static bool fail_write = false;
static bool fail_rename = false;
static bool fail_write_close = false;

extern "C" int fsync(int fd) {
    ++flush_calls;
    if (interrupt_flush == flush_calls) _exit(73);
    if (fail_flush == flush_calls) { errno = EIO; return -1; }
    return static_cast<int>(syscall(SYS_fsync, fd));
}

extern "C" ssize_t write(int fd, const void* data, size_t size) {
    if (fail_write) { errno = ENOSPC; return -1; }
    return static_cast<ssize_t>(syscall(SYS_write, fd, data, size));
}

extern "C" int rename(const char* from, const char* to) {
    if (fail_rename) { errno = EACCES; return -1; }
    return static_cast<int>(syscall(SYS_renameat, AT_FDCWD, from, AT_FDCWD, to));
}

extern "C" int close(int fd) {
    const bool writing = fail_write_close && (fcntl(fd, F_GETFL) & O_ACCMODE) == O_RDWR;
    const int result = static_cast<int>(syscall(SYS_close, fd));
    if (writing) { errno = EIO; return -1; }
    return result;
}

static std::string Read(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

static std::string Sha256(std::span<const std::uint8_t> bytes) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
    assert(SHA256(bytes.data(), bytes.size(), digest.data()));
    constexpr char hex[] = "0123456789abcdef";
    std::string result(SHA256_DIGEST_LENGTH * 2, '0');
    for (std::size_t i = 0; i < digest.size(); ++i) {
        result[i * 2] = hex[digest[i] >> 4];
        result[i * 2 + 1] = hex[digest[i] & 0x0f];
    }
    return result;
}

static void Replace(const fs::path& path, const std::string& bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    assert(output.good());
}

static void RepairHeader(std::string& bytes) {
    const auto size = bytes.size() - 16;
    const auto checksum = crc32(0, reinterpret_cast<const Bytef*>(bytes.data() + 16), size);
    for (std::size_t i = 0; i < 4; ++i) {
        bytes[8 + i] = static_cast<char>(size >> (i * 8));
        bytes[12 + i] = static_cast<char>(checksum >> (i * 8));
    }
}

static Journal Record(const fs::path& root) {
    Journal record;
    record.key = {root.string(), "test-user", 2, {1, 2, 3}, 0};
    record.party = {4, 5, 6};
    record.admission = 7;
    record.origin = {0x18010000, 10, 20, 30, 0.5f, -1};
    record.fallback = {0x15000000, 0, 0, 0, 0, 1};
    record.save_generation = 4;
    record.pending = {2200};
    return record;
}

static void TestJournalRoundTrip(const fs::path& root) {
    const auto path = root / "round-trip.bin";
    auto record = Record(root);
    std::error_code ec;
    auto lease = CharacterLease::Acquire(path, ec);
    assert(lease && !ec);
    assert(!CharacterLease::Acquire(path, ec) && ec);
    assert(StoreJournal(*lease, record, ec) && !ec);
    assert(LoadJournal(path, record.key, ec) == record && !ec);
    const auto bytes = Read(path);
    // Version 1 has the same body without the retained-claim count.
    auto legacy = bytes;
    const auto snapshot_at = 16 + 2 + record.key.save_dir.size() + 2 + record.key.user.size() + 1 + 16 +
                             4 + 16 + 8 + 1 + 24 + 24 + 8;
    legacy.erase(snapshot_at, 8);
    legacy.resize(legacy.size() - 2);
    legacy.pop_back(); // v4 native-return-selected byte
    legacy[4] = 1;
    RepairHeader(legacy);
    Replace(path, legacy);
    assert(LoadJournal(path, record.key, ec) == record && !ec);
    assert(StoreJournal(*lease, record, ec));
    assert(Read(path) == bytes);
    auto version3 = bytes;
    version3.pop_back();
    version3[4] = 3;
    RepairHeader(version3);
    Replace(path, version3);
    assert(LoadJournal(path, record.key, ec) == record && !ec);
    assert(StoreJournal(*lease, record, ec) && Read(path) == bytes);
    auto version2 = bytes;
    version2.pop_back();
    version2.erase(snapshot_at, 8);
    version2[4] = 2;
    RepairHeader(version2);
    Replace(path, version2);
    assert(LoadJournal(path, record.key, ec) == record && !ec);
    assert(StoreJournal(*lease, record, ec) && Read(path) == bytes);
    for (int field = 0; field < 4; ++field) {
        auto foreign = record.key;
        if (field == 0) foreign.user = "another-user";
        if (field == 1) foreign.slot = 3;
        if (field == 2) foreign.character[0] = 9;
        if (field == 3) foreign.cycle = 1;
        assert(!LoadJournal(path, foreign, ec) && ec);
        assert(Read(path) == bytes);
    }
    record.phase = Phase::Visiting;
    assert(StoreJournal(*lease, record, ec));
    auto illegal = record;
    illegal.phase = Phase::Prepared;
    assert(!StoreJournal(*lease, illegal, ec));
    illegal = record;
    illegal.origin.x += 1;
    assert(!StoreJournal(*lease, illegal, ec));
    illegal = record;
    illegal.party[0] += 1;
    assert(!StoreJournal(*lease, illegal, ec));
    illegal = record;
    illegal.pending.clear();
    assert(!StoreJournal(*lease, illegal, ec));
    assert(BeginReturnSaveFence(record, 5));
    record.native_return_selected = true;
    assert(StoreJournal(*lease, record, ec));
    assert(LoadJournal(path, record.key, ec) == record);
    illegal = record;
    illegal.native_return_selected = false;
    assert(!StoreJournal(*lease, illegal, ec));
    record.phase = Phase::Complete;
    assert(StoreJournal(*lease, record, ec));
    auto next = PrepareNextParty(record, PartyId{9, 8, 7}, record.origin, record.fallback);
    assert(next && next->admission == 0 && next->pending.empty());
    assert(next->retained == std::vector<EarnedClaim>({{2200, record.party, 7, false}}));
    assert(StoreJournal(*lease, *next, ec));
    assert(LoadJournal(path, record.key, ec) == next);
    illegal = *next;
    illegal.retained.clear();
    assert(!StoreJournal(*lease, illegal, ec));
    illegal = *next;
    illegal.retained[0].party[0] ^= 1;
    assert(!StoreJournal(*lease, illegal, ec));
    // The host's identity is bound after the pre-join origin has been persisted.
    next->party = {11, 12};
    next->admission = 42;
    next->phase = Phase::Visiting;
    assert(StoreJournal(*lease, *next, ec));
    illegal = *next;
    illegal.pending = {2200};
    assert(!StoreJournal(*lease, illegal, ec));
    next->retained[0].committed = true;
    assert(StoreJournal(*lease, *next, ec));
    const auto retained_bytes = Read(path);
    for (const bool invalid_status : {true, false}) {
        auto corrupt = retained_bytes;
        if (invalid_status) corrupt.back() = static_cast<char>(255);
        else corrupt[corrupt.size() - 2] = static_cast<char>(2); // retained-claim status before v4 marker
        RepairHeader(corrupt);
        Replace(path, corrupt);
        assert(!LoadJournal(path, next->key, ec));
    }
    Replace(path, retained_bytes);
    illegal = *next;
    illegal.retained[0].committed = false;
    assert(!StoreJournal(*lease, illegal, ec));
    assert(!PrepareNextParty(*next, PartyId{8}, next->origin, next->fallback));
    std::puts("PASS: subsequent parties preserve earned-claim provenance and monotonic delivery");
}

static void TestUnadmittedRollback(const fs::path& root) {
    const auto path = root / "unadmitted-rollback.bin";
    auto previous = Record(root);
    std::error_code ec;
    auto lease = CharacterLease::Acquire(path, ec);
    assert(lease && StoreJournal(*lease, previous, ec));
    previous.phase = Phase::Visiting;
    previous.pending.clear();
    previous.committed = {2200};
    assert(StoreJournal(*lease, previous, ec));
    previous.phase = Phase::Returning;
    assert(StoreJournal(*lease, previous, ec));
    previous.phase = Phase::Complete;
    assert(StoreJournal(*lease, previous, ec));
    const auto previous_bytes = Read(path);

    const PartyId next_party{20, 21, 22};
    auto current = PrepareNextParty(previous, next_party, previous.origin, previous.fallback);
    assert(current && StoreJournal(*lease, *current, ec));
    current->admission = 88;
    current->phase = Phase::Visiting;
    assert(StoreJournal(*lease, *current, ec));
    auto changed = *current;
    changed.committed = {2201};
    assert(!RollbackUnadmittedJournal(*lease, changed, previous, ec) && ec);
    assert(LoadJournal(path, current->key, ec) == current && Read(path) != previous_bytes);

    fail_rename = true;
    assert(!RollbackUnadmittedJournal(*lease, *current, previous, ec) && ec);
    fail_rename = false;
    assert(LoadJournal(path, current->key, ec) == current);
    assert(RollbackUnadmittedJournal(*lease, *current, previous, ec) && !ec);
    assert(LoadJournal(path, previous.key, ec) == previous && Read(path) == previous_bytes);

    const auto prepared_path = root / "unadmitted-prepared.bin";
    auto prepared_lease = CharacterLease::Acquire(prepared_path, ec);
    auto prepared_previous = previous;
    prepared_previous.phase = Phase::Prepared;
    assert(prepared_lease && StoreJournal(*prepared_lease, prepared_previous, ec));
    prepared_previous.phase = Phase::Visiting;
    assert(StoreJournal(*prepared_lease, prepared_previous, ec));
    prepared_previous.phase = Phase::Returning;
    assert(StoreJournal(*prepared_lease, prepared_previous, ec));
    prepared_previous.phase = Phase::Complete;
    assert(StoreJournal(*prepared_lease, prepared_previous, ec));
    const auto prepared_previous_bytes = Read(prepared_path);
    auto prepared_attempt = PrepareNextParty(prepared_previous, PartyId{23, 24, 25},
                                             prepared_previous.origin, prepared_previous.fallback);
    assert(prepared_attempt && StoreJournal(*prepared_lease, *prepared_attempt, ec));
    auto failed_store_attempt = *prepared_attempt;
    failed_store_attempt.admission = 90;
    failed_store_attempt.phase = Phase::Visiting;
    assert(RollbackUnadmittedJournal(*prepared_lease, failed_store_attempt,
                                     prepared_previous, ec) && !ec);
    assert(LoadJournal(prepared_path, previous.key, ec) == prepared_previous &&
           Read(prepared_path) == prepared_previous_bytes);

    const auto first_path = root / "unadmitted-first-join.bin";
    auto first_lease = CharacterLease::Acquire(first_path, ec);
    auto first = Record(root);
    first.pending.clear();
    first.party = next_party;
    first.admission = 0;
    first.phase = Phase::Prepared;
    assert(first_lease && StoreJournal(*first_lease, first, ec));
    first.admission = 89;
    first.phase = Phase::Visiting;
    assert(StoreJournal(*first_lease, first, ec));
    assert(RollbackUnadmittedJournal(*first_lease, first, std::nullopt, ec) && !ec);
    assert(!fs::exists(first_path));
    std::puts("PASS: unadmitted rollback restores only the exact prior complete journal or durably removes a first-time record");
}

static void TestJournalRejectsCorruption(const fs::path& root) {
    const auto path = root / "corruption.bin";
    auto record = Record(root);
    std::error_code ec;
    auto lease = CharacterLease::Acquire(path, ec);
    assert(lease && StoreJournal(*lease, record, ec));
    const auto bytes = Read(path);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i == 4) continue; // v4 and v5 share this no-claim body and remain compatible.
        auto corrupt = bytes;
        corrupt[i] ^= 1;
        Replace(path, corrupt);
        assert(!LoadJournal(path, record.key, ec) && ec);
    }
    // Recompute the checksum to exercise the parser, not just checksum rejection.
    const std::size_t slot_at = 16 + 2 + record.key.save_dir.size() + 2 + record.key.user.size();
    const std::size_t phase_at = slot_at + 1 + 16 + 4 + 16 + 8;
    const std::size_t pending_at = phase_at + 1 + 24 + 24 + 8 + 8;
    for (const auto& [offset, value] : {std::pair{std::size_t{16}, char{127}},
                                      std::pair{slot_at, char{10}},
                                      std::pair{phase_at, char{127}},
                                      std::pair{pending_at + 1, char{127}}}) {
        auto malformed = bytes;
        malformed[offset] = value;
        RepairHeader(malformed);
        Replace(path, malformed);
        assert(!LoadJournal(path, record.key, ec));
    }
    for (const bool truncated : {false, true}) {
        auto malformed = bytes;
        if (truncated) malformed.pop_back();
        else malformed.push_back('x');
        RepairHeader(malformed);
        Replace(path, malformed);
        assert(!LoadJournal(path, record.key, ec));
    }
    Replace(path, bytes.substr(0, bytes.size() - 1));
    assert(!LoadJournal(path, record.key, ec));
    assert(!StoreJournal(*lease, record, ec));
    assert(Read(path) == bytes.substr(0, bytes.size() - 1));
    Replace(path, bytes);
    auto invalid = record;
    invalid.origin.x = std::numeric_limits<float>::quiet_NaN();
    assert(!StoreJournal(*lease, invalid, ec));
    invalid = record;
    invalid.key.slot = 10;
    assert(!StoreJournal(*lease, invalid, ec));
    invalid = record;
    invalid.pending.assign(1025, 2200);
    assert(!StoreJournal(*lease, invalid, ec));
    invalid = record;
    invalid.pending = {0};
    assert(!StoreJournal(*lease, invalid, ec));
    invalid = record;
    invalid.pending = {2200, 2200};
    assert(!StoreJournal(*lease, invalid, ec));
    invalid = record;
    invalid.fallback.map = 0x1d000000;
    assert(!StoreJournal(*lease, invalid, ec));
    assert(Read(path) == bytes);
    fs::resize_file(path, 65537);
    assert(!LoadJournal(path, record.key, ec));
    Replace(path, "BBSP legacy pending progress");
    assert(!LoadJournal(path, record.key, ec));
    assert(!StoreJournal(*lease, record, ec));
    assert(Read(path) == "BBSP legacy pending progress");
}

static void TestWriteFailures(const fs::path& root) {
    const auto path = root / "failures.bin";
    auto record = Record(root);
    std::error_code ec;
    auto lease = CharacterLease::Acquire(path, ec);
    assert(lease && StoreJournal(*lease, record, ec));
    const auto bytes = Read(path);
    record.phase = Phase::Visiting;
    fail_write = true;
    const bool written = StoreJournal(*lease, record, ec);
    fail_write = false;
    assert(!written && ec && Read(path) == bytes);
    for (bool* failure : {&fail_rename, &fail_write_close}) {
        *failure = true;
        const bool stored = StoreJournal(*lease, record, ec);
        *failure = false;
        assert(!stored && ec && Read(path) == bytes);
    }
    flush_calls = 0;
    fail_flush = 1;
    assert(!StoreJournal(*lease, record, ec) && ec);
    fail_flush = 0;
    assert(Read(path) == bytes);
    flush_calls = 0;
    fail_flush = 2;
    assert(!StoreJournal(*lease, record, ec) && ec);
    fail_flush = 0;
    assert(LoadJournal(path, record.key, ec) == record);
    assert(StoreJournal(*lease, record, ec) && !ec);
    flush_calls = 0;
    fail_flush = 1;
    const auto interrupted_directory = CharacterLease::Acquire(root / "new-directory" / "record", ec);
    fail_flush = 0;
    assert(!interrupted_directory && ec);
    assert(CharacterLease::Acquire(root / "new-directory" / "record", ec) && !ec);
}

static void TestLeaseAndRestart(const fs::path& root) {
    std::error_code ec;
    auto record = Record(root);
    for (int boundary : {1, 2}) {
        const auto path = root / ("interruption-" + std::to_string(boundary));
        auto lease = CharacterLease::Acquire(path, ec);
        assert(lease && StoreJournal(*lease, record, ec));
        const pid_t blocked = fork();
        assert(blocked >= 0);
        if (blocked == 0) _exit(CharacterLease::Acquire(path, ec) ? 1 : 0);
        int status;
        assert(waitpid(blocked, &status, 0) == blocked && WIFEXITED(status) && WEXITSTATUS(status) == 0);
        lease.reset();
        const pid_t child = fork();
        assert(child >= 0);
        if (child == 0) {
            auto own = CharacterLease::Acquire(path, ec);
            if (!own) _exit(1);
            record.phase = Phase::Visiting;
            flush_calls = 0;
            interrupt_flush = boundary;
            StoreJournal(*own, record, ec);
            _exit(2);
        }
        assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 73);
        auto recovered = LoadJournal(path, record.key, ec);
        assert(recovered && recovered->pending == record.pending);
        assert(recovered->phase == (boundary == 1 ? Phase::Prepared : Phase::Visiting));
        assert(CharacterLease::Acquire(path, ec));
    }
    const auto target = root / "untouched";
    Replace(target, "original");
    fs::create_symlink(target, root / "symlink.lock");
    assert(!CharacterLease::Acquire(root / "symlink", ec));
    fs::create_symlink(target, root / "record-symlink");
    auto lease = CharacterLease::Acquire(root / "record-symlink", ec);
    assert(lease && !StoreJournal(*lease, record, ec));
    assert(Read(target) == "original");
    auto first = CharacterLease::Acquire(root / "move", ec);
    auto second = CharacterLease::Acquire(root / "replace", ec);
    assert(first && second);
    *second = std::move(*first);
    assert(!StoreJournal(*first, record, ec));
    assert(!CharacterLease::Acquire(root / "move", ec));
    assert(CharacterLease::Acquire(root / "replace", ec));
    second.reset();
    assert(CharacterLease::Acquire(root / "move", ec));
}

static void TestDurableEventAck(const fs::path& root) {
    auto record = Record(root);
    std::error_code ec;
    const auto path = root / "earned-ack";
    auto lease = CharacterLease::Acquire(path, ec);
    assert(lease && StoreJournal(*lease, record, ec));
    record.phase = Phase::Visiting;
    assert(StoreJournal(*lease, record, ec));
    const Peer host{"host", 7};
    Admission admission;
    admission.Begin(host, record.party, record.admission, 100, {});
    Message message; message.party = record.party; message.admission = record.admission;
    message.sequence = 100;
    for (Kind kind : {Kind::Hello, Kind::Ready, Kind::Baseline, Kind::Ack}) {
        message.kind = kind; assert(admission.Accept(host, message));
    }
    auto ack = PersistEarnedEvents(*lease, record, admission, ec);
    assert(ack && ack->kind == Kind::Ack && ack->sequence == 100 && !ec);
    message.kind = Kind::Event; message.sequence = 101; message.events = {2300};
    assert(admission.Accept(host, message));
    const auto before = record;
    for (int failure : {1, 2}) {
        flush_calls = 0; fail_flush = failure;
        assert(!PersistEarnedEvents(*lease, record, admission, ec) && ec);
        fail_flush = 0;
        assert(record == before); // A failed directory flush must not announce saved delivery.
    }
    ack = PersistEarnedEvents(*lease, record, admission, ec);
    assert(ack && ack->sequence == 101 && !ec);
    assert((record.pending == std::vector<EventId>{2200, 2300}));
    const auto saved = LoadJournal(path, record.key, ec);
    assert(saved && saved->pending == record.pending);
    ack = PersistEarnedEvents(*lease, record, admission, ec);
    assert(ack && record.pending.size() == 2);
    auto foreign = record; foreign.party[0] ^= 1;
    assert(!PersistEarnedEvents(*lease, foreign, admission, ec) && ec);
    std::puts("PASS: earned-event Ack follows durable journal, failed flushes retain retry and duplicate intents do not grow");
}

static void TestPreparedWorldSnapshot(const fs::path& root) {
    const auto path = root / "prepared-home.bin";
    auto record = Record(root);
    const PartyId party = record.party;
    const std::array<std::uint8_t, 6> payload{1, 0, 2, 0, 3, 4};
    std::error_code ec;
    auto lease = CharacterLease::Acquire(path, ec);
    assert(lease);
    SnapshotRef ref;
    assert(StoreWorldSnapshot(*lease, record.key, party, payload, ref, ec) && !ec);
    assert(ref.bytes == payload.size() && ref.crc32 != 0);
    record.world_snapshot = ref;
    assert(StoreJournal(*lease, record, ec) && !ec);
    assert(LoadJournal(path, record.key, ec) == record);
    const auto loaded = LoadWorldSnapshot(path, record.key, party, ref, ec);
    assert(loaded && std::ranges::equal(*loaded, payload));
    auto wrong = record.key;
    wrong.character[0] ^= 1;
    assert(!LoadWorldSnapshot(path, wrong, party, ref, ec) && ec);
    auto other_party = party;
    other_party[0] ^= 1;
    assert(!LoadWorldSnapshot(path, record.key, other_party, ref, ec) && ec);
    const auto snapshot_path = WorldSnapshotPath(path, party);
    const auto intact = Read(snapshot_path);
    Replace(snapshot_path, intact.substr(0, intact.size() - 1));
    assert(!LoadWorldSnapshot(path, record.key, party, ref, ec) && ec);
    Replace(snapshot_path, intact);
    auto bad_ref = ref;
    ++bad_ref.bytes;
    assert(!LoadWorldSnapshot(path, record.key, party, bad_ref, ec) && ec);
    auto changed_payload = payload;
    changed_payload[0] ^= 1;
    SnapshotRef changed_ref;
    assert(!StoreWorldSnapshot(*lease, record.key, party, changed_payload, changed_ref, ec) && ec);
    assert(Read(snapshot_path) == intact);
    fs::remove(snapshot_path);
    record.phase = Phase::Visiting;
    assert(!StoreJournal(*lease, record, ec) && ec); // Missing referenced snapshot cannot advance admission.
    Replace(snapshot_path, intact);
    record.phase = Phase::Prepared;
    fail_write = true;
    auto second_party = party;
    second_party[0] ^= 4;
    SnapshotRef second_ref;
    assert(!StoreWorldSnapshot(*lease, record.key, second_party, payload, second_ref, ec) && ec);
    fail_write = false;
    assert(!fs::exists(WorldSnapshotPath(path, second_party)));
    assert(Read(snapshot_path) == intact);
    std::puts("PASS: prepared own-world snapshot binds character and party, rejects corruption, and preserves prior payload on write failure");
}

static void TestSelectedSaveBackup(const fs::path& root) {
    const auto journal_path = root / "selected-save-journal.bin";
    auto record = Record(root);
    record.key.user = "7";
    record.key.character = {7, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0};
    std::error_code ec;
    auto lease = CharacterLease::Acquire(journal_path, ec);
    assert(lease && !ec);
    std::vector<std::uint8_t> save(0x140000, 0x5a);
    const auto hash = Sha256(save);
    const auto legacy_backup = root / "selected-save-journal.selected-save-19.bin";
    Replace(legacy_backup, "preserve legacy backup");
    const auto backup = root / ("selected-save-journal.selected-save-19-" + hash + ".bin");
    assert(StoreSelectedSaveBackup(*lease, record.key,
        (root / "userdata0002").string(), 7, 8, 19, hash, save, ec) && !ec);
    const auto original = Read(backup);
    assert(original.size() > save.size());
    assert(StoreSelectedSaveBackup(*lease, record.key,
        (root / "userdata0002").string(), 7, 8, 19, hash, save, ec) && !ec);
    assert(Read(backup) == original);
    assert(Read(legacy_backup) == "preserve legacy backup");
    auto second_save = save;
    second_save[0] ^= 1;
    const auto second_hash = Sha256(second_save);
    const auto second_backup = root /
        ("selected-save-journal.selected-save-19-" + second_hash + ".bin");
    assert(second_hash != hash);
    assert(StoreSelectedSaveBackup(*lease, record.key,
        (root / "userdata0002").string(), 7, 8, 19, second_hash, second_save, ec) && !ec);
    assert(fs::is_regular_file(second_backup) && Read(second_backup) != original);
    const auto wrong_hash = hash.front() == '0' ? "1" + hash.substr(1) : "0" + hash.substr(1);
    const auto wrong_hash_backup = root /
        ("selected-save-journal.selected-save-19-" + wrong_hash + ".bin");
    assert(!StoreSelectedSaveBackup(*lease, record.key,
        (root / "userdata0002").string(), 7, 8, 19, wrong_hash, save, ec) && ec);
    assert(!fs::exists(wrong_hash_backup));
    auto changed = save;
    changed[0] ^= 1;
    assert(!StoreSelectedSaveBackup(*lease, record.key,
        (root / "userdata0002").string(), 7, 8, 19, hash, changed, ec) && ec);
    assert(Read(backup) == original);
    assert(!StoreSelectedSaveBackup(*lease, record.key,
        (root / "userdata0002").string(), 7, 8, 19, std::string(64, 'A'), save, ec) && ec);
    assert(Read(backup) == original);
    std::puts("PASS: selected-save backup is durable, immutable, and idempotent for exact selected identity/generation/hash bytes");
}

static void TestManagedCycleEpochFence(const fs::path& root) {
    const auto path = root / "managed-cycle.bin";
    auto key = Record(root).key;
    key.cycle = 6;
    const std::string before(64, 'a'), after(64, 'b'), later(64, 'c');
    std::error_code ec;
    auto lease = CharacterLease::Acquire(path, ec);
    assert(lease && !ec);
    ManagedCycleEpoch stable;
    stable.key = key;
    stable.epoch = 0;
    stable.native_cycle = 7;
    stable.stage = ManagedCycleStage::Stable;
    assert(StoreManagedCycleEpoch(*lease, stable, ec) && !ec);
    const auto intent = BeginManagedCycleIntent(stable, 6, 10, before);
    assert(intent && intent->target_key_cycle == 0x80000001u);
    assert(StoreManagedCycleEpoch(*lease, *intent, ec) && !ec);
    const auto applied = ObserveManagedCycleApplied(*intent, 6, 7);
    assert(applied && StoreManagedCycleEpoch(*lease, *applied, ec) && !ec);
    const auto serialized = ObserveManagedCycleSerializeEnd(*applied, after, 10);
    assert(serialized && StoreManagedCycleEpoch(*lease, *serialized, ec) && !ec);
    assert(!CommitManagedCycleEpoch(*serialized, 11, before, before));
    assert(!CommitManagedCycleEpoch(*serialized, 11, after, later));
    const auto committed = CommitManagedCycleEpoch(*serialized, 11, after, after);
    assert(committed && committed->key.cycle == 0x80000001u && committed->epoch == 1);
    assert(StoreManagedCycleEpoch(*lease, *committed, ec) && !ec);
    const auto loaded = LoadManagedCycleEpoch(path, key, ec);
    assert(loaded && *loaded == *committed);

    const auto ordinary = ObserveManagedCycleSerializeEnd(*committed, later, 11);
    assert(ordinary && ordinary->stage == ManagedCycleStage::SaveSerialized);
    assert(StoreManagedCycleEpoch(*lease, *ordinary, ec) && !ec);
    const auto saved = CommitManagedCycleEpoch(*ordinary, 12, later, later);
    assert(saved && saved->epoch == 1 && saved->key.cycle == 0x80000001u);
    assert(StoreManagedCycleEpoch(*lease, *saved, ec) && !ec);

    // Process-local ABI3 generations reset on restart. A content match rebases that
    // fence, but a recognized pre-image must cancel an unpersisted transition instead
    // of promoting it based on the previous process's Applied observation.
    auto old_process_stable = *saved;
    old_process_stable.serialized_generation = 9;
    assert(StoreManagedCycleEpoch(*lease, old_process_stable, ec) && !ec);
    const auto stable_resume = RebaseManagedCycleForResume(old_process_stable, later, 0);
    assert(stable_resume && stable_resume->stage == ManagedCycleStage::Stable &&
           stable_resume->serialized_generation == 0);
    assert(StoreManagedCycleEpoch(*lease, *stable_resume, ec) && !ec);
    auto restarted_ordinary = ObserveManagedCycleSerializeEnd(*stable_resume, before, 0);
    assert(restarted_ordinary && restarted_ordinary->stage == ManagedCycleStage::SaveSerialized);
    assert(StoreManagedCycleEpoch(*lease, *restarted_ordinary, ec) && !ec);
    restarted_ordinary = RebaseManagedCycleForResume(*restarted_ordinary, later, 0);
    assert(restarted_ordinary && restarted_ordinary->stage == ManagedCycleStage::Stable);
    assert(StoreManagedCycleEpoch(*lease, *restarted_ordinary, ec) && !ec);
    restarted_ordinary = ObserveManagedCycleSerializeEnd(*restarted_ordinary, before, 0);
    assert(restarted_ordinary && StoreManagedCycleEpoch(*lease, *restarted_ordinary, ec) && !ec);
    const auto restart_saved = CommitManagedCycleEpoch(*restarted_ordinary, 1, before, before);
    assert(restart_saved && StoreManagedCycleEpoch(*lease, *restart_saved, ec) && !ec);

    const auto resume_path = root / "managed-cycle-resume.bin";
    auto resume_lease = CharacterLease::Acquire(resume_path, ec);
    auto resume_stable = stable;
    resume_stable.serialized_sha256 = before;
    assert(resume_lease && StoreManagedCycleEpoch(*resume_lease, resume_stable, ec));
    auto resume_intent = BeginManagedCycleIntent(resume_stable, 6, 9, before);
    assert(resume_intent && StoreManagedCycleEpoch(*resume_lease, *resume_intent, ec));
    auto resume_applied = ObserveManagedCycleApplied(*resume_intent, 6, 7);
    assert(resume_applied && StoreManagedCycleEpoch(*resume_lease, *resume_applied, ec));
    auto resume_transition = ObserveManagedCycleSerializeEnd(*resume_applied, after, 9);
    assert(resume_transition && StoreManagedCycleEpoch(*resume_lease, *resume_transition, ec));
    auto prehash_resume = RebaseManagedCycleForResume(*resume_transition, before, 0);
    assert(prehash_resume && prehash_resume->stage == ManagedCycleStage::Stable &&
           prehash_resume->key == key && prehash_resume->epoch == 0);
    assert(!CommitManagedCycleEpoch(*prehash_resume, 1, before, before));
    assert(StoreManagedCycleEpoch(*resume_lease, *prehash_resume, ec) && !ec);

    const auto expected_path = root / "managed-cycle-expected.bin";
    auto expected_lease = CharacterLease::Acquire(expected_path, ec);
    assert(expected_lease && StoreManagedCycleEpoch(*expected_lease, resume_stable, ec));
    assert(StoreManagedCycleEpoch(*expected_lease, *resume_intent, ec));
    assert(StoreManagedCycleEpoch(*expected_lease, *resume_applied, ec));
    assert(StoreManagedCycleEpoch(*expected_lease, *resume_transition, ec));
    auto expected_resume = RebaseManagedCycleForResume(*resume_transition, after, 0);
    assert(expected_resume && expected_resume->stage == ManagedCycleStage::Applied &&
           expected_resume->baseline_generation == 0);
    assert(StoreManagedCycleEpoch(*expected_lease, *expected_resume, ec) && !ec);
    auto fresh_transition = ObserveManagedCycleSerializeEnd(*expected_resume, after, 0);
    assert(fresh_transition && StoreManagedCycleEpoch(*expected_lease, *fresh_transition, ec));
    const auto fresh_commit = CommitManagedCycleEpoch(*fresh_transition, 1, after, after);
    assert(fresh_commit && fresh_commit->key.cycle == 0x80000001u);
    assert(StoreManagedCycleEpoch(*expected_lease, *fresh_commit, ec) && !ec);

    auto ambiguous = *resume_transition;
    ambiguous.serialized_sha256 = before;
    assert(!RebaseManagedCycleForResume(ambiguous, before, 0));
    assert(!RebaseManagedCycleForResume(*resume_transition, later, 0));

    const auto next_intent = BeginManagedCycleIntent(*saved, 7, 12, later);
    assert(next_intent && next_intent->target_key_cycle == 0x80000002u);
    assert(StoreManagedCycleEpoch(*lease, *next_intent, ec) && !ec);
    const auto next_applied = ObserveManagedCycleApplied(*next_intent, 7, 7);
    assert(next_applied && StoreManagedCycleEpoch(*lease, *next_applied, ec) && !ec);
    assert(!ObserveManagedCycleApplied(*next_intent, 7, 6));

    const auto intact = Read(ManagedCycleEpochPath(path));
    assert(intact.size() > 16);
    Replace(ManagedCycleEpochPath(path), intact.substr(0, intact.size() - 1));
    assert(!LoadManagedCycleEpoch(path, key, ec) && ec);
    Replace(ManagedCycleEpochPath(path), intact);
    auto foreign = key;
    foreign.character[0] ^= 1;
    assert(!LoadManagedCycleEpoch(path, foreign, ec) && ec);

    const auto prior_path = root / "managed-cycle-prior-home.bin";
    auto old_home = Record(root);
    old_home.key = key;
    old_home.pending.clear();
    auto prior_lease = CharacterLease::Acquire(prior_path, ec);
    assert(prior_lease && StoreJournal(*prior_lease, old_home, ec));
    old_home.phase = Phase::Visiting;
    assert(StoreJournal(*prior_lease, old_home, ec));
    old_home.phase = Phase::Returning;
    assert(StoreJournal(*prior_lease, old_home, ec));
    old_home.phase = Phase::Complete;
    old_home.committed = {2200};
    assert(StoreJournal(*prior_lease, old_home, ec));
    const auto prior_bytes = Read(prior_path);
    auto tagged_key = key;
    tagged_key.cycle = 0x80000001u;
    const PartyId next_party{7, 8, 9};
    const Destination origin{0x18010000, 11, 22, 33, 0.6f, -1};
    const Destination fallback{0x15000000, 0, 0, 0, 0, 1};
    const auto migrated = MigrateCompletedCycleHome(old_home, tagged_key, next_party,
                                                     origin, fallback);
    assert(migrated && migrated->phase == Phase::Prepared && migrated->key == tagged_key &&
           migrated->retained.empty());
    auto completed_epoch_one = old_home;
    completed_epoch_one.key = tagged_key;
    completed_epoch_one.party = next_party;
    completed_epoch_one.phase = Phase::Complete;
    auto tagged_epoch_two = tagged_key;
    tagged_epoch_two.cycle = 0x80000002u;
    const auto migrated_epoch_two = MigrateCompletedCycleHome(
        completed_epoch_one, tagged_epoch_two, PartyId{10, 11, 12}, origin, fallback);
    assert(migrated_epoch_two && migrated_epoch_two->key == tagged_epoch_two &&
           migrated_epoch_two->retained.empty());
    auto pending_old_home = old_home;
    pending_old_home.phase = Phase::Visiting;
    assert(!MigrateCompletedCycleHome(pending_old_home, tagged_key, next_party, origin, fallback));
    auto skipped_cycle = tagged_key;
    skipped_cycle.cycle = 0x80000002u;
    assert(!MigrateCompletedCycleHome(old_home, skipped_cycle, next_party, origin, fallback));
    assert(Read(prior_path) == prior_bytes);

    const auto history_dir = root / "managed-cycle-history";
    fs::create_directories(history_dir);
    auto history_key = key;
    history_key.save_dir = history_dir.string();
    history_key.cycle = 0x80000001u;
    static constexpr char cycle_hex[] = "0123456789abcdef";
    std::string history_character;
    for (const auto byte : history_key.character) {
        history_character.push_back(cycle_hex[byte >> 4]);
        history_character.push_back(cycle_hex[byte & 15]);
    }
    const auto history_base = history_dir / ("home-" + std::to_string(history_key.slot) +
                                               "-" + history_character + ".bin");
    const auto history_one = history_dir / ("home-" + std::to_string(history_key.slot) +
        "-" + history_character + "-cycle-80000001.bin");
    const auto history_two = history_dir / ("home-" + std::to_string(history_key.slot) +
        "-" + history_character + "-cycle-80000002.bin");
    auto history_home = old_home;
    history_home.key = history_key;
    history_home.phase = Phase::Prepared;
    history_home.party = {31, 32, 33};
    history_home.admission = 0;
    history_home.pending.clear();
    history_home.committed.clear();
    history_home.retained.clear();
    auto history_lease = CharacterLease::Acquire(history_one, ec);
    assert(history_lease && StoreJournal(*history_lease, history_home, ec));
    history_lease.reset();
    auto next_key = history_key;
    next_key.cycle = 0x80000002u;
    const bool unresolved_history = HasUnresolvedPriorCycleHome(history_two, next_key, ec);
    assert(unresolved_history && ec == std::errc::operation_in_progress);
    history_lease = CharacterLease::Acquire(history_one, ec);
    assert(history_lease);
    history_home.phase = Phase::Visiting;
    assert(StoreJournal(*history_lease, history_home, ec));
    history_home.phase = Phase::Returning;
    assert(StoreJournal(*history_lease, history_home, ec));
    history_home.phase = Phase::Complete;
    assert(StoreJournal(*history_lease, history_home, ec));
    history_lease.reset();
    assert(!HasUnresolvedPriorCycleHome(history_two, next_key, ec) && !ec);
    auto history_anchor_lease = CharacterLease::Acquire(history_base, ec);
    ManagedCycleEpoch reanchor;
    reanchor.key = history_key;
    reanchor.epoch = 1;
    reanchor.target_key_cycle = history_key.cycle;
    reanchor.serialized_sha256 = before;
    assert(history_anchor_lease && !StoreManagedCycleEpoch(*history_anchor_lease, reanchor, ec) &&
           ec == std::errc::file_exists);
    ManagedCycleEpoch prior_cycle;
    prior_cycle.key = key;
    prior_cycle.stage = ManagedCycleStage::Stable;
    assert(StoreManagedCycleEpoch(*prior_lease, prior_cycle, ec)); // Preserve a complete same-cycle home.
    auto prior_intent = BeginManagedCycleIntent(prior_cycle, 6, 20, before);
    assert(prior_intent && StoreManagedCycleEpoch(*prior_lease, *prior_intent, ec));
    auto prior_applied = ObserveManagedCycleApplied(*prior_intent, 6, 7);
    assert(prior_applied && StoreManagedCycleEpoch(*prior_lease, *prior_applied, ec));
    auto prior_serialized = ObserveManagedCycleSerializeEnd(*prior_applied, after, 20);
    assert(prior_serialized && StoreManagedCycleEpoch(*prior_lease, *prior_serialized, ec));
    auto prior_committed = CommitManagedCycleEpoch(*prior_serialized, 21, after, after);
    assert(prior_committed && StoreManagedCycleEpoch(*prior_lease, *prior_committed, ec));
    assert(Read(prior_path) == prior_bytes);
    ManagedCycleEpoch forbidden_anchor;
    forbidden_anchor.key = key;
    forbidden_anchor.key.cycle = 0x80000001u;
    forbidden_anchor.epoch = 1;
    forbidden_anchor.target_key_cycle = forbidden_anchor.key.cycle;
    forbidden_anchor.serialized_sha256 = before;
    assert(!StoreManagedCycleEpoch(*prior_lease, forbidden_anchor, ec) && ec);
    assert(Read(prior_path) == prior_bytes);
    const auto anchor_path = root / "managed-cycle-anchor-old-home.bin";
    auto anchor_lease = CharacterLease::Acquire(anchor_path, ec);
    auto anchor_home = old_home;
    anchor_home.phase = Phase::Prepared;
    assert(anchor_lease && StoreJournal(*anchor_lease, anchor_home, ec));
    anchor_home.phase = Phase::Visiting;
    assert(StoreJournal(*anchor_lease, anchor_home, ec));
    anchor_home.phase = Phase::Returning;
    assert(StoreJournal(*anchor_lease, anchor_home, ec));
    anchor_home.phase = Phase::Complete;
    assert(StoreJournal(*anchor_lease, anchor_home, ec));
    assert(!StoreManagedCycleEpoch(*anchor_lease, forbidden_anchor, ec) && ec);
    assert(LoadJournal(anchor_path, key, ec) == anchor_home);
    std::puts("PASS: cycle-6 transition can add an intent sidecar beside a complete home; a fresh cap-7 anchor cannot replace prior history");
    std::puts("PASS: managed NG epoch requires pre-increment intent, applied mutation, serialized hash and matching later receipt; ordinary saves retain proof");
}

void TestJournal() {
    char pattern[] = "/tmp/bbsc-party-test-XXXXXX";
    const char* made = mkdtemp(pattern);
    assert(made);
    const fs::path root = made;
    TestJournalRoundTrip(root);
    TestUnadmittedRollback(root);
    TestJournalRejectsCorruption(root);
    TestWriteFailures(root);
    TestLeaseAndRestart(root);
    TestDurableEventAck(root);
    TestPreparedWorldSnapshot(root);
    TestSelectedSaveBackup(root);
    TestManagedCycleEpochFence(root);
    fs::remove_all(root);
    std::puts("PASS: journal");
}
