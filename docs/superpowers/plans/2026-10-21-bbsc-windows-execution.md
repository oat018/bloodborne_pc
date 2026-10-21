# BBSC Windows Development Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans. Update the checkboxes and [progress tracker](../../WINDOWS-PROGRESS.md) after each verified checkpoint.

**Goal:** Port the approved BBSC behavior onto the verified Windows upstream, prove save/admission/recovery correctness, and stop before EXE/ZIP packaging.

**Architecture:** Keep Supermedo's static Windows `bbnet` and runtime as the integration base. Adapt the historical BBSC policy, journal, protocol and game hooks selectively, without copying the old dynamic loader or replacing upstream networking. Network callbacks enqueue bounded session-bound work; verified game-thread callbacks own game mutations.

**Tech Stack:** Windows x64, MSYS2 CLANG64, Clang 23.1.3, CMake 4.4.4, Ninja 1.13.2, Python 3.14.8, existing OpenSSL/zlib/protobuf/Win32 dependencies.

**Spec:** [Approved product design](../specs/2026-10-18-bbsc-anywhere-coop-design.md). The [Windows direction plan](2026-10-21-bbsc-windows-only-migration.md) controls scope/platform; this document refines its execution order and evidence gates, without replacing product requirements. [Manifest](../../handoff/WINDOWS-MIGRATION-MANIFEST.md), [test matrix](../../handoff/WINDOWS-TEST-MATRIX.md) and [setup evidence](../../handoff/WINDOWS-SETUP-2026-10-21.md) supply the current facts.

## Global constraints

- Upstream source/build pin: `de2e38af055f09f819946951075c6bb119e62ae6`. Historical BBSC runtime pin: `ea8ac53801e49b5dc79c072971c0e05efcd7651f`. Record a reviewed new pin before changing either comparison base.
- `origin` is `kaoitp/bloodborne_pc`; `upstream` is `Supermedo/bloodborne_pc`. Preserve `pre` history. Checkpoints target `windows` / `origin/windows`; no history rewriting or force pushes.
- Use native CLANG64. Linux fixture results do not count as Windows results. Test only copied profiles; keep saves, credentials, game assets and private logs outside Git.
- Native managed admission stays OFF until G1 passes. Ordinary network support or a passing synthetic callback test cannot qualify it.
- Main-story maps and Hunter's Dream are supported candidates; Chalice/unknown destinations are rejected. Cross-map matching requires the same nonempty password, selected host and join attempt; preserve unrelated same-map rules.
- Share only eligible events earned after first playable entry and before actual departure. Death/loading within the party does not reset eligibility. Never persist host presentation snapshots as guest progress.
- Persist only verified additive boss/story progress and enumerated rewards. Preserve already-earned progress, stats/equipment/loot, conflicting NPC choices and ending choices. Never copy entire flag groups or roll back the whole save.
- Commit origin and a previously unlocked fallback lamp before world mutation. Restore an exact point only with safety evidence; otherwise use that captured fallback. Preserve pending recovery when evidence is insufficient.
- Preserve Kit/Blank menu, normal animations/text, host and guest departure behavior. Unsupported destinations end co-op with an explanation and safe return.
- No game launch during the present planning/preflight checkpoint. Future G1–G3 are user gameplay checkpoints, not waived requirements. Do not launch a game without a later user instruction.
- Stop before `packaging/windows/package.sh`, EXE/ZIP release generation or server deployment. Building development test/game executables is not packaging.
- Use Luna for substantial independent work with disjoint file ownership. Native adapter changes have one owner. Check usage when available; below 10% remaining in the 300-minute window, checkpoint and push `origin/windows`. If usage reporting is unavailable, record that rather than inventing a percentage.

## Review focus

1. Failed/expired identity, proof, backup or journal preparation must prevent every join side effect; a valid retry must still work (W06/W08/G1).
2. Stale callbacks, host changes and exit-before-room-update must not resurrect admission or progress eligibility; death/loading must retain the same admission (W04/W06/W08/G1).
3. Flush/replacement/receipt failure or crash between grant and receipt must retain earned credit without duplicate grants (W05/W09/G2).
4. Slot reuse, user/profile switching and ambiguous saturated NG7 transitions must not consume another identity's journal or return point (W05/W07/W10/G3).
5. Unsafe points, unavailable fallback lamps and unsupported maps must preserve recovery intent and explain the failure; no guessed host lamp or unverified coordinate warp (W10/G3).

## Working files and verification convention

W01 establishes the integration checkout. Paths below are relative to that checkout unless explicitly labeled historical. Proposed **new** test files/targets are specifications, not existing upstream APIs. Existing BBSC names/signatures come from the pinned historical headers; any necessary change must be reflected in all consumers and in the adapter contract before those consumers are implemented.

