# BBSC Anywhere Co-op Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let password-matched friends join across story maps or the Dream, earn only new shared progress, and return safely to their own world after leaving or restarting.

**Current handoff (2026-10-19; supersedes historical status below):** Development
is paused at the user's requested checkpoint on branch `pre`. Do not package
AppImage/EXE. On resumption, complete the agreed source features first and stop
immediately before packaging. Implemented source includes ABI3 save receipts,
ordered/targeted party protocol foundations, native character-bound home snapshots,
managed restore hooks and the journal-v4 post-destination save fence. Normal joins
do not create managed records; admission/event consumers are not enabled. Resume
with native admission/capability/backup binding, offline module loading and return
warp/retry, explicit event/item catalog and application, exact managed NG cycle
isolation, then Windows integration. Win32 journal APIs exist in source, but its
CMake source inclusion remains under `NOT WIN32`; no Windows build is verified.
Focused Linux module/party/join/transport checks passed. Exact-point return success
is unit-tested only; the native join fixture checks mismatch refusal, not a complete
successful native return. See the current summary in the evidence document.

Earlier dated paragraphs and unchecked plan steps are historical design/progress,
not an authoritative statement that every integration seam remains unchanged.

**Release priority (user, 2026-10-19):** Complete the requested feature paths first,
then produce Linux AppImage and Windows EXE pre-releases with explicit untested
areas. Continue implementation without repeated gameplay checkpoint pauses. Keep
focused build/save-durability/peer-binding/idempotency checks; move the broad
same-map/cross-map/death/disconnect/restart/NG acceptance matrix after packaging.
Do not equate deferred manual validation with completed implementation. Remaining
release work, in order: connect verified identity and journal to native admission
and return recovery; wire the ordered event channel to first-playable boundaries;
finish explicit boss/story/item application and native-save acknowledgement;
implement managed cycle transitions and conflict deferral; port journal durability
to Windows; build both packages and describe outstanding gameplay checks.
An unavailable ending save defers live NG verification, not independent code work.

**Architecture:** Keep native joins, world presentation, inventory operations, and warps in BBSC's existing game-thread code. Add a small testable party policy/state module and durable journal, connect them to existing HTTP/signaling, and expose only the runtime save/recovery facts the module cannot obtain reliably today. Resolve native and broker integration gates before enabling changes to saves.

**Tech Stack:** C++23, existing C runtime, CMake/Ninja, standard library/POSIX file operations, installed nlohmann_json, existing shadNet/NP signaling, assert-based tests. No new dependencies.

**Spec:** [Approved design](../specs/2026-10-18-bbsc-anywhere-coop-design.md). Read both documents before execution.

**Execution:** Native authorized by the user. Partial automatic foundation work is implemented; the user has completed the first [manual evidence checkpoint](../specs/2026-10-18-bbsc-integration-evidence.md), and automatic work has resumed. Full native build/tests now pass in bbport-dev. A Kit-blocking dma-buf scan read failure was reproduced/fixed and covered by portable plus real-GPU tests; host-only Kit delivery is now confirmed by the user and log. Same-map native join and voluntary return are now confirmed by the user and both captures after fixing the split-host relay setting. The guest capture identifies map 0x18010000 (Central Yharnam). Legacy shared-progress merging imported 103 world flags, so checkpoint launches now explicitly disable it; the agreed event-only replacement remains pending. The next cross-map run confirms transport but exposes repeated target clearing and return retaining the host location. The first native-home correction failed gameplay: the engine also overwrites the sign block. A separate process-local live pre-join snapshot now restores sign+forced blocks at co-op send-home, with an engine-overwrite regression. Host also polls password co-op searches through the existing bounded worker; real loopback HTTP-worker tests verify candidate delivery and cancellation. Full module build and regressions pass; the user now confirms successful joins and correct original-point voluntary returns across two sessions. Logs confirm exact origin/return tuples. Parting Blank diagnostics exclude param reset and show the vanilla menu reaching no-resonance fallback. Native compatibility correction now sets the missing cloned-item discriminator and makes11251 use its native leave effect directly without that menu, retaining animation15. The next user test disproves presentation success: cross-map guest still drinks/host gets missing text, and host fires without ending the party. User confirms same-area joins have the desired exit animation/messages. The next four-case test shows the host fallback never receives an actual local use cue, so it has been removed. User explicitly requests the original stop-co-op/stop-bell menu; it is restored from the vanilla row. Legacy post-summon forced reload recreated a correctly loaded guest; a new actual game-tick regression reproduces that extra StageTransition and verifies its removal, one placement and invader exclusion. Native menu and return regressions pass. User now confirms one cross-map load, correct guest Blank animation/messages and working host/guest exit. Host closure logs RemoveMember then guest OnRoomDisappeared without OnP2PTimeOut; the generic session-disconnected notice is consistent with intentional room closure in that trial. Lighting across all maps and the later durable/event-only gates remain unverified. This is not the planned durable recovery implementation. Stable identity, durable recovery and admission/event gates remain open. Resume from the ledger, not from task 1 again.

**Checkpoint 2026-10-19:** Native join/Blank/original-point return is user-confirmed.
Selected slot1 and the copied-profile create → reload → Load Game delete →
create-again sequence now have verified native/save evidence. All captures and
hashes are recorded in the evidence document. Do not repeat these observations.
Runtime ABI2 receipts were live-confirmed across ephemeral mounts. ABI3 now adds
locked generation/byte-count/SHA256 binding to the committed copy; automated
runtime/module tests and live generation/hash correlation pass. Neither bridge
alone proves incarnation or a newly earned event.

**Resume here:** Native character ID allocation, reload stability and same-slot
replacement distinction now pass. Integrate that discriminator with the verified
selected character/save path and resolve uncapped cycle binding before enabling
the durable journal or event/reward consumers. A deleted
slot retains its previous userdata payload; path, slot, name, timestamps or the
mere presence of that file are insufficient. Tasks1 and4 remain partial; the
policy/journal foundations do not complete admission, recovery, native safety
fallback, broker capability binding or full boss/story reward coverage. The
legacy whole-group merge stays disabled in controlled checkpoint launches.
The opt-in save lifecycle hooks are read-only and do not create identity tokens.

Verified native identity source: local-data+688/+690 are labelled ServerUserId /
ServerChrId; the game requests CharaIdNum=1 and persists the returned PublishCharaId.
The repository gameserver's empty-list stub is replaced with durable SQLite-backed
allocation (see evidence). Local route/storage/HTTP/container checks pass. The user
installed image0.6-pre1-native-id1 on Unraid with persistent/data storage, and the
private allocation endpoint returns a positive ID. The exact previous test profile
was backed up as native-id-before with BBSCTrace slot1 holding ID0. The first
allocation trial is accepted as native-id-allocated: native slot1 load, own-world
serialization/copy/commit/receipt, positive ID in current and backup, normal close,
and unchanged slot0. The next process reload is accepted as native-id-reloaded,
retaining the exact positive ID with successful commits and unchanged slot0.
The replacement trial is accepted as native-id-reused: clear1, metadata commit,
reserve1, successful native save and a different positive ID; BBSCProbe slot0 is
unchanged. Preserve all accepted snapshots; these three identity trials need no
repeat. Next resolve runtime key/cycle binding rather than guessing another UUID.
Do not repeat the already-verified deletion observer experiment. Existing nonzero
unset sentinels may bypass native allocation; do not silently clear or adopt them.
The native module now correlates selected-slot memory IDs with the canonical
BBSC_SAVE_DIR userdata file in the opt-in ready-own-world trace, without mtime
selection. Native invalid/foreign/corrupt evidence tests and the full build/runtime,
six native suites and131 Python tests pass. The live reader observation now passes
as native-id-correlated: exact slot1 native IDs and canonical selected file match
before the first current-process character commit; later save/receipt succeeds
and slot0 stays unchanged. Do not repeat this reader/allocation/reuse experiment.
Next resolve the uncapped-cycle transition/save contract before connecting the
complete CharacterKey and offline recovery coordinator. No CharacterKey/recovery consumer is
enabled, and the capped NG count is still evidence only.

