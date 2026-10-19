// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bbsc_party.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <limits>
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

static void TestJournalRejectsCorruption(const fs::path& root) {
    const auto path = root / "corruption.bin";
    auto record = Record(root);
    std::error_code ec;
    auto lease = CharacterLease::Acquire(path, ec);
    assert(lease && StoreJournal(*lease, record, ec));
    const auto bytes = Read(path);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
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

void TestJournal() {
    char pattern[] = "/tmp/bbsc-party-test-XXXXXX";
    const char* made = mkdtemp(pattern);
    assert(made);
    const fs::path root = made;
    TestJournalRoundTrip(root);
    TestJournalRejectsCorruption(root);
    TestWriteFailures(root);
    TestLeaseAndRestart(root);
    TestDurableEventAck(root);
    TestPreparedWorldSnapshot(root);
    fs::remove_all(root);
    std::puts("PASS: journal");
}