New BBSC implementation files will live under `gpu/bbnet/bbsc/`: `bbsc_party.cpp`, `bbsc_party_journal.cpp`, `bbsc_persistence.{h,cpp}`, `bloodborne_re.cpp`, `bbsc_memscan.{h,cpp}`, `bbsc_patch_tables.h`, `bbsc_cinematic.{h,cpp}` and `bbsc_messages.inc`, adapted from the historical tree. Public BBSC declarations stay under `gpu/bbnet/include/core/`; BBSC-only wire declarations must have one shared owner, not duplicate vendor definitions. Native code/hooks are introduced only by W07 under W03's contract. Upstream vendor counterparts remain in their existing folders; the historical `network/p2p_sockets.cpp`/`sockets.h` have no same-name upstream counterparts.

Every implementation task follows: add the named regression assertions → run and record the expected failure → implement the narrow change → rerun the focused tests and native build → review → commit. Documentation/source-discovery tasks instead require reproducible source references, unresolved questions and reviewer findings. Do not fabricate a red test for a documentation-only task.

For each command, record checkout SHA, patch/diff state, CLANG64/tool versions, command, exit code, cases/skips and evidence location. A future command below is an acceptance command, not a claimed current result. Use `-UNDEBUG` for assertion-based C++ tests. Keep generated files in the private evidence directory and use a writable per-run TEMP/TMP root.

## W00 — Verified preparation checkpoint

**Files/evidence:** setup report, migration manifest, Windows test matrix.

- [x] Verify pinned upstream build with `bash build.sh` and executable outputs.
- [x] Verify CUSA03173 01.09 metadata, executable fingerprint and `INSTALL_DIR_SAVEDATA=CUSA00207`.
- [x] Restore and hash-check the copied save; user confirms characters and normal basic gameplay; original archive unchanged.
- [x] Audit all 11 historical BBSC test sources and nine upstream test targets; run the safe preflight subset and record compile failures separately.
- [x] Identify missing game-thread/admission/save/UI contracts without treating them as implemented.

**Exit:** reproducible preparation and honest test inventory. The five failed upstream test-target builds are W02 work; this checkpoint does not claim full Windows test parity or BBSC functionality.

## W01 — Establish the integration workspace and history path

**Dependencies:** W00. **Game:** no.

**Files:** create `docs/handoff/WINDOWS-INTEGRATION-BASE.md` in the integration branch; preserve this plan, tracker, AGENTS and handoff entry points.

**Interfaces:** consumes the two pinned trees and manifest. Produces a documented integration branch/tree pin plus a reviewed path-ownership map; W02–W11 use that checkout, not the untouched baseline.

- [x] Keep the app-managed baseline worktree unchanged as the reference. Create a separate app-managed implementation worktree at the pinned upstream; use branch `codex/bbsc-windows-integration`.
- [x] Inventory the upstream-versus-`windows` ancestry/path differences. Record every retained upstream path, BBSC-only addition and minimal runtime bridge; copy documentation/BBSC source only by explicit path selection in later tasks.
- [x] Record a normal, non-rewriting integration route back to `windows`: preserve both histories with a reviewed merge, keep upstream ownership for its runtime/renderer/launcher paths, and retain BBSC/documentation additions. Do not perform a blanket merge/checkout or mark ancestry-only merges as a code migration.
- [x] Before any final merge, require a complete path/diff review and integrated checks. Preserve the current `windows` history and `origin/pre`; never force-update either.
- [x] Verify `git rev-parse HEAD`, `git status --short --branch`, exact submodule pins and the path map; review and commit the workspace record.

**Exit:** one known implementation checkout, explicit ownership and a history-preserving integration route. Until this is recorded, do not port runtime files in the historical checkout.

## W02 — Make no-game Windows verification repeatable

**Dependencies:** W01. **Game:** no.

**Files:** modify `tests/test_gpu_runtime_stubs.cpp`, `tests/test_upscaler_support.cpp`, `gpu/CMakeLists.txt`, `.gitignore` (ignore private runner evidence); create `scripts/test_windows_no_game.ps1`; add a fixture runner test in `tests/test_windows_no_game_runner.py` if needed for selection/failure propagation.

**Interfaces:** consumes existing nine upstream target names and Win32 compatibility definitions. Produces a runner that accepts explicit test groups (`baseline`, `bbsc`, `all`), records each command's exit and returns nonzero on failure. The runner must not invoke `run.py`, a launcher, or `bb-probe`; unavailable future BBSC targets are a reported missing group, not a pass.

