# BBSC integration evidence and checkpoint 1

Date: 2026-10-18. Execution method: Native.

This is a partial implementation checkpoint, not a working release of the proposed
co-op behavior. New party policy/journal helpers are built into the online module's
source list but have no gameplay callers. Existing BBSC behavior is unchanged.
One existing runtime fix now reports directory-flush errors instead of claiming a
save is durable. Do not test new joins, progression rewards, or recovery on real
saves on the strength of standalone tests.

## Verified source and game facts

- The supplied game is at `/run/media/system/storage/Bloodborne/CUSA03173`.
  Reading its `sce_sys/param.sfo` through `scripts/prepare.py:sfo` reports
  `CUSA03173`, `01.09`.
- Executable SHA-256:
  `d65f0b4f01d59166aed16f8604196d8b7dd805abbf0758b356e8f1354c9429f9`.
  `parse_self` decodes ten ELF program headers; the reported missing segment is
  index 9. This inspection is not proof of successful runtime loading.
- These existing native entry points have bytes `55 48 89 e5 41 57 41 56` in the
  decoded executable: `0x01332bc0`, `0x013cde30`, `0x017ddc50`, `0x014da0a0`.
  Prologue matches confirm addresses for this dump; they do not verify return
  values, safe-point checks, active character identity, or a reject-capable join.
- Actual story-map directories are `m21_00`, `m21_01`, `m22_00`, `m23_00`,
  `m24_00`, `m24_01`, `m24_02`, `m25_00`, `m26_00`, `m27_00`, `m28_00`,
  `m32_00`, `m33_00`, `m34_00`, `m35_00`, `m36_00`, each with `_00_00` suffixes.
  Map area/block packing agrees with `BbscAreaName`. The policy accepts only
  these exact maps, never a numeric range or an unknown m29 instance. This list
  is not yet an event/reward coverage catalog.
- The existing `BbscBossRewards` (17 entries) and `BbscBossGoodsLots` (18 entries)
  are reusable mapping evidence. They include ordinary repeat consumables and
  therefore must not simply become the new progression catalog. The game event
  files exist, but a full verified event/prerequisite/item catalog is still pending.

## Additional native/save research

The native snapshot entry cited by existing `ArmBbscRejoinV7` is `0x0131e5c0`.
For this dump it maps to ELF file offset `0x013225c0`, with first bytes
`55 48 89 e5 41 57 41 56`. Disassembly reads the pointer at `0x0553e878` and
allocates/copies state. This agrees with a snapshot operation but does not prove
which native completion acknowledges an earned reward. No new hook was installed.
Container `BBPROBE5` offsets must not be mistaken for native virtual addresses;
this inspection uses the decoded ELF program-header mapping.

Native filename formatting research found UTF-16 `%hS/userdata%04d` at
`0x0488bf2e` and `0x0488c1d0`. Calls to formatter `0x02762d00` at
`0x0283ff5b` and `0x02842bb3` pass the file index in `ecx` (copied from `r13d`
and `r12d`, respectively). The containing functions begin at `0x0283f690` and
`0x02841ca0` according to `.eh_frame_hdr`. These functions operate on save jobs;
no proof yet ties one field to the currently playable character. In particular,
metadata index 10 must never be promoted into a character identity. Research
artifacts are retained in the ignored execution workspace; no hook was enabled.

