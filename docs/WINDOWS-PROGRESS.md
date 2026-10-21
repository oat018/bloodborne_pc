# BBSC Windows progress

Updated: 2026-10-21 (Asia/Bangkok). **Current checkpoint: Resumed by user: W01–W04 VERIFIED in their stated scope; W05 IN_PROGRESS.** No game launch is authorized.

## Read this first

- [Detailed execution plan](superpowers/plans/2026-10-21-bbsc-windows-execution.md)
- [Platform/scope direction](superpowers/plans/2026-10-21-bbsc-windows-only-migration.md)
- [Migration manifest](handoff/WINDOWS-MIGRATION-MANIFEST.md), [test matrix](handoff/WINDOWS-TEST-MATRIX.md), [machine/game/save evidence](handoff/WINDOWS-SETUP-2026-10-21.md)

`VERIFIED` means the stated scope has evidence. `READY` means work may start. `WAIT_DEP` means an earlier task's deliverable is missing. `WAIT_GAME` means user gameplay evidence is required. `IN_PROGRESS` and `BLOCKED` require an explicit reason. Checkboxes mirror the detailed plan; do not report a feature percentage from completed setup tasks.

## Work tracker

| ID | Work / expected deliverable | Status | Needs game? | Evidence / next action |
|---|---|---|---|---|
| W00 | Native baseline, copied-profile preparation, source/test inventory | VERIFIED | Already tested by user | Native build exit 0; characters and basic gameplay user-confirmed; no-game results below. Full test parity and BBSC are outside this checkbox. |
| W01 | Separate integration branch/worktree and reviewed file/history map | VERIFIED | No | Managed `codex/bbsc-windows-integration` at upstream pin, reviewed [ownership/history map](handoff/WINDOWS-INTEGRATION-BASE.md). Explicit documentation paths copied; runtime unchanged. |
| W02 | Windows no-game harness and reproducible runner | VERIFIED | No | Nine targets BUILD/RUN PASS, four SPIR-V validators PASS; baseline exit 0, Python 51 PASS/1 SKIP; runner regression 3 PASS. Detailed evidence below. |
| W03 | Exact static adapter/caller/thread/lifetime contracts | VERIFIED (source map only) | No for source map | [Reviewed contract map](handoff/WINDOWS-ADAPTER-CONTRACT.md); actual native world consumer/identity/grant/UI contracts remain blocked for G1–G3. No guessed adapter APIs approved. |
| W04 | Portable party/protocol/catalog and focused targets | VERIFIED | No | Portable policy/protocol/catalog targets pass; reviewed unknown-event guards. No live transport/native qualification. |
| W05 | Windows locking/journal/backups/receipts and identity fixtures | IN_PROGRESS | No for fixtures | Next: fresh temporary-profile filesystem RED tests; use W03 source owners and preserve unresolved native receipt/identity gates. Not started at usage stop. |
| W06 | Static HTTP/Matching2/signaling/transport integration | WAIT_DEP W03/W04/W05 | No for mocks | Selective BBSC hunks; no copied Linux vendor tree; activation stays OFF. |
| W07 | Native ABI/tick/scanner/selected-character adapter | WAIT_DEP W03/W05/W06 | Fixtures no; live later | No assumed native offsets or worker-as-game-thread substitution. |
| W08 | Admission/first-playable coordination and rejection fixtures | WAIT_DEP W05/W06/W07 | Fixtures no; G1 yes | Prove all entry routes; callback enqueue is not cancellation. |
| W09 | Eligible additive progress and exactly-once rewards | WAIT_DEP W05/W07/W08 | Fixtures no; G2 yes | Resolve persisted inventory, native grant/receipt and each catalog row; keep unqualified application disabled. |
| W10 | Safe return, offline recovery, unsupported-area handling | WAIT_DEP W05/W07/W08/W09 | Fixtures no; G3 yes | Preserve pending recovery and captured fallback; never consume a foreign identity's record. |
| W11 | Final integrated verification/review and checkpoint | WAIT_DEP W01–W10/G1–G3 | No new launch if evidence remains current | Normal history-preserving integration/push to `origin/windows`; stop before packaging. |