- [x] Reproduce the recorded RED builds: four renderer test targets fail on `sigjmp_buf`; `upscaler-support-test` fails on `setenv`/`unsetenv`. Add assertions for writable temporary paths and environment restoration.
- [x] Adapt the stub declaration to the actual Windows fault-recovery type from `src/compat_win.h`; retain the POSIX branch. Replace test-only environment/temp assumptions using Windows-supported calls and actual temp paths. Resolve further link gaps against the real symbols, without weakening runtime ABI checks or assertions.
- [x] Ensure every assertion-based target has `-UNDEBUG`, including `motion-shader-test`. Classify CPU-only, SPIR-V generation and headless Vulkan-device tests separately; the source comment saying no device is required is inaccurate for three targets.
- [x] Build all nine targets. Run CPU targets directly; run `motion-shader-test` with an isolated output directory and validate generated `.spv` files with installed `spirv-val`; run the three headless Vulkan tests without game windows.
- [x] Add the runner with explicit allowlisted commands, isolated TEMP/TMP and durable per-test summaries. Test that a failing child, missing target and skipped prerequisite remain visible and cannot yield an all-pass summary.
- [x] Run the six audited upstream Python modules through the baseline group. Retain the eight gameserver-ID tests and direct historical catalog run as separately pinned reference evidence; the integration BBSC group remains unavailable until W04/W06 introduce its actual sources/targets. Recheck the native build, review and commit.

**W02 reopened (2026-10-21):** The original PowerShell5 native invocation did not wait for GUI-subsystem executables and could reuse a stale exit code. A delayed GUI exit7 regression reproduced false PASS; the corrected invocation passes five runner regressions and waits for real exit. Fresh combined execution exposes access violations in scene-resolution-test, taa-shader-test and camera-motion-test. Their prior PASS claims are invalidated; diagnose and rerun baseline before W05/W06 acceptance. Do not skip these required checks.

**W02 Vulkan-Hpp test ABI correction (2026-10-21):** Installed Hpp has NDEBUG-sensitive DispatchLoaderBase fields. Release bbgpu defines NDEBUG while assertion-enabled test consumers use -UNDEBUG, causing incompatible dispatcher definitions. A private two-TU split reproduces invalid dispatcher state; an aligned control passes. Permit a test-only preinclude shim (tests/test_gpu_vulkan_abi.h) for Hpp-consuming fixtures, with gpu/CMakeLists.txt binding/guarding the library's actual explicit NDEBUG policy while parsing Hpp, then restoring active fixture assertions. The bbgpu target unconditionally defines NDEBUG, so do not infer its state from configuration names or change production flags. Include all relevant test TUs; do not change production assertion configuration or simply disable test assertions. Verify matching Hpp layout plus effective fixture assertions, rebuild and run all three real headless targets, then the required combined suite/native build. No result is accepted from the minimal repro alone. This refines test compilation, not renderer behavior or native gameplay scope.

**W02 scene fixture setup correction (2026-10-21):** After ABI alignment, TAA/camera exit0; scene-resolution-test reaches the valid SlotVector capacity assertion. The fixture default-constructs its image/view containers but inserts without Create/ResizeDestructive, unlike production usage. Permit bounded constructor capacities in tests/test_scene_resolution.cpp only, with capacities justified by fixture allocations. Preserve the SlotVector assertion and production implementation; retain the failing pre-fix fixture output, then run the unchanged rendering/readback checks. Unsupported required format coverage must emit an explicit SKIP marker so the existing runner cannot report it as PASS.

**W02 environment ruling (2026-10-21):** The managed worktree's long absolute path can exceed Ninja's Windows path limit. A short junction alias to the same checkout is permitted; record its resolved target and do not create a second source copy. Run upstream build preparation/patches as required before treating dependency compile failures as test regressions. Acceptance and native/game boundaries are unchanged.

**Acceptance:** `powershell -NoProfile -File scripts/test_windows_no_game.ps1 -Group baseline` exits 0 only when all selected baseline checks pass, with skips counted; no game/process/profile is opened. W04 adds the BBSC group after its real targets exist.

## W03 — Resolve static adapter and caller contracts

**Dependencies:** W01; W02 may run independently with disjoint ownership. **Game:** no for source mapping; native confirmation remains G1–G3.

**Files:** create `docs/handoff/WINDOWS-ADAPTER-CONTRACT.md`; inspect upstream `src/probe.c`, `src/runtime_file.c`, `src/runtime_savedata.c`, `src/runtime_memory.c`, `gpu/bbgpu.h`, `gpu/shim/bbgpu.cpp`, `gpu/bbnet/bbnet_glue.cpp`, `thd/`, and vendor Matching2/signaling/HTTP; compare historical `bbnet.h`, `runtime_net.c`, `bloodborne_re.cpp`, guest hooks and persistence sources.

**Interfaces:** consumes verified lifecycle `bbnet_configure` → image setup via `bbnet_set_image` → `bbnet_initialize`, plus resolver registration `bbnet_register`. Produces an exact contract document: real symbol, declaration/file, caller, thread, ownership/lifetime, failure/cancellation result, teardown and test seam for each boundary. It becomes the signature authority for W05–W10.