NG research resolves bank2003/command21 to the inlined increment in017C0420,
alongside the Lua export and0131D260; all clamp at7. All27 original EMEVD files
were checked: m21 events12100000/index43,12100002/index35 and12100180/index14
contain the three matches. These observations do not prove a committed cycle.
Nearby command28 targets CSTrophy, not a proven ending/save boundary. Resume at
increment→selected native snapshot→successful commit correlation and interrupted
7→7 replay handling; do not repeat completed identity gameplay trials.

Serializer014EA560/reader014EAA80 establish a separate eight-byte cycle section
at header+2C/+30. Its first word is the capped count; the second is not proven
as an uncapped counter. Serialization can select cached global+159C via1598.
The opt-in identity diagnostic now reports live/saved counts separately and tests
invalid section bounds/counts plus valid disagreement. This is evidence only;
matching counts and successful closes still do not authorize cycle recovery.

Current checkpoint: shared bounded serialized-snapshot parser/SHA256 and three
pre-increment observers are built/tested, opt-in and read-only. The user has no
near-ending/NG+7 fixture. Existing guest BBSCTrace was used to compare
load→successful serialize-end→copy-input→saved-file hashes/commits without joining
or recreating characters. Actual cycle transition/replay remains unverified, so no recovery/event consumer
is enabled. Atomic runtime generation/hash correlation now passes the live trial.

The live buffer-provenance checkpoint now passes as native-snapshot-correlated:
three successful own serialize/copy pairs, final file equals last pair, loaded
hash matches prior baseline and all other character slots unchanged. Do not
repeat this trial. Generation2 observes the second commit, not the final third
save. Resume at atomic runtime generation/hash binding; no actual NG transition
has been exercised, and cycle/admission/recovery/event gates remain open.

ABI3 implementation now hashes the exact temporary copy and holds the receipt
lock through rename, directory durability and publication. A failed close or
hash clears bytes/hash and cannot advance generation; a hash diagnostic failure
does not fail the game's save. Runtime and module must be rebuilt together.
Live checkpoint now passes as native-receipt-correlated. Generation2/error0,
1310720 bytes and hash269a8e27…0840 match the second successful native
serialize/copy/commit. The final file instead matches the later third pair,
ef99a2dd…cade; do not attribute that third snapshot to the observed generation2.
All nine other character slots are unchanged; current/backup IDs agree, normal
closure, no cycle entry. Full raw/save/manifests are retained without overwriting
prior captures. This checkpoint is complete; do not repeat it. Resume at the
uncapped cycle transition/intent/replay contract and complete CharacterKey before
offline recovery. The user has no near-ending/NG+7 fixture; do not fabricate a
transition or infer an exact cycle from saturated count7. No new gameplay trial
is needed for the completed identity, snapshot or ABI3 receipt checks.

Post-increment evidence now has three opt-in byte-checked sites:0131D2A8,
01334C05 and017C045A. They validate the final store plus retained arithmetic
registers; the shared event epilogue additionally requires command21 and success.
The Lua guard can reach its epilogue without incrementing and is rejected.
Actual hook-provider fixture replay executes captured native arithmetic for0→1,
6→7 and7→7 and verifies a skipped guard produces no applied observation. This
closes the instruction/observer seam, not the live ending→save→restart contract.
Do not request another unchanged fresh-character load to test an NG transition.
Next correlate a durable transition intent, applied observation and a subsequent
selected snapshot/ABI3 receipt, preserving ambiguous or interrupted7→7 evidence.
No actual cycle was changed in the copied profile. Windows runtime now explicitly
links the existing OpenSSL Crypto dependency for ABI3 hashing; no Windows build
or package validation is claimed.

Task5 protocol components now pass focused tests: type0x28/version1, fixed46-byte
header, ≤1024-byte packets, bounded baseline chunks; ordered per-guest admission;
peer-bound targeted routing; room-owner/membership snapshots; bounded receive
queue; retained delivery retries and cumulative Ack after durable intent storage.
Journal v3 adds the prepared snapshot reference; retained claims preserve earlier
party/admission across subsequent parties, and journal v1/v2 remain readable. The actual host character is required for initial placement.
Task5 remains partial: native first-playable timing, event capture, retry cadence
and recovery must still be connected to these components. Catalog membership and
native inventory/save completion are not established by a packet or intent Ack.
Do not treat this as enabled event-only progress sync.

Task6 home adapter now has focused native-fixture coverage: canonical selected-file
and native-ID CharacterKey, cycle0..6 only when saved/live counters agree, leased
origin plus native-resolved own-world respawn destination, and preservation of
pending/corrupt/foreign records. Native Lua dispatch01339870 has a verified null-name
zero-return branch, now used by a Linux guard for existing unresolved managed records.
This rejects before the event is delivered, without inventing a return value.
Actual hook-provider replay verifies RSI changes reach the predicate and unrelated
Lua events/other characters remain unaffected. Own-world capture/load curtain and
Blank leave handling moved out of sampled JSON formatting. A bounded 2 MiB own-world
payload is now bound to the verified in-memory CharacterKey, persisted in an immutable
per-party snapshot, and referenced by journal v3 with size/CRC validation. Legacy
slot-only world files are not adopted as managed snapshots. Focused journal/native
fixtures cover wrong-character/party, truncation/corruption, missing-reference and
write-failure rejection. Task6 remains partial: normal joins do not invoke prepared
capture; the snapshot is not yet consumed by restart recovery, and capability/backup,
candidate binding, safe fallback, return/save confirmation and Windows journal work
remain before activation.

## Pre-release implementation priority (user update)

Implement and connect the agreed features before the broad manual acceptance
matrix. Do not wait on the unavailable ending/NG+7 fixture to implement independent
admission, targeted transport, durable event acknowledgements, return coordination
and packaging. Keep per-change build checks and focused assertions for save loss,
wrong-host events and duplicate rewards; defer full-map/soak/Windows gameplay
validation until the feature implementation is connected. Track implemented,
enabled and manually verified separately. Do not describe a disabled foundation
as a completed feature or bypass unresolved save identity checks to meet a date.

## Development environment