- [x] W00 preparation and evidence inventory
- [x] W01 integration workspace/history map
- [x] W02 Windows test harness
- [x] W03 adapter contract map
- [x] W04 portable policy
- [ ] W05 durable storage/identity
- [ ] W06 network/session integration
- [ ] W07 native adapters
- [ ] W08 admission/first-playable
- [ ] W09 shared progress/rewards
- [ ] W10 return/recovery
- [ ] W11 reviewed before-packaging checkpoint

## Mandatory future game gates

| Gate | Status | Required observations | What cannot substitute |
|---|---|---|---|
| G1 — native admission/session | WAIT_GAME after W08 | Forced rejection before world mutation plus permitted retry; verified hooks/thread; story/Dream placement, first-playable continuity, Kit/Blank/menu/animations | Policy fixtures, held callbacks, null Lua dispatch or baseline single-player gameplay |
| G2 — persistent progress/items | WAIT_GAME after G1/W09 | Actual selected slot/serializer/inventory; eligible event/grant, save/restart, interruption/replay without duplicates | Live inventory counts, catalog constants or synthetic receipt success |
| G3 — return/recovery | WAIT_GAME after G1/G2/W10 | Exact point/fallback, both Blank paths, disconnect/process restart, unsupported maps, identity/NG isolation | Temporary-file journals alone or a generic home warp |

- [ ] G1 reviewed at tested integrated SHA
- [ ] G2 reviewed at tested integrated SHA
- [ ] G3 reviewed at tested integrated SHA

These gates are planned, not requests to open the game now. Offline development can continue where dependencies allow. Native managed admission remains OFF; release readiness cannot pass with an unmet mandatory gate. Higher-NG profiles and private-service reachability remain explicit prerequisites, not assumed available.

## Current no-game test evidence

Source/build pin for upstream checks: `de2e38af055f09f819946951075c6bb119e62ae6`. Historical BBSC source: `ea8ac53801e49b5dc79c072971c0e05efcd7651f` (unchanged runtime in the documentation branch). CLANG64 versions and machine are in the setup report.

| Group | Result | Meaning / limitation |
|---|---|---|
| Upstream Python `test_patches`, `test_prepare`, `test_link_libc`, `test_fov`, `test_input_defaults`, `test_trophies` | PASS: 51; SKIP: 1; total 52 | Temporary-fixture tests; skipped real-executable patch fingerprint case expects `out/eboot.elf` in the upstream checkout. The real game fingerprint was checked separately during setup. |
| Upstream `command-buffer-test`, `motion-history-test`, `ui-composition-test`, `prompt-icons-test` | Four BUILD/RUN PASS | Native standalone CPU checks, assertions enabled; no game or save access. |
| Upstream `motion-shader-test`, `scene-resolution-test`, `taa-shader-test`, `camera-motion-test` | Four BUILD_FAIL; run NOT RUN | Shared test stub uses undefined `sigjmp_buf` on Windows. W02 owns the compatibility fix and any subsequent link/runtime gaps. |
| Upstream `upscaler-support-test` | BUILD_FAIL; run NOT RUN | Undeclared `setenv`/`unsetenv`; hard-coded `/tmp` also needs Windows fixture adaptation. |
| Historical `test_bbsc_character_ids.py` | Eight PASS | Gameserver SQLite/loopback allocation tests; not local-save identity, slots or NG evidence. No private/public server contacted. |
| Historical `test_bbsc_party_events.cpp` + `bbsc_party.cpp` | BUILD/RUN PASS | Direct CLANG64 fixture checks 22 candidate events and three item policies; no CMake migration or native grant qualification. |
| Other historical BBSC native/module fixtures | NOT RUN | OS/source/target adaptation classified in the test matrix; no historical Linux PASS copied here. |

The first Python attempt failed because the sandbox TEMP root denied access; rerunning the unchanged tests with private project TEMP/TMP passed. The gameserver HTTP fixture initially hit sandbox WinError 10013; the same suite passed with approved native loopback execution. These are environment observations, distinct from the five unresolved upstream test compile failures.