- [x] Trace image allocation, relocation, memory protection, callback lifetime, replacement and shutdown. Map supported guest-hook infrastructure and game tick; `thd::DispatchAfter` is a network worker, not a game-thread scheduler.
- [x] Trace both registered Matching2 backends (THD and vendor fallback), request, callback, polling/signaling, and any claimed native result events through their concrete producer/consumer, then Lua join, multiplayer initialization, placement and reload. Distinguish HTTP punctuation bytes `0x29/0x2A` from historical custom signaling capability type `0x29`; neither proves a native result-event route. If no such producer/consumer exists in pinned source, record that absence and the required G1 diagnostic. Mark every route that can mutate network/session or world state and the earliest cancellable consumer. No held callback or null Lua dispatch qualifies alone.
- [x] Trace selected save/profile/slot and the actual writer→close path. Map candidate serializer/item-grant hooks and completion receipts. Do not equate `_commit`/close or memory mutation with a durable receipt.
- [x] Map Kit/guest Blank/host Blank/menu/text/animation/overlay ownership and startup recovery before online initialization.
- [x] Record proposed adapter declarations only after concrete owners are identified. For unresolved native facts, define the exact diagnostic and expected G1/G2/G3 observation; keep those contracts blocked and do not invent offsets.
- [x] Review thread/ABI/cancellation and save contracts; commit the map. Work on portable units may proceed while native validation is pending, but no enabled hook may depend on an unresolved contract.

**W03 source correction (2026-10-21):** Independent source review found the earlier asserted native result IDs unsupported: pinned HTTP uses those bytes as URL punctuation, and historical BBSC uses only 0x29 as a custom signaling capability type. The revised trace requirement above preserves discovery of actual native producers/consumers rather than inventing event IDs. This affects W03 and downstream W06/W08 diagnostics; G1 remains mandatory. Also map THD's existing SetImage/SetSignalingGate/HostExtra guest-memory access and distinguish it from the stricter BBSC game-thread requirement.

**Exit:** every implementation dependency has either a source-backed contract or a named blocked native observation, with no guessed API. Complete source mapping is distinct from native validation.

## W04 — Port portable party/protocol/catalog policy

**Dependencies:** W01/W02; W03's native work is not required for pure policy. **Game:** no.

**Files:** add/adapt historical `gpu/bbnet/include/core/bbsc_party.h`, `bbsc_party_events.h`, `gpu/bbnet/bbsc/bbsc_party.cpp` (new implementation home), only the BBSC protocol declarations needed from `np_signaling_bbsc.h`; adapt `tests/test_bbsc_party_events.cpp`, protocol/state portions of `test_bbsc_party.cpp`; create `tests/test_bbsc_party_policy.cpp` to avoid dragging POSIX journal fixtures into policy tests; register selected targets in `gpu/CMakeLists.txt`.

**Interfaces:** preserve `Core::Bbsc::Party::{Message, CapabilityMessage, Peer, CharacterKey, Admission, Delivery}`, `EncodeMessage`/`DecodeMessage`, `EncodeCapability`/`DecodeCapability`, `CanCrossMapMatch`, `IsSupportedStoryMap`, and the 22-event/3-item candidate catalogs from historical headers. Separate declarations from unrelated vendor-state dependencies as needed, updating both protocol consumers together.

- [x] Add failing tests for malformed/oversized/duplicate packet data, wrong peer/party/admission, ordering/replay, expiry, closed admission resurrection, password mismatch/empty password and unsupported maps.
- [x] Port policy and wire encoding with those existing types; do not wire native hooks or import Linux vendor directories. Preserve sender/receiver declaration compatibility.
- [x] Retain catalog assertions: 22 boss rows, three unique-item candidates, deferred endgame mutations, and no automatic Cord consumption. Candidate fixture success does not approve an in-game grant mapping.
- [x] Register `bbsc-party-policy-test` and `bbsc-party-events-test` with assertions enabled. Add them to the runner's BBSC group; run both, review and commit.

**Acceptance:** both targets build and exit 0 under CLANG64; fixture evidence is labeled portable Windows policy, not live networking or native progress.

## W05 — Port Windows journal, identity and durable save primitives

**Dependencies:** W03/W04. **Game:** no for filesystem/process fixtures; native writer/identity confirmation later.

**Files:** adapt historical `bbsc_party_journal.cpp` into `gpu/bbnet/bbsc/`, upstream `src/runtime_file.c`, `src/runtime_savedata.c` only if required, and a reviewed pure-C receipt declaration under `src/runtime_file_receipt.h` / `src/runtime.h`; adapt `tests/test_bbsc_party_journal.cpp`, `tests/test_bbnet_save_host.c`; create `tests/test_bbsc_storage_windows.cpp` and register it. Extend `scripts/test_windows_no_game.ps1` and its runner regression tests so required native fixture SKIPs remain visible and cannot be reported as full PASS.