- User requires **distrobox `bbport-dev`** for all builds and automated development tests. Use its installed dependencies; do not download a substitute toolchain or install packages on the host.
- From a host terminal: `distrobox enter bbport-dev -- bash -lc 'cd /home/kao/.codex/worktrees/c51a/bloodborne_pc && <command>'`.
- From the agent container, prefix that command with `host-spawn -no-pty` (subject to tool sandbox access). Confirm `CONTAINER_ID=bbport-dev` before compiling.
- Build this worktree's sources and retain its outputs here. Inspect submodule state before full configuration; do not reset another checkout's submodules. Existing standalone results do not replace full-module verification.
- This environment also needs the primary build's existing CONFIG adapters for system Zydis/Protobuf, the repository's pinned magic_enum fallback (the system package lacks `magic_enum/magic_enum.hpp`), and newer Vulkan headers for the existing VALVE mixed-float-dot code. These are copied/reused under ignored `out/build-deps`; no host packages or production dependency versions were changed. The tested configure flags appear below.
- Gameplay remains a user-run checkpoint using copied saves. The checkpoint script enters `bbport-dev`, explicitly selects the baseline executable via `BB_PROBE`, and shows output while keeping a log. Use `--check` for a non-gameplay dependency/path check.

## Global Constraints

- "Support main-story maps and the Hunter's Dream. Chalice Dungeons are excluded."
- "Cross-map matching requires the same nonempty co-op password."
- "Do not copy whole flag groups."
- "Do not persist passwords or stale runtime pointers."
- "The game thread owns admission, world changes, event application, and return."
- "Do not change invasion placement/progress or enable cross-map public matching."
- "Never overwrite an unresolved recovery record with a new party."
- "The release catalog must cover the agreed boss/story progression throughout the supported maps."
- Preserve current byte verification, save backups, user exclusions, co-op capacity, same-map matching rules, and unrelated level/channel restrictions.
- Keep the return journal until return/save confirmation; keep deferred event rewards independently afterward. No whole-save rollback or import of older host progress.
- User now explicitly selects private test servers: shadNet `100.119.93.51:31313`, WebAPI `http://100.119.93.51:31315`, game server `http://100.86.192.108` (native port 20443). Pin these plus `SHADPS4_P2P_RELAY=100.86.192.108:3478` in both checkpoint-2 launch roles and disable local-game-server fallback. Verify relay registration and bidirectional frames, not only HTTP/TCP reachability. This supersedes the earlier public-first test target; production launcher defaults are unchanged. Obtain and plan changes against actual compatible broker source if protocol changes prove necessary; do not modify bbhost_service.py as a substitute.
- This checkout is already a managed worktree with a detached HEAD. Do not create another checkout or change global Git identity. Git author is unset; retain changes if commits cannot run and report that limitation.

## Review Focus

1. Offline restart with BBSC/online toggled off must still recover the recorded character without network initialization (Tasks 4, 8, 9).
2. Reordered baseline/event messages and a late third guest must never turn old host progress into newly earned progress (Tasks 5, 7, 9).
3. A consumed key or a crash between item grant and save/ledger completion must not issue the item twice (Tasks 3, 7).
4. A recycled slot, different local user, changed game cycle, or second process must never reuse another character's return record (Tasks 3, 4, 8).
5. A stale broker placement, renamed directory without a durable flush, or invalidated original point must not produce an unsafe join/return (Tasks 3, 6, 8).

## File ownership and boundaries

| Path | Responsibility |
| --- | --- |
| Create `gpu/bbnet/include/core/bbsc_party.h` | Value types, eligibility, ordered admission/event state, journal contracts; no live guest memory. |
| Create `gpu/bbnet/shadps4/core/bbsc_party.cpp` | Policy, candidate parsing using installed JSON, wire parsing, admission/event bookkeeping and deterministic return decisions. |
| Create `gpu/bbnet/shadps4/core/bbsc_party_journal.cpp` | Bounded encoding/decoding, atomic durable storage, character lock, reconciliation metadata. |
| Create `gpu/bbnet/shadps4/core/bbsc_party_events.inc` | Verified map and event/item catalog; no speculative flags. |
| Modify `gpu/bbnet/shadps4/core/bloodborne_re.cpp` | Game-thread adapters, native hooks, old-helper integration and coordinated recovery. Do not restructure unrelated sections. |
| Modify `gpu/bbnet/include/core/bloodborne_re.h` | Cross-thread packet enqueue and recovery-only entry declarations. |
| Modify `gpu/bbnet/shadps4/core/libraries/network/http.cpp` | Parse/validate selected candidate/claim and bind placement to the current attempt. |
| Modify `gpu/bbnet/shadps4/core/libraries/np/np_signaling/np_signaling_{bbsc.h,state.h,helpers.h,helpers.cpp}` | Versioned party packets, peer resolution, targeted transmission and queueing. |
| Modify `gpu/bbnet/bbnet.h`, `bbnet_glue.cpp`, `src/runtime_net.c`, `src/runtime.h`, `src/runtime_file.c` | Versioned save facts and a recovery-only module entry that works offline. |
| Modify `gpu/bbnet/CMakeLists.txt` | New sources and explicit `bbsc-party-test` target, without booting a game or requiring a Vulkan device for test execution. |
| Create `tests/test_bbsc_party.cpp`, `tests/test_bbsc_party_journal.cpp`; extend `tests/test_file_mods.c`, `tests/test_runtime.c`, `tests/test_online.c` | Focused state/journal/protocol tests and actual runtime save/module-boundary tests. |
| Create `docs/superpowers/specs/2026-10-18-bbsc-integration-evidence.md` | Record verified native hooks, save identity/commit semantics, broker fields, and catalog coverage. |
| Modify `docs/BBSC.md` | Final usage, compatible builds, recovery behavior and measured limitations. |

Keep production helpers internal unless another task needs the declared interface.
Do not create a generic networking, storage, transaction, or quest framework.

## Shared interface decisions

Use namespace `Core::Bbsc::Party` and standard fixed-width integer types. Interfaces below are planned additions, not existing functions.

```cpp
using PartyId = std::array<std::uint8_t, 16>;
using EventId = std::uint32_t;
struct CharacterKey {
    std::string save_dir, user;
    std::uint8_t slot;
    PartyId character; // stable character discriminator, not a random value each launch
    std::uint32_t cycle;
};
struct Destination {
    std::uint32_t map;
    float x, y, z, heading;
    std::int32_t lamp;
};
enum class Phase : std::uint8_t { Prepared, Visiting, Returning, Complete };
enum class ReturnChoice { OriginalPoint, SavedLamp, NativeInitial, Unavailable };
struct ProgressionReward { std::uint32_t goods; std::int32_t count; std::uint32_t claim_flag; };
struct EventDefinition {
    EventId id;
    std::vector<std::uint32_t> completed_flags, added_flags, required_flags, conflicting_flags;
    std::vector<ProgressionReward> rewards;
};
struct Journal;    // defined with Task 3's exact persisted fields
struct Peer;       // stable NP identity resolved from an established connection
struct Message;    // validated wire envelope defined in Task 5
class Admission;   // ordered host/guest boundary and earned-event state, Task 5
bool IsSupportedStoryMap(std::uint32_t map);
bool CanCrossMapMatch(std::uint32_t from, std::uint32_t to,
    std::string_view ours, std::string_view theirs, bool cooperative);
ReturnChoice ChooseReturn(bool original_safe, bool saved_lamp_valid,
    bool native_initial_valid);
```

Task 1 freezes native signatures and real protocol fields in the evidence file;
do not invent them in advance. Remaining interfaces are fixed in their owning
tasks below. Any necessary change must update every consuming task and test.

## Execution and verification conventions