Private logs: `.superpowers/windows-offline-python-tests.log` (initial sandbox failure), `windows-offline-python-localtemp.log` (passing rerun), `windows-offline-cpp-build.log`, `windows-offline-remaining-build.log`, `windows-bbsc-character-id-tests.log` (initial socket denial) and `windows-bbsc-character-id-tests-native.log` (passing rerun). They remain ignored and may contain machine paths.

## Updating progress

After each task: record implementation commit, command/exit/case/skip summary, reviewer outcome, active flags, unresolved gate and the next READY row. Update the detailed plan checkbox and this tracker together. Native gate evidence must name its tested code SHA; a later behavioral change invalidates the affected gate until rerun.

Current runtime facts: the main `windows` checkout still carries historical code; the separate pinned upstream is a successful Windows baseline. They have not been integrated. No servers redeployed, no packaging run, no claim of complete BBSC Windows support.

## Plan review checkpoint

Independent Luna review checked requirement coverage, source pins, native/fixture boundaries, work ordering, history preservation and packaging limits. Corrected vendor-relative paths and the historical-P2P-to-upstream mapping against actual files, clarified W01 versus W03 resumption, and removed dependency/hook-qualification ambiguity. Reviewer found the plan publishable as a waiting checkpoint; final wording/link/whitespace checks passed. All local Markdown links in the seven changed planning/evidence documents resolve. This review approves the plan's clarity/evidence scope; it does not qualify the unimplemented native features or turn known BUILD_FAIL results into passes.

## Maintenance log

- 2026-10-21 — User requested mandatory plan/progress discipline in `AGENTS.md`. Added requirements to read the active plans/tracker before work, identify task/dependencies, record ongoing progress and actual verification/review evidence, synchronize checkboxes, document deviations and leave a precise resume point at every handoff/checkpoint. Applies to subagents and future sessions. Verified document links and whitespace; no runtime changes, tests rerun or game launch. W01 remains READY and implementation is still waiting for the next instruction. Publication is verified separately against the checkpoint commit/remote ref.

## W01 execution checkpoint — 2026-10-21

User authorized plan execution and gpt-6-luna delegation; no-game, managed admission OFF and no packaging/deployment remain in force. Source inspected: controller `26d74ee802098928165387a40af87a04d03693c9` plus documentation diff; integration `de2e38af055f09f819946951075c6bb119e62ae6` plus selected documentation only. App-managed implementation worktree: `C:/Users/numbe/.codex/worktrees/bbsc-windows-integration/bloodborne_pc`, branch `codex/bbsc-windows-integration`. Baseline remains detached and unchanged.

Commands: `git worktree list`, `git rev-parse HEAD`, `git status --short --branch --ignore-submodules`, `git merge-base`, `git ls-tree -r` and rename-aware path comparison. Exact pin/ancestry/path/gitlink results are in the integration-base record. All six integration submodules initially uninitialized at exact upstream pins; dependency initialization is setup for W02, not a gate or build PASS. No runtime tests/build/game run for W01 (NOT RUN, documentation-only). Independent Luna review found no factual blockers; controller corrected reversed upstream-only gitlink wording before review. No native contract or gameplay evidence inferred.

Ruling: preserve selected plan/spec/handoff documentation by explicit paths on implementation branch; keep `windows` as historical runtime plus checkpoint documentation until the reviewed W11 merge. Push interim documentation to `origin/windows` without claiming runtime integration. Do not duplicate or transplant old runtime/vendor trees. Usage checks: 44%, then 42% remaining in five-hour window; weekly 44%. Stop below 10% as requested.

Publication and documentation validation are recorded in the following checkpoint entry. Next: W02 harness and W03 source contract map, disjoint ownership; W04 waits for W02. Game gates remain WAIT_GAME.