**Interfaces:** retain `CharacterLease`, `StoreJournal`, `LoadJournal`, `StoreSelectedSaveBackup`, `ReturnSaveTracker` and `ManagedCycleEpoch` contracts from the historical header where portable. The static selected-save receipt callback's exact declaration and publication point must match W03's reviewed document.

- [x] Add RED filesystem tests in fresh temporary profiles: two-process lease exclusion, path/reparse rejection, immutable generation+content backups, cross-profile/slot/character/epoch mismatch, partial write, flush failure, replacement failure and restart with pending intent.
- [x] Implement Win32 lock/flush/replacement/error paths with explicit achieved durability. Publish a receipt only after the required commit sequence succeeds; preserve pending work if metadata durability or selected identity cannot be established.
- [x] Port crash fixtures using child Windows processes with controlled checkpoints; never terminate a real game. Reopen files in a new process and prove incomplete work remains pending and complete work is idempotent.
- [x] Cover slot deletion/reuse and native 6→7 / saturated 7→7 ambiguity using synthetic identities. Server-issued ID tests do not substitute for client save identity.
- [x] Register/run `bbsc-storage-windows-test`, `bbsc-party-journal-test` (separate from the policy target) and `bbnet-save-host-test`, plus W04 tests and native build. Review and commit.

**W05 RED evidence classification (2026-10-21):** The first step's recorded preimplementation RED is the fresh fixture link failure on absent Win32 primitive definitions, followed by Windows fixture GREEN. This is implementation-absence/link RED, not a retained pre-fix behavioral regression. Runtime review regressions lack a retained behavioral RED artifact (NOT RUN); the later preserved pre-longpath comparison passes both versions (NOT REPRODUCED). Do not relabel either as behavioral RED or claim the extended-path change caused the original runner failure. Completion here requires all listed functional/process/error/path/identity checks to pass with actual exits; native identity/serializer/grant gates remain separate and unpassed.

**W05 source-ownership correction (2026-10-21):** Historical bbsc_persistence.{h,cpp} contain native persistent-session patches and default-on feature switches, not journal/filesystem primitives. Do not transplant them during W05; their necessary native responsibilities belong to W07 under verified hook contracts. W05 retains filesystem algorithms in bbsc_party_journal.cpp and separates runtime-file observations from future selected-character/serializer proof. The new receipt declaration must be reviewed and recorded in W03 before implementation; it is not the old dynamic BbNetHost ABI.

**W05 filesystem qualification (2026-10-21):** Control files are private BBSC state, with cooperating clients serialized by CharacterLease. Hold the complete validated ancestor chain, reject static reparse components and the tested pre-replacement leaf swap, and fail closed on operation faults. This is not a guarantee against an uncoordinated hostile process with the same OS-account write access swapping a leaf after the final check; keep that narrow path-based replacement limit explicit. It does not waive path/reparse tests or authorize native pending retirement. Process interruption/reopen and checked API flush/replace evidence are distinct from an unproven general power-loss/parent-directory durability guarantee.
**Exit:** Windows-native temporary-file/process evidence for storage semantics, independent of game saves. Actual game-selected slot and serializer receipts remain G2/G3 checks.

## W06 — Integrate static networking and session controls

**Dependencies:** W03/W04/W05. **Game:** no for mocks/loopback; private-server connectivity is a separate prerequisite.

**Files:** upstream `gpu/bbnet/bbnet_glue.cpp`, `gpu/CMakeLists.txt`. With explicit vendor root `gpu/bbnet/vendor/shadps4/core/libraries/`, inspect/modify `network/http.cpp`, `np/np_matching2/{np_matching2_mm.*,np_matching2_internal.cpp,np_matching2_signaling.cpp}`, `np/np_signaling/{np_signaling*,np_signaling_helpers*,np_signaling_state*}`, and the actual upstream P2P files `net/{net_p2p.cpp,net_p2p.h,net_p2p_ioctl.cpp,net_p2p_ioctl.h}` plus codec/transport under `gpu/bbnet/vendor/shadps4/core/net/` as required by the reviewed seam. There are no upstream `network/p2p_sockets.cpp` or `network/sockets.h`; compare those historical responsibilities to the actual P2P implementation rather than creating a competing legacy socket layer. Adapt `tests/test_bbsc_http_poll.cpp`, `tests/test_bbsc_matching2_admission.cpp`, `tests/test_bbsc_party_transport.cpp`.

**Interfaces:** upstream static lifecycle and W03 adapter declarations; W04 protocol and party identity. Apply only reviewed BBSC hunks to actual counterpart files; do not replace transport/vendor ownership.

