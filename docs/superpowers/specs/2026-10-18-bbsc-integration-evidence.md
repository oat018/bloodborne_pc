# BBSC integration evidence and checkpoints

## Current checkpoint summary (2026-10-21)

This summary is authoritative; later numbered sections and 2026-10-19 paragraphs
are dated evidence and must not be read as current implementation status. The feature
is still partial and not ready to package.

- Latest usage-limit checkpoint keeps native admission activation OFF via
  `IsBbscPrivateCoopAdmissionRequired() == false`. New source has strict type-0x29
  capability routing, HMAC proofs without exposing the native password, held
  Matching2 success callbacks, a game-thread coordinator and immutable selected-save
  backups before Prepared publication. Private-attempt lifetime and Matching2
  cleanup/shutdown race fixes have focused tests and scoped re-review. Native
  guest preparation/rollback has focused tests and a clean scoped review. Root's
  combined module, six focused executables and three runtime checks pass;
  native cancellation is still unproven. No live cancellation or new
  event/reward application is claimed.
- Current integrated source includes ABI3 committed-copy receipts (runtime producer,
  BBSC consumer); ordered, peer-bound transport foundations; selected-character home
  snapshots; offline recovery-only startup and bounded native-lamp retry; journal-v4
  post-destination SerializeEnd / ABI3 receipt fence; restart-aware managed cycle
  epochs/rollover; a 22-boss, three-required-item catalog with explicit endgame
  deferrals; and native dynamic flag-span/group capture with an immutable,
  selected-character/receipt-gated optional flag reader.
- HTTP observation now retains successful native sign/search/claim identity,
  exact unique target selection and the observed nonempty password match. It
  follows the final worker-poll result delivered to the game, rejects stale or
  ambiguous attempts, and survives normal completed-request read/delete cleanup.
  Pending abort/delete and context termination still invalidate pending work.
  It is role-neutral observation, not a remote-peer attestation or admission gate.
- Focused synthetic tests cover production cycle observers for fresh native 6→7 and
  saturated 7→7, same-epoch save receipt windows, restart cancellation from the exact
  native-6 preimage, and recognized postimage resume with a reset process generation.
  They also cover cap-7 return completion, prior-home rollover guards, catalog
  policy, real flag-hook installation and synthetic serialized flag reads. These
  do not prove native gameplay placement or progression in a running game.
- Still disabled or unfinished: normal-join home preparation/admission, selected-host
  capability/password/backup binding, first-playable event consumer, live native
  event acquisition validation, event/item application and save-confirmed retirement.
  Windows build/recovery and gameplay validation are also open.
- `gpu/bbnet/CMakeLists.txt` now includes `bbsc_party_journal.cpp` for Windows.
  Native Windows journal file operations check path components and reject existing
  reparse points. This is a source change; Windows compilation/linkage/runtime are
  unverified, recovery hooks remain disabled, and hostile concurrent parent-path
  replacement is outside the path-based checks' guarantee.
- Admission clarification: the Lua dispatcher's null-name return proves only that
  the named Lua handler is skipped. The native caller at `0x0131A7AD` ignores its
  return value and continues bookkeeping at `0x0131A7B2`; this is not proof that
  the native join is cancelled. A safe pre-world wait/reject path is still required
  before normal managed admission can be enabled.