W01 publication: integration documentation commit `ae563c811ecc81c98ad4c861d5f16d3638e5209f`; controller checkpoint `45e9de324a579aaeb14a3e0f09dfd1d685a41e55` pushed, local `HEAD` equals `origin/windows`. Controller link check 23/23; selected integration documentation link check 57/57. Changed controller whitespace PASS. The copied archival FSR patch has original whitespace findings, intentionally preserved with identical Git blob; not a runtime failure. W02 dependency checkout initially failed on a long FSR filename; long-path retry and restoration of one omitted pinned header succeeded, six exact gitlinks initialized, FSR clean. W02 and W03 now have separate Luna owners (test harness versus adapter-contract document); no native code edits delegated concurrently.

### W02 ongoing evidence

Tested integration source `ae563c8` before owned test fixes: CLANG64 configure completed; `upscaler-support-test` reproduced undeclared `setenv`/`unsetenv` (RED/BUILD_FAIL, execution NOT RUN). Initial renderer builds hit Ninja path length >260 characters; a motion-shader dependency also encountered `std::lock_guard` in FSR before upstream build-script patches were applied. These setup blockers do not replace the required `sigjmp_buf` RED. Ruling: W02 may use a short junction alias to this same app-managed integration checkout and apply only upstream's normal build.sh preparation. Record alias and commands in W02 evidence; do not patch unrelated runtime to bypass setup. Scope/dependencies/acceptance unchanged; W02 remains IN_PROGRESS. Native game tests remain NOT RUN.

### W03 review checkpoint

First source-map draft is not accepted yet. Controller spot-check found the active `BbThd::Enabled()` branch in `bbnet_glue.cpp`: initialization and symbol registration can select `BbThd::Initialize`/`BbThd::Register` and return before vendor NP registration. Draft vendor-only Matching2 mapping omitted the `thd_matching2.cpp` create/join/leave and worker-side room/peer mutations preceding callbacks. Returned to W03 owner for exact dual-route registration/consumer/lifetime mapping; independent review is running. W03 remains IN_PROGRESS; no dependent native interface is approved. Source check: `git show de2e38a:gpu/bbnet/bbnet_glue.cpp` and `git show de2e38a:gpu/bbnet/thd/thd_matching2.cpp`. This is a source-map finding, not native gameplay validation.

Independent W03 Luna review confirmed the active-route omission and requested exact poll/wait symbols. It also found `commit_file` ignores host `close()` result: existing returned status covers checked flush/replacement operations, not a complete durable close receipt. Fix requested before source-map acceptance; W05 must preserve this explicit gap until implemented/tested. No native or gameplay claim changed.

W02 RED milestone: all four renderer stub compilations reproduced `unknown type name sigjmp_buf`; upscaler reproduced seven undeclared `setenv`/`unsetenv` calls. Short junction `C:/bbsc-w02` resolves to the same managed integration checkout, no source copy. GREEN work is ongoing: Windows stub type and upscaler compilation now pass, test-only C++ runtime link gap under investigation. Runner regression tests first failed on absent durable summary as intended (RED); not yet an accepted harness PASS. Native full build/run acceptance remains NOT RUN/pending. Latest usage check: five-hour 34%, weekly 43% remaining.

W03 second review: active/fallback registration and close-result corrections accepted; remaining image boundary omission returned to owner. Actual THD `SetImage` validates size/prologue then retains raw pointer; `SetSignalingGate` writes image byte via context-start caller, and `HostExtra` reads live guest state on create-room worker. These existing source behaviors require explicit mapping and are not approval for BBSC worker-thread memory access. Independent review confirmed `0x29/0x2A` native-result premise unsupported (HTTP URL punctuation, historical custom capability 0x29 only). Updated W03 plan trace requirement before relying on it; affected W06/W08 diagnostics must identify real producers/consumers, with absent native route remaining a G1 gap. No gameplay gate waived.

### W03 source-map acceptance — 2026-10-21

VERIFIED for source discovery only, after independent Luna review and two correction passes. Inspected upstream `de2e38af055f09f819946951075c6bb119e62ae6` and historical `ea8ac53801e49b5dc79c072971c0e05efcd7651f` via `git show`, `git grep -n`, `git ls-tree`; controller source at `45e9de3` plus documentation diff. Map covers real THD/vendor registration, request/server/peer effects before callbacks, callback worker/teardown, loader image ownership, THD guest-memory accesses, title/user/save mount/commit, and blocked native/UI/recovery ownership. Concrete native diagnostics remain G1–G3; no API invented or admission enabled.