**W06 route correction (2026-10-21, W03/preflight):** Include the actual selected THD route gpu/bbnet/thd/{thd_matching2.cpp,thd_session.cpp,thd_np.cpp} as needed, alongside vendor fallback. BbThd::Enabled() selects mutually exclusive registration in bbnet_glue.cpp; vendor-only fixtures cannot qualify the selected THD route. Add focused backend-selection and THD request/session fixtures with policy rejection before network submission, permitted retry, correlated late cleanup, peer/departure and teardown checks. Preserve shared HTTP registration. A bounded generation-tagged queue may expose a fixture consumer, but no game-thread consumer or native admission qualification is invented; those remain W07/W08/G1. This corrects file/route coverage without changing approved behavior or enabling managed admission.

**W06 execution split / source-contract limit (2026-10-21):** W06-A maps/adapts the HTTP attempt observer, proof/session policy and bounded immutable generation-tagged queue (necessary pure helpers under gpu/bbnet/bbsc/ permitted); W06-B integrates actual THD/vendor network/session and peer/transport consumers with focused fixtures; W06-C retains the pinned service reference/credits and loopback ID tests independently. One owner controls all shared native adapter/CMake/runner changes; reference files have a separate owner. The reviewed W03 map proves no upstream BBSC game-thread tick or Kit/Blank game-input consumer exists. Therefore W06 prepares network policy/producers and a fixture-only queue consumer; binding a real native game-thread/input consumer belongs to W07/W08 and G1, with managed activation OFF. This transfers the unresolved native binding check to its actual owner, preserves the approved Kit/Blank requirement and G1 tests, and does not invent a callback-thread consumer or waive native qualification.

Retain the historical `scripts/bbsc_gameserver/` sources/credits and `tests/test_bbsc_character_ids.py` as explicit protocol/SQLite reference fixtures in the integration branch, with loopback-only tests. This does not deploy a service or change private server state.

- [ ] Write RED tests for private-attempt HMAC proof, selected peer/connection, expiration/retry, stale host/password changes, correlated cleanup and shutdown races, relay framing and both departures.
- [ ] Bind network callbacks to a bounded immutable generation-tagged queue; test its fixture consumer, cancellation and teardown. No network-thread live memory access. Native game-thread consumer binding remains W07/W08/G1 under the source-contract limit above.
- [ ] Integrate matchmaking/password/connect-first payload and network-facing Kit/Blank policy using actual network/session consumers while native managed admission stays disabled. Native Kit/Blank input/action binding remains W07/W08/G1.
- [ ] Run all three focused targets, W04/W05 tests and `bash build.sh`; compare unrelated upstream network behavior before/after. Review and commit.

**Exit:** focused Windows static-module fixtures pass. Live join, placement and private-service compatibility are not claimed until G1.

## W07 — Port native hook, scanner and selected-character adapters

**Dependencies:** W03/W05/W06. **Game:** no for scanner/ABI fixtures and disabled-hook implementation/diagnostics; native hook qualification and managed feature activation require the relevant G1/G2/G3 evidence.

**Files:** adapt historical `bloodborne_re.cpp`, `include/core/bloodborne_re.h`, `bbsc_memscan.{h,cpp}`, `bbsc_patch_tables.h`, `bbsc_cinematic.*`, required guest-code-hook shims; replace Linux-only scanner/memory fixtures with `tests/test_bbsc_memory_windows.cpp` and `tests/test_bbsc_memscan_windows.cpp`; split portable join assertions from `test_bbsc_join.cpp` into focused Windows fixtures.

**Native binding ownership:** W06 supplies only network/session producers and a fixture consumer. Resolve and implement disabled native game-thread queue and Kit/Blank input/action bindings here and in W08 using exact mapped callers; retain G1 as their qualification/activation prerequisite. Do not treat W06 fixture dispatch as a native consumer.

**Interfaces:** exact image/ABI/tick/queue and selected-character contracts approved in W03. Retain actual historical coordinator/tick names only where the mapped Windows callers support them.

- [ ] Add RED scanner fixtures for inaccessible/guard pages, partial reads, changing mappings, wrong image bytes, instruction boundaries, trampoline register/stack preservation and teardown/image replacement.
- [ ] Implement Windows query/read/protection/cache-flush paths using the pinned runtime; reject unknown signatures safely. Do not retain procfs, DMA-BUF or Linux guest-stack assumptions.
- [ ] Add disabled native diagnostics for callback thread/tick, image identity, selected save/slot/character/NG epoch and serialization direction; redact private data and do not mutate a save to collect identity.
- [ ] Run new Windows memory/scanner targets, native join fixtures and full selected suite/build. Review and commit; leave activation disabled pending G1.

**Exit:** compile/fixture-qualified adapter with no guessed native contract. Live hook qualification is a separate gate.

## W08 — Admission and first-playable coordination