**Source boundary:** Use the BBSC already carried in BBPORT (`gpu/bbnet/`) as the
primary implementation and reference base. The inspected [bbport revision
`8185b88d80573fa300595d5c54450183b4109935`](https://github.com/deadinside28/bloodborne_pc/tree/8185b88d80573fa300595d5c54450183b4109935)
is the current comparison baseline. The inspected external [CLSAMG22/BBSC revision
`315f76c38b1ca5843b082f93f47927a9193ad94a`](https://github.com/CLSAMG22/BBSC/tree/315f76c38b1ca5843b082f93f47927a9193ad94a)
is supplementary only for behavior missing from BBPORT's existing BBSC, not an
equal or replacement base. These are inspected versions, not a permanent freeze.
Keep feature behavior under `gpu/bbnet/`; the only current runtime exceptions are `src/runtime_net.c` for offline
recovery module loading (otherwise runtime offline mode never opens the module) and
`src/runtime_file.c` for the canonical selected mount path before first dirty close
(the module cannot resolve the private `/savedata0` mount). The path-only ENOENT result
has zero generation/bytes/hash and is not save proof. Tests supporting these exceptions:
`out/online-test --bbsc-recovery-offline` and `--bbsc-recovery-module` check offline
imports/workers and fail-closed invalid-image loading; `out/online-test --bbsc-save-host`
checks path-only ENOENT then a real dirty-close receipt. Necessary future bbport edits
must state the concrete gap, why BBSC cannot supply the needed runtime fact, and a
focused test; this is not a prohibition or renewed approval gate.

Focused final checks in `bbport-dev` are logged under
`.superpowers/sdd/2026-10-18-bbsc-anywhere-coop/`: `cycle-final-{build,party,join,
transport,events}.log` and `cycle-cap7-return-{build,join}.log`. The runtime bridge
checks are `offline-recovery-resume-build.log` and `offline-recovery-resume-checks.log`.
The latest caller regression checks are `cycle-restart-caller-{build,party,join}.log`.
All listed focused checks pass. No game was launched, no Windows build or AppImage/EXE
was produced.
The later continuation is independently checked in
`continuation-final-checks-2026-10-20.log`: `bbnet` plus party, native join/flag-proof,
catalog, targeted transport and real HTTP-worker tests pass, followed by all three
runtime bridge checks above. Scoped source reviews covered the Windows path changes,
native flag capture/reader and HTTP observation lifecycle. The integrated native
fixture installs the actual `0x0156AC56`/`0x0156AC94` pair in one mapped image before
testing proof attribution; the HTTP fixture reads and deletes completed native
requests without erasing their successful observation. These remain synthetic
fixtures and do not complete live co-op acceptance or a whole-branch review.
The pre-existing dirty FSR-Vulkan submodule must be preserved.

## Historical checkpoint 1

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

### Native server-character identity lead and local allocator (2026-10-19)

Resuming from the accepted lifecycle captures, a full aligned-word comparison of
slot1 create/reload/recreate yields 961 words stable in the first pair and changed
in the replacement. 951 lie before0x6740, in the serialized SprjGaitem prefix;
three are at0x19ff4/19ff8/1a000, and seven at0x1035b4..104574. These are observations,
not 961 possible UUIDs. The full list is retained in the ignored
`identity-reuse-candidates-all.json`; no values are promoted into recovery keys.

Native code provides a more useful interpretation of the trailing16 bytes of
this capture's player section:

- Writer014F23A0 calls SprjGaitem writer01A88440, then writes local-data+10/120
  bytes and subsequent player sections. Its last two8-byte writes at014F2557 and
  014F258E source local-data+688 and+690. The matching reader014F2660 restores
  those fields. The captured header's player offset54 plus length19fb0 ends at
  1a004; the final16 bytes therefore start at19ff4.
- Constructor014F1510 initializes+688 to8000000000000000 and+690 to0 at014F1B2B /
  014F1B32. Debug UI014F3E40 binds those exact fields to UTF-16 labels
  `ServerUserId : %d` at049650B8 and `ServerChrId : %d` at049650DC. The previous
  tentative item-reference interpretation of this tail is superseded by these
  concrete native labels. These are network identities, not an uncapped NG count.
- Native request builder01E8BE50 writes `CharaIdNum` (049A8254), numeric1, and
  dispatches request kind2; the message-name relocation at05350B80 names
  `SyncCharaIdRequest`. Reply decoder01E7F240 reads `PublishCharacterIdList` at
  01E7FAC0 and the first object's `PublishCharaId` at01E81CBF; the parsed64-bit
  result is placed at reply+10 by01E8815F.
- Native caller01C43EE0 requests allocation only if local-data+690 is zero;
  otherwise it copies the stored value to network manager+ A90. Callback01C440A0
  stores reply+10 into local-data+690 at01C440E5 and copies it to that manager.
  Callback01EA7570 also assigns the returned ID to both locations. This establishes
  request/parse/save wiring statically, not successful allocation in a game.
- The repository server previously returned a successful empty list. The native
  reply default is8000000000000000; some old accepted snapshots contain it in
  ServerChrId, whereas the latest recreate snapshot contains0. A nonzero sentinel
  can bypass the native allocation request. Do not silently clear it, mistake it
  for a valid identity, or reuse such a character for the first allocation trial.

The local gameserver now reserves positive integer IDs, bounded by2^53-1 for exact
JSON numeric representation, in `<--data path>.characters.sqlite3`. SQLite full
synchronous transactions finish before IDs are returned. Successful allocations
remain reserved across process restarts; retries can waste IDs but cannot return
an already-reserved ID from this database. A uniqueness conflict or storage error
returns ResKind1 with no IDs. Malformed counts reject, and corrupt database startup
fails instead of silently resetting its contents. This uses Python's standard
library and leaves the existing online-data JSON format unchanged.

Test-first evidence: the original registered route fails the positive-ID assertion
(0 entries instead of1) and incorrectly reports success without storage. The
replacement passes eight focused tests covering native response shape/bounds,
separate-process reuse of the persistent database, concurrent requests, invalid
counts, unavailable storage, corruption preservation and actual loopback HTTP
success/failure and integral JSON numeric counts (1 and1.0, excluding booleans,
fractions and nonfinite values). The Python suite runs inside bbport-dev; the exact final result
is recorded in the session ledger. No game or original save was launched/modified.

Pending: identify the actual private-server deployment/source path, update that
service, then observe a disposable character with ServerChrId0 obtaining a positive
ID, saving, and retaining it across reload. A replacement in a reused slot must get
a different positive ID. This server patch is not deployed by the local test and
does not establish live per-character/cycle binding; durable recovery and earned
progress consumers remain disabled. Previously verified deletion-hook captures do
not need repetition. Retain this registry on persistent container storage/backups.

### Unraid image prepared; deployment pending (2026-10-19)

The user confirms Docker-loaded images `localhost/bbsc-gameserver:0.6-pre1` and
`localhost/bbsc-shadnet:0.6-pre1`, both with `--init --pull=never`. The gameserver
Host Path is `/mnt/app-data/appdata/bbsc/gameserver`, mapped to `/data`.

The matching cached gameserver image is amd64/Linux, ID
`7c2a52e066e42d91c4a9c98513573bafe29e0edfab6c3f2fa2a5ebd6ea9ec700`.
Its `/opt/bbsc-container/server.py` starts the repository's Python server at
`/opt/bbsc-gameserver/bbsc_gameserver.py` with `--data /data/online-data.json`.
Inspection used stopped disposable containers; neither installed game was started.
The image's old server script differs from current source only by the allocator
patch, and its bbhost_service.py hash matches the repository exactly.

`Containerfile.native-id` copies only that server script into the existing image.
The resulting `localhost/bbsc-gameserver:0.6-pre1-native-id1` image ID is
`bf1cc68f2025f1edceb6a735ee73c28fc2bac72553e8c15e65ab629cb162a660`.
The complete image Config equals the original Config; the original tag/ID is
unchanged. Build and verification were orchestrated inside bbport-dev, using
`distrobox-host-exec` for the host's existing Podman; no substitute toolchain or
packages were installed.

Two isolated, network-disabled test containers exercised the real image entrypoint
against a disposable volume. Both numeric1/1.0 requests succeeded, IDs were present
in SQLite before the HTTP reply, HTTP/STUN health passed, and the registry was
written as UID99. Removing the first container and creating another on the same
volume retained both earlier IDs and added two distinct IDs (four reserved total).
All test containers/volumes were removed afterward; private services/data were
not touched. Result retained as ignored `container-update/smoke-result.json`.

Export: `out/containers/bbsc-gameserver-0.6-pre1-native-id1.tar.gz` (137 MiB display),
Docker archive SHA-256
`a554dfd08f6ce0523c3114c984de8205e77a374bd42ae37fb539e5ef621ad924`.
The archive manifest names the new tag, and its final-layer server bytes match the
locally tested source. A portable companion checksum file is included. This is
local image/packaging evidence, not proof that Docker load or native gameplay has
run on Unraid. The README gives load/template/rollback instructions; next user
checkpoint is installing the gameserver image while retaining the confirmed data
mapping. ShadNet needs no image change. Native ID/save/reload correlation remains
pending before any durable recovery consumer can be enabled.

### Private deployment reachable; first native allocation trial ready

The user reports completing the Unraid image update. A request to the private
server's ss.info succeeds, and one SyncCharaIdRequest with CharaIdNum1 returns
ResKind0 and one positive, exact integer PublishCharaId. This reserves a single
unused test ID; it does not assign it to a game character or prove restart
persistence on Unraid. The result is retained in ignored
`container-update/private-deployment-check.json`.

The existing guest launcher --check in bbport-dev passes for game HTTP, shadNet,
WebAPI, bidirectional relay and executable/module dependencies. Taking the guest
profile lock, every file matches the accepted recreate-after-delete manifest;
slot1 BBSCTrace's native ServerChrId is still0. The complete disposable save
folder and previous trace are copied to `character-identity-samples/native-id-before`
before gameplay, with their baseline hashes and the native tail offset recorded.
Slot0 BBSCProbe remains unchanged. No game was launched by the agent.

Next user checkpoint: the existing checkpoint-3-character-a guest launcher, load
only BBSCTrace, remain online for30seconds, move slightly, quit normally and close
the window after saving. No join, deletion or creation is needed for this first
allocation observation. Inspect the resulting trace/slot1 payload before asking
for a second process reload. This remains an allocation experiment; recovery and
progress consumers stay disabled.

### First native character ID persisted (2026-10-19)

After the user's allocation trial, BBSCTrace slot1 has a positive, exactly
representable ServerChrId instead of the recorded0; its native ServerUserId is
also assigned. The current userdata0001 and native backup0001 carry the same pair.
The raw trace confirms slot1 deserialization, three own-world serializer/copy
sequences and successful character commits, positive generation2/error0 receipt
for the exact slot1 host path, and normal window closure. No slot reservation or
clear event appears, and no failed commit is present. BBSCProbe userdata0000 is
byte-identical to its pretrial baseline.

The full save snapshot, raw trace, filtered evidence and provenance manifest are
preserved at ignored `character-identity-samples/native-id-allocated` without
replacing previous captures. Raw SHA-256:
`082e15aba8763cda2b34953cc5999794db337825d06fc440860e86a43708a516`.
userdata0001 SHA-256:
`0b920441b6f1b741150da973b4a348d3807aa6de7dcfe4ee7796a2323faecd80`.
Validation ran inside bbport-dev under the guest-profile lock and verified the
prior snapshot hashes before acceptance. No game/save was modified by the agent.

This verifies the first native allocation/save path. Next load BBSCTrace in a new
process and save/close, then compare the recorded positive ID; do not repeat its
first allocation, delete it, or create a replacement yet. Cross-process stability,
reused-slot distinction, cycle binding and recovery consumers remain unverified.

### Native ID survives a new game process (2026-10-19)

The user's reload trial retains the exact ServerUserId/ServerChrId pair recorded
in native-id-allocated. RE capture process61660 differs from the prior process56302;
this is a real reload rather than another save in the same game process. The trace
shows slot1 deserialization, own-world serialize-success/copy, successful commit
and positive/error0 receipt for userdata0001, then normal closure. There are no
clear/reservation events or failed commits. BBSCProbe userdata0000 is unchanged.

Under the profile lock, validation checks every prior archived save hash and prior
raw-log hash before accepting the new full snapshot, raw/filtered trace and
provenance manifest at `character-identity-samples/native-id-reloaded`.
Raw SHA-256:
`669ed883c098f3bac3f60d9ef301501fa7789457449face93fc95a092041dc04`.
userdata0001 SHA-256:
`2670d9be8dbf60c327ffa1f83e178d7e7e6d889d71992f44403450dac3913ecc`.
The check ran in bbport-dev; the agent did not launch a game or write to saves.

Next verify that normal deletion/recreation of ONLY disposable BBSCTrace in slot1
allocates a different positive ID. This is a new identity distinction test, not a
repeat of the already-proven deletion observer. Keep BBSCProbe slot0 intact and
preserve both current accepted identity snapshots. Reused-slot distinction,
uncapped cycle binding and safe durable recovery remain open until their own
checks pass; no recovery/event consumers have been enabled.

### Reused slot gets a different native character ID (2026-10-19)

The user's replacement trial passes the full native ordering: slot1 clear at
01B45DFE, successful metadata commit, slot1 reservation at01B3E633, own-world
serialize-success/copy, successful character commits and positive generations1/3/4
with error0 for the exact userdata0001 path, then normal closure. The new character
has the same name and same native user ID but a different positive ServerChrId.
Current userdata0001 and backup0001 both carry this new identity. BBSCProbe slot0
remains byte-identical to the baseline. No failed commits are present.

Validation inside bbport-dev takes the profile lock and verifies all prior
native-id-reloaded archive hashes before saving the full new snapshot, raw trace,
filtered evidence and provenance manifest to `character-identity-samples/native-id-reused`.
Raw SHA-256:
`d9ddcf69fdff1b654bfad1fc27210173c76959b0009bdf8532ff5564085c2297`.
userdata0001 SHA-256:
`07e95ba3ba5232637388e5e80ee10e5dd58faa493338180afc6422231e7229d2`.
No original saves were modified or game launched by the agent.

The controlled native allocation→new-process reload→same-slot replacement checks
are now complete. They establish the positive native character ID as a concrete
incarnation discriminator for this server/capture path; they do not complete the
runtime CharacterKey, uncapped cycle identity, visiting-world save ownership or
recovery admission contracts. Existing zero/sentinel IDs remain ineligible; do not
repair them silently. No further repetition of these three trials is requested.

Follow-on static cycle inspection confirms why incrementing the saved clear-count
alone is insufficient: AddClearCount01334BB0 has size106 and is registered by an
indirect function pointer written at01341776→05548E78. A direct-call graph therefore
finds no external E8/E9 callers; that does not mean the callback is unused. Its
increment at01334BE4 and saturation at01334BF6..01334C03 cannot distinguish saved
cycles beyond7. Fresh complete-function/access and registration disassemblies are
retained as `native-cycle-1334b90.*` / `native-cycle-13416c0.*`, plus the graph.
No NG transition hook, counter or cycle mutation has been enabled from this static
observation; a transition/durable-identity contract remains to be established.

### Native identity/file correlation adapter (2026-10-19)

`ReadBbscNativeCharacterIdentity` now reads the selected slot from global+1490,
positive ServerUserId/ServerChrId from `[0553B130]+8` local-data+688/+690, and
compares those IDs with the selected userdata file's header-described player
section tail. It requires an own-world, nonloading, local-type0 context and the
explicit BBSC_SAVE_DIR provided by bbnet_glue. It canonicalizes that directory,
rejects a symlink character file, and reads only userdata0000..0009 for the native
selected slot. There is no newest-file selection or current-process mtime gate.
A matching file from before process startup is valid evidence. Legacy callers of
BbscActiveSaveSlot retain their previous behavior and write-time gate.

The adapter rejects absent/unassigned/sentinel/out-of-range identities, invalid
slots, visiting/loading/nonlocal state, unreadable native pointers, malformed1.09
header/section ranges, wrong file size, mismatched persisted IDs and absent
explicit directory. It reports the live capped clear count as evidence only;
no CharacterKey is constructed from it. The opt-in identity trace polls at most
once per second and logs at most64 changed valid tuples, including canonical save
path, allowing assignment after the first ready tick to be observed. It runs only
inside the existing ready-own-world identity trace and does not write native
memory, files, flags or journals. Recovery/admission consumers remain disabled.

Test-first native regression: the minimal reader stub fails the selected-slot /
ServerUserId assertion at test_bbsc_join.cpp:377. The reader then passes fixtures
for positive IDs, pre-run mtime, capped count7 as evidence, invalid counts/IDs,
metadata/other slot, loading/visiting/nonlocal contexts, unreadable local data,
corrupt format/offset/length/overflow, mismatched ID, truncation, missing selected
file, symlink file and missing explicit profile. The native-data fixture remains
byte-identical after reading. Existing native return/menu/slot/lifecycle tests
continue passing.

Fresh bbport-dev verification: full `BB_PGO=off BB_LTO=OFF bash build.sh --test`
passes, as do all six explicit native memory/memscan/real-RX9060XT-dma-buf/party/
join/HTTP-worker targets. Python unittest discovery passes131 tests in13.336s.
The existing CMake CMP0167 and RADV conformance notices are informational. No game
was running during the build; FSR reverse-patch checks passed before build.sh,
so its reset fallback was not used. No container/server update is needed for this
native module change.

Further read-only NG research found the label EndingStep::STEP_GetGameClearItem
at virtual049881E2 referenced at019572D4. The surrounding registration table stores
that name at05573B70 and a callback01956F90 at05573B78. A nearest-preceding FDE entry
at01956FB0 has size8 and does not cover the reference; it is not treated as this
registration function's boundary. The extracted surrounding region is only a
research lead, not proof of a committed new-cycle boundary. Literal-name searches
in the unpacked script/talk bundles and common EMEDF found no AddClearCount or
cycle call site; absence of those names is not proof no transition exists. No
cycle hook or persistent counter has been enabled from these observations.

Next controlled runtime check: load existing BBSCTrace in copied profile A and
save/close, then verify `[BBSC-native-identity]` names the selected slot1, the
already-recorded native IDs and exact canonical userdata0001 path. This checks the
new live reader, not another allocation/recreation experiment. Do not delete or
recreate the character. The uncapped-cycle, visiting snapshot ownership, offline
startup and reject-capable admission contracts remain open.

### Live selected native identity/file correlation confirmed (2026-10-19)

The user's new-reader trial passes. The trace deserializes slot1 at line738,
reports `[BBSC-native-identity]` at line1176 with the exact previously recorded
native user/character IDs and canonical guest userdata0001 path, then makes the
first current-process character commit at line1209. Receipt generation was still0
before the identity line. This confirms correlation with a persisted save from
before process startup, without waiting for a current-process character write.
Later serialization/copy/commits succeed, a generation2/error0 receipt names that
same path, and the game closes normally. No failed commits, clear or reservation
events occur; BBSCProbe slot0 is unchanged and slot1 retains the same native IDs.

Inside bbport-dev under the profile lock, validation verifies the prior reuse
snapshot/raw hashes, the exact trace tuple/path and before-first-character-commit
ordering, plus the current saved IDs and slot0 baseline. Full snapshot, raw log,
filtered evidence and provenance manifest are preserved as
`character-identity-samples/native-id-correlated`. Raw SHA-256:
`396750e73583e0a27db2fac5b93b964733db0d31ebecefc970f22e7dd2b6619d`.
userdata0001 SHA-256:
`66ebb0bac13185611a84d3e155a41b55fb36e2a53847583a638fa8f23b2d679e`.
No game was launched or save written by the agent. These native-reader trials
need no repetition. Uncapped cycle binding, visiting snapshot ownership, offline
startup/recovery and reject-capable admission remain separate open contracts;
no journal mutation/recovery consumer has been enabled.

### Native NG increment event sites identified (2026-10-19)

Read-only inspection identifies three implementations: Lua-exported01334BB0,
0131D260 and the inlined increment017C0FD9 inside017C0420. Each increments
manager0553B130+68 and clamps to7. No external direct E8/E9 callers were found
for the first two; indirect callers remain possible.

Dispatcher017B93A0 subtracts2000 before bank table017B9544. Entry3 reaches
017B9486 and calls017C0420. That handler subtracts1 before command table017C2C38;
entry20 reaches017C0F9A and the increment. This identifies bank2003/command21
without relying on absent script names. All27 original `event/*.emevd.dcx`
files contain exactly three matches, each with argument bytes01000000, in
`m21_00_00_00.emevd.dcx`:

| Event ID | Zero-based instruction index |
| --- | --- |
| 12100000 | 43 |
| 12100002 | 35 |
| 12100180 | 14 |

The scan validates decompressed size, table bounds,48-byte event/32-byte
instruction records, argument bounds and unique owning event for each match.
Two initial scratch scans rejected the zero-argument offset0xFFFFFFFF sentinel;
the corrected scan and exact three-site self-check pass in bbport-dev.
Scratch `native-cycle-event-increment-sites.json` SHA256:
35090fb4f5dbbe78a84ef58e5f517e0fa329657fe796ce901c86b091c79e8631.
Original m21 DCX SHA256:
1edbb9cd5035822e9dee5eb33075a3b096ab050e865d625c5f49d37c86014de8.
Callgraph, dispatcher and increment disassemblies are retained beside the report.

Nearby command2003/28 targets manager055404E8, named **CSTrophy** by native string
0493AE80. It is not a proven ending/save boundary; scratch filenames containing
`ending-manager` predate this identification. Event instruction order does not
prove a new-cycle snapshot committed. Next correlate increments with selected
native serialization and successful commits, including interruption/replay and
the saturated7→7 case. No hook, durable counter or save mutation was introduced;
no new gameplay trial is requested.

### Live versus serialized capped cycle evidence (2026-10-19)

Serializer014EA560 records the cycle section's offset at header+2C and its length
at header+30. At014EA6B5..6DC, global+1598 selects global+159C when set, otherwise
manager0553B130+68. It writes four bytes at014EA6F0, then writes manager+6C as a
separate four-byte field at014EA716. Reader014EAA80 restores the first word to
manager+68 at014EAF0E and the second to manager+6C at014EAF3A. The second word is
not established as an uncapped cycle and must not be adopted as one.
Native01937D09..10 caches manager+68 into global+159C and sets1598 before
01937D51 replaces the manager value. Thus serialization can deliberately retain
the cached count; a successful file commit alone is insufficient cycle proof.

Read-only copied profile inspection agrees with the header: userdata0000 has
cycle section19EC6/8 and userdata0001 has1A09A/8, both with two zero words.
Scanning762 root-reference regions found18 with +6C accesses; these include
unrelated objects and do not prove an uncapped root counter. Disassemblies and
`native-cycle-adjacent-accesses.json` remain in the plan workspace.

The opt-in native identity trace now reports both live and saved capped counts.
The existing reader validates the declared eight-byte section, rejects overlap
with the player section, overflow/out-of-file offsets and saved counts outside
0..7. Different valid live/saved values remain visible as separate observations;
equality grants no recovery permission, especially7→7. The adjacent word is not
interpreted. Existing ready-own-world gating, native ID checks and explicit
canonical save ownership remain in force. No snapshot atomicity or committed
transition is inferred from this diagnostic read.

TDD: the minimal unimplemented saved-count field fails the new zero-count
assertion (exit134); the reader then passes the actual adapter suite, including
live7/saved0, saved7, negative/8, section overlap/overflow/wrong size and an
arbitrary adjacent word. No native memory or save writes were added. Runtime
trace presentation of this additional field has not been tested in gameplay;
the prior identity trials remain accepted and need no repetition.

### Native snapshot provenance checkpoint prepared (2026-10-19)

The user has no near-ending or NG+7 fixture and asks to use the existing copied
test save. Real ending transitions, especially saturated7→7 and interruption,
remain a separate unverified gate. Do not ask the user to finish a fresh character
or manufacture an ending by writing flags/counts.

The shared read-only `ReadBbscSerializedSnapshot` now parses bounded format41
native snapshots, extracts the positive user/character IDs and capped count, and
hashes all140000 bytes with the module's existing OpenSSL dependency. The selected
file reader uses that same parser and reports its hash. Native lifecycle evidence
copies and parses buffers at deserialize, successful serialize-end and copy-input;
serialize-begin and failed serialize-end never claim a completed snapshot.
`[BBSC-save-snapshot]` binds each hash/IDs/count to its lifecycle sequence, stage,
slot and own-world label. An unchanged ID/count with changed payload has a
different hash. Cached visiting output retains its visiting label.

Three additional opt-in SaveLifecycle sites observe pre-increment count:
0131D260/01334BB0 use the byte-checked eight-byte stack prologue;017C0FD2 uses
`8B506831C989D6`, after the RIP-relative lookup. The latter avoids unsupported
RIP-relative instructions in the existing trampoline. Exact bytes at all three
sites match the inspected1.09 ELF. No new trampoline implementation is needed.
The stage is explicitly `cycle-increment-entry`, success remains unknown, and no
counter or save is changed. Entry can still precede a native guard/no-op. All
these hooks use the existing identity-trace opt-in and128-event bound; absence
after the bound is not proof that no transition occurred. Hash work also stops
after this bound.

TDD evidence in bbport-dev: null parser RED, then independent Python hashlib
fixture784549a4d1d963aff045cb64b9a38ceaf0b358993310b8dad3d61fb131cbc390
and malformed/changed-payload checks GREEN; missing native-buffer snapshot RED
then GREEN; missing cycle observer RED then all three paths GREEN. The actual
adapter checks cover changed copy input, explicit copy slot, guest/cached output,
failed/unfinished serialization, invalid counts and unchanged native buffers.
Full `build.sh --test` and all six explicit native suites pass; actual RX9060XT
dma-buf and real HTTP-worker tests remain green. Pre-build no-game/FSR checks and
guest `--check` pass, including all four private services and bidirectional relay.

Next human checkpoint uses only the existing guest BBSCTrace: load online, move,
wait roughly30 seconds/save indicator, quit normally and close after saving.
No join, character deletion/recreation or NG transition is required. Compare the
loaded hash with accepted native-id-correlated, successful serialize-end hashes
with copy-input and the final file hash/commits, and receipt path/generation.
Preserve all raw bytes and the first-character baseline. If hashes differ between
native stages, investigate that transformation instead of treating timing as
snapshot proof. This new buffer-provenance observation is distinct from the
completed native identity trials. It has not yet been run in gameplay.

ABI2 receipts are unchanged and still do not atomically bind generation to a
snapshot hash. This diagnostic checkpoint alone will not enable cycle identity,
admission, event/reward writes or restart recovery.

### Live native snapshot/file provenance confirmed (2026-10-19)

The user's existing-BBSCTrace trial passes the new buffer-provenance checkpoint.
The deserialize hash matches accepted native-id-correlated exactly. Three own-world
successful serialize-end/copy-input pairs have identical hashes within each pair,
followed by three successful userdata0001 commits:

| Serialize/copy sequence | SHA256 | Commit log line |
| --- | --- | --- |
| 3/4 | 15a25bcef1906e13dd60b7a066de95969828faff5500f369d5454e32bac9649d | 1212 |
| 7/8 | 6b7252fc245a604ff00c4562acf421dc5d9aee6354151eb5186ad769e99cff0e | 1372 |
| 11/12 | a5d00c3bcdb3a3a6dc5a223586487dd067d4205d19a321cedd08d5bd7446aa68 | 1919 |

The final saved userdata0001 hash equals the last pair. Native user4127 /
character2723151752040083 and capped count0 remain consistent in current and
backup saves. All nine other userdata character slots are byte-identical to the
accepted prior baseline. No slot clear/reservation, failed commit or NG increment
was observed; the window closed normally. Generation2/error0 names the exact
canonical character path after the second commit; the ready-file diagnostic also
reports the second pair's hash. That receipt precedes the third save, so it must
not be described as the final snapshot's generation/hash receipt.

The three new hooks install: total170/176 versus prior167/173. The same six older
RE hooks fail in both captures (OnSummonResult.Move/OtherError, ForceSummonSuccess,
ChrAction.UseItem.Inventory, Event.WarpNextStage, Warp.RespawnTransform.Resolve).
This checkpoint does not establish that those unrelated trace sites work, nor
does installing an NG observer prove an actual NG transition.

Under the profile lock in bbport-dev, prior archive hashes and baseline raw log
were verified, then full saves/raw log/filtered evidence/provenance manifest were
preserved as `character-identity-samples/native-snapshot-correlated`, without
overwriting prior samples. Raw log SHA256:
470d86e54065e3dd7167988e86bcdaf6fd16da531e3c618c70080a46690ac83a.
No game was launched or save modified by the agent. This buffer-provenance trial
is complete and needs no repetition. Next establish an atomic snapshot/hash
binding to the runtime receipt generation; ABI2 does not provide it. Real NG
transitions/replay, offline recovery and admission/event consumers remain open.

Checkpoint verification before commit (2026-10-19): fresh `python3 -m unittest
discover -s tests` passes131 tests; all six explicit native suites pass in
bbport-dev, including the actual GPU dma-buf and HTTP worker checks. Native test
targets are up to date and `git diff --check` passes. The earlier full native
build and this live snapshot trial remain applicable to the unchanged production
sources. Local save/log/image artifacts are ignored; the existing FSR submodule
working edits are not part of this checkpoint commit.


### ABI3 committed-copy receipts (2026-10-19 16:15 +07)

The runtime now binds canonical path, process-local generation, byte count and
full SHA256 to one successful committed temporary copy. It opens the actual
copy read-only, checks its inode against the closing writer, streams it through
the existing OpenSSL EVP implementation and checks the byte count. The receipt
mutex covers hashing, rename, directory durability and publication; queries
cannot observe a partly published tuple. This is historical copy evidence,
not a claim that a later externally edited file still contains those bytes.

A close/durability failure or hash failure clears bytes/hash and retains the
previous generation with an error. A hash diagnostic failure still allows the
normal atomic game save to complete. Generations advance only with successful
hash and durable close. The receipt struct grew, so BBNET_ABI_VERSION is3;
actual loader tests reject ABI1/2 before reading their callback fields. Runtime
and module must be rebuilt together. build.sh links libcrypto, already supplied
by the project's OpenSSL dependency and bbport-dev; no packages were installed.

TDD: the real file-operation fixture first failed the expected new hash/length
assertion (exit134, receipt-hash-red.log), then passed. Independent hashlib golden
values cover a three-byte save and a100003-byte streamed payload. Tests exercise
interrupted reads, lock ownership at rename, a later external edit retaining the
historical receipt, hash-read failure without losing the game save, a subsequent
successful new generation, and all existing file/directory flush, rename, write,
mount/remap and unwritten-close failures. The actual dlopen host callback returns
the expected four-byte SHA256/length; the selected-slot adapter forwards both.

Verification in bbport-dev:
- BB_PGO=off BB_LTO=OFF bash build.sh --test: PASS, including actual runtime loader.
- Rebuilt final native module/fixtures after the diagnostic log-format change;
  all six native suites PASS, including the real RX9060XT dma-buf test and actual
  HTTP worker. Final logs: receipt-hash-native-build.log / native-suite.log.
- guest --check: four endpoints reachable, relay frames verified both ways;
  current executable/module dependencies pass, no game started.
- git diff --check: PASS. Pre-existing FSR submodule changes were preserved.

The receipt trace preserves its original path prefix and appends
`[bytes <n>, sha256 <hex>]`. Existing accepted identity/provenance captures remain
valid. New live checkpoint uses the same copied guest BBSCTrace: move/wait for
saves, quit to title, wait for saving, close normally. Compare positive ABI3
receipt hashes with native successful serialize/copy pairs and successful closes;
check the final file separately if further saves followed a receipt. Verify other
slots against native-snapshot-correlated. No join, recreation, ending or server
image update is needed. The user runs gameplay; no live ABI3 result is claimed.
Actual NG transition/7→7 replay evidence is unavailable. CharacterKey, durable
recovery, admission and earned-event consumers remain incomplete/off.


### Live ABI3 generation/hash correlation accepted (2026-10-19 16:23 +07)

The user completed the existing copied guest BBSCTrace checkpoint. Under the
profile lock, bbport-dev validation verified every prior archived save/raw hash
before checking and archiving this new capture as native-receipt-correlated.
Raw log SHA256:
`54477f526e06deb5053f94bbf9b8f674d33a5295364300bac6ec710fc303440e`.

Deserialize seq0 equals the prior accepted final userdata0001 hash
`a5d00c3bcdb3a3a6dc5a223586487dd067d4205d19a321cedd08d5bd7446aa68`.
Three successful own-world native serialize/copy pairs retain slot1,
ServerUserId4127, ServerChrId2723151752040083 and capped count0:

| Serialize/copy seq | Full snapshot SHA256 | Successful commit line |
| --- | --- | --- |
| 3/4 | c67d3bc342570756b880fce38d802de9ae5314e7ebba153c86451cb07dcad04e | 1231 |
| 7/8 | 269a8e27e0dcd48843d10d579b14c88e9d09af20ca0dc3197d493a5337f20840 | 1392 |
| 11/12 | ef99a2dd63b6670e887c3793462bcd90e0d8d4170f635bf1eee03f596e71cade | 1808 |

At raw line1453, ABI3 query0/generation2/error0 reports the exact canonical guest
userdata0001 path,1310720 bytes and the second pair's SHA256. Native file identity
at line1454 independently reports that same second hash. This confirms the new
receipt binding in gameplay; it does not relabel the later third save as
generation2. The final userdata0001 equals the third pair's hash. Current and
backup native IDs/count agree. All nine other character userdata files remain
byte-identical to the prior accepted snapshot, including BBSCProbe slot0.

The process closes normally, with no failed character commit, clear, reserve or
cycle entry observed. The same six pre-existing RE hook installation failures
match the prior capture exactly; this is not an all-hooks-pass claim. Full saves,
raw bytes, filtered evidence and a provenance/hash manifest are archived without
overwriting previous captures. The completed identity/snapshot/ABI3 receipt
checks need no repeat. Actual NG transition and interrupted/replayed7→7 remain
unverified; the saturated field is still evidence only. No recovery, admission
or event consumer was enabled, and no original saves were written by the agent.


### Native increment applied seam (2026-10-19 16:38 +07)

Three additional SaveLifecycle observers remain under the existing identity-trace
opt-in. Sites0131D2A8 and01334C05 are after the final clamp/store, with seven
absolute instruction bytes `488b00483b45f8`. Site017C045A is the shared event
handler epilogue, with eight absolute bytes `498b4500483b45d0`. The existing
provider verifies and replays each whole-instruction span without relocation.

Engine/Lua observations require the retained previous count0..7, incremented
register=previous+1, final register=min(previous+1,7), root+68 pointer and matching
live memory. Lua's guarded skip reaches01334C05 too; it fails these checks. The
native01334C03 store was deliberately excluded because its five-byte span would
cover01334C05, a separate guard branch target. The event epilogue additionally
requires the original command21 descriptor at task+ B0, its successful r14b,
root+68 pointer, previous/incremented/final registers and matching memory. Other
commands and invalid pointers cannot create an applied observation. Logs append
previous count and stage `cycle-increment-applied`/success1. This denotes an
observed native store, never a successful save or durable cycle identity.

TDD real adapter: missing post-store evidence first failed the new assertion
(exit134, cycle-applied-red.log), then passed. Fixtures cover0→1,6→7,7→7,
wrong registers, skipped guard, other event commands, bad pointers and changed
memory; observers leave fixture memory unchanged. A separate replay fixture
executes the actual captured arithmetic through the production
bbport_guest_code_hooks.cpp and common/decoder.cpp provider, rather than a hook
stub. All three patched spans install and execute correctly; the skipped Lua
path enters the patched epilogue but produces no applied evidence. The frame
setup/guard/return are test scaffolding, not the original whole game function.
This does not simulate an ending, native save, interruption or process restart.

Source provenance: initial inspection mistakenly compared the decoded ELF with
the original SELF hash and stopped before extracting bytes. Corrected validation
reads the original eboot.bin, verifies SELF SHA256
`d65f0b4f01d59166aed16f8604196d8b7dd805abbf0758b356e8f1354c9429f9`,
reuses scripts/prepare.py:parse_self and checks the decoded bytes equal the cached
ELF. Decoded ELF SHA256 is
`cec1b276e7f9e4db978e57f524f41fbaac594530a3437b002e23f3fab14b4f86`.
All three new hook signatures match. Captured engine0131D280..2A8 and
Lua01334BDD..4C05 arithmetic bytes are identical. Source proof and extracted bytes
are retained in cycle-applied-sites.json/log. No game files were modified.

Further read-only inspection of root+6C confirms direct assignments1/2/3 at
0193FEC5/0193FEDB/0193FEF0, selected by nearby bit checks21/22/23. These writes
provide no support for an uncapped monotonic cycle counter; its full semantics
remain unclaimed. Do not reinterpret the serializer's opaque second word as an
exact cycle. Runtime Windows target also now explicitly finds/links
OpenSSL::Crypto, required by the ABI3 receipt code; Windows build/runtime remains
unverified. No package or server-image change is implied.

Verification: bbport-dev full `BB_PGO=off BB_LTO=OFF bash build.sh --test`
PASS; the final six native suites PASS (including actual RX9060XT dma-buf,
13 join-adapter checks and real HTTP worker). Whitespace check PASS. Logs:
cycle-applied-full-build.log, cycle-applied-native-build.log/native-suite.log,
cycle-applied-replay-build.log/replay.log. No Python production changes; no
redundant Python-suite rerun. No game launched, save edited or new manual
checkpoint requested. Actual ending/restart remains unverified and recovery
consumers remain off. No fresh whole-branch review or Windows build is claimed.


### Task5 party message codec, routing still disabled (2026-10-19 16:48 +07)

Read Task5 brief before work. The included SHAD signaling sender, receiver,
Matching2 classifier and BBR1 relay paths were checked before reserving0x28.
ClassifyInternal routes only0x21 to Matching2 and0x10 to Control; other signaling
payloads already use the signaling queue. BBR1 forwards opaque frame payloads.
No packet-type conflict was found; no relay/classifier change is needed for the
codec. Existing0x22 presentation snapshots and0x26 legacy progress packets are
rejected by this decoder rather than converted to new events.

Wire definitions live beside np_signaling_bbsc.h; the policy header shares the
kind enum instead of mirroring numeric values. Layout, little endian:
magic4/type1/version1/kind1/reserved-zero1/party16/admission8/sequence8/chunk2/
chunks2/event-count2, followed by4-byte IDs. The header is46 bytes. Packets are
bounded at1024 bytes;244 IDs occupy1022 bytes. Baseline chunk indices/counts are
bounded to5 chunks for the1024-event storage envelope; aggregate/catalog checks
belong to the admission consumer. Events require a positive sequence and nonempty
list; controls have no event payload or chunking. An empty single-chunk baseline
is valid. All messages require nonzero party/admission, nonzero unique event IDs,
supported kind/version/type and exact lengths. No untrusted struct-layout casts
or allocations based on an unchecked event count are used.

TDD real party target: after correcting a range-loop copy warning under-Werror,
the stub codec fails the independent50-byte golden packet assertion at line60
(exit134, party-codec-red.log). Implemented codec passes golden encode/decode,
all truncated prefixes, extra bytes, oversize input/counts, reserved/version/type/
kind corruption, zero identity/admission/events, duplicate events, control payload
rejection, baseline chunk bounds and the244/245-ID limit. Maximum u64 counters
and u32 ID round-trip without truncation. Policy, return and real journal tests
also pass. This is framing validation only: positive IDs are not proof of release
catalog membership or of events earned after admission.

Final bbport-dev verification: rebuilt bbnet and all six native suites PASS,
including codec/journal and actual increment-hook replay. Actual ABI3 dlopen save
callback, rejected-module behavior and online-import/trophy isolation PASS.
Logs: party-codec-native-build.log/native-suite.log/online-suite.log. No new Python
production changes; no redundant Python-suite rerun. git diff --check PASS.

Task5 is not complete: no new send/receive branch, peer selection, admission
state machine, retransmission or durable Ack consumer is enabled. No party packet
or new event/reward was emitted, and no private server/image change or gameplay
checkpoint was requested. Cycle-store/save/restart binding, native admission and
recovery remain open. These tests do not replace the real two-player acceptance
matrix or Windows validation.


## 2026-10-19 — Ordered admission, peer routing and durable intent delivery

User prioritizes feature implementation toward Linux/Windows pre-release before
broad gameplay validation. No new gameplay checkpoint was requested in this batch.

Implemented `Admission` with host account plus connection binding, party/admission
identity, ordered Hello/Ready/baseline/Ack setup, independently chunked late-guest
baselines, bounded reordered event delivery and departure cutoff. Old baseline
events do not become earned events. Four guest admissions are tested independently.
`Delivery` retains baseline and event packets for retry until peer-bound cumulative
Ack; rejects duplicate, stale and future acknowledgements. Caller retry cadence and
actual native first-playable boundary remain integration work.

The actual signaling helper routes type0x28 only to the selected established peer;
source address/port resolves uniquely to that account/connection before queueing.
Unknown/inactive/ambiguous peers and malformed messages are rejected. Matching2
publishes owned copies of room owner and membership after create/join, membership
changes, leave, kick and context teardown. Game-thread inbox selection is restricted
to that room; a guest accepts only its actual owner. Room/owner changes purge stale
selection/messages. The receive callback copies and deduplicates bounded messages;
it performs no game-memory mutation. Native initial arrival also waits for the
actual host character rather than selecting another living guest as a fallback.
This last adjustment is built/fixture-checked, not newly gameplay-confirmed.

`PersistEarnedEvents` merges earned intent into the real atomic journal and returns
Ack only after successful StoreJournal. Flush failure returns no Ack and leaves the
caller's record unchanged; retry is idempotent even if rename already installed the
candidate. This is intent durability, not native inventory/save completion.

Ruling: a single current-party pending list could relabel an unfinished claim when
the next party began. Journal version2 retains event/party/admission/commit status
for earlier claims, carries them from Complete to a new Prepared record, and permits
binding the new host identity after origin capture. Earlier proof cannot be erased,
retargeted, duplicated or moved from committed back to pending. Version1 decodes
without retained claims and upgrades on successful write; corrupt/unknown records
are preserved and rejected. Maximum1024 total distinct claims and65536 bytes remain.

Focused bbport-dev results: bbnet module build PASS; bbsc-party-test,
bbsc-party-transport-test and bbsc-join-test PASS. Coverage includes actual helper
routing and actual Core inbox/snapshot callbacks, reordered/late admission,
retry retirement, journal fsync faults, successive-party provenance, v1 upgrade,
malformed retained status/admission and monotonic commit. Initial retained-claim
check build failed at the missing PrepareNextParty linker symbol before its
implementation; that is not a behavioral RED test. Earlier admission, delivery,
inbox and durable-Ack behavioral failures and subsequent passes are retained in
party-*-red/green logs. Final logs: party-retained-build.log,
party-retained-checks.log and party-retained-final-{build,checks}.log.

Task5 remains partial: the native coordinator does not consume this new inbox or
emit the new party protocol yet. No new event grant, production journal or private
server mutation was performed. Restart recovery, fallback validation, native save
fence, managed cycle binding, complete boss/story/item catalog and Windows journal
implementation remain release work. No Linux AppImage or Windows EXE from this
batch is claimed feature-complete, and no fresh whole-branch review is claimed.


## 2026-10-19 — Native home capture and pending-recovery join rejection

Task6 started for its independent home/admission seam while Task5's native protocol
coordinator remains partial. The identity adapter now constructs a CharacterKey
from the canonical selected save folder, native ServerUserId/ServerChrId encoded
as16 deterministic little-endian bytes, slot and saved/live cycle0..6. Capped7 or a
counter mismatch cannot authorize a journal capture. The live identity lookup is
separate from persisted snapshot validation; a damaged/missing userdata file must
not hide an existing recovery record from the join guard. Lookup alone never
adopts a cycle or applies a journal.

`PrepareBbscNativePartyHome` captures actual local placement and the current own-world
native respawn point resolved by0154AFC0 before load, writes them through the existing
atomic journal writer and returns its held character lease. Files live beside the
selected userdata in `.bbsc-party/home-<slot>-<native-ids>.bin`; the filename excludes
cycle so an unresolved earlier journey cannot be silently bypassed. Invalid/foreign
cycle records are preserved. Pending recovery cannot be replaced by a new origin;
Complete records may start another party while retaining earned provenance.
This stores destinations only: finite original coordinates are not collision proof,
and a resolved native respawn point is not a separately proven unlocked-lamp catalog.
The complete admission caller must additionally establish own-world snapshot durability,
backup, capability and safe fallback policy. It is not yet enabled for normal joins.

Verified reject-capable seam: cached1.09 ELF SHA256
`cec1b276e7f9e4db978e57f524f41fbaac594530a3437b002e23f3fab14b4f86`.
The Lua dispatcher entry01339870 moves RSI(name) into R12 and tests it at01339898.
A null name jumps to01339B22, returns EAX0 and runs the native canary/stack cleanup
without event dispatch. The new Linux guard checks only JoinSession_White: an
existing pending, locked, symlinked, malformed or unverifiable managed record sets
RSI0 in the provider's register snapshot, and the caller displays a recovery notice.
The actual provider restores that modified RSI. Existing absent-record joins and
unrelated Lua names/character IDs remain unaffected. Files native-lua-join-entry
and native-lua-null-return retain extracted bytes/disassembly. Initial objdump of
PS4-typed ELF was rejected by the tool; file-backed PT_LOAD extraction followed by
binary x86-64 disassembly succeeded. No unsupported offsets were substituted.

A small executable predicate fixture through the actual hook provider verifies
rejection suppresses the action and returns0; Complete permits the action after
lease release. It does not execute the complete game's Lua handler body. Native
home fixtures call the real capture adapter, actual journal/lease/fsync and a native
respawn resolver ABI adapter. RED key assertion, RED home capture, RED pending gate
and RED damaged-userdata bypass are recorded, then focused GREEN results. Tests
cover saved/live identity binding, deleted-slot replacement, capped7 rejection,
origin/fallback round trip, held lease, pending origin preservation, complete record,
corrupt/symlink journal, invalid userdata, unrelated character and counter mismatch.

Found runtime work inside sampled diagnostic formatting: own-world pre-join capture,
load curtain and Blank leave handling were called only by WriteLuaSessionEvent.
Moved those existing calls to TraceEntry's unsampled Lua handling, after the pending
recovery guard and before trace filtering. Formatting now observes rather than
initiating those operations. The move is compiled and inspected; no gameplay run
or complete-TraceEntry executable fixture is claimed.

bbport-dev final module plus join/party/transport builds and all three suites PASS
in party-home-final-{build,checks}.log. Earlier red/green artifacts use party-home-*
(including party-home-key-red and party-home-invalid-save-red). No agent game launch,
original save write, production managed journal, server/image change, commit, push
or package in this batch. Full first-playable protocol, managed cycles, own-world
snapshot durability, restart warp/save confirmation and Windows journal remain open.
No new manual checkpoint is required to continue independent implementation.

### Identity-bound prepared own-world snapshot adapter (2026-10-19)

The prepared-home journal now references a separately stored, immutable own-world
payload. The existing map-group capture is bound in memory to the verified native
`CharacterKey`; legacy slot-only files intentionally have no such binding and cannot
be adopted by this adapter. `PrepareBbscNativePartyHome` serializes the captured map
groups with a 2 MiB limit, writes a party-specific `BBWS` envelope containing the
full character key and party, fsyncs the temporary file, atomically renames it and
flushes its directory. Journal version3 records the payload byte count and CRC32;
the writer refuses to persist that reference unless the exact snapshot validates.
The adapter returns no prepared home when the capture is missing, belongs to another
character, exceeds the bound or cannot be durably written. Snapshot files are
immutable per party ID; retries with identical content are idempotent, and changed
content for the same ID is rejected.

Focused regressions pass for correct native identity/party binding, wrong-character
and wrong-party reads, truncation and reference mismatch, same-party overwrite,
missing referenced data, write failure preservation, existing unresolved-journal
preservation and all preexisting native join/journal behavior. Built `bbnet`,
`bbsc-join-test` and `bbsc-party-test` in `bbport-dev`; both focused executables pass.
This is still an adapter slice: normal joins do not call the prepared-home adapter,
and the new payload is not yet loaded by restart recovery. Backup/capability gates,
recovery restore/warp/save confirmation, native admission coordination, managed
cycle transition handling and the Windows journal remain incomplete. CRC32 detects
accidental damage and binds content to the journal reference; this is not a
cryptographic authenticity check.

### 2026-10-19 — Managed own-world recovery at the native home-load boundary

The managed payload now enters the existing game-thread own-world restore path at
the verified native respawn-point resolver hook `0x01938F65`. The earlier generic
load hook remains a defer-only boundary for managed records. A pending `Visiting`
or `Returning` journal restores only when the selected live and persisted native
identity still matches its `CharacterKey`, the game is back in its own world, the
payload envelope/reference/CRC and map groups validate, and every target group has
live storage in a ready flag manager. The restore reuses the existing group merge
rules. It leaves the managed journal and snapshot unchanged because native save
confirmation is not yet connected.

`Prepared`, `Complete`, corrupt/unverifiable records, foreign selected characters,
host-world state, truncated snapshots, missing storage, and malformed flag managers
do not enter managed restoration. The 15-second legacy sidecar fallback is held
while the current identity has a prepared, pending or unverifiable managed record;
identity-bound in-memory recovery is also rejected if the selected key changes.
Legacy slot-only bytes remain unbound and are never imported into the managed
payload.

Focused `bbsc-join-test` fixture coverage checks empty/not-ready capture rejection,
the `Prepared` and host-world phase/world guards, defer to the native resolver,
correct character restore, foreign identity rejection, truncated-payload and
missing/malformed-manager preservation, and the journal/payload remaining intact
after restore. The focused `bbnet bbsc-join-test` build and join fixture pass in
`bbport-dev`; logs are `managed-home-recovery-{build,checks}.log` under the ignored
`.superpowers/sdd/2026-10-18-bbsc-anywhere-coop/` directory.

This connects only restart load recovery. It does not enable normal-join admission,
offline loading/warp, a completion transition or native-save confirmation. Pending
recovery can consequently be reapplied on later verified home loads until a future
save-confirmation implementation safely retires it. CRC32 remains accidental-damage
detection, not cryptographic authenticity.

### Win32 prepared-home persistence adapter (2026-10-19)

The native journal and own-world snapshot storage no longer unconditionally use
POSIX file APIs. The `_WIN32` path uses an exclusive, reparse-point-checked lock
handle for the character lease; bounded reads open the reparse point itself and
reject directories/reparse entries; writes use a unique same-directory temporary,
`FlushFileBuffers`, and `MoveFileExW(MOVEFILE_WRITE_THROUGH)` for publication.
Immutable snapshot publication omits `MOVEFILE_REPLACE_EXISTING`, so a raced
same-party writer cannot replace the first payload. Snapshot path suffixes use the
native wide path representation. POSIX retains `flock`, `fsync`, and atomic rename;
immutable snapshot publication now uses same-directory `link` to refuse replacement.

The Linux `bbport-dev` build of `bbnet`, `bbsc-party-test`, and `bbsc-join-test`
passes, as do the party journal/protocol and native join fixtures. Logs are
`windows-journal-linux-build.log`, `windows-journal-party-checks.log`, and
`windows-journal-join-checks.log`. This environment has no MinGW or clang cross
compiler, so this is source-level portability work only: no Windows compile,
filesystem/runtime test, or EXE/package validation is claimed. Windows ACL,
sharing-violation, crash-durability and replacement behavior still require a real
Windows build and focused runtime fault tests. This adapter does not enable normal
join capture, admission, event application, offline warp or journal retirement.

### 2026-10-19 — Return destination/save fence v4 checkpoint

Prepared-home journal v4 persists a monotone `native_return_selected` marker. The
guest send-home caller selects the saved native respawn-point ID through the verified
last-lamp branch; the native point-to-map resolver supplies an off-map target's map.
An off-map destination's zero XYZ fields are unresolved metadata, never warp input.
The older raw `native-selector-1.09.txt` artifact was disassembled with the wrong ELF
mapping and is non-authoritative; `root-native-selector.{bin,txt}` uses corrected
PT_LOAD extraction. Legacy raw process-local origin coordinates remain restricted to
records without managed pending/unverifiable state and do not authorize managed return.

The production return tracker accepts SerializeBegin only after a durable selection
marker and exact post-load destination confirmation. It resolves the recorded native
point after its map loads, compares live position, and requires own-world/player-ready
state. First destination confirmation invalidates any earlier in-flight hash/receipt.
Only a successful later SerializeEnd hash matching a newer ABI3 selected-file receipt
can complete the Returning journal. Failed or unresolved checks preserve it.

Focused party unit coverage passes for correct/foreign key, native point/map/position,
world/player readiness, pre-destination rejection, generation/hash/path matching and
completion proof. The native join fixture exercises durable v4 marker reload and
fail-closed destination mismatch: despite a newer receipt it retains Returning and its
snapshot. A matching synthetic destination is covered by the tracker unit fixture; a
successful production native resolver return has not been demonstrated by this fixture
or gameplay. `bbnet`, party/join/transport targets and their focused executables pass
inside `bbport-dev`; logs are `checkpoint-v4-final-{build,party,join,transport}.log`.

This checkpoint still does not establish a full return coordinator: destination
selection/confirmation and receipt handling are wired, but restart/offline module
activation and return warp/retry behavior are unfinished. Normal-join admission,
capability/backup policy, event catalog/application, exact NG7 isolation, Windows
compile/runtime and broad gameplay remain open; no package was built.

### 2026-10-20 — Offline restart path connected, native outcome pending

The Linux runtime image-ready seam now loads the module's recovery-only exports even
when `BB_ONLINE=0`; this does not invoke ordinary module initialization, import
registration, NP/auth setup or worker creation. The actual DSO fixture confirms no
new thread and no online import. Its image is deliberately invalid, so the native
installer rejects it and preserves the pending journal; this proves fail-closed
loading, not successful gameplay recovery.

The native respawn resolver uses a boundary-specific identity reader that permits
the game loading flag only there. It still binds only the selected native slot, exact
canonical mounted file, native user/character IDs and equal persisted cycle. Other
identity readers still reject a loading character. Before every native selector or
StageTransition mutation, the return tick checks the armed key and reloads the
Returning journal while holding its character lease; foreign/missing/corrupt records,
changed fallback/reference, or character switches withhold the mutation. Automatic
reload requests are capped at three; if native destination confirmation is not
observed, the durable record remains pending. Selector choice is not arrival proof.

Before any dirty close in a new process, the runtime receipt callback may return the
canonical resolved `/savedata0` file path with ENOENT and all generation/size/hash/error
proof fields zero. This is only mount/path provenance. BBSC then reads and validates
that exact file and native identity; the path-only result cannot satisfy native save
completion.

Focused `bbport-dev` build and checks pass: `offline-recovery-resume-build.log` and
`offline-recovery-resume-checks.log` in `.superpowers/sdd/2026-10-18-bbsc-anywhere-coop/`.
The tests cover load-boundary vs ordinary identity, recovery-only loader, no-thread
behavior, selected-save path-only ENOENT, and the existing journal/identity/transport
fences. No game was launched. Actual selector/StageTransition gameplay, restart
arrival, destination confirmation and final save receipt are unverified; Windows
recovery, normal-join admission, event application/catalog and NG7 isolation remain
unfinished. The earlier notes above that say offline module loading/warp is absent
are superseded for this Linux source implementation only; no end-to-end return claim
is made.

## 2026-10-20 — Room-bound party inbox selection

The normal BBSC game tick now refreshes the type0x28 peer allowlist from the
owned Matching2 room snapshot. Hosts resolve only `room.members`; guests resolve
only `room.owner`. Each account must map to exactly one currently established
signaling account+connection; missing or ambiguous mappings clear the allowlist.
The receive queue remains bounded and game-thread-only. This adds no send,
admission, event application, item grant or journal mutation caller.

The join fixture calls the refresh and exercises owner-only delivery plus ambiguous
connection rejection. The module and `bbsc-join-test`, `bbsc-party-test`,
`bbsc-party-transport-test`, and `bbsc-party-events-test` build and pass in
`bbport-dev`; logs are `party-peer-selection-build.log` and
`party-peer-selection-checks.log`. Candidate trace data exposes a numeric account
token/user ID, while room ownership is available only after asynchronous JoinRoom.
No source-verified mapping or reject-capable native gate before world mutation is
connected. The native save decoder also does not yet prove that event flags/items
landed in a selected-character save. Therefore event/reward writes remain disabled;
cap7 exact-cycle isolation and Windows journal integration remain open.

The recovery installer also validates the native selector and StageTransition byte
signatures against the existing 1.09 `NativeCallSignature` table before activating
those direct calls. The join fixture now invokes the production return tick with
counted ABI stubs and checks success, selector rejection, loading guard, foreign
character, absent/corrupt journal, preserved Returning state and retry exhaustion
across a repeated same-character/party restore. This covers the branch logic only;
the synthetic stubs do not establish native in-game placement or load behavior.

2026-10-20 cycle-resume evidence update: journal and native-hook tests now cover
generation reset across process restart, recognized pre-hash cancellation, expected
post-hash reserialization, equal-hash 7→7 ambiguity, consecutive tagged epochs, empty
claim rollover, unresolved prior-home blocking and missing-sidecar anchor rejection.
The join fixture calls the production cycle and return observers in callback order for
a tagged cap-7 character, requires native destination confirmation before SerializeBegin,
matches the later ABI3 selected-save receipt, stabilizes the epoch, and completes the
return journal on the following tick. Focused logs are
`cycle-resume-{build,party,join,transport}.log` and
`cycle-cap7-return-{build,join}.log`. The runtime boundary is synthetic; no live game
or Windows build was run. Normal admission/capability and event application remain
disabled, so this evidence does not establish the complete anywhere-coop feature.

### 2026-10-20 usage-limit checkpoint verification

Final combined `bbport-dev` build of `bbnet` and all six focused BBSC test
executables passed: party/journal, native join/flag proof, event catalog, targeted
transport, HTTP observer/HMAC, and Matching2 admission dispatcher. The ABI3 save-host,
offline recovery loader and actual recovery-module negative-image checks also passed.
Transcript: ignored `usage-checkpoint-final-checks.log` in this plan's evidence
workspace. Admission activation stays false; the private-attempt lifetime,
leave/retry and shutdown races listed above remain blockers. No package, Windows
build or live new admission/reward test was performed.

### 2026-10-21 continuation: private intent and Matching2 cleanup

The HTTP observer now reports None/Pending/Ready/Expired/Invalid plus a monotonic
private-attempt generation. A nonempty-word type0 create establishes private intent
before an outgoing claim exists, allowing an advertising guest's own HMAC proof.
Expiry preserves private classification. A malformed create with an empty word
and missing/string SummonType remains Invalid; only a valid explicit public or
non-co-op create clears that intent. Completed HTTP handle cleanup still preserves
committed observation. Focused assertions and scoped fix re-review pass.

Matching2 retains a context/incarnation/request/room/cleanup-nonce barrier until
the exact successful internal leave reply. Failed cleanup blocks retry; old replies
cannot clear newer same-room membership. Failed stale-incarnation JoinRoom replies
retire only their exact cancelled request. Stop and rejection share one idempotent
cleanup transition, claiming the original error callback before asynchronous leave
submission. Accepted Resolve returns true only after inserting into a running
dispatcher under its queue lock; this is not proof of native callback consumption.
The fixture includes actual mm.cpp and OnMatchingReply, with a fake authenticated
client backend. Root independently built and ran the HTTP and Matching2 targets in
bbport-dev; both passed. Scoped Matching2 fix re-review is clean. No live server
transport or exhaustive concurrent/TSAN interleavings were tested.

Read-only native mapping located the game-facing request callback at VA0xCBD4A0:
JoinRoom event2 routes to VA0xCBE5C0, which queues result events0x2A/0x29 through
VA0xCBC3F0 and clears callback argument field+0x3C. The no-payload/error branch also
queues0x29. The verified ELF is the same SHA256 recorded above; extraction used
PT_LOAD file offset VA+0x4000. This demonstrates callback side effects, not a ready
bit or a proven pre-world cancellation gate. The consumer and game-facing polling
path remain unmapped at this point; retained successful traces contain no correlated
callback-fire records. The native predicate remains false while this is unresolved.
Native guest preparation/rollback review and the combined module check are still in
progress. These observations do not close first-playable event application, rewards,
Windows native recovery, or the before-packaging feature goal.

The admission continuation subsequently passed root's combined check: bbnet build,
all six focused executables (party/journal, native join, event catalog, targeted
transport, HTTP, Matching2) and the ABI3 save-host/offline loader/negative-image
recovery-module cases. Transcript is the local ignored
`admission-continuation-final-checks.log`. Native/journal scoped review found no
actionable issue. The production-tick guest fixture uses a real temporary selected
save, immutable backup and journal, with HTTP/Matching2 fixture boundaries; it
does not launch the game. It checks own-sign IDs against the selected CharacterKey,
optional guest selection proof, wrong peer/challenge/generation/path/hash/character,
queued callback through Lua observation to playable state, and rejected admission
restoring a prior Complete journal without losing deferred claims.

Unadmitted rollback now lives beside StoreJournal: it checks the exact unearned
Prepared/Visiting record and previous Complete state before reusing the existing
encoding/AtomicWrite path. A changed record is refused rather than rewound.
First-time removal is durable on Linux; Windows removal is explicitly unsupported,
with native managed-admission sections still disabled there. The native Lua
observation contains no Matching2 request identity, so the one-shot's source-side
context/room/nonce checks do not establish exact native callback attribution.
The next source slice is first-playable type0x28 delivery and durable intent Ack;
event/item mutation and native gate activation remain disabled.

Bounded reward-save research found a concrete further seam: player serializer
VA0x0156AA50 calls VA0x014EA560 at VA0x0156AC2B. That helper invokes the virtual
method at vtable+0x10 on `[[0x0553B130]+8]`, whose runtime object also owns carried
inventory. This does not identify the virtual method's goods codec, storage
representation, or save/load direction. Existing persisted flag proof therefore
cannot confirm carried/storage goods or distinguish consumed from never granted.
Do not retire an item claim based on a native grant attempt or a live inventory
count. The legacy boss-reward callers overlap all three required-item sources;
they must share the eventual idempotent delivery path before it is enabled.

Further native request-queue inspection mapped singleton VA0xCBC8E0/global0x56C7238
and append helper VA0xCBC3F0 (vector+0xA8, mutex+0xD0), but not the consumer of its
0x29/0x2A records. Another producer also queues0x29, so that discriminator alone
cannot identify a completed Matching2 join. A source proof of the consumer or a
new correlated copied-profile forced-reject trace is still required.

Selected-save backup naming now includes both the process-local receipt generation
and the SHA-256 verified from the provided bytes. Different valid snapshots with
the same generation can coexist after a process restart, while an exact repeat is
idempotent and existing files remain immutable. The old generation-only files are
preserved. The focused journal check and scoped review pass; no automatic full-save
restore API or Windows execution was tested. The party test links the project's
existing OpenSSL::Crypto target for this verification.

Final usage checkpoint: bbnet, all six focused executables and three runtime bridge
checks PASS, exit0, after first-playable delivery and backup collision changes.
Local transcript: usage-checkpoint-2026-10-21-final-checks.log. The delivery fixture
exercises durable baseline, events after the boundary, journal-write failure/retry,
death/loading and stale-room native exit. Review found local-exit and host-Leave
receiver recreation gaps; saved source closes both guest channel and admission token
on terminal paths. Root rechecked those resets. A later fixture failure was isolated
to its temporary flag-manager mapping; saving/restoring that fixture state fixed
the full run without changing production recovery or weakening its assertion.
No live native cancellation, inventory codec, reward application, Windows execution
or package was verified. Native admission remains false. This is a source checkpoint,
not feature-complete pre-release approval.