Documentation link/content/whitespace checks PASS; runtime tests/build NOT RUN (source-discovery task); fixture/native/game evidence remains separate. W03 review explicitly accepted complete source mapping while unresolved native observations stay blocked. W02 remains IN_PROGRESS; W04 waits W02; W05 waits W04 and cannot assume native receipt qualification. Latest usage: five-hour 29%, weekly 42% remaining. No game launched, no packaging/server deployment.

W03 publication: integration docs commit `d91f501`, controller `27d0c77` pushed and `HEAD` equals `origin/windows`; `origin/pre` remains `a37b419ec03b1286c3d7accd954487de6d6d6429`. W02 early independent review requested two fixes before acceptance: assert environment restoration after the RAII object is destroyed, and use fresh shader output to prevent stale SPIR-V from satisfying current-run evidence. Runner records must label CPU/shader/headless Vulkan categories. Implementer is addressing these; W02 remains IN_PROGRESS, full suite/build evidence pending.


W02 support scope: added /.superpowers/ ignore to integration .gitignore because upstream lacks it and the runner writes private durable summaries there. git check-ignore .superpowers/w02-report.md PASS; plan file ownership updated. This prevents accidental evidence/profile tracking without changing runtime behavior.

W02 build milestone: implementer reports full `bash build.sh` PASS (365 steps; development bb-probe/bb-gpu-capabilities/bb-play outputs produced, none executed). Source integration `d91f501` (documentation-only successor to `ae563c8`) plus uncommitted W02 code; final accepted code SHA/evidence pending. First baseline runner attempt FAIL: Python argv flattening, missing CLANG64 DLL PATH (`0xC0000135`), shader output freshness and three headless Vulkan targets' unresolved runtime/`bb_setjmp`/`CreateFileMapping2` linkage. Implementer added runner argv/PATH/fresh-output/category handling and test-only aborting stubs; reviewer checking exact ABI and unreachable-path behavior. Full baseline rerun underway; do not infer all tests PASS from full build. W02 remains IN_PROGRESS.

W02 scoped source re-review: no remaining source blocker. RAII restoration now asserted after scope, shader output uses fresh GUID directory and exact four nonempty outputs, CPU/shader/headless categories recorded. Reviewer cross-checked new Windows stub declarations against runtime and Windows ABI; they abort if called and cannot emulate successful runtime memory/thread behavior. Execution is still the acceptance gate: any abort is a FAIL to investigate, not waive. Latest usage 25% five-hour / 41% weekly remaining.

Controller independent W02 runner verification: at integration `d91f501` plus uncommitted current W02 diff, command `C:/msys64/clang64/bin/python.exe -m unittest discover -s tests -p test_windows_no_game_runner.py -v` with CLANG64 DLL PATH exited 0; 3 tests PASS in 45.959s, 0 skips. Cases: failing child, missing prerequisite, missing target cannot produce passing group. Tool output is independent execution evidence; full baseline acceptance still awaits implementer report. Usage now five-hour 23%, weekly 41% remaining.

W02 final baseline evidence inspected by controller: private integration `.superpowers/w02-baseline-pass.log` records all nine native build/run commands PASS, four exact fresh SPIR-V files validated via `spirv-val --target-env vulkan1.3` PASS, six Python modules with 52 total cases (51 PASS, 1 SKIP real-ELF prerequisite). Runner command summary is 23 PASS/0 FAIL/0 SKIP/0 MISSING (command counts, distinct from Python case counts). Final top-level exit/full-build/assertion flag report and reviewer acceptance pending before W02 checkbox closure. No game/launcher/profile opened.

### W02 accepted checkpoint — 2026-10-21

VERIFIED: integration source `d91f501629f9d2a51eb04fae797ff475c6e032e6` plus the reviewed W02 diff. Files: `.gitignore` private evidence rule, Windows-compatible aborting runtime test stubs, portable upscaler environment/temp fixtures, assertion-enabled CMake targets, allowlisted PowerShell no-game runner and three runner regression tests. All nine assertion targets compile with `-UNDEBUG`. Native ABI declarations reviewed against their actual owners; abort stubs were not reached in successful tests.