**Dependencies:** W05/W06/W07. **Game:** no for fixtures; G1 is mandatory before managed activation.

**Files:** mapped native join/Matching2 consumers, `bloodborne_re.cpp`, party delivery/journal, adapted join/admission/party tests.

**Interfaces:** `TickBbscAdmissionCoordinator`, `TickBbscPartyEventDelivery`, party identity, durable baseline and per-guest challenge/ack, after confirming their mapped callers in W03.

- [ ] Add RED tests for every entry route with a side-effect ledger: denied preparation yields zero Lua join/multiplayer init/placement/reload operations; permitted retry succeeds; callback enqueue alone is insufficient.
- [ ] Wire per-guest identity, immutable backup and durable intent before approval. Fail closed when any step lacks proof.
- [ ] Test a single first-playable boundary, durable baseline acknowledgement before cover release, events during transfer, death/loading continuity, late guests, departure-before-room-update, stale callbacks and bounded timeout/exit.
- [ ] Run focused admission/join/party tests and native build; review/commit diagnostics-ready code with activation OFF.

### G1 — Native admission/session gate (user gameplay)

- [ ] Use separate copied profiles/accounts. Establish private network/server reachability without redeploy/reset.
- [ ] Verify native signatures/thread/ABI on the supported image, forced rejection before **all** world mutations, then a permitted retry. Include alternate room/signaling routes.
- [ ] Verify same-password cross-map join into a story map and Dream, actual host placement/single load, first playable boundary, death/loading continuity, host/guest Blank/Kit menu and normal animations/text.
- [ ] Record matched positive/negative traces and user observations at the tested SHA; fix failures and rerun affected cases.

**Gate:** only after reviewed G1 evidence may managed admission be enabled for controlled testing. If the user is unavailable, mark WAIT_GAME and continue only independent offline work; never mark G1 passed from fixtures.

## W09 — Additive progress and exactly-once necessary items

**Dependencies:** W05/W07/W08; offline research can precede G1, native application cannot be qualified before G1/G2. **Game:** offline fixtures first, then G2.

**Files:** native serializer/grant callers in `bloodborne_re.cpp`, party event/catalog/journal sources and join/event/journal fixtures; create `docs/handoff/WINDOWS-REWARD-EVIDENCE.md`.

**Interfaces:** selected character/cycle and fresh receipt contract from W03/W05; eligibility from W08; validated persisted inventory codec and real grant/save completion path, discovered here before enabling any application.

- [ ] Resolve the historical serializer candidates and actual carried/storage format in this pinned executable. Add synthetic decoder tests for invalid/truncated/wrong-character data; record unresolved native observations rather than inventing offsets.
- [ ] Audit catalog rows against actual event/parameter evidence. Required item candidates: goods `4000` from event `12411800`, `4020` from `13501850`, Cord `4323` from `12601800`. Validate each before enabling it; never consume Cord or mirror an ending choice automatically.
- [ ] Add RED tests for pre-entry/post-exit events, host historical snapshots, already-ahead guest state, NPC conflict, present/stored/consumed reward, inventory full, duplicate retry, crash before/after grant, stale receipt and wrong slot/epoch.
- [ ] Persist intent before mutation; restore the guest's own world before additive application. Route legacy reward callers through one ledger. Retire credit only on matching fresh serialization and durable selected-save receipt; keep unconfirmed rewards pending.
- [ ] Run all focused tests/build; review and commit with unqualified applications disabled.

### G2 — Native persistence/reward gate (user gameplay)

- [ ] On copied profiles, confirm selected slot/character, controlled eligible boss/story and item observations, exact carried/storage/consumed behavior and no host-old-progress import.
- [ ] Save, exit and restart to prove durable effect; interrupt the controlled test around grant/receipt/ledger boundaries and confirm no duplicate or lost credit.
- [ ] Record enabled catalog rows and deferred conflicts individually. Fixture-only rows remain unqualified; no blanket catalog PASS.

## W10 — Safe return, recovery and unsupported transitions

**Dependencies:** W05/W07/W08/W09; offline fixture work can proceed before G2, live qualification requires G1/G2. **Game:** offline fixtures, then G3.

**Files:** native own-world/return/cycle and Blank/session-end code in `bloodborne_re.cpp`, journal/persistence, UI adapter and return/journal/join fixtures.

**Interfaces:** `CharacterKey`, journal phase and return fence/receipt services from W05; close-of-eligibility from W08; reward pending/confirmed state from W09.

