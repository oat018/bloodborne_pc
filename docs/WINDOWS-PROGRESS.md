# BBSC Windows progress

Updated: 2026-10-21 (Asia/Bangkok). **Current checkpoint: preflight checked; development plan prepared; waiting for the user's next instruction.** No game was opened during this planning pass.

## Read this first

- [Detailed execution plan](superpowers/plans/2026-10-21-bbsc-windows-execution.md)
- [Platform/scope direction](superpowers/plans/2026-10-21-bbsc-windows-only-migration.md)
- [Migration manifest](handoff/WINDOWS-MIGRATION-MANIFEST.md), [test matrix](handoff/WINDOWS-TEST-MATRIX.md), [machine/game/save evidence](handoff/WINDOWS-SETUP-2026-10-21.md)

`VERIFIED` means the stated scope has evidence. `READY` means work may start. `WAIT_DEP` means an earlier task's deliverable is missing. `WAIT_GAME` means user gameplay evidence is required. `IN_PROGRESS` and `BLOCKED` require an explicit reason. Checkboxes mirror the detailed plan; do not report a feature percentage from completed setup tasks.

## Work tracker

| ID | Work / expected deliverable | Status | Needs game? | Evidence / next action |
|---|---|---|---|---|
| W00 | Native baseline, copied-profile preparation, source/test inventory | VERIFIED | Already tested by user | Native build exit 0; characters and basic gameplay user-confirmed; no-game results below. Full test parity and BBSC are outside this checkbox. |
| W01 | Separate integration branch/worktree and reviewed file/history map | READY | No | Keep pinned baseline as reference; implement on `codex/bbsc-windows-integration` after the next instruction. |
| W02 | Windows no-game harness and reproducible runner | WAIT_DEP W01 | No | Resolve five upstream target compile failures; all assertion tests keep `-UNDEBUG`; add explicit group/failure reporting. |
| W03 | Exact static adapter/caller/thread/lifetime contracts | WAIT_DEP W01 | No for source map | Source findings exist; native hook/cancellation/save contracts still require discovery and later live confirmation. |
| W04 | Portable party/protocol/catalog and focused targets | WAIT_DEP W02 | No | Catalog-only fixture already passes; full policy/transport integration is not ported. |
| W05 | Windows locking/journal/backups/receipts and identity fixtures | WAIT_DEP W03/W04 | No for fixtures | Actual Windows child-process interruption and filesystem semantics must be proven. |
| W06 | Static HTTP/Matching2/signaling/transport integration | WAIT_DEP W03/W04/W05 | No for mocks | Selective BBSC hunks; no copied Linux vendor tree; activation stays OFF. |
| W07 | Native ABI/tick/scanner/selected-character adapter | WAIT_DEP W03/W05/W06 | Fixtures no; live later | No assumed native offsets or worker-as-game-thread substitution. |
| W08 | Admission/first-playable coordination and rejection fixtures | WAIT_DEP W05/W06/W07 | Fixtures no; G1 yes | Prove all entry routes; callback enqueue is not cancellation. |
| W09 | Eligible additive progress and exactly-once rewards | WAIT_DEP W05/W07/W08 | Fixtures no; G2 yes | Resolve persisted inventory, native grant/receipt and each catalog row; keep unqualified application disabled. |
| W10 | Safe return, offline recovery, unsupported-area handling | WAIT_DEP W05/W07/W08/W09 | Fixtures no; G3 yes | Preserve pending recovery and captured fallback; never consume a foreign identity's record. |
| W11 | Final integrated verification/review and checkpoint | WAIT_DEP W01–W10/G1–G3 | No new launch if evidence remains current | Normal history-preserving integration/push to `origin/windows`; stop before packaging. |

- [x] W00 preparation and evidence inventory
- [ ] W01 integration workspace/history map
- [ ] W02 Windows test harness
- [ ] W03 adapter contract map
- [ ] W04 portable policy
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