No product code is changed by writing this plan. During execution, every nontrivial
task follows red/green: add its named assertions, run to see the intended failure,
implement only its scope, rerun that test, then commit only its files when Git
identity is available. Never make an entire-task commit with unrelated staged files.

After Task 2 creates the target, use:

```bash
cmake -S gpu -B out/gpu -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBB_PGO=off -DBB_LTO=OFF \
  -DZydis_DIR="$PWD/out/build-deps" -DProtobuf_DIR="$PWD/out/build-deps" \
  -DCMAKE_DISABLE_FIND_PACKAGE_magic_enum=TRUE \
  -DVulkan_INCLUDE_DIR="$PWD/out/build-deps/vulkan-include"
ninja -C out/gpu bbsc-party-test
out/gpu/bbsc-party-test policy
```

Run these commands inside `bbport-dev`. If configuration still fails, inspect its
missing dependency/submodule report; do not fetch GPU assets or reset submodules
merely to test party policy. Standalone foundation outputs already exist in `out/`. A missing target/module is a setup
failure, not a passing or skipped acceptance test. Test sections use `policy`,
`journal`, `protocol`, `events`, and `return`; no argument runs all sections.

## Task 1: Verify native boundaries and the existing broker exchange

**Files:** Read the existing files listed above; create the integration evidence document. Inspect `src/runtime_savedata.c` and `gpu/shim/bbport_guest_code_hooks.{h,cpp}` without changing them unless evidence establishes a required caller change.

**Interfaces:** Consumes the approved design and existing hooks. Produces concrete verified signatures/byte checks for stopping a join, selected-character identity, safe placement/fallback, and save completion; redacted broker request/response field schemas; a complete map/event coverage checklist.

- [x] **Step 1: Establish the build and controlled test characters.** Inspect submodule status and `build.sh` before using it; preserve unrelated submodule edits instead of taking its reset path. Build the existing online module with the normal safe build path; confirm output includes `Online module: on` and produces `out/gpu/libbbnet.so`. Back up disposable characters; identify actual game data and two-player access without requesting credentials in chat.
- [ ] **Step 2: Trace acceptance and save identity.** Use existing `BB_SAVE_TRACE=1` and BBSC RE capture during one controlled join/leave. Locate a byte-verified, reject-capable point before host-state mutation. Prove selected slot/character identity from the game and its mounted save path; newest-file time alone fails the gate.
- [ ] **Step 3: Verify native save and destination evidence.** Trace a successful and failed save close; inspect `commit_file`/`sync_directory`, which currently ignores directory-sync errors. Identify the game's save-generation boundary and native point-safety/fallback calls. Read existing `BbscBossRewards`/`BbscBossGoodsLots` instead of rebuilding their mappings blindly.
- [ ] **Step 4: Capture the configured broker with test passwords.** Exercise same/different/empty passwords and different maps using existing summon capture. Record which candidate/claim fields bind the selected host, password semantics, and placement, and when peer capability exchange can run before a world mutation. Redact accounts/tokens/passwords from retained evidence.
- [ ] **Step 5: Freeze evidence and go/no-go results.** The evidence file must state each observation and its source/capture. Missing reject capability, reliable identity, save confirmation, or safe native fallback blocks the relevant mutation. If public-broker identity/password binding is impossible, obtain the actual compatible shadNet broker source and amend this plan with exact server files, fixtures, and tests before server edits; retain the full scope rather than weakening it.

**Gate result:** Verified observations, not guessed offsets or an assertion that a public server works. Continue independently testable policy/journal work while external observations are pending; do not enable live mutations until their prerequisites pass. Commit only the evidence document if possible.

## Task 2: Add the verified map/event catalog and smallest policy test seam

**Files:** Create `bbsc_party.h`, `bbsc_party.cpp`, `bbsc_party_events.inc`, `tests/test_bbsc_party.cpp` at the paths above; modify `gpu/bbnet/CMakeLists.txt`.

**Interfaces:** Consumes Task 1's map/event evidence and `BbscBossRewards`/`BbscBossGoodsLots`. Produces the shared value types and policy signatures above, plus `std::span<const EventDefinition> EventCatalog()`; Task 1 maps each definition's ID/completed_flags to verified completion evidence. Use owned storage in the catalog and spans for read-only views.

- [ ] **Step 1: Add failing `policy` assertions.** In particular:

```cpp
assert(CanCrossMapMatch(story_a, dream, "friends", "friends", true));
assert(!CanCrossMapMatch(story_a, dream, "", "", true));
assert(!CanCrossMapMatch(story_a, dream, "friends", "other", true));
assert(!CanCrossMapMatch(story_a, chalice, "friends", "friends", true));
assert(!CanCrossMapMatch(story_a, dream, "friends", "friends", false));
assert(!IsSupportedStoryMap(0xffffffff));
```

  Define `story_a`, `dream`, and `chalice` from Task 1's verified fixtures. Also assert unique catalog IDs, required reward coverage, and exclusion of NPC alternatives/exclusive phase bits.
- [ ] **Step 2: Register `bbsc-party-test` and run `ninja -C out/gpu bbsc-party-test`.** Expect missing policy definitions/catalog assertions to fail, rather than an unrelated compiler/dependency failure.
- [ ] **Step 3: Implement only the declared policy/catalog.** Use the existing game's password comparison semantics. The catalog explicitly distinguishes already-claimed and consumed required items; ordinary repeat consumables are not progression rewards. Verify every supported boss/story row against actual data; development subsets do not satisfy the coverage gate.
- [ ] **Step 4: Run `out/gpu/bbsc-party-test policy`.** Expected: `PASS: policy`, including catalog coverage. Add sources to `BBNET_SOURCES`; compile with `-UNDEBUG -Wall -Wextra -Werror` for the test target.
- [ ] **Step 5: Commit only the catalog, policy, test, and CMake changes.** Suggested subject: `feat: define verified BBSC party eligibility and event catalog`.

## Task 3: Persist party recovery and earned-event intents durably

**Files:** Create `bbsc_party_journal.cpp`; extend `bbsc_party.h`, `bbsc_party.cpp`, `tests/test_bbsc_party.cpp`, and `gpu/bbnet/CMakeLists.txt`.

**Interfaces:** Produces `Journal { CharacterKey key; PartyId party; std::uint64_t admission; Phase phase; Destination origin, fallback; std::uint64_t save_generation; std::vector<EventId> pending, committed; }`, `bool StoreJournal(const CharacterLease&, const Journal&, std::error_code&)`, `std::optional<Journal> LoadJournal(const std::filesystem::path&, const CharacterKey&, std::error_code&)`, and noncopyable/movable `CharacterLease` with `static std::optional<CharacterLease> Acquire(const std::filesystem::path&, std::error_code&)`, holding an OS lock for its lifetime. The lease is a required write argument (ledgered execution ruling). Persist original-world snapshot references under the same party identity; keep consumed/reward claim evidence across completed party records.

- [ ] **Step 1: Add failing `journal` tests**, named `TestJournalRoundTrip`, `TestJournalRejectsForeignCharacter`, `TestJournalRejectsCorruption`, and `TestLeaseExcludesSecondProcess`, for round-trip identity/phase/coordinates; truncation/version/count overflow; NaN/infinity; wrong user/character/cycle; two-process lock contention; and legacy-file preservation. Assert a complete-return record can retire without erasing deferred event intents. Core assertions (variables are local test fixtures):