- [ ] Add RED tests for origin+unlocked-fallback durable capture before mutation; preserve original origin through deaths and follow loads.
- [ ] Funnel guest Blank, host Blank, confirmed disconnect, quit/restart and unsupported destination into one lifecycle: close eligibility → restore own world → safe point/fallback → eligible rewards → durable completion.
- [ ] Test unsafe/missing coordinates, invalid lamp, failed destination confirmation, failed save, expired callback, process interruption, slot reuse, different user/profile and native 6→7/managed saturated 7→7. Pending records survive ambiguous evidence.
- [ ] Integrate native return/retry and explanatory UI using W03 contracts; retain normal Blank text/animation and prevent host lamps from becoming fallback choices.
- [ ] Run storage, identity, session, rewards and return suites plus native build; review and commit.

### G3 — Native return/recovery gate (user gameplay)

- [ ] Validate exact safe return and captured fallback, guest/host Blank, disconnect and process restart with copied profiles; verify saved destination after reload.
- [ ] Validate unsupported transition explanation and safe departure without following the host into Chalice/unknown maps.
- [ ] Validate slot/character changes and available NG cases cannot consume foreign recovery. A missing higher-NG profile is an explicit unpassed coverage gap, not a simulated native success.

## W11 — Final review and before-packaging checkpoint

**Dependencies:** W01–W10 and G1–G3 for readiness. Offline review can occur earlier, but cannot close unmet gates. **Game:** no additional launch if all gate evidence is current at the final code SHA; affected behavioral changes invalidate their gate evidence.

**Files:** integrated source/tests, `docs/BBSC.md`, tracker, manifest/adapter/reward evidence, upstream licensing/credits and release-path notes.

- [ ] Run `bash build.sh` and `scripts/test_windows_no_game.ps1 -Group all` in the integrated Windows checkout. Record actual test count, skips, failures, dependency versions and source pin; no imported Linux PASS labels.
- [ ] Review complete diff against upstream and the requirement coverage table. Reject unrelated renderer/input changes, duplicate vendor/module trees, disabled-only feature claims and unqualified reward rows.
- [ ] Compare launcher/data/config/DLL paths with upstream packaging references by inspection only; preserve credits/licenses. Do not run packaging.
- [ ] Confirm G1–G3 evidence covers the final behavior and that original archives remain unchanged. Unmet mandatory gates mean NOT READY; do not relabel them optional to finish.
- [ ] Complete the reviewed history-preserving integration onto `windows`, rerun affected checks after conflict resolution, commit and push `origin/windows` normally. Verify local HEAD equals the remote ref after push.
- [ ] Update tracker with tested commit, outcomes, limitations and next separately authorized packaging step. Stop and wait.

## Dependency and progress rules

The source-contract chain is `W00 → W01 → W03`. The independent test/policy chain is `W01 → W02 → W04`. Both converge at `W05`, then `W06 → W07 → W08 → G1`. Qualification continues through `W09 → G2 → W10 → G3 → W11`; the task dependencies permit offline W09/W10 work before the relevant gameplay gate, while qualification waits for it.

The detailed dependency on each task is authoritative: portable policy W04 does not require native hooks, and offline W09/W10 work can run while a game gate waits if it uses only completed contracts. Gates are never inferred from task completion. Keep one native-adapter owner; use independent agents for policy/tests/catalog source research with disjoint files.

Update [WINDOWS-PROGRESS.md](../../WINDOWS-PROGRESS.md) with statuses READY, IN_PROGRESS, WAIT_DEP, WAIT_GAME, VERIFIED or BLOCKED and evidence. A checkbox changes to `[x]` only when its stated acceptance result is verified. Missing tests are NOT RUN, skips are SKIP, compile errors are BUILD_FAIL. Do not use an estimated feature percentage.

The user authorized execution on 2026-10-21. W01 is verified; continue from the tracker. The no-game instruction remains active, so G1–G3 wait for later explicit authorization. Existing upstream gameplay success is W00 evidence only.

## Product requirement coverage

| Approved requirement | Implementation and qualifying gate |
|---|---|
| Story maps/Dream; exclude Chalice/unknown maps | W04/W06/W10, G1/G3 |
| Same nonempty password for cross-map; preserve same-map rules | W04/W06/W08, G1 |
| Guest may visit a map they have not unlocked | W06/W08, G1 |
| Durable origin/heading and previously unlocked fallback | W05/W10, G3 |
| Host world is temporary; only jointly earned progress persists | W08/W09, G1/G2 |
| Additive boss/story progress; no rollback or NPC-choice overwrite | W09, G2 |
| Safe exact-point return or captured fallback | W10, G3 |
| Explicit necessary items; no duplicate/consumed regrant | W09, G2 |
| First-playable eligibility; death/loading continuity; actual departure closes it | W08/W10, G1/G3 |
| Unsupported destination explains exit and returns safely | W10, G3 |
| Existing BBSC service first; compatible private service if necessary | W06/G1, preserving protocol; no setup-time redeploy |
| Preserve agreed Kit/Blank/menu/text/animation behavior | W03/W06/W08/W10, G1/G3 |
