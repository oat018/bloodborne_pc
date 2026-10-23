# Windows execution reference — scoped BBSC integration

Updated2026-10-23. Execute inline under the [active alignment/delta plan](2026-10-23-bbport172-bbsc002-alignment.md), [Windows direction](2026-10-21-bbsc-windows-only-migration.md), [latest product scope](../specs/2026-10-18-bbsc-anywhere-coop-design.md) and [progress tracker](../../WINDOWS-PROGRESS.md).

## Current execution order

1. R02: complete the pinned BBport1.7.2 + BBSC0.0.2 client/mod composition using the release's existing loader and configuration. Network patch/build checks are already recorded; actual mod and gameplay remain open.
2. R03: reproduce unchanged baseline on self-hosted services, with distinct copied-profile clients/accounts. Record join/placement, earned progress/save-reload, normal exit/return and interrupted-session reload separately. Use only relevant build/ABI/registration checks and changed-seam tests; no blanket old test-suite or module migration.
3. R04: implement only reproduced gaps in the three goals. Each change needs a pinned baseline gap/reproduction, existing responsible caller, reason configuration/reuse is insufficient, minimal delta and focused negative/positive tests.
4. Repeat the three-goal scenario and review changed paths for scope. Keep other upstream game systems unchanged, protect original saves, and retain explicit unrun/unsupported limitations.

Self-hosted support is required; public-server/THD/bbhost/cross-emulator development is not scheduled. Existing account/authentication/transport remains baseline. Custom HMAC, general storage/NG/save codec or whole-game quest/item infrastructure is not a mandatory prerequisite. Safety validation at a changed boundary remains mandatory.

## Legacy task IDs and native outcome gates

| Reference | Use only when the actual delta needs it |
|---|---|
| W01–W03 | Source ownership, minimal build/test harness and actual hook/thread/ABI mapping for the selected baseline; do not port all old adapters. |
| W04–W05 | Reuse portable policy/storage helpers if the baseline lacks a required outcome; no new framework by default. |
| W06/W08, G1 | Changed cross-map candidate/password/host placement: prove correct peer, stale/failure rejection and no guest-world mutation on failed join. |
| W07 | Map only actual hooks needed for a demonstrated gap. Network workers must not mutate live world data. |
| W09, G2 | Changed earned boss/story progress and necessary rewards: prove eligibility, preserved own progress, no duplicates and save/reload. |
| W10, G3 | Preserve baseline return/recovery. Baseline smoke covers normal exit/disconnect and interrupted-session reload; changed persistence also needs affected wrong-character/write-failure/retry tests. |
| W11 | Final scoped verification, source review and checkpoint. No packaging or production redeployment. |

These are evidence references, not eleven independent features that must be recreated. Native outcomes remain gates for new custom behavior. Existing baseline gameplay can be reproduced on copied profiles while custom managed admission/application remains OFF; it is not custom acceptance.

## Test and evidence policy

Record source/artifact SHA, dirty state, exact commands/waited exits, PASS/SKIP/NOT RUN/BUILD_FAIL, private evidence path, limitations and next action. Test changed code, baseline smoke and the delivered multiplayer scenarios. Broader tests are justified by a concrete failure or affected path, not by a desire to rebuild upstream quality infrastructure. Keep fixture/native/baseline/custom gameplay evidence separate.

The full previous W checklist/source history is [archived here](https://github.com/kaoitp/bloodborne_pc/blob/8cae08bc1884136e493fc107a64b1244794ec597/docs/superpowers/plans/2026-10-21-bbsc-windows-execution.md). Historical implementation/failure evidence remains in that snapshot and the tracker; this scope correction does not claim it passed on1.7.2 or delete the old source.