```cpp
assert(StoreJournal(*lease, visiting_record, ec));
assert(LoadJournal(path, visiting_record.key, ec)->phase == Phase::Visiting);
assert(!LoadJournal(path, another_character, ec));
assert(!CharacterLease::Acquire(path, ec)); // another process owns this record
```
- [ ] **Step 2: Run `out/gpu/bbsc-party-test journal`.** Expect the newly named assertions to fail before implementing storage.
- [ ] **Step 3: Implement bounded version-1 encoding.** Fix endianness and explicit field lengths; validate sizes before allocating. The storage envelope allows at most 1024 unique nonzero event intents, `save_dir` at most 1023 bytes, and `user` at most 31 bytes; actual native application additionally requires verified catalog membership (ledgered execution ruling). Bind live paths to the configured save root. Include a payload checksum using the already-installed zlib CRC32 and test byte-flipped payloads; link the existing ZLIB target. Serialize no password, pointer, or guessed slot. Check temporary write/close, file flush, atomic replacement, and parent-directory flush; clean only this operation's temp file.
- [ ] **Step 4: Add and run write-failure/restart tests.** Use real temporary directories and subprocess interruption at write, flush, replacement, and directory-flush boundaries; verify restart reads a complete old/new record and any reported failure leaves an honest recoverable state. Use narrow I/O fault injection, not a production storage framework. Expected: `PASS: journal`.
- [ ] **Step 5: Commit only journal/state/test changes.** Suggested subject: `feat: persist BBSC party recovery and earned events`.

## Task 4: Expose confirmed save facts and allow recovery with networking off

**Files:** Modify `src/runtime_file.c`, `src/runtime.h`, `src/runtime_net.c`, `gpu/bbnet/bbnet.h`, `gpu/bbnet/bbnet_glue.cpp`; extend `tests/test_runtime.c` and `tests/test_online.c`.

**Interfaces:** Produces C value `BbNetSaveReceipt { uint64_t generation; char path[1024]; int32_t error; }`, host callback `int32_t (*save_receipt)(const char *save_path, BbNetSaveReceipt *out)`, and exported `int bbnet_recovery_init(const BbNetHost *host)`. Append the callback to `BbNetHost`, bump `BBNET_ABI_VERSION` to 2, and update every initializer/caller. A receipt confirms durable completion for that path/generation, not active-character identity. Identity still comes from Task 1's verified selected-character evidence.

- [ ] **Step 1: Add failing runtime assertions** for successful dirty character-save close producing one newer receipt; failed write/fsync/rename/directory flush producing no successful receipt; metadata/unwritten closes not confirming a character save; and two slots/users having separate receipts. Ensure `sync_directory` failures reach the close result. The test snapshots receipt values before each operation:

```c
assert(after_ok.generation > before.generation);
assert(after_failed.generation == before_failed.generation);
assert(after_other_slot.generation == before_other_slot.generation);
```
- [ ] **Step 2: Run the targeted runtime executable/test case using the existing test harness.** Add a `--bbsc-save-receipts` case to `tests/test_runtime.c`; run `out/runtime-test --bbsc-save-receipts`. Expect the new receipt behavior to fail before implementation.
- [ ] **Step 3: Implement receipts and the versioned host boundary.** Receipts are keyed by canonical committed host path; publish after locks are released, with bounded/thread-safe queries. Do not declare a game transaction saved merely because an unrelated file closed. Correlate receipts with Task 1's actual game save boundary, including a regression where a snapshot begun before the reward closes afterward: that older snapshot cannot confirm the reward. Wait for a proven subsequent native save, not just the next close counter.
- [ ] **Step 4: Add and implement `--bbsc-offline-recovery` in `tests/test_online.c`.** In a fresh process with `BB_ONLINE=0` and a matching pending journal, load the recovery-only entry before `runtime_net_image_ready`. It configures only the minimum game/memory/recovery hooks; it does not register online imports, sign in, initialize signaling, or call full `bbnet_init`. No journal means unchanged offline startup. Wrong/missing module with pending recovery gives an explicit error before unsafe character play, not a successful test skip.
- [ ] **Step 5: Run both cases and existing online-import tests.** After `bash build.sh --test`, run `out/runtime-test --bbsc-save-receipts`, `out/online-test --bbsc-offline-recovery`, and `out/online-test`. Expected: named `PASS` results with the module present; ABI-1 module mismatch is explicit. Commit the runtime/ABI/test changes together.

## Task 5: Establish capability, ordered admission, and durable event acknowledgement

**Files:** Extend `bbsc_party.h`, `bbsc_party.cpp`, `tests/test_bbsc_party.cpp`; modify the four NP signaling files in the ownership table and `gpu/bbnet/include/core/bloodborne_re.h`.

**Interfaces:** Produces `Peer { std::string online_id; std::int32_t connection; }`, `enum class Kind : std::uint8_t { Hello, Ready, Baseline, Event, Ack, Leave }`, `Message { PartyId party; std::uint64_t admission, sequence; Kind kind; std::uint16_t chunk, chunks; std::vector<EventId> events; }`, `std::optional<Message> DecodeMessage(std::span<const std::uint8_t>)`, `std::vector<std::uint8_t> EncodeMessage(const Message&)`, `void Admission::Begin(const Peer&, PartyId, std::uint64_t admission, std::uint64_t boundary, std::span<const EventId> baseline)`, `bool Admission::Accept(const Peer&, const Message&)`, `std::span<const EventId> Admission::Earned() const`, `void Admission::Close()`, and `u32 SendBbscPartyToPeer(s32 connection, const void *packet, u32 size)` in NP signaling helpers. Add `void OnBbscPartyPacket(std::int32_t connection, const void *packet, std::uint32_t size)` to the Bloodborne header; it copies/queues only.

- [ ] **Step 1: Add failing `protocol` tests** named `TestOrderedAdmission`, `TestWrongPeerRejected`, and `TestLateGuestBaseline` for Hello/Ready/Baseline/Event/Ack/Leave ordering, missing/duplicate chunks, old admission IDs, wrong peer, unsupported version, short/oversized packets, and replay. Give a late guest an independent baseline; repeat with the existing four-guest capacity. Core assertions after each fixture's `Begin` call:

```cpp
assert(admission.Earned().empty()); // old host events are in baseline only
assert(!admission.Accept(wrong_peer, new_event));
assert(admission.Accept(host_peer, new_event));
assert(admission.Earned().size() == 1);
assert(!DecodeMessage(truncated_packet));
```
- [ ] **Step 2: Run `out/gpu/bbsc-party-test protocol`.** Expect only the new parser/admission checks to fail.
- [ ] **Step 3: Implement version-1 party messages in the existing SHAD packet namespace.** Reserve type `0x28` only after checking all included sender/classifier/relay paths for conflicts, including `network/p2p_sockets.cpp`'s `ClassifyInternal`; modify that caller only if its routing needs an explicit new type. Keep each encoded datagram at most 1024 bytes, including its header; chunk a large baseline with a count bounded by catalog size. Define fixed field widths and enums beside existing `np_signaling_bbsc.h` declarations; no struct-layout casts of untrusted bytes.
- [ ] **Step 4: Implement targeted peer routing and retry.** Resolve source address/port against an established selected-host connection before queueing. Capability/baseline preparation must be available before world mutation; Task 1 must prove it. Host baseline capture and admission share a game-thread sequence boundary; retain intervening events, retry unacknowledged delivery, and acknowledge only Task 3's durable guest record. Bound queues by catalog/admission state rather than an unbounded retry list.
- [ ] **Step 5: Run `protocol`, `journal`, and `ninja -C out/gpu bbnet`.** Expected: both `PASS` sections and a successful native-module build. No old `0x26` boss list or presentation snapshot is treated as a new event under this feature. Commit protocol, mirrored declarations, and tests together.