Commands (CLANG64): `bash ./build.sh` from `C:/bbsc-w02` (junction to managed integration checkout) exited 0, 365 build steps; no produced game/program launched. `powershell -NoProfile -ExecutionPolicy Bypass -File C:/bbsc-w02/scripts/test_windows_no_game.ps1 -Group baseline -RepoRoot C:/bbsc-w02 -BuildDir C:/bbsc-w02/out/gpu -SummaryPath C:/bbsc-w02/.superpowers/w02-baseline-final.log` with explicit CLANG64 Ninja/Python/spirv-val paths exited 0: 23 command PASS, 1 Python prerequisite SKIP, 0 FAIL/MISSING. This includes nine BUILD/RUN pairs, four fresh exact SPIR-V files validated for Vulkan 1.3, and six Python modules with 52 cases (51 PASS/1 SKIP missing real ELF). Three renderer tests used a headless Vulkan device, no game window. Native full build alone was not used as test evidence.

Runner unittest discovery exited 0, 3 PASS (implementer and independent controller run); explicit `-Group bbsc` exited 1 with MISSING as required before W04/W06. `-Group all` NOT RUN in W02; it currently includes the baseline plus unavailable-BBSC failure. Historical gameserver 8-case and catalog evidence retain their separate pins; not rerun or relabeled. Private evidence: integration `.superpowers/w02-report.md`, `w02-baseline-final.log`, `w02-bbsc-unavailable.log`, `out/gpu-build.log`; controller `.superpowers/sdd/2026-10-21-bbsc-windows-execution/w02-controller-runner-check.md`. Initial RED/setup failures and review fixes are recorded above. Expected upstream build-script FSR patch diff stays private/uncommitted, gitlink pin unchanged; historical archival patch untouched.

Independent Luna source/evidence re-review found no remaining blocker after RAII restoration and fresh shader-output fixes. Whitespace/scope check PASS. W04 is next READY; W05 remains WAIT_DEP W04, with real native identity/receipt/grant qualification still pending G1–G3. No game launch, packaging or server deployment. Latest usage five-hour 21%, weekly 41% remaining; continue while above requested stop threshold.

W02 publication: implementation commit `2b48dd8dd0eb4213be9275184e489177779111aa` on local integration branch; controller evidence commit `88961c59a88800356c8a5f80d99e48be53b27ec1` pushed and equals `origin/windows`. Runtime integration is still not merged into `windows`. W04 now IN_PROGRESS with a fresh Luna implementer; owns portable policy/protocol headers and sources, focused tests, CMake target additions and runner BBSC group only. Native adapters remain untouched. Follow the reviewed W03 blocked facts.

Publication limitation: automatic approval review rejected a separate remote push of local `codex/bbsc-windows-integration` (currently `2b48dd8`), stating that unmerged code/worktree publication to an additional branch was outside explicit authorization and could be sensitive egress. No remote integration branch was created; no workaround attempted. Requested user choice for optional integration-branch backup. Required documentation checkpoints to `origin/windows` remain published. Local code commits are preserved; W04 offline work continues. The combined rejected command did not execute its preceding copy, so tracker synchronization is retried as a standalone file operation only.

User publication decision: keep `codex/bbsc-windows-integration` local for now. Do not retry remote integration-branch publication. Continue authorized documentation checkpoints on `origin/windows`; local W02 code remains preserved in `2b48dd8`. Latest usage five-hour 17%, weekly 40% remaining; W04 continues with stop-below-10 instruction communicated to owner.

### W04 ongoing checkpoint

Tested integration `2b48dd8` plus W04 diff. Selected portable party/catalog sources moved to new `gpu/bbnet/bbsc/` home; canonical `bbsc_protocol.h` preserves historical v1 0x28/0x29 wire types/enums/sizes. Both policy/events targets BUILD/RUN PASS; BBSC runner 2 PASS/0 FAIL/SKIP/MISSING, fixture-only. Initial attempted RED was environmental (missing CMake PATH), explicitly NOT a behavioral RED; subsequent link and DLL PATH setup failures recorded privately. No native hooks/vendor source touched.