Read-only save-format reference:
[Noxde/Bloodborne-save-editor](https://github.com/Noxde/Bloodborne-save-editor/tree/b8eecb5d2a73066954cdb54b65349ea65b1ded62),
commit `b8eecb5d2a73066954cdb54b65349ea65b1ded62`. Its `offsets.rs` locates mutable
character fields relative to an appearance/inventory anchor; `resources/offsets.json`
labels `Ng` at username-relative offset 68831. These are research leads, not
verified native identity/cycle contracts. No editor code was run, copied into the
product, or used to modify saves. No reliable stable character discriminator was
established from this reference; character names or appearance hashes cannot
safely substitute for it.

## Broker evidence

No broker source is tracked in this worktree. An existing source checkout was
found outside it at:

`/home/kao/Projects/bloodborne_pc/out/bbsc-server-src/src/bloodborne_summon_broker.cpp`

Its SHA-256 is `340b337125fc07d4ecc3a3b954ba406e7d0dc9e29846f754e24545dc3fd56cfd`.
The adjacent header hash is
`694c3adb2bc508979f1ea9f26377fd18ea7c7b2c0be5b07e5b0c2618bd0ff4de`.
Neither file was modified or copied into the product.

`MatchesSearch` compares nonempty `SummonWord` strings exactly; an empty request
does not require equality. `anywhereSummons` also bypasses area/region/channel
checks and some normal level/distance checks. Its options and prepare/claim
placement flow must be inspected against actual exchanges before using it for
the agreed policy. This source does not prove what the configured public server
runs or that peer capability can be negotiated before a native world mutation.

## Automated work completed at this checkpoint

- Password/map eligibility, including rejecting empty or mismatched passwords,
  invasion roles, unsupported source/target maps, and same-map use of this
  cross-map-only policy. Native same-map matching is not changed.
- Deterministic choice of original point, recorded lamp, or native initial point
  *after* an adapter supplies their validation results. No collision-safety query
  or native return has been implemented.
- Versioned, bounded, checksummed character/party journals; required character
  leases; identity mismatch/corrupt/legacy record rejection; immutable original
  destination during a party; preserving earned/pending IDs on updates; checked
  file/directory flushing; and process-interruption tests. These IDs are durable
  intents, not authorization to apply arbitrary game flags.
- `StoreJournal` requires its live `CharacterLease`, rather than the plan's initial
  raw-path signature, to make exclusivity mandatory. One game thread must own
  writes. Character derivation, journal path selection, own-world snapshot links,
  and native save confirmation remain gates before any gameplay caller is added.
- Actual `runtime_file.c` regression: file flush failure preserves the old file;
  directory flush failure after replacement returns an error and accurately
  reports uncertain durability. No save-receipt ABI or offline recovery entry is
  claimed implemented.

The initial agent container had no C/C++ compiler or initialized build submodules in this
checkout. Fedora compiler/header packages were downloaded and extracted only
under `/tmp/bbsc-toolchain`; nothing was installed into the operating system.
Standalone tests compile the same sources with C++23, assertions, and `-Werror`.
Subsequently, the user-designated `bbport-dev` environment was used for full
configuration/build. The system-package adapters and newer header copies from
the primary checkout are documented in the plan; no OS installation was needed.
`Online module: on` is confirmed, `libbbnet.so` links, and the actual CMake
`bbsc-party-test` and `bbsc-memory-test` binaries both pass. `bash build.sh --test`
also completes successfully, including runtime, file/mod, semaphore, online import,
pad and content tests. A Python `ctypes` late-load probe is unsuitable because
`libbbgpu` intentionally uses initial-exec TLS and must be loaded at process
startup; the freshly linked native `online-test` successfully loads the module
through the supported runtime boundary instead.

Observed results:

- `out/gpu/bbsc-party-test`: `PASS: policy`, `PASS: return`, `PASS: journal`.
- `out/file-mods-test`: atomic-save error regressions and existing guest mod/file
  behavior pass. Injected I/O-error messages are expected test evidence.
- `out/gpu/bbsc-memory-test`: existing mapping/permission and legacy-kernel
  fallback regression passes using freshly compiled current sources.
- Python baseline: 123 tests, OK, 28 skipped for unavailable native artifacts.
  Final verification results are recorded in the execution ledger.

An independent reviewer inspected all current production/test changes and ran
fresh party and runtime-file builds. It found no Critical/Important issue in the
inactive foundation. Its test-output-path finding was corrected so CMake emits
`out/gpu/bbsc-party-test`, matching documented commands. The full native build now passes; broker compatibility, save identity, rewards,
and live recovery remain unproven.

## Checkpoint 1: single-player save trace

The user authorized using copied saves and requested a pause when gameplay is
needed. A copy, with no links back to original save files, is prepared at:

`.superpowers/sdd/2026-10-18-bbsc-anywhere-coop/checkpoint-1/user/savedata`

The entire scratch directory is Git-ignored. Its `out/bb-probe` and GPU-capability
helper point to the existing prebuilt executable in the primary checkout. This
is **baseline evidence collection using that existing build**, not a test of the
new helpers or runtime fix. Original saves have not been changed and no game has
been launched by the agent.

The first user run failed before opening the game: `BB_PREBUILT=1` selected
`bin/bb-probe`, but the existing executable is `out/bb-probe`. The script did not
set `BB_PROBE` and redirected the error to a log. This was a launcher error, not
save/character evidence.

The user requires distrobox **`bbport-dev`** for development dependencies. The
checkpoint script now enters that container, explicitly sets `BB_PROBE` to the
baseline executable, uses its installed libraries, and displays output with
`tee` while retaining the log and failing exit status. The earlier extracted
runtime libraries are no longer used. `--check` verifies paths and linked
libraries without starting the game. Verified in `bbport-dev`: checkpoint `--check` passes; freshly compiled
policy/return/journal and runtime-file/mod tests pass (`out/bbport-dev-check`).
The user subsequently completed the baseline load/quit run below; rendering of
the newly compiled worktree runtime has not yet been exercised.

Run the prepared script from a terminal:

```bash
bash /home/kao/.codex/worktrees/c51a/bloodborne_pc/.superpowers/sdd/2026-10-18-bbsc-anywhere-coop/checkpoint-1/run-save-trace.sh
```

It selects only the copied user/data/config paths, disables online/BBSC for this
baseline, enables `BB_SAVE_TRACE`, and writes `checkpoint-1/save-trace.log`.

1. Select a known character from the copied saves and load into its own world.
2. Wait until loading finishes and stand at a known lamp. Note the character and
   lamp; there is no need to ring a bell or have another player for this checkpoint.
3. Use the normal menu to quit to the title screen, then close the game normally.
4. Report whether loading/quitting succeeded and which character/lamp was used.
   The agent can read the local `save-trace.log`; do not paste account credentials.
   If startup fails, retain that same log instead of changing the original profile.

Expected evidence: guest file reads/writes for the selected character's
`userdata0000` through `userdata0009`, their ordering around character selection,
and a normal save-close result. `userdata0010` is not a character slot. The trace
must refer only to this copied profile. This is needed to determine whether the
runtime's observed selected-file operations can identify the active character
without using file modification times, and where native save confirmation can
be correlated. A trace alone does not yet prove a stable recycled-slot identity.

## Checkpoint 1 observed result

The user completed loading **EiEi** at **Cathedral Ward**, waiting in-world,
quitting to title and closing the window. The retained 655-line trace has SHA-256
`9c48a293b5b20afd5c6b00efe28571ccff6718e5197ddfe1348490570b8f398d`.

- The first two metadata read opens (`userdata0010`) precede the only character
  read open, `userdata0001` (lines 300–304).
- `userdata0001` is 1,310,720 bytes. Nine writable opens each finish with
  `commit -> 0`, including the final character commit at line 625.
- `userdata0010` is 262,144 bytes. Eight writable opens each finish with
  `commit -> 0`; the final metadata commit is line 637. Metadata is read again
  at lines 649–654 before the window closes normally at line 655.
- All observed writable save copies are under the isolated checkpoint profile,
  user `1`, `CUSA00207/SPRJ0005`, even though the input game title is CUSA03173.
  Runtime mount evidence takes precedence over assuming the input title's path.

This establishes the loaded file for this run and observed successful baseline
close results. It does **not** establish stable character/cycle identity, behavior
when character menus preview other slots, or a native snapshot boundary. The
baseline predates the directory-flush fix; its `commit -> 0` cannot prove that
fix was active. No co-op join/warp was exercised. The character/lamp label is user-reported,
not inferred from map asset names.

## Gates still open after checkpoint 1

| Gate | Evidence still required |
| --- | --- |
| Selected identity | Actual selected-slot I/O ordering plus a stable character discriminator and cycle identity; never merely the newest file. |
| Join rejection | A byte-checked native hook that demonstrably prevents host-world mutation on failed preparation, not just a trace callback. |
| Save receipt | Actual native snapshot/commit correlation, including a save started before a reward and completed afterward. |
| Event catalog | Every supported boss/story completion, prerequisites/conflicts, required item and claim evidence checked against game data. |
| Broker/capability | Same/different/empty-password exchanges, host/claim/placement identity binding, and capability timing before native mutation. |
| Return | Native original-point validation, pre-party fallback, restoration order and offline recovery entry. |

The later join/broker/return gates need a host and guest. Two isolated local
instances with separate profiles and distinct server accounts may be sufficient,
but their availability and compatibility are not assumed. Do not replace those
checks with a mock and report real co-op as passing. Checkpoint 1 trace is now available. Continue the remaining gates from these
observations; keep the execution ledger and copied profile.

## Next gameplay checkpoint: native co-op ordering (pending)

This needs two game instances with separate copied profiles and distinct existing
shadNet accounts. A second human/machine is not intrinsically required, but
successful simultaneous instances, account access and local P2P must first be
confirmed; none is assumed from the single-player result. The user has chosen to try two windows on this machine. Do not launch either
instance automatically.

Use the existing BBSC flow, not the inactive new party helpers. Both sessions
must set their own `BB_DATA_DIR`, `BB_USER_DIR`, `BB_GPU_USER_DIR`, `BB_CONFIG`
and explicit `BBSC_SAVE_DIR` to isolated copies. Use `BB_ONLINE=1`, `BB_BBSC=1`,
`BB_SAVE_TRACE=1`, `SHADPS4_BLOODBORNE_RE_TRACE=1` and
`SHADPS4_CAPTURE_BLOODBORNE_SUMMON=1`. Keep account secrets local in the normal
launcher/environment; do not include them in commands or messages. Run with the
chosen baseline executable set explicitly through `BB_PROBE`. The original save
profile must never be selected. Before providing launch scripts, verify the
executable/module pair and distinct account/profile configuration.

Manual sequence once those prerequisites are ready:

1. On disposable characters, join normally in the same supported map with matching
   test passwords. Do not kill bosses or complete quests. Record whether each bell
   activates, whether the guest becomes controllable, and both players' maps.
2. Leave the session using the normal menu. This measures existing teardown,
   without asserting that the new durable return behavior exists.
3. Try different supported maps with matching test passwords. Record whether the
   candidate is found and native placement/reload occurs. Failure is useful
   evidence; do not enable broader/public matching to force success.
4. Repeat the candidate search with different passwords, then with empty passwords;
   stop/cancel if a cross-map candidate can be claimed. These measure current
   broker semantics, not acceptance tests for the unfinished new feature.
5. Quit normally and retain local logs/captures. The agent will inspect only the
   needed fields and record redacted fixtures; do not paste raw request bodies.

Required evidence is candidate→claim→selected-host placement binding, source peer
resolution/capability timing, and native hook ordering before the first world
mutation and first guest readiness. Save-close order must be correlated separately
with a proven snapshot boundary. Existing `JoinSession_White` logging alone is
not a reject-capable admission gate. This checkpoint does not waive the stable
character discriminator, complete event catalog or safe-return gates.

### Prepared two-window launchers

`checkpoint-2/host/user/savedata` and `checkpoint-2/guest/user/savedata` are
independent, byte-matched copies of the completed first-checkpoint profile (no
symlinks or shared save inodes). Both therefore start with EiEi at Cathedral Ward.
Each has separate data/config/log/capture paths. The helper enters `bbport-dev`,
uses the freshly compiled worktree executable and module, sets windowed 720p/30fps,
and asks for an existing shadNet ID plus a hidden password locally. It stores no
account password. An account-ID hash prevents accidental reuse across roles;
profile locks prevent concurrent use of one profile.

From two host terminals, run:

```bash
bash /home/kao/.codex/worktrees/c51a/bloodborne_pc/.superpowers/sdd/2026-10-18-bbsc-anywhere-coop/checkpoint-2/run-coop-trace.sh host
```

```bash
bash /home/kao/.codex/worktrees/c51a/bloodborne_pc/.superpowers/sdd/2026-10-18-bbsc-anywhere-coop/checkpoint-2/run-coop-trace.sh guest
```

Both `host --check` and `guest --check` pass in `bbport-dev`. A dummy duplicate
account test confirms refusal before any game launch. Shell syntax and save-copy
independence checks pass. No real login or game was started by the agent.

**First run only the same-map step above:** load EiEi in each window at Cathedral
Ward, use the same disposable in-game matching password (for example `bbsctest`),
use the BBSC Kit: ring **Hunt Bell** on host and **Kin Bell** on guest, and report whether
they connect and the guest can move. Do not kill bosses or progress quests. Leave
using **Parting Blank** and close both windows. Keep `host/coop-trace.log`, `guest/coop-trace.log`
and each profile's captures for local inspection. This is an observation of the
existing BBSC path, not proof of the new join/warp/progress behavior. Analyze this
capture before requesting the later cross-map/password matrix.

### User-selected private servers for checkpoint 2

The user now directs testing to their servers, superseding the public-first
capture target. Both launch roles explicitly set:

- `BB_SHADNET_SERVER=100.119.93.51:31313`
- `BB_SHADNET_WEBAPI=http://100.119.93.51:31315`
- `BB_COMMUNITY_SERVER=http://100.86.192.108` (ss.info uses port 20443)
- `BB_BBSC_LOCAL_SERVER=off`, so a failure does not silently use a local game server.

Initial `scripts/online.py check` from `bbport-dev` reports all three reachable.
The helper's `--check` now verifies these targets in addition to the executable,
module and profile paths. Successful connectivity is not proof of login, broker
password semantics or successful co-op. Use accounts registered on this shadNet
server. No production launcher defaults or remote server configuration was edited.

Verified after the change: both role `--check` invocations pass. Decoded remote
ss.info advertises game API `http://100.86.192.108:20443` and summon WebAPI
`http://100.119.93.51:31315`; it also retains a PlayStation website link. No
login, summon advertisement or game launch was performed.

### Checkpoint 2 blocked: Kit not delivered

The user reports missing BBSC Kit in both windows. The prior native-memory fix
(commit 31a2e53) is present in this worktree and matches the earlier combined fix
6adfb0f for memory.h/bloodborne_re.cpp. This is not evidence of a stale build.
Both captures show the native `IsValidSos.Update` tick firing thousands of times;
the Kit tick is called from it. The captured headers confirm seamless patches
were installed. Some optional RE hooks report overlap/byte/relative-instruction
failures; those messages alone do not explain Kit delivery.

The default GPU log threshold is warning, which hid existing table-preparation
and Kit inventory messages. The checkpoint helper now sets `BB_GPU_LOG=info`;
Kit's existing five-second guard also reports player/loading or missing-inventory
waits. These changes add diagnostic output only; no grant/eligibility condition
was relaxed. Original failed logs are preserved as `coop-trace.kit-missing.log`.

A standalone self-memory scanner probe passes in bbport-dev, including a protected
mapping boundary; the native host-mapping regression also passes. Actual table
readiness and character/inventory gates still need the new in-game info log.
Rerun **host only**, load EiEi, wait 30 seconds in-world and quit normally. Do not
repeat two-window matching until Kit delivery is understood.

### Kit root cause and scanner fix

The new host trace repeatedly reports `goods 4313 in 0 of 0 table(s), SpEffect
7511 in 0 table(s) - NOT READY`. It is preserved as
`checkpoint-2/host/coop-trace.kit-param-not-ready.log`. Reading the actual game's
DFLT-compressed gameparam binder confirms the expected headers: goods has 295
rows/first ID 1, SpEffect has 7855 rows/first ID 0. The existing predicates match.

A headless Vulkan reproduction using cached system-memory dma-buf export on the
RX 9060 XT shows `process_vm_readv = -1, errno = EFAULT`, whereas `pread` through
`/proc/self/mem` reads the same bytes correctly. The old scanner finds zero hits.
This reproduces a missing device-mapping read path, separate from the prior VMA
query fix. No game or original save was launched/modified for this reproduction.

The Linux scanner now lazily opens `/proc/self/mem` when its ordinary read fails
or is short, fills only the remaining bytes in the selected readable VMA, retries
EINTR, and closes the descriptor. It never directly dereferences potentially
unmapped memory. Grant/eligibility rules are unchanged.

Regression evidence: the new portable test failed before the fix at the simulated
EFAULT case, then passes with short/interrupted reads, protected pages, range and
hit bounds, chunk overlap, failed fallback and disappearing VMA cases. The explicit
real-GPU test now finds both needles in a 2 MiB exported dma-buf, including one
crossing the 1 MiB piece boundary. Both actual CMake targets pass. `bbnet` rebuild,
native `online-test` and `bbsc-memory-test` pass. In-game Kit delivery still needs
one host-only load/30-second wait using the existing checkpoint command; do not
claim successful delivery from the scanner tests alone.

### Kit delivery confirmed in-game

The user confirms receiving the Kit after the dma-buf scanner fix. The retained
`checkpoint-2/host/coop-trace.kit-confirmed.log` shows one goods table and one
SpEffect table found, successful pre-grant backup, and both missing goods 11201
and 11251 added with `there now: true`. The other Kit items were already present
in carried inventory or storage and were not granted again. Normal window close
is recorded. The trace also confirms sign-in to the selected private shadNet
server and `matching2_enabled=true`; a client-version warning is present but did
not prevent this observed sign-in. No co-op join has been established yet.

Kit delivery is no longer the checkpoint blocker. Resume the prepared two-window
same-map test using Hunt Bell (host) and Kin Bell (guest), with distinct shadNet
accounts and one shared disposable matching password; do not kill bosses/advance
quests during this capture. New party progress/return features remain inactive.

### Same-map join failure: split-host relay destination corrected

Both users sign in, the guest advertises and the host accepts candidates; moving
from Cathedral Ward to Central Yharnam did not resolve signaling. Host physical
UDP port 47986 and guest port 59666 are distinct, excluding a shared-port collision
for this run. Both connections time out with `0x80552711` (no answer). Host relay
counters reach 123 frames sent/0 received, and neither log records relay registration.
The previous logs are retained as `coop-trace.join-relay-wrong.log` per role.

The checkpoint helper pinned TCP/WebAPI/game endpoints but omitted the existing
relay override. The client consequently selected the shadNet host
`100.119.93.51:3478`; the deployed relay is on game host `100.86.192.108:3478`.
A bounded UDP probe with disposable random identities receives no registration
ACK from the former, and a valid BBR1 ACK from the latter. The earlier combined
server packaging also places this service in the game-server process.

Both roles now explicitly set `SHADPS4_P2P_RELAY=100.86.192.108:3478`.
Their `--check` additionally registers two disposable identities and validates
frame bytes plus source-endpoint preservation in both directions. Both roles
pass this real relay round-trip test. It uses no player accounts or gameplay.
No server configuration, client protocol or password filtering was changed.
Restart both game windows for the environment change, then repeat the same-map
Hunt Bell/Kin Bell checkpoint. Actual native join remains to be confirmed.


## Same-map join and voluntary return confirmed

The user confirms join and return work after the relay override. Both latest logs
are preserved as `checkpoint-2/{host,guest}/coop-trace.same-map-confirmed.log`.
The captures are host `cusa03173-109-p49049-1792313688289.jsonl` and guest
`cusa03173-109-p48392-1792313634589.jsonl`. Guest handoff records `same_map`,
current/received map `0x18010000` (Central Yharnam), and `transported_host=true`.
Host log line 3116 and guest line 3489 show two hunters / one cooperator, with
session states 3 and 6 respectively. This establishes native same-map joining,
not cross-map placement or the new admission protocol. Earlier attempts in the
same run timed out; the later join completed successfully.

Guest line 4019 records voluntary OnLeaveMagic; line 4189 restores 960 own-world
flag groups, and line 4369 has state 0, character type 0, own_world=1. This and
the user's observation establish ordinary leave/return. They do not establish
exact original XYZ/heading, disconnect recovery, restart recovery or lamp fallback.

Guest line 4191 also shows the *existing* shared-progress implementation OR-ing
103 world flags in 30 groups into the guest's world (zero new boss flags). Its
whole-group policy is incompatible with the approved event-only specification;
no inference is made that those flags were earned together. Checkpoint launcher
now sets `SHADPS4_BBSC_SHARED_PROGRESS=0` for both roles using the existing
opt-out. This affects future launches, does not undo already saved test progress,
and leaves temporary host world presentation enabled. Original saves remain
untouched; no rollback of the copied guest save is performed.

Next manual checkpoint: start fresh host/guest processes with the same helper,
place host at Cathedral Ward and guest at Central Yharnam, use the same nonempty
password and Hunt Bell/Kin Bell. Observe arrival beside host with correct map
rendering, then Parting Blank and observe the guest's return map/location. Do
not defeat bosses or advance quests in this transport checkpoint. If successful,
repeat with host in Hunter's Dream and guest at Central Yharnam. Stop on failure
and retain both windows' logs; do not infer durable return from ordinary leave.


## Cross-map transport confirmed; retries and incorrect return reproduced

User confirms cross-map join eventually works after two or three re-rings/Parting
Blank resets, but voluntary leave retains the host map/location. This supersedes
any interpretation of the previous same-map result as correct spatial return.
Both logs are archived as `coop-trace.cross-map-return-failed.log` (host SHA256
`c2b20348bc82a1e4566cad803f46b30757cd52df5aac05dc492093d4ea288b1d`,
guest `b963df2ea05793c33a719a9e06bb8f4ceab28a244184d08c936912c3b2d293ed`).
Captures: host p54494 / guest p54680, start timestamps 1792314272480 /
1792314277889. Guest starts in 0x18010000, joins 0x17000000, later follows
0x18000000. Return map selectors choose 0x17000000 then 0x18000000, respectively,
while own-world flag restoration succeeds. Legacy shared-progress merge is absent
in this run, as intended by the checkpoint override.

Match diagnosis: guest repeatedly receives host placement, rewrites AreaId, then
immediately clears the target on an advertisement ACK without a placement header.
Compatible broker source explains the ACK: Preparing -> Advertised returns no
header after accepting the host AreaId, before the host's next search/claim.
This does not mean the host disappeared. The actual cache regression reproduces
the failure deterministically (bbsc-join-test, assertion that target survives ACK).
The existing timeout remains bounded and missing headers must not refresh it.

Return diagnosis: native map selector 0x01332BC0 reads forced map +0x14F0 when
+0x1520 is set. Send-home 0x0138A4A0 unconditionally raises that byte and calls
the selector at return address 0x0138A51C. BBSC's join/follow reload writes the
host into this forced block. Separately, native own-world restoration at
0x01941FFC..0x01942028 copies the game's home/sign placement +0x14A0/+0x14B0/
+0x14C0 into forced +0x1500/+0x1510/+0x14F0. Existing connect-first code also
overwrites those home/sign fields with the host, based on an older unconfirmed
lighting hypothesis. Both uses need separation; a test must exercise the actual
placement writer and map-choice guard, not only ReturnChoice policy.

Corrections implemented and automatically verified in bbport-dev:

- Keep a fresh connect-first target across headerless ACKs, without updating its
  original deadline. Same-map placement still clears the cross-map target; expiry
  clears target and cached placement. A departed host may therefore remain a
  target until the existing ten-minute deadline; no new admission guarantees.
- Leave the native home/sign placement untouched when writing received/summoned
  host placement. At only the co-op send-home map selector (caller 0x0138A51C,
  own_world=0, player type1), copy valid finite native home fields into forced
  placement. Join/follow calls and invaders retain their previous behavior.
- Log live position at JoinSession_White alongside native home and last lamp,
  allowing the next manual run to distinguish bell-time from join-time origin.

The actual-code test includes bloodborne_re.cpp directly with dead-code stripping
and only logging/runtime-VMA test stubs; it drives the real cache, placement writer
and map-choice guard against mapped synthetic native state. Observed RED at
empty-ACK target loss, RED at overwritten native home map, RED at uncorrected
forced return map, then GREEN for both groups. Full bbnet build succeeds and
party/journal, host-memory, memscan and native module/import checks pass. The
online-import test's default localhost server is unreachable, as expected; it
validates module loading/import routing, not private-service connectivity.
Independent scoped review reports no Important/Critical findings.

This repairs existing native transport/return code, not the planned persistent
return implementation. Native home is not yet proved to be latest join-time XYZ,
and no collision/lamp fallback, crash recovery or stable-character derivation is
newly enabled. Do not mark Task8 or the full feature complete. The next checkpoint
must also check loading/lighting because the removed sign-field writes originally
came from a rendering hypothesis. Original saves have not been modified.


## Native-home assumption disproved; independent live-origin capture

The next user run still joins slowly and returns at the host location. Both logs
are preserved as `coop-trace.native-return-fix-failed.log` (host p73126, guest
p73142). At JoinSession_White, the diagnostic reads the actual guest at map
0x18010000, XYZ (-193.40,-28.65,68.50), heading2.360, lamp2412951, while native
sign map is 0xffffffff with zero coordinates. The handoff later sees sign map
0x18010000; by leave it is host0x18000000 at (17.58,-9.51,102.65). The engine
itself rewrites the sign block, disproving the previous stable-home assumption.
The prior regression did not simulate that engine overwrite; it now does and
observably fails before the correction.

A separate process-local origin now captures the live native placement on
JoinSession_White, before trace sampling. Capture requires own world, local type0,
no load and supported story map. Guest rejoin does not replace it; a new own-world
join refreshes it; quit to title clears it. Exact co-op send-home restores both
the sign and forced blocks from this independent copy, checking native global
ownership, supported map, finite coordinates and writable fields. Tests drive
the real pointer chain, overwrite the sign block as observed, and check rejoin
preservation, fresh next-party coordinates and rejection of missing/invalid/
unsupported origins. This is a process-local correction: durable return journal,
character identity, geometry/lamp fallback and restart recovery remain open.

Host get-list captures are ~35 seconds apart. Guest fast-sign repeats5 times at
3-second intervals, then waits for its next native request; these two cadences
still delay preparation/claim after fixing target clearing. Host now uses the
same bounded5x3s polling worker for nonempty-password co-op searches. Initial
mixed query is unchanged; repeats contain only type0 co-op entries. Public and
invasion-only searches keep their cadence. Empty well-shaped results retry;
candidates or malformed/error responses stop. Abort/delete/shutdown wake the
wait through the existing request cv with state reads protected by its mutex.

The HTTP regression uses a real ephemeral loopback server and actual sceHttp
init/template/connection/request worker, with only native placement/log/path
stubs. It verifies the initial mixed query becomes co-op-only on retry, delivers
the candidate, respects fast-sign opt-out, leaves public/invasion-only searches
unchanged and aborts a waiting request without another network poll. No accounts
or private services are used by this test. Full bbnet build plus join/http/party/
journal/memory/memscan/native-import checks pass in bbport-dev. Scoped independent
review finds no Important/Critical issues; real hook timing, end-to-end latency,
original-point arrival and rendering still need another gameplay checkpoint.


## Live origin return confirmed; Parting Blank presentation differs

User confirms the latest correction joins and returns correctly, including a
second session in the same process. Archived logs `coop-trace.origin-return-confirmed.log`
show first guest origin/return map0x18000000, XYZ(17.58,-9.51,102.65), heading2.094;
second origin map0x18010000, XYZ(-193.40,-28.65,68.50), heading0.785, joins
0x18000000 and selects its recorded original map/point on leave. Both guest roles
are session6/player_type1/chr_type1. This establishes ordinary voluntary return
for these two trials, not disconnect/restart recovery or geometry/lamp fallback.

New user observation: second Parting Blank has different animation/text/menu,
including End Co-op and an unavailable-function help line, but still returns
correctly. The resolved inventory ID remains11251 in both trials. First trial
is same-map; second cross-map includes the extra reload. Existing trace does
not capture active Kit goods row parameters, so param reset vs native menu
classification cannot yet be distinguished.

Verified directly against supplied enggb/menu.msgbnd.dcx (which contains Thai
localized strings): FMG200/302123 is the Disrupt Resonance menu label; BBSC
intentionally replaces it with End co-op. FMG201/302123 is the unavailable
function help, which BBSC currently leaves unchanged. FMG204/302123 similarly
states no resonance/function unavailable. Native formatter0x01BE8930 selects
200+201/302123 on its fallback branch; supported selector values use other
entries. Why the second join reaches that branch remains unproved. Do not
silently replace the help text and claim the underlying state fixed.

Added diagnostic-only sampling from the existing native goods lookup after
repository/player readiness, logging changed row/context (ref/category/animation/
menu/limit/dialog/enable/icon and patched name/info/caption mask). Added11251
to the existing trace-only goods filter, and a byte-verified formatter trace
for its flag/selector and current guest roles. No inventory, params, item
behavior, menu choices or return decisions are changed. Next manual diagnostic
checkpoint: one same-map join/leave, then one cross-map join/leave in the same
process; observe Parting Blank menus/animation at each use.


## Parting Blank: stable params, missing native ID discriminator and unusable menu

User screenshots preserve inventory name Parting Blank, End co-op fallback menu,
unavailable help/confirmation, plus reported drinking instead of Blank animation.
Copies are in checkpoint-2/guest/blank-presentation-{1,2,3}.png. Diagnostic run
p100302 (guest), p100892 (host) logs are archived as
`coop-trace.blank-presentation-reproduced.log`. Active row11251 remains unchanged:
ref7/category2/animation15/menu4/dialog20000432/enable0x5f/text_mask0x7 throughout.
This excludes param reload/reset as the cause in this run. Menu formatter receives
flag0/selector0 for all four entries while guest state6/player_type1/chr_type1,
selecting302123 unavailable fallback. It opens before native item application.

Read-only original gameparam confirms111 has effect7/animation15/menu4/dialog
20000432. Native item-apply018F9720 additionally treats IDs specially: r15d is
goods ID, r13 is GameData after the successful executor; goods102 clears byte
+0x680 at018F987C, only111 sets it to1 at018F98AC. Cloned11251 bypasses this
discriminator and inherits its previous value. No assumption is made about the
name/purpose of unused goods102.

Correction: a verified hook at018F98B4 (C7 83 90 04 00 00 00 00 00 00, one
complete non-relative instruction) sets the same discriminator only for local
11251. It runs before trace sampling, with readable-data and checked-write
guards. Other goods/remote players retain native behavior. The Kit row for11251
now sets opmeMenuType63 and dialog36 to0, keeping effect7 and animation15.
Using the Kit item ends co-op directly according to its existing description:
guest leaves; host ends the party. The broken vanilla player-selection menu and
its unavailable confirmation are no longer part of this Kit item's use flow.
This does not modify vanilla111 availability, inventory IDs, grants, return origin,
progress or signaling.

Actual callback test observed RED for missing discriminator then GREEN. Actual
PrepareBbscKitRows test observed RED for copied menu4 then validates direct-use0,
dialog0, unchanged leave effect/animation, repeat preparation and source item
retirement. Independent scoped review finds no Important/Critical issues and
confirms native register/offset/instruction evidence. The tests establish adapter
behavior, not the visual animation or host/guest leave after bypassing the menu.
Next manual checkpoint must verify those live; do not claim presentation fixed
until then.


### Blank follow-up: guest drinks, host does not end the party

User reports the discriminator/menu correction did not fix cross-map presentation:
guest still drinks; host gets `?ActionEventInfo?`; host Blank fires but does not
send the guest home. User subsequently identifies a working baseline: both start
in the same area before joining, then exit animation/messages are correct. That
same-area behavior is the acceptance target; it must be preserved. Cross-map
loading is the distinguishing condition, not an established root cause yet.

Archived both logs as `coop-trace.cross-map-blank-failed.log`. Captures: host
p118014 and guest p118075. Guest native apply+executor succeeds twice; hook018F98B4
writes the discriminator both times; native OnLeaveMagic follows immediately.
Host capture of those uses is a remote player, not its own character. Its local
Blank resolves11251 at1792320920267 but does not call NativeApply/OnLeaveMagic.
Thus the previous value-write regression never proved visual animation or host
exit, and it is not sufficient evidence of a presentation fix.

Native SpEffect leave walk018FF705..018FF786 proves state6 goes to OnLeaveMagic,
while a host with remote-manager+98==-1 takes the individual-target branch and
never invokes it. The direct Kit row has no vanilla menu to supply a target.
New verified hook018FF70E (one8-byte non-relative compare) only queues local11251
in hoststate3 when that target is -1. A next-game-tick callback revalidates the
same living local character, active item, nonloading state and Lua pointer, then
uses the native OnLeaveMagic/MarkSlotsLeaving pair. A valid native target is left
alone; one dispatch per host session prevents repeated effect ticks. Death,
load/session transition or different character/item cancels a stale cue.

Actual callback+game-tick regression observed RED (host event never called) then
GREEN using executable native-call fixtures. It covers native target already
valid, remote character, other item, one-shot dispatch, loading, death, session
change and next-party rearm. This tests dispatch, not actual multiplayer teardown.

Guest animation and missing text remain unresolved. Read-only result probes at
018C377A and018C3A08 log actual local11251 animation/availability return values
and role/type; action-message formatter01AE78B0 logs the requested messageID and
caller. Each displaced span is complete and non-relative. These probes do not
change animation values, role fields or message text. ESD inspection used the
[SoulsFormats reader](https://raw.githubusercontent.com/JKAnderson/SoulsFormats/master/SoulsFormats/Formats/ESD.cs)
for the long-format state/condition layout; original game files are unchanged.
Next manual checkpoint compares same-area guest exit with cross-map guest exit,
and tests host exit after cross-map join. Do not claim these symptoms fixed until
that live evidence exists. Independent reviewer was unavailable due usage limit;
parent reviewed ABI and instruction spans, but no independent review completed.


### Four-case checkpoint: restore native menu, remove duplicate join reload

User confirms same-area guest exit is correct; cross-map guest still drinks and
host sees `?ActionEventInfo?`. Host Blank does nothing after either join type.
User additionally observes cross-map guest loading twice, versus once for same
area, and explicitly requests the native stop-co-op/stop-bell-signal menu back.
Both logs archived `coop-trace.blank-four-cases.log` (hostp181943, guestp181969).
Actual animation query returns15 and availability1 in both roles; there is no
local host-effect queue event. This disproves the prior host-effect fix as an
integration solution: its isolated callback test did not prove the cue occurred.
The cue hook/queue/tick branch and its isolated test have now been removed.

The host's departure formatter requests ID0 at0138A2A8 for broken departures,
versus20000404 for a normal departure. Guest arrivals likewise include ID0 versus
102100. Native virtual role getter018FE420 reads GameData+5D8; the old kept-reload
path is known to rebuild a guest with role0. Actual traces show native summon
finishing in the host map, followed by `BBSC join warp: full reload requested`
and v8 reinsert. The second load is explicitly requested by legacy
TickBbscJoinTeleport, not required by transport or the original summon. Its old
lighting workaround is now removed, together with the unused WARP_JOIN default
and flag. RequestBbscSessionWarp remains for subsequent host travel. The native
summon arrival/placement, original-point return and invader exclusion remain.
Guest presentation/role recovery from this removal is a live hypothesis until
verified; do not equate the query's animation15 with rendered firing animation.

Native menu restoration regression observed RED (menu0 instead of4). Final row
copies menu63/dialog36 from the original111 row on each prepare, retaining its
native stop-bell/end-co-op choice instead of bypassing menu selection. This is
explicit user preference; do not remove the menu again to mask an invalid role.

A new actual post-summon game-tick regression installs executable native
StageTransition/placement fixtures and supplies deterministic tick times. RED
reproduces an extra StageTransition after the native summon already reached the
host map. GREEN verifies no second load, one in-map placement, no repeated
placement, target retained until the correct map arrives, and no invader warp.
The native Blank menu/effect/animation-ID regression also passes. These tests do
not establish correct live lighting, visual animation, messages or host teardown.

Read-only result/message probes retained for the next checkpoint. Formatter
logging now excludes per-frame unrelated HUD messages; no text IDs are relabeled.
Next human checkpoint: start in different maps, join (expect one loading cycle),
verify no lighting regression, guest native menu + exit/animation/message/origin;
rejoin, host native menu + end co-op. Test stop-bell selection too. Same-area guest
behavior remains the acceptance baseline. Durable/event-only feature gates remain
open and legacy shared-progress import stays disabled.


### Live native-menu/single-load confirmation

User confirms one cross-map loading cycle, correct guest Blank animation and
messages, guest return, and host Blank successfully sending the guest home.
The guest sees a session-disconnected notice on host-initiated exit and asks
whether this indicates an accidental disconnect. Archived both logs as
`coop-trace.native-menu-single-load-confirmed.log`; captures hostp200905,
guestp200923. Both guest joins show native load finishing in18000000 followed
only by in-map placement; no extra full join reload. Guest arrival now requests
native message102100; voluntary guest leave requests OnLeaveMagic. Host Blank
is followed by RemoveMember and guest OnRoomDisappeared, then send-home.
Neither capture has an OnP2PTimeOut event. In this tested sequence the generic
session-disconnected notice accompanies host-initiated room closure; it does not
show an independent network failure. Do not globally relabel/suppress this notice,
as the same general room-loss path can also occur for unintended host loss.
No text/behavior patch is requested or applied for that notice in this checkpoint.

One native summon load is the intended current path. The removed second load was
a legacy lighting workaround and rebuilt guest state; correct visual behavior in
all maps/lighting phases remains unverified. This confirmation does not establish
durable offline return, geometry/lamp fallback or joined-event-only progression.


### Resume durable-return gate: character-selection evidence checkpoint

User authorizes continuing after the successful native join/Blank checkpoint.
Durable recovery remains blocked on reliable selected-character identity and save
acknowledgement, not journal serialization. Existing BbscActiveSaveSlot selects
recent file writes and must not authorize a durable restore. Additional static
inspection of0283FE55..02840007 shows the formatter's r13 index starts at0 and
loops over a count derived from a save container (minus7); file reads here cannot
be presumed to mean the selected playable character.02840150 receives job,
filename and that loop index. Neither it nor a last write is promoted to identity.

The authorized guest save copy contains Kaori in userdata0000 and EiEi in
userdata0001 (read-only UTF-16 known-name match); userdata0010 has both names and
remains metadata. Research inspects the existing game and copied saves only.

An opt-in `SHADPS4_BBSC_IDENTITY_TRACE` probe now logs once on each ready solo
character incarnation. It requires own-world/nonloading/living/local type0 and
session0. Candidate small words in the existing player-slot manager (0x90 bytes),
local data (0x100 bytes) and FrpgSaveLoadMan context (0x60 bytes at0556D438) are
bounded/read-checked and filtered to[-1,10]. These are unlabeled research values:
they can be counts, flags or pointer fragments, and are NOT slot/character/cycle
contracts. No raw save buffers, credentials, passwords or inventory are logged;
no save, journal, world or character is changed by the probe. The player-slot
manager+68 getter is not asserted to represent the save index.

The probe defaults off and is enabled only by the ignored checkpoint-2 helper.
Actual helper regression observed RED with absent evidence then checks bounded
small-word results, invalid pointers, zero/unaligned/oversize lengths. It does
not prove semantic identity. Manual checkpoint: in guest window only, load Kaori,
return to title/load EiEi, then return to title/load Kaori and quit normally,
waiting about10 seconds in the world for each. Do not ring/join. Compare candidate
transitions with native save trace before selecting a field, then statically
verify its setters and character-recreation/cycle semantics. Do not enable
persistent return or reward-save acknowledgement based on this probe alone.

### Character-switch capture: selected-save field narrowed

The user completed the Kaori → EiEi → Kaori checkpoint. Preserved guest capture:
`coop-trace.identity-kaori-eiei-kaori.log` in the ignored checkpoint directory.
Native file reads and successful commits follow userdata0000 → userdata0001 →
userdata0000. Ready maps/last lamps are respectively 0x18010000/2412951,
0x18000000/2402950, and 0x18010000/2412951. This is evidence for the tested two
characters only, not a stable character identifier.

Player-slot-manager+68 remains zero in all three incarnations: reject it as the
selected-slot source. SaveLoadMan+3C changes7 →8 →7, but static inspection shows
it is the save read job's container entry, not a general active-character key:
01831E90 takes a filename slot, adds7 at01831F57, and stores the resulting entry
at0183212B; a separate branch stores7 at018320F8. Callers also request slot10
(metadata), and the title scan at019C531B iterates slots. Therefore this field
can retain another read operation's selection. The small-word trace also omits
values above10; absence from that trace does not mean the field is absent.

Static evidence identifies a stronger selected-save candidate: global05556678
+1490. At0193FB12 the chosen load slot is retained in load-state+154; after
successful read,0193F809..0193F81E copies it into global+1490 immediately before
deserializing the character with0156B970. The save path0156D570 reads this field
at0156D5B2 when saving the current slot. Getter0156C5B0 and setter0156D430 access
the same field. New-character allocation writes a free slot or-1 at
01B3E633/01B3E648/01B3E654. The delete/load utility at01C2B440 also sets it, so
the field alone does not authorize recovery outside the ready-character lifecycle.

The existing opt-in ready-solo probe now additionally reads just the four bytes
at global+1490 as `global_save_selection`. No new hook or save mutation is added.
Its live correlation remains pending. Even if that correlation succeeds, slot
reuse, stable character discriminator, NG-cycle identity and mounted-path binding
still require evidence before durable journal recovery is enabled. Existing
timestamp-based selection remains unsuitable and is not replaced speculatively.

### Selected-slot correlation confirmed; legacy lookup correction

User completed the second checkpoint (EiEi → title → Kaori). Preserved guest
capture `coop-trace.selected-slot-eiei-kaori.log` shows reads/commits of
userdata0001 then userdata0000. The ready-solo `global_save_selection` samples
are respectively1 then0, matching both the actual native file accesses and the
static load/save evidence above. These once-per-incarnation samples do not prove
continuous stability, co-op lifecycle stability or a stable character UUID.

BbscActiveSaveSlotUnchecked now uses that native selected slot and rejects values
outside0..9 and an unreadable global. Filesystem times retain only the existing
legacy current-run-write eligibility gate; they no longer select a character.
An explicit BBSC_SAVE_DIR is authoritative, including failure: another profile
is never used as a substitute. Existing fallback layouts remain supported but
multiple qualifying owners now reject rather than pick the newest. Both apply
the process user filter and retain the excluded-user1004 rule. This correction
does not enable the new durable journal/recovery coordinator.

Actual-code regression uses native global memory and temporary mock save files:
with selected0 and newer1, old lookup selects the wrong slot (observed RED);
new lookup returns0, then1 after native selection changes. It also covers
metadata/invalid slots, missing selected file, unreadable global, explicit profile
priority and no fallback, multiple owners, excluded1004, and preservation of the
old current-run-write eligibility gate. Native slot selection is not sufficient
to distinguish a newly created character reusing a deleted slot; stable identity,
cycle, runtime mounted-path binding and save boundaries remain open gates.

### Native selected slot: co-op gameplay confirmation

User reports normal arrival/Blank/return behavior after the slot correction.
Both logs are preserved as `coop-trace.native-slot-coop-confirmed.log` in their
respective copied profiles. Guest trace observes slot1 before/after the party;
the guest-items caller also explicitly selects user `save`, slot1 while visiting.
Two joins preserve separate origins: map18000000 at(17.56,-9.51,102.66), heading
2.094, then map18010000 at(-193.40,-28.65,68.50), heading0.785. Each native
send-home restores its matching origin; the second does not reuse the first.
This confirms the tested co-op lifecycle, not durable restart recovery.

Further read-only native research establishes the local-data constructor
014F1510 via allocation014E54E9 and slot-root+8 assignment014E5516. Its vtable
05325B00 has RELATIVE relocation +18→014F2660, which reads the persisted0x120
bytes at local-data+10 after014EAA80. This gives a concrete save serialization
boundary, not an identity field. First0x54 save-header comparison across early
and current EiEi saves, current backup, and Kaori rejects the header as a proven
stable discriminator: observed varying fields include time/section positions,
while constant fields do not distinguish the characters. Large words were hashed
in research output. Local-data CE/CF/D0 and118..11A copied to character metadata
are also used by appearance/preset setters; do not label them creation dates.
The `CreateDate` string and network `SyncCharaId` strings have not established a
persistent playable-character UUID. No candidate is used for durable recovery.

### Character-incarnation checkpoint (isolated profiles, pending)

Prepare two new empty profiles, checkpoint-3-character-a and
checkpoint-3-character-b, with copied launcher settings/host overrides only.
Existing guest/host profiles and original saves are untouched. Both use slot0,
the same guest account, character name BBSCProbe, and the same default creation
choices. Run the ignored `run-character-identity.sh all`: create A and quit after
15 seconds ready; reload A, move and wait15 seconds, quit; create B in the second
empty profile with the same choices and quit after15 seconds ready. No character
deletion or co-op is required. Each stage prompts through the existing launcher.

After each successful game exit, validation requires ready slot0 observations,
an actual successful userdata0000 commit after readiness, exact expected save
size0x140000 and the expected UTF-16 test name. Unexpected/mixed-slot captures,
metadata-only/failed/premature commits, missing name and wrong sizes reject.
Existing samples are not overwritten; later stages require the preceding stage's
manifest. Snapshots preserve the save directory with SHA-256 manifest; retained
evidence extracts only save/identity/exit-status lines rather than copying network
authentication trace. Original launcher logs stay in their isolated profile.

bbport-dev preflight and validator self-tests pass; out-of-order reload/create
stages demonstrably refuse before launch. Both profiles have empty save folders.
Private HTTP/shadNet/WebAPI and bidirectional relay checks pass. No game launched
by the agent. This checkpoint supplies a same-character save pair plus a distinct
new character with matching visible creation choices, allowing volatile data and
appearance/name matches to be rejected before statically verifying any stable
candidate. It does not assert that such a candidate already exists.

### Fresh-profile preparation failure and correction

User reports save creation failure before character creation. Profile A trace
shows SPRJ0005 mounting as existing/read-only while userdata0010 and backup0010
both return ENOENT; no character save is written. The checkpoint erroneously
precreated the empty save container. Actual runtime mount logic rejects CREATE
for any existing container (ERR_EXISTS); this was a checkpoint setup error.

A scratch C regression includes the actual runtime_savedata.c and exercises the
real filesystem/mount logic with only file-mount routing stubbed: precreated empty
container→CREATE ERR_EXISTS; absent container→readonly ERR_NOT_FOUND, then CREATE
success/status CREATED; subsequent existing readonly mount succeeds. This does
not launch the game or establish successful native game initialization.

Checkpoint creation now keeps only the parent directory. Before each create
stage the helper removes a precreated container only with rmdir, rejecting any
nonempty directory or symlink. Reload stages preserve the existing container.
The two checkpoint-specific launchers/preflight accept an existing parent with
an absent container. The ordinary checkpoint-2/co-op launcher is unchanged.
Regression observed RED when empty container remained, then GREEN; nonempty data
and symlinks are preserved/rejected. Existing failed log is archived as
coop-trace.fresh-save-precreated-failed.log. With games closed, both empty
containers were removed, and bbport-dev syntax/self-tests/server/relay preflight
pass. Actual gameplay creation awaits the user retry of the same `all` command.

### Completed fresh-character evidence and save-close foundation

The user completed create-a, reload-a and create-b. All three captured save
directories pass their 13-file SHA-256 manifests. Native ready/commit evidence
confirms successful initialization and subsequent saving after the preparation
correction. Current and backup samples share the expected BBSCProbe name at
0x91e8 and FACE section at 0x118a8. The persisted local-data block has no aligned
four-byte candidate that stays equal across A's saves while differing from B.
There are 310 stable-different words elsewhere, all before the character name
(0x6740..0x78c4). Native reader 0x01A88760, called by 0x014F26B4 through global
0x0553e990, belongs to SprjGaitem.cpp. Inventory differences are not evidence of a
persistent character identifier. Identity, recycled-slot and NG-cycle gates stay
open; these samples do not authorize durable recovery.

Task 4 now has an independently tested filesystem foundation:
runtime_file_save_receipt returns bounded, mutex-protected observations keyed by
canonical parent plus committed filename. Only dirty atomic userdata0000..0009
closes qualify. Successful file flush, close, rename and directory flush advance
a process-local generation; failures retain the previous generation and record
positive errno, including a failed first save whose destination does not exist.
Metadata, reads, unwritten and zero-byte writes do not confirm a character save.
No callback or recovery export is installed; BbNetHost and ABI version 1 remain
unchanged. An observation cannot confirm which native snapshot or reward was
saved and is not yet consumed by the game adapter.

Fault injection also exposed a preexisting atomic-write failure: close could
commit a temporary file after write/pwrite had failed. Atomic files now retain
the first write/truncate/explicit-flush error and abort replacement on close.
EINTR is retried for write, pwrite, truncate and fsync before recording failure.
Regression tests exercise file/directory flush errors, ENOSPC, invalid truncate,
rename failure, missing first destination, separate slots/users, canonical
directory aliases, exclusions and interrupted operations. Focused tests pass in
bbport-dev. Independent scoped review raised first-destination and EINTR issues;
both received RED-to-GREEN regressions and the recheck found no remaining
actionable issue. This is raw filesystem evidence, not the complete Task 4
versioned boundary, native save correlation or offline recovery implementation.

Final verification: `bash build.sh --test` completed with exit 0 in bbport-dev
after all corrections, including file-save, runtime, pad, semaphore, online-import
and content tests. The online loader test's unavailable localhost shadNet is an
expected test environment result, not a private-server connectivity check.
`git diff --check` passes. No game was launched by the agent; this runtime change
has automated fault-injection evidence, not a subsequent gameplay save test.

### Mounted-save boundary, ABI 2 and capped native clear count (2026-10-19)

Native registration at 0x01341729/0x0134174a associates GetClearCount with
0x01334b90; AddClearCount registration points to 0x01334bb0. The getter reads
`[0x0553b130]+0x68` after its native guard. The setter increments this field and
saturates at 7 (0x01334bdd..0x01334c03). This explains the previously observed
player-slot-root+0x68 value; it is neither selected slot nor a unique discriminator
for cycles beyond that cap. Disassembly is retained in the ignored plan workspace
as native-clear-count-{registration,access}.{bin,txt}. No recovery key uses it.

BbNetHost now appends save_receipt and BBNET_ABI_VERSION is 2. Runtime and module
must be rebuilt/shipped together. Both old ABI and missing callback are explicitly
rejected. A failed module initialization no longer leaves module_handle eligible
for runtime_net_image_ready: an actual-loader fixture reproduced that old behavior
with a failing init, then passes after moving assignment to the success branch.

The receipt query accepts either its previous canonical host path or a mounted
/savedata path. Mounted queries resolve through the actual runtime mount table,
then use the same per-host-file observations. Remounting queries the new target;
unmounted/oversized paths fail without returning a stale receipt. This is a
point-in-time filesystem observation, not atomic active-character identity.

The native adapter validates global+0x1490 in 0..9 and queries precisely
/savedata0/userdataNNNN via the host callback. Its only gameplay caller is the
existing opt-in read-only identity trace. With a ready own-world character, it
samples at most once per second and logs only changed receipts (at most 64 lines).
It also labels the native clear count as capped at 7. Module defaults remain
trace-off; legacy journal selection, admission and progress behavior are unchanged.

Observed RED: querying a mounted guest path failed despite a committed host
receipt; the native selected-slot adapter initially returned ENOSYS; rejected init
still received image_ready. GREEN: actual file tests, native adapter assertions
and real dlopen/runtime loader fixture tests. The fixture tests are part of
build.sh --test and do not skip when missing. Final full build/tests exit 0 in
bbport-dev (mounted-save-host-full-build.log), including ordinary online imports
with the real rebuilt module. All seven native join/menu/return regressions pass.
Private game/shadNet/WebAPI and bidirectional relay preflight passes; whitespace
check passes. No game was launched by the agent. No new scoped independent review
is claimed for this increment.

Next human checkpoint: only the existing copied guest profile. Load EiEi, move a
few steps, wait about 15 seconds ready, quit to title normally, reload EiEi, wait
about 15 seconds and exit normally. No bells or second window. Expected evidence:
selected slot1 queries point into that guest profile's actual userdata0001,
successful native character closes advance process-local receipts with error0,
and ordinary save/reload still works. This verifies the new ABI/runtime behavior
in gameplay; it does not establish incarnation, uncapped cycle or native snapshot
correlation. Offline recovery and durable return/reward consumers remain disabled.

### Gameplay save passed; transient mount observation corrected

The user completed the EiEi checkpoint. Archived trace:
coop-trace.ephemeral-mount-observed.log, SHA-256
822dbf040ef858eaaa95512d6831304da4e1563f10984bea4e82f07daab0cd86.
Validation finds two userdata0001 read opens, seven character commits all returning
0, two ready observations with native selected slot1, and normal window closure.
Both ready maps are 0x18010000, clear count0. The copied userdata0001 retains
EiEi and size0x140000. Ordinary native saving/reloading therefore passed with the
new runtime. No other character read/commit is substituted as selection evidence.

The receipt portion of that checkpoint did NOT pass: query returned ENOENT and
generation0 despite successful commits, with 83 missing-mount diagnostics.
runtime_savedata.c unmounts the save-job directory after each operation; ready
gameplay generally has no /savedata0 mount. The previous test expectation that
receipt queries must fail after unmount was incompatible with this native lifecycle.

Correction: retain a bounded, mutex-protected canonical target for each save alias
when runtime_file_mount succeeds. Only the receipt resolver uses these historical
targets. Normal file opens/stat/translation still require an active mount.
Remount replaces the alias's target; a different or unresolvable root cannot fall
back to the former root. No password/pointer is retained; this process-local
history contains no stable incarnation proof and does not enable recovery.
ABI2 layout remains unchanged. Header documentation now explicitly describes a
last runtime target rather than an assertion that a mount remains active.

Actual file regression reproduced RED on commit→unmount→receipt, then GREEN.
Checks cover historical query plus denied ordinary open; remount to another root
before any commit; independent slots; canonical symlink root; missing new root;
oversized/traversal rejection; and metadata/read/unwritten exclusions. The real
dlopen host-callback test now also queries after unmount and verifies ordinary
file access remains denied. Full bash build.sh --test passes with exit0 in
bbport-dev (ephemeral-mount-receipt-build.log); whitespace check passes. The
new alias history has automated evidence, not a post-fix gameplay capture yet.

Short follow-up human checkpoint: existing copied guest window only; load EiEi,
move a few steps, wait15 seconds ready and close normally. Reload is unnecessary
because ordinary saving/reloading already passed. Expected new evidence is a
positive receipt generation, query0/error0 and that guest profile's canonical
userdata0001 path after the save job has unmounted. Incarnation, uncapped-cycle,
native snapshot correlation and offline recovery gates remain open.

### Post-unmount receipt confirmed in gameplay

The user completed the short EiEi follow-up. Archive:
coop-trace.save-receipt-confirmed.log, SHA-256
da41b4a6167bcb0f6f91c9ad3a8fa599302b0b91d41c71114e0fc6d505a18593.
Native selection is slot1. The initial ENOENT/generation0 precedes the first
character commit; once saved, the ready-world diagnostic reports query0,
generation3, close error0 and the exact canonical guest profile userdata0001
path (line1449). All four character commits return0, with normal window closure
and no repeated missing-mount warning. The final quit commit occurs after that
sample; generation3 is an observed sample, not a claim that all four were sampled.

This closes the live check for the ABI2 save-fact bridge and transient-unmount
fix. Ordinary save/reload was already confirmed in the preceding checkpoint.
No repeat of this unchanged check is required. These remain process-local close
facts, not native snapshot ownership or stable incarnation proof; durable return,
offline recovery and earned-progress/reward consumers are still gated.

### Native character lifecycle and serialization observations

Further read-only 1.09 disassembly establishes these candidate boundaries:

| Address | Observation |
| --- | --- |
| 0x0156d570 | Character save request: edi is slot (-1 selects global+1490), esi supplies queue options, nonzero dl takes the clear-slot path. Busy/queue failures can reject it. |
| 0x0156aa50 | Serializes into rsi, capacity edx. rcx is an optional written-size output pointer, not a mode. Local player data is written through 0x014ea560. |
| 0x0156b8f3 | Common serializer return: bl is success; successful output pointer is retained at original rsp+28. This remains an in-memory result. |
| 0x0156b970 | Deserializes a character buffer; selected slot and caller/load context still need lifecycle correlation. |
| 0x01831977 | Copy-input point in the individual-file queue branch: ecx is logical slot and r15 the buffer. This precedes the 0x02837100 copy and is not queue acceptance. |
| 0x01b3e633 | Character-creation flow chooses the first unused metadata slot and writes edi to global+1490, then marks its used byte. Reservation can still be cancelled. |

The normal request updates metadata through 0x014f9490 and calls the serializer
at 0x0156d662 before enqueueing at 0x0156d77a→0x01831810. The clear branch instead
clears the used byte and zeros a 0x140000 buffer. UI caller 0x01686af2 passes
dl=1; script wrapper 0x01c2a9fd passes slot=-1/dl=0. A direct-call scan of the tiny
0x0156d430 slot setter finds no direct callers; direct writes in the creation/load
flows mean that setter alone cannot prove incarnation changes.

The serializer reads live world state when global+1592 is nonzero. When away, it
can serialize earlier own-world buffers at +1698/+16b8/+16d8. A later successful
close or serializer entry therefore does not, by itself, prove that newly earned
world progress was captured. No reward acknowledgement is enabled on that basis.

Queue-copy tracing currently covers the +48=true branch of the native save
context, observed as1 in both earlier guest and fresh-character captures. The
other native branch uses a different file layout; it is not covered by this
diagnostic point and must not be silently promoted into the same contract.

Six read-only hooks now emit bounded [BBSC-save-native] observations only with
SHADPS4_BBSC_IDENTITY_TRACE enabled. Module defaults do not install these hooks;
ordinary trace hook counts account for disabled sites. Every six-byte signature
matches the supplied local image, including whole position-independent spans at
the three interior points. Slot/buffer/size/stack reads are guarded, metadata and
invalid slots excluded, failed serialization does not read an uninitialized
output pointer, and callbacks do not write native data or registers. Buffer
addresses are temporary diagnostic correlations only, never persisted journal
identities. Observations are not stable UUIDs, accepted saves or earned-event proof.

Actual helper regression went RED for missing decoding, then GREEN. A second RED
caught misinterpreting the optional size-output pointer as a mode; corrected
decoding leaves that pointer uninterpreted. Eight native regression cases and
the rebuilt module's ordinary online-import test pass in bbport-dev. The copied
checkpoint validator retains its prior slot0 checks and adds create/reload/reuse
checks, ordered buffer observations, post-ready commit/receipt, correct slot1/name
and preservation of the first character. Its self-checks and actual out-of-order
preflight guards pass without launching a game.

Next human checkpoint uses existing disposable profile A, preserving BBSCProbe
in slot0. Slot1 exists as a native preallocated all-zero file; absence is not the
right free-slot preparation test. The runner does not remove it. It performs
three separate user-run processes: create BBSCTrace and save normally; reload
BBSCTrace and save; then delete only that new test character and create BBSCTrace
again with the same choices. Each stage waits about15 seconds ready before normal
quit/closure. Full save snapshots/manifests are retained and verified before the
next stage, including before the user performs deletion. Original saves and the
regular co-op profiles are not used. The helper refuses existing sample output,
unexpected first-character changes or reordered stages. No game was launched by
the agent. Incarnation, recycled-slot semantics and native snapshot correlation
remain unproven until the captures are reviewed; offline recovery remains gated.


### Load-menu deletion capture remains unverified

The create and reload samples were retained, but three subsequent reuse trials
must not be accepted as complete. Each saved and closed normally with positive
slot1 receipts; none recorded native clearing before reservation. The user
confirms deleting BBSCTrace in Load Game and seeing it disappear before New Game.
The latest raw trace/full save snapshot is preserved separately in
`character-identity-samples/lifecycle-recreate-unverified-3`, raw SHA-256
`6200974b15e068aaf77f29b0ada9cfcce61755182d9b1ce9818683069f0586cd`.

Adding a read-only observer at 0x01686708 and then removing its unnecessary
loaded-character-state dependency did not produce a live clear event. Both are
insufficient to establish which menu path was taken. Static inspection finds
two additional callbacks at 0x01b3f1a0 and 0x01b45de0 that read the slot from the
task object, clear metadata occupancy and invalidate the matching latest-slot
reference. Their clear instructions are 0x01b3f1be and 0x01b45dfe, both the whole
five bytes c6 44 32 10 00. New opt-in observers decode metadata from RDX and slot
from RSI independently of loaded-character state, and append the hook offset
to diagnostic events. Their relationship to this user's Load Game path still
requires live evidence; they are not durable incarnation proof.

Regression tests first failed for these two callback sites, then passed with
readable metadata/slot bounds, absent-character state, unchanged registers and
unchanged occupancy bytes. All eight native regression cases and the bbnet
module build pass in bbport-dev. Existing missing-clear captures remain rejected;
there is no relaxation of reuse validation. The next necessary human observation
is limited to opening profile A, deleting only BBSCTrace from Load Game, waiting
for saving to finish and closing without creating or loading a character. A
slot-clear event and the subsequent metadata commit will identify the live path.
Offline recovery and earned-event consumers remain disabled.

### Load Game deletion callback confirmed

The subsequent deletion-only checkpoint confirms `slot-clear slot 1` at
`0x01b45dfe`, followed by successful `/savedata0/userdata0010` commit and normal
window closure. No character reservation, deserialization or serialization was
observed in this run. The full userdata0000 and userdata0001 payloads are byte
identical to the pre-deletion snapshot; deletion persists through metadata rather
than zeroing that character file. File existence, nonzero payload and a name in
userdata0001 therefore cannot establish that slot1 remains occupied.

The read-only bbport-dev validator checked these conditions and preserved the
full save snapshot, filtered evidence, raw log and SHA manifest in
`character-identity-samples/lifecycle-delete`. Raw log SHA-256:
`bd3ba57de69ca59cfbe91249d5fb3bfd371d7591b5fca6c927420a14a0540970`.
The changed metadata includes bytes at 0x89c..0x89f (1 to -1) and 0x108d (1 to 0),
but also many unrelated payload changes. These offsets remain observations,
not an established save format or character identity. No new mutation or recovery
consumer is enabled by this observation. Deletion needs no repeated gameplay
check; stable identity across restart and slot reuse remains a separate gate.

### Creation-only follow-up after the verified deletion

The helper now supports `run-native-save-lifecycle.sh recreate-after-delete`.
It uses the preserved `lifecycle-delete` snapshot as its starting state, verifies
the deletion trace SHA-256 and every archived file, then requires every current
save file to match that snapshot before launching the existing guest checkpoint.
The old `recreate` phase refuses to run once the verified deletion exists.
The user creates BBSCTrace once, waits in-world and quits normally; no repeated
deletion is requested.

The validator checks each process separately: the earlier process must contain
slot1 clearing at the live-confirmed Load Game site followed by a successful
metadata commit and normal closure, with no character load or creation; the new
process must independently reserve slot1, serialize/copy the same buffer, commit
and observe its own positive receipt for the exact selected save path. It does
not concatenate process-local generation counters. The final sample records the
new raw-log hash and the earlier deletion-trace hash alongside full save hashes.

Self-tests were RED for the missing deletion validation, then GREEN in bbport-dev.
Negative cases cover missing/wrong-slot/wrong-site/failed/unordered deletion,
missing closure and mixed creation. Actual CLI tests using temporary profiles
verify successful two-process provenance and rejection of changed deletion logs
or backup snapshots before creating an accepted sample. Extracted real-profile
preflight passes without launching a game; the obsolete phase is rejected.
This prepares the missing reuse observation only. It does not establish a stable
native UUID, uncapped cycle identity or durable recovery implementation.

### Slot reuse after deletion confirmed

The user completed `recreate-after-delete`. The accepted sample records native
slot1 reservation at 0x01b3e633, successful own-world serialization/copy ordering,
character commits and positive generations1/3/4 for the exact guest userdata0001
path, followed by normal closure. Revalidation in bbport-dev verified both saved
snapshots, the prior deletion-trace binding, all manifest hashes and unchanged
userdata0000 against the original first-character baseline. The accepted raw log
matches the current profile log byte-for-byte; SHA-256:
`80f24530177c71457968427aaa7b7af7527639976f2bf548dec9261ed621f1a8`.

Create, reload, deletion and subsequent slot reuse now have captured native
boundaries. This completes these observations, not stable incarnation identity.
An observational comparison found961 aligned four-byte words unchanged across
the first create/reload but different after reuse, including fields before the
previous item region. Such differences can be ordinary game state; they are not
adopted as a UUID or persistent character discriminator. The comparison artifact
is `lifecycle-reuse-comparison.json`. No repeated gameplay checkpoint is needed
for the already verified create/load/delete/reuse operations.


### End-of-day commit verification: 2026-10-19

The full runtime build and tests passed in bbport-dev with BB_PGO=off and
BB_LTO=OFF, including file durability/error receipts, actual dlopen host callbacks,
rejected module initialization and real module import routing. Six explicit
native targets then passed: memory permissions, portable scan, actual RX 9060 XT
exported dma-buf scan, party policy/return/journal, native join/Blank/lifecycle
adapters, and real loopback HTTP worker polling/cancellation. Identity capture
self-tests, including the separate-process deletion/recreation validation, passed.
The expected localhost shadNet-unavailable test output does not establish private
server connectivity; prior user gameplay captures provide that separate evidence.
No game was launched, and original saves were not modified. This is a checkpoint
commit; unfinished identity, admission, recovery and event/reward integration
remain gated as described above.