## Task 6: Enforce password/host binding and capture home before the join

**Files:** Modify `http.cpp`, `bloodborne_re.cpp`, `bbsc_party.h`, `bbsc_party.cpp`, `tests/test_bbsc_party.cpp`, and `gpu/bbnet/include/core/bloodborne_re.h` as needed for the minimal caller seam.

**Interfaces:** Produces `JoinAttempt { std::uint64_t generation; std::string host; std::uint32_t source_map, target_map; std::int64_t expires_ms; }` and `bool ValidateJoinCandidate(std::string_view request_json, std::string_view candidate_json, const JoinAttempt&, std::uint64_t current_generation, std::int64_t now_ms)` in party policy; only memory-resident request parsing sees the password. The game adapter uses `bool PrepareBbscPartyJoin(const JoinAttempt&)` and `bool BbscPartyJoinAllowed()` on the game thread, consuming Tasks 1, 3, 4, and 5.

- [ ] **Step 1: Add failing `TestCandidateBinding`** using redacted Task 1 fixtures with dummy passwords. Assert same-map behavior remains unchanged, while cross-map empty/mismatched password, wrong host, expired/changed attempt, unsupported map, and non-co-op roles reject. Test a claim arriving after another host/password was selected:

```cpp
assert(ValidateJoinCandidate(request, candidate, attempt, attempt.generation, before_expiry));
assert(!ValidateJoinCandidate(request, candidate, attempt, attempt.generation + 1, before_expiry));
assert(!ValidateJoinCandidate(request, candidate, attempt, attempt.generation, attempt.expires_ms + 1));
```
- [ ] **Step 2: Run `out/gpu/bbsc-party-test policy`.** Expect the new fixture/binding cases to fail.
- [ ] **Step 3: Gate every candidate-to-placement transition.** Parse with installed JSON, preserving existing API payloads and comparison semantics. Apply checks to search results, preparation/claim consumption, advertisement rewriting, and native join. If public responses lack required evidence, take Task 1's private-broker branch; do not accept an unbound placement header as proof.
- [ ] **Step 4: Integrate durable pre-join capture at Task 1's reject-capable hook.** Require confirmed character identity, safe fallback, backup, capability, and character lease; store origin and own-world snapshot references before entering Visiting. Any failed prerequisite rejects before changing the guest world. Do not overwrite original origin during follow warps or rejoin after a kept death.
- [ ] **Step 5: Run policy/journal/protocol sections, rebuild `bbnet`, and perform one controlled failed-write join.** Expected: no host-world mutation and an actionable notice on failure; valid joins retain native map loading/lighting correction and target the real host. Commit only the eligibility/admission caller changes.

## Task 7: Replace permanent broad sync and make progression rewards idempotent

**Files:** Modify `bloodborne_re.cpp`, `bbsc_party.cpp`, `bbsc_party_journal.cpp`, and `tests/test_bbsc_party.cpp`; extend the catalog only with verified coverage.

**Interfaces:** Consumes `Admission`, `EventCatalog`, `CharacterKey`, durable intents and save receipts. Produces `bool RecordBbscPartyEvent(EventId event)` (persist eligible intent before acknowledging) and `void TickBbscPartyProgress()` (game-thread collection/application/reconciliation). Existing inventory and flag helpers remain the native adapters.

- [ ] **Step 1: Add failing `events` tests**, named `TestOnlyEventsAfterAdmission`, `TestNoRewardAfterDeparture`, and `TestNoNpcOverwrite`. Old baseline events give zero earned flags/items; a post-boundary event queues once through duplicate packets/death/load; an event after departure rejects; already-ahead guests preserve their state; NPC conflicts stay untouched. Include a map first visited later and a second admission to the same host:

```cpp
assert(admission.Earned().empty());
admission.Close();
assert(!admission.Accept(host_peer, later_event));
assert(admission.Earned().empty());
```
- [ ] **Step 2: Add reward replay assertions.** For each required catalog reward, distinguish carried, stored, claimed-and-consumed, pending-grant, and saved-grant states. Crash after grant/before receipt, after receipt/before ledger completion, or with inventory full must not duplicate or erase credit; deferred prerequisites must not mark an unsafe boss completion.
- [ ] **Step 3: Run `out/gpu/bbsc-party-test events`; implement the new collection/application path.** Stop collecting permanent progress from `bbsc_progress_host_flags` and kind-1 snapshot ORs under this feature. Retain host world/Dream presentation and own-world restoration. Observe verified event completions after admission, not mere appearance of old completion flags.
- [ ] **Step 4: Integrate all existing reward callers.** Inspect and update `NoteBbscBossRewardEvent`, `TickBbscBossRewards`, `GiveBbscBossGoods`, and `BbscGiveHostWorkshopTools` so no older host unlock bypasses admission or grants twice. Use original guest claim evidence rather than host-overlaid flags. Preserve ordinary earned loot and existing error handling; never mark a reward delivered before actual inventory/save confirmation.
- [ ] **Step 5: Run `events`, `journal`, `protocol`, and rebuild `bbnet`.** Expected: named `PASS` results, full supported catalog coverage, and no permanent raw-group merge in the new flow. Commit collection/reward/caller changes together.

## Task 8: Coordinate safe home return and restart recovery

**Files:** Modify `bloodborne_re.cpp`, `bbsc_party.h`, `bbsc_party.cpp`, `bbsc_party_journal.cpp`, `bbnet_glue.cpp`, and `tests/test_bbsc_party.cpp`; touch `bbsc_persistence.*`/`bbsc_patch_tables.h` only for a demonstrated teardown/hook need.

**Interfaces:** Produces `void BeginBbscPartyReturn(std::string_view reason)` and `void TickBbscPartyRecovery()` in the game adapter, using the fixed `ChooseReturn(...)` signature and Task 4's offline entry/save receipts. Incoming callbacks cannot invoke native warp or restore directly.

- [ ] **Step 1: Add failing `TestReturnDestination` assertions.** Invalid coordinates, failed collision validation after newly earned progress, and a new character with no lamp exercise the native fallback branch:

```cpp
assert(ChooseReturn(true, true, true) == ReturnChoice::OriginalPoint);
assert(ChooseReturn(false, true, true) == ReturnChoice::SavedLamp);
assert(ChooseReturn(false, false, true) == ReturnChoice::NativeInitial);
assert(ChooseReturn(false, false, false) == ReturnChoice::Unavailable);
```
- [ ] **Step 2: Add lifecycle/restart assertions** for Prepared/Visiting/Returning/Complete; explicit leave/confirmed disconnect/unsupported follow destination; interruption at each phase; wrong-character and recycled-slot load; and a deferred reward surviving completed return followed by another party. Preserve the existing 15-second stranded-session grace, not an immediate response to one lost packet.
- [ ] **Step 3: Run `out/gpu/bbsc-party-test return`; implement the single coordinator.** Stop old host presentation, close admission, restore own-world/Dream, apply safe earned progress at the native load boundary, validate/select return destination, and perform native return. Route explicit-leave, stranded-guest, quit/title, follow-host, and restart recovery through it. Do not let legacy restore/merge timers race the coordinator.
- [ ] **Step 4: Retire only after native save confirmation.** Archive return state after own world/destination/save are confirmed. Keep deferred event/reward intents separately; legacy raw progress files must be recovered under their existing lifecycle before new admission and never reinterpreted as newly earned events. Errors preserve journals and announce the actual failure.
- [ ] **Step 5: Run all party sections and the offline-recovery/runtime-save cases, then rebuild `bbnet`.** Expected: all named `PASS` results; recovery works with `BB_ONLINE=0` and `BB_BBSC=0` when a pending record exists, without enabling online imports. Commit coordinator, necessary native lifecycle changes, and tests together.

## Task 9: Verify the full flow, document compatibility, and review the branch

**Files:** Modify `docs/BBSC.md` and the integration evidence document; add any missing regression assertion to its owning test file. No new production scope.

**Interfaces:** Consumes all earlier deliverables. Produces the design's acceptance-matrix results, exact tested builds/server, and an explicit list of any gate still failing. A mock broker or a single-player launch is not evidence of real co-op success.

- [ ] **Step 1: Run focused native regression checks.** Run `ninja -C out/gpu bbnet bbsc-party-test bbsc-memory-test`, `out/gpu/bbsc-party-test`, and `out/gpu/bbsc-memory-test`; run the runtime save/online/offline cases from Tasks 4 and 8. Expected: real `PASS` output, no missing-module skips. Do not claim the broader suite passed without running it.
- [ ] **Step 2: Run the design's complete two-player acceptance matrix on disposable saves.** Test story-to-story, both Dream directions, locked guest destination, old host progress, new shared boss/key, different guest progression, death/loading, own leave, network loss, host crash, and Chalice transition. Record before/after maps, flags, inventory, journal phases, and save generations without recording passwords.
- [ ] **Step 3: Test process interruption and late guests.** Kill/restart the guest at every journal/native-save/reward boundary, including offline restart; test unsafe origin/no-lamp fallback, two local users, another character, and a three-player session with a late joiner. Confirm active guests retain separate boundaries/origins. Repeat only failed/changed scenarios rather than running unrelated soak tests.
- [ ] **Step 4: Update user documentation and evidence.** Explain same-password co-op, supported areas/builds, returning after disconnection/restart, pending rewards, and actual server compatibility. Record all map/event coverage and failed gates. Remove diagnostic secrets; leave controls out of the launcher unless a verified usability need requires a separate scope decision.
- [ ] **Step 5: Obtain the execution method's required independent final review.** Review connected callers, save data loss/duplication, stale-peer handling, and scope against the approved spec. Fix reportable findings and rerun the affected checks before completion; no push/deployment is implied by this plan. Commit documentation and reviewed fixes with exact file staging when author identity is available.

## Coverage and handoff

| Approved decision | Owning tasks |
| --- | --- |
| 1: story/Dream scope | 1, 2, 6, 8, 9 |
| 2: matching nonempty password | 1, 2, 5, 6, 9 |
| 3: guest may visit locked map | 6, 8, 9 |
| 4: durable original return/restart | 3, 4, 6, 8, 9 |
| 5: presentation separate from earned progress | 2, 5, 7, 9 |
| 6: additive progress without NPC conflict | 2, 7, 9 |
| 7: validate original/fallback point | 1, 3, 8, 9 |
| 8: verified missing progression items, no duplication | 1, 2, 3, 7, 9 |
| 9: first readiness/death/loading/departure boundaries | 5, 6, 7, 8, 9 |
| 10: unsupported destination ends co-op safely | 2, 8, 9 |
| 11: existing server first, private if required | 1, 5, 6, 9 |

Self-review: all eleven decisions and five Review Focus cases have owning tests;
new interfaces are defined at their producing tasks; unknown native/broker/data
facts have explicit verification gates rather than invented addresses or reward
tables. Offline module startup and durable runtime save receipts are necessary
callers discovered during planning, not optional features.

The user selected **Native** execution. Keep the shared join/save/recovery lifecycle
in this session, test each actual change, and preserve the ledger across checkpoints.
The first independent review covers only the inactive foundation, not completion
of the feature. Native integration and the complete branch review still require
the outstanding evidence and acceptance tests. The user asked to supply manual
gameplay results at checkpoints; do not request execution-method approval again.

Identity checkpoint update: Kaori → EiEi → Kaori file reads/commits are confirmed.
Player-slot root+68 is disproved; SaveLoadMan+3C is a last-read container entry,
including metadata/title scans, and cannot authorize durable recovery. Native
load/save code now identifies global+1490 as the selected-save candidate. The
opt-in ready-solo probe adds one four-byte observation for live correlation;
next manual check is EiEi → title → Kaori, about10 seconds ready each, then quit.
Step1.2 remains open for live selection, stable character/reused-slot/cycle and
mounted-path proof; no durable journal/recovery or reward receipt is enabled.

Second identity checkpoint passed: native selected slot1 →0 matches EiEi/Kaori
reads and commits. Legacy storage lookup now uses this native slot; timestamps
only preserve the existing current-run write gate. Explicit profile lookup never
falls through; ambiguous fallback owners reject. Actual-code regression observed
wrong-character selection RED and all seven join-test cases GREEN in bbport-dev;
module rebuild and native online imports pass. Scoped independent review found no
actionable issue in this change or its six callers (no Windows runtime test).
Next gameplay gate: existing two-window copied profiles, guest EiEi, different
starting maps, one join and voluntary Blank return. Verify usual arrival/return,
single load/animation/menu, and guest own-world restoration. Stable discriminator,
recycled-slot/NG-cycle, mounted-path and save-boundary gates remain open.

Co-op slot checkpoint passed in gameplay and both captures: guest stays on slot1,
guest-items storage selects slot1, and two joins restore separate origin tuples.
No repeat of the existing join/Blank matrix is needed for unchanged code. Further
read-only constructor/vtable/serializer tracing rejects header/time/appearance
fields as proven identity; stable incarnation remains unestablished. Next manual
evidence checkpoint uses two isolated empty profiles: create BBSCProbe A with
defaults, reload/move/save A, then create BBSCProbe B with identical defaults.
The ignored run-character-identity.sh all orchestrates three single-window runs,
validates/save-snapshots each and never deletes existing characters. Helper
preflight/self-tests and stage-order rejection pass in bbport-dev. This adds no
production mutation; durable recovery and reward receipts remain gated.

Checkpoint-3 setup correction: user could not create save data. Precreating empty
SPRJ0005 made native CREATE reject the existing container; userdata0010/backup0010
reads fail ENOENT. Actual runtime mount reproduction confirms ERR_EXISTS for the
empty-container case and success for an absent container. Fresh stages now retain
only the parent; an rmdir-only preparer rejects nonempty/symlink containers, and
checkpoint-3 launchers allow absent container. RED→GREEN preparer checks plus
actual mount regression and preflight pass in bbport-dev. Existing co-op/original
saves and production module are unchanged. Await retry of the same identity
checkpoint command; no native creation success claimed from preflight alone.