Independent early review found unchecked arbitrary nonzero EventId at Admission::Accept/Earn, requiring a catalog-boundary rejection test/filter or proven downstream validation. Also required explicit different-nonempty-password and oversized-packet assertions. Returned these for real behavioral RED/fix; no W04 acceptance checkbox closed. Private report `.superpowers/w04-report.md`, runner `.superpowers/w04-bbsc-run.log`. Latest usage14% primary; stop new work below10 and checkpoint status honestly. W04 remains IN_PROGRESS; no native/game evidence inferred.

W04 review fix: meaningful behavioral RED reproduced when Admission accepted unknown event `0xDEADBEEF`; GREEN after catalog membership guards at Admission baseline/event receive and Delivery baseline/Push boundaries. Added real oversized packet, capability framing, differing-password and same-map assertions. Both focused builds/executions and BBSC group PASS (2/0/0/0). Independent Luna re-review accepted guards and confirmed all three item-source events are in the 22-row catalog; no substantive source blocker. Controller incremental native build and combined runner verification are the final checkpoint checks, currently running/pending. Usage latest12% five-hour; no new task dispatched before closing this checkpoint.

### W04 verified / usage stop checkpoint — 2026-10-21

VERIFIED for portable Windows policy only. Tested source integration `2b48dd8dd0eb4213be9275184e489177779111aa` plus reviewed W04 diff. Added selective party/policy/catalog source under `gpu/bbnet/bbsc/`, canonical shared wire header, two assertion-enabled test targets and actual runner BBSC group. Preserved historical message/capability API, framing, 22 boss/3 item candidate catalogs, deferred endgame choices and Cord preserve-only behavior. Unknown IDs now rejected at all Admission/Delivery catalog boundaries. No native hooks/vendor transport or save writes added; managed admission remains OFF.

Behavioral RED: added unknown `0xDEADBEEF` acceptance assertion failed before catalog guard; GREEN after narrow fix. Other initial CMake PATH/link/DLL errors were setup failures, not falsely labeled behavioral RED. Focused builds/direct runs exit 0; `-Group bbsc` exit 0, 2 PASS/0 FAIL/SKIP/MISSING. Independent Luna re-review accepted wire compatibility, guard coverage and password/map rules; no substantive blocker.

Controller final checks at the same source+diff: CLANG64 `bash build.sh` from junction `C:/bbsc-w02` exited 0 (331 native build steps; no generated executable launched); evidence `.superpowers/w04-native-build.log` and `out/gpu-build.log`. `powershell -NoProfile -ExecutionPolicy Bypass -File C:/bbsc-w02/scripts/test_windows_no_game.ps1 -Group all -RepoRoot C:/bbsc-w02 -BuildDir C:/bbsc-w02/out/gpu -SummaryPath C:/bbsc-w02/.superpowers/w04-all-final.log` exited 0: 25 command PASS, 1 SKIP, 0 FAIL/MISSING. Includes nine upstream BUILD/RUN pairs, four fresh SPIR-V validations, Python 52 cases (51 PASS/1 SKIP real ELF) and both portable BBSC runs. Private report `.superpowers/w04-report.md`, focused runner `w04-bbsc-run.log`. C++ executable command counts are not individual assertion counts. Required game gates remain unpassed; catalog fixture is not native item-grant evidence.

STOP: usage tool reports 91% used / 9% remaining in the five-hour window (weekly 39% remaining). User requested stop below10%; no W05 work started or agents active on implementation. W05 READY; exact resume is its filesystem/process RED fixtures with copied temporary profiles, then narrow Windows persistence implementation under W03 source-backed owners. Do not invent native selected-slot/serializer/receipt contracts; G1–G3 remain future user gameplay gates. No game launch, packaging or server deployment. Integration branch stays local per user choice; only documentation checkpoints publish to origin/windows. Final commit/publication verification follows.