The user completed all three fresh-character stages; each snapshot's 13-file
manifest verifies. Creation/reload now have gameplay evidence. Comparisons of
current/backup A and B reject the persisted local block as a proven stable ID;
310 stable-different prefix words belong to a SprjGaitem reader and are not
promoted to identity. Task 1.2's incarnation/recycled-slot/cycle gates stay open.

Task 4 partial foundation: actual runtime filesystem tests now exercise canonical
per-file save-close observations, success generations and positive errors,
including first-save failure, independent users/slots and metadata/read/zero-write
exclusions. Atomic files abort replacement after fatal write/truncate/fsync
errors; transient EINTR retries. Focused RED-to-GREEN tests and scoped review
corrections pass in bbport-dev. This uses tests/test_file_mods.c, the existing
actual file implementation harness, instead of a new runtime-test mode. No host
callback, ABI change, native snapshot correlation, offline startup or recovery
consumer is enabled. Task 4 remains unchecked until those connected requirements
are implemented and verified; raw close generations cannot acknowledge rewards.

2026-10-19: Task 4's versioned save-fact boundary is implemented. BbNetHost ABI2
appends the callback, validates it before copying the host, and rejects ABI1.
Ruling: allow the existing receipt query to accept mounted /savedata paths in
addition to host paths — reuse the actual runtime mount table rather than infer
the save directory from environment/layout — this still does not prove character
incarnation or snapshot ownership. The game adapter uses selected slot0..9 to
query only for opt-in diagnostics; no durable consumer is enabled. Ruling: move
runtime module_handle publication after successful init — rejected modules must
not install image hooks — actual-loader RED-to-GREEN regression covers this.
build.sh --test includes --bbsc-save-host and --bbsc-reject-host with a real dlopen
fixture; all tests and real-module online imports pass in bbport-dev. Native
join/menu/return regressions and guest private-server/relay preflight also pass.

Native GetClearCount/AddClearCount evidence identifies player-slot-root+0x68 and
proves saturation at7; it cannot distinguish later cycles. The existing read-only
identity probe logs this limitation plus changing mounted receipts at most once
per second/64 lines. Next human gate is one copied guest window: EiEi load, move,
wait15s, quit to title, reload EiEi, wait15s, quit. Verify save/reload and native
slot1→mounted userdata0001/error0 generations. Task 4 stays partial: save-snapshot
correlation and recovery-only/offline initialization are still unimplemented.

The user completed that checkpoint: EiEi/slot1 has two native read loads, seven
successful character commits, two ready observations and normal close. Ordinary
save/reload passes. Receipt observation failed ENOENT/generation0 because native
save jobs unmount between operations (83 missing-mount messages). Ruling:
receipt guest aliases describe the last runtime mount target even after unmount,
not an active mount — retaining process-local canonical history fixes the actual
lifecycle without reopening save access — this remains raw fact, not identity.
Normal file operations still use only active mounts; remount/invalid target
replaces or invalidates history so former-root receipts cannot leak through.
Actual file RED-to-GREEN reproduces the gameplay sequence; the dlopen fixture
also queries after unmount. Full bbport-dev build/tests and diff check pass.
Short follow-up: guest EiEi load/move/wait15s/normal close only; no reload or co-op.
Verify query0, positive generation, close error0 and the correct canonical path.

Short follow-up passed in gameplay: native slot1, query0/generation3/error0 and
the exact guest userdata0001 canonical path. Four character commits return0,
normal close, no repeated missing-mount warning. Evidence archived as
coop-trace.save-receipt-confirmed.log. The earlier normal save/reload plus this
sample close the changed runtime/ABI2 save-fact checkpoint; do not repeat it for
unchanged code. Task 4 remains partial pending native snapshot correlation and
offline startup; Task 1 still needs stable incarnation/uncapped-cycle proof and
the reject-capable admission point before durable gameplay integration.

Native research now separates save request/clear (0156D570), serialization
(0156AA50→0156B8F3), deserialize (0156B970), individual-file copy input (01831977)
and new-slot reservation (01B3E633). Ruling: observe these boundaries read-only
under the existing opt-in identity trace — direct slot setter calls miss direct
creation/load writes, and world sections can come from older cached own-world
buffers while visiting — none of these observations alone can confirm a reward.
The serializer's fourth argument is an optional size-output pointer, not a mode;
regression caught and corrected that interpretation before gameplay use.

Six signatures match the supplied image; helper decoding tests went RED→GREEN.
Default module settings skip these hooks. Eight native cases and rebuilt-module
online imports pass in bbport-dev. Existing identity validator is reused rather
than a new save parser. New ignored run-native-save-lifecycle.sh all uses copied
profile A for create BBSCTrace→restart/reload→delete only BBSCTrace/recreate with
same choices,15s ready/normal closure each. Slot0 BBSCProbe hash must stay equal
to its earlier snapshot. Slot1 is a preallocated zero file, not an absent file.
Every stage is snapshotted; prior snapshot hashes are verified before the next,
especially before deletion. Reordered stages and overwrites refuse before launch.
Self-checks/private-server preflight pass; pending human captures must verify
actual control flow before any stable-identity/recovery consumer is enabled.

2026-10-19 follow-up: managed own-world payload loading is connected at the verified
native respawn-point resolver `0x01938F65`. Only a matching Visiting/Returning
record can apply, and only at that home-load boundary after native identity, own-
world state, snapshot integrity, ready flag-manager metadata and every target group
storage validate. `Prepared`, malformed/unverifiable, foreign, truncated, host-world
and missing-target cases preserve live/in-memory state. Legacy slot recovery does
not fall through while a current-key managed record is prepared, pending or
unverifiable. Journal and payload remain unchanged after restoration; no native
save-confirmation event is wired, so repeated verified home loads may reapply.
Focused `bbport-dev` build of `bbnet bbsc-join-test` and `bbsc-join-test` pass;
logs are `managed-home-recovery-{build,checks}.log` in the ignored SDD directory.
This is restart restore only: normal-join admission, offline initialization/warp,
completion/save confirmation, backup/capability checks, managed cycle handling and
Windows persistence remain release blockers.

2026-10-19 checkpoint: return save provenance is now connected to the managed native
return selector. Journal v4 stores monotone `native_return_selected`; the selected
native point ID drives the game's last-lamp branch, and its map is resolved through
the verified native point-to-map routine. Unresolved off-map XYZ values are metadata
only. Do not use the older raw `native-selector-1.09.txt` artifact; corrected PT_LOAD
disassembly is `root-native-selector.{bin,txt}`. A managed return completes only after
the selected native destination is observed after load, then a later SerializeBegin/
successful SerializeEnd hash matches a newer ABI3 receipt for the exact selected save.
The first destination confirmation invalidates older save evidence. Pending managed
records block legacy raw-coordinate fallback.

Focused `bbnet`, `bbsc-party-test`, `bbsc-join-test` and
`bbsc-party-transport-test` build/checks pass in `bbport-dev`; see
`checkpoint-v4-final-{build,party,join,transport}.log`. Join fixture intentionally
proves mismatch refusal/pending snapshot preservation; positive exact-point proof is
limited to the pure tracker fixture, not a live/native-return integration test. This
supersedes the earlier status that no native save-confirmation consumer was wired,
but does not close restart/offline loading/warp/retry. Normal admission, capability,
backup, catalog/event application, NG7, Windows runtime and broad gameplay remain open.