Final local implementation checkpoint: `50317971cc073f41791e7e8a5fded543a2e0ebd1` on `codex/bbsc-windows-integration`; not pushed by explicit user choice. Its non-submodule status is clean. Final staged whitespace check found only an extra EOF blank line in the newly imported party header; removed it (whitespace-only, no behavior change) and staged check then passed. Controller checkpoint document links: 22 checked, 0 missing; whitespace PASS. Expected upstream build-script FSR working-tree patches remain uncommitted at unchanged gitlink, separate from historical archival work. No W05 work started. Publication below concerns documentation on origin/windows only, never a claim the local implementation is remotely backed up or merged.

## W05 resumed — 2026-10-21

User authorized continuing with Luna delegation and periodic usage checks. Primary window reset: 100% remaining; weekly 38% remaining. Stop new work and checkpoint if either available remaining window falls below10%. Resume source: local integration `71340097185d2203dbee7828a1049c6331f857b0` (W04 code `5031797`, later docs only), controller `5c0e67f`; both clean excluding expected FSR build patches. Integration branch stays LOCAL per prior explicit user choice; documentation checkpoints still publish origin/windows.

W05 IN_PROGRESS. Dependencies W03 source map/W04 portable policy complete. Work split: journal/lease/backup/identity fixtures and runtime file commit/receipt fixtures use disjoint files; controller owns CMake/runner registration and shared documentation. Native-adapter/runtime receipt boundary has one owner. No native game identity/serializer/grant or admission qualification is inferred. W03 contains no approved receipt callback declaration yet: resolve and review a source-backed runtime-file receipt service before wiring any consumer. Game launch, packaging and redeploy remain unauthorized.

W05 early evidence: storage fixture link RED reproduced missing CharacterLease/StoreJournal/LoadJournal/backup definitions; no game/saves touched. Source correction recorded in detailed plan: historical `bbsc_persistence.*` are native persistent-session patches/default switches, so no bulk copy in W05. Runtime owner proposed a separate pure-C receipt observation service based on existing `do_close`/`commit_file`, not dynamic BbNetHost or selected-character proof; contract review is pending before implementation. Particular review points: failed close invalidates prior same-path receipt, process-local generation freshness, profile/mount rebinding, all checked write/flush/close/replace steps, and explicit achieved Windows durability.

W05 receipt contract reviewed and recorded in W03 adapter document before wiring: standalone `RuntimeFileReceipt` + `runtime_file_save_receipt`, owned by runtime_file.c/current mount facts, no dynamic host ABI or native slot/serializer declaration. Failed close invalidates same-path evidence; generation stays monotonic; inactive guest mount fails; profile/mount changes clear observations. Only checked API-level flush/close/hash/replace observation, with explicit Windows metadata/power-loss limits. Native pending retirement remains blocked on selected-character/serializer/grant G2/G3 evidence. Audit also required held path/reparse validation through actual operations and process-death lease tests; no prior check-then-path-use guarantee is assumed.

W05 runner support: new native filesystem fixtures can report a required case SKIP (for example unavailable reparse privilege). Existing runner classified only child exit code and Python-specific skip summaries, so an exit-zero native fixture could be marked PASS despite untested required checks. Controller added a behavioral regression first; RED execution pending. Plan registration scope explicitly includes runner/test updates; no skip is silently waived. Storage owner is replacing privileged symlink setup with an unprivileged junction fixture and retaining historical assertions, not claiming its initial narrow three-case fixture completes W05.

W05 controller runner verification at integration `7134009` plus current diff: regression `test_required_fixture_skip_cannot_be_reported_as_pass` failed as expected (exit1) because skip output was followed by a PASS command record. Added explicit native SKIP handling and private BBSC TEMP/TMP restoration; full runner unittest discovery now exit0, 4 PASS in27.561s, no skips. Private evidence integration `.superpowers/w05-runner-skip-red.log`, `w05-runner-green.log`. Source review pending. Storage owner reports two-process exclusion, release after child exit, terminated-child Visiting+pending/reopen and unprivileged junction rejection now PASS with no SKIP; broader historical/fault/path-swap checks remain ongoing, so W05 is not verified. Current usage five-hour84%, weekly36% remaining.
