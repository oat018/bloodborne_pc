# BBSC Remaining Work Implementation Plan

> **Windows direction amendment — 2026-10-21:** On branch `windows`, follow the [Windows-only migration plan](2026-10-21-bbsc-windows-only-migration.md) using Supermedo/bloodborne_pc. Earlier platform, upstream, distrobox, packaging and `origin/pre` instructions below describe the historical Linux baseline and are superseded on this branch. Product requirements remain in force. Existing test results do not establish Windows readiness; this amendment does not migrate runtime code.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans. Track completion with the checkboxes below.

**Goal:** Complete the agreed BBSC features and stop immediately before AppImage/EXE packaging.

**Architecture:** Extend BBPORT's existing BBSC module. Reuse the native join, party channel, journal, save receipts and return hooks; enable permanent changes only after their evidence and recovery checks pass.

**Tech Stack:** Existing C++23/C runtime, CMake/Ninja, OpenSSL, ZLIB and shadNet. Build/test inside distrobox `bbport-dev`.

**Spec:** [Approved design](../specs/2026-10-18-bbsc-anywhere-coop-design.md). See [original detailed plan](2026-10-18-bbsc-anywhere-coop.md) and [evidence](../specs/2026-10-18-bbsc-integration-evidence.md).

## Global constraints

- Baseline: `ea8ac53801e49b5dc79c072971c0e05efcd7651f` on `origin/pre`.
- Use the BBSC already in `gpu/bbnet/`; avoid unrelated bbport changes and preserve dirty FSR work.
- Native admission is currently OFF. Passing fixtures does not prove native cancellation.
- Existing source includes private-attempt tracking, correlated cleanup, immutable backups, rollback, first-playable delivery, durable intent Ack and recovery foundations. Do not rebuild these from scratch.
- Permanent event/item application and Windows native recovery are incomplete.
- Use Luna subagents for substantial work, with disjoint ownership; the controller handles small edits and integration.
- Below 10% remaining in the 300-minute usage window: freeze scope, document, commit and push `origin/pre`.
- User runs necessary gameplay tests on copied profiles. Do independent source work first; defer the broad gameplay matrix until after packaging authorization.

## Review focus

- A failed join must not mutate the guest world or leave a reusable stale admission.
- Death/loading must preserve eligibility; actual departure must close it despite stale room metadata.
- Interrupted grants must neither duplicate items nor erase earned credit.
- Reused slots, changed characters and NG epochs must never inherit foreign progress or return points.
- Missing destination/save evidence must preserve recovery data and use only a verified fallback.

## Order and dependencies

`1 → 2 → 4`; `3 → 5`; `4 + 5 → 6`; `6 → 7 → 8`.
Steps 1 and 3 can be investigated independently. Steps 4 and 6 share native adapter code and must not have competing file owners. Keep activation OFF until step 8.

### 1. Prove the native admission boundary

**Files:** `gpu/bbnet/shadps4/core/bloodborne_re.cpp`, Matching2 internal/mm sources; `tests/test_bbsc_join.cpp`, `tests/test_bbsc_matching2_admission.cpp`.

- [ ] Map the consumer of native result events `0x29/0x2A`, or add a narrowly scoped correlated diagnostic capture.
- [ ] Establish whether signaling callbacks/room polling can enter native join before the held JoinRoom callback.
- [ ] Test forced rejection followed by a permitted retry on copied profiles if source evidence is insufficient.
- [ ] Require rejection before Lua join, multiplayer Init, placement and reload; an empty trace alone fails this check.
- [ ] Record the proven path and regression assertions. Do not treat null Lua dispatch or callback enqueue as cancellation.

**Pass:** Failed identity/proof/backup/journal preparation prevents world mutation, and a later valid retry still works.

### 2. Close the first-playable delivery integration

**Files:** `bloodborne_re.cpp`, `include/core/bloodborne_re.h`, existing party/signaling code; `tests/test_bbsc_join.cpp`, `tests/test_bbsc_party.cpp`.

- [ ] Verify complete catalog baseline and admission share the host game-thread boundary; retain events during baseline transfer.
- [ ] Verify the initial cover waits for durable baseline Ack, with a bounded safe failure/exit path.
- [ ] Cover death/loading, late guests, local exit before room updates, host Leave, invalid catalog IDs and no receiver resurrection.
- [ ] Verify existing ordinary BBSC join/cover behavior remains unchanged outside managed admission.

**Pass:** Only post-boundary events become durable intents; neither later loads nor retries start another baseline in the same party.

### 3. Establish persisted inventory evidence

**Files:** Native serializer/proof code in `bloodborne_re.cpp`; `tests/test_bbsc_join.cpp`; integration evidence document.

- [ ] Resolve the item-data owner's virtual serializer reached from `0x014EA560`, called at `0x0156AC2B`.
- [ ] Establish actual carried/storage goods encoding and save/load direction; do not invent offsets.
- [ ] Add production decoder assertions bound to the selected character, serialization bytes and fresh committed receipt.
- [ ] Separate present, stored, previously claimed/consumed and never-granted states; use a durable claim ledger where the native save cannot prove history.

**Pass:** A grant attempt or live inventory count cannot masquerade as persisted item delivery. Request only the missing controlled item observations if necessary.

### 4. Apply safe earned boss/story progress

**Files:** `bloodborne_re.cpp`, `bbsc_party.cpp`, `bbsc_party_journal.cpp`, `include/core/bbsc_party_events.h`; existing party/join/event tests.

- [ ] Audit each supported catalog row's additive flags, prerequisites and shared-story effects.
- [ ] Restore the guest's own world before applying earned changes at the verified native boundary.
- [ ] Preserve already-ahead progress; defer conflicting NPC choices and ending/NG mutations rather than copying flag groups.
- [ ] Retire applied event credit only after matching post-application serialization and save receipt; test interruption/restart.

**Pass:** Old host presentation never becomes earned guest progress, and unconfirmed/deferred events remain recoverable.

### 5. Deliver required rewards exactly once

**Files:** Same native/journal adapter; `NoteBbscBossRewardEvent`, `TickBbscBossRewards`, `GiveBbscBossGoods`, `BbscGiveHostWorkshopTools`; existing journal/join tests.

- [ ] Reuse the explicit item catalog: goods `4000` from `12411800`, `4020` from `13501850`, and Cord `4323` from `12601800`.
- [ ] Route overlapping legacy reward callers through the same admission/claim rules; prevent pre-admission and duplicate grants.
- [ ] Persist grant intent before native mutation; confirm actual delivery with step 3 evidence before retiring credit.
- [ ] Test carried/stored/consumed items, full inventory, retry, crash after grant and crash around receipt/ledger updates.

**Pass:** No duplicate reward or lost earned credit; Cord is never automatically consumed or used to mirror an ending choice.

### 6. Finish normal-party return and cycle coordination

**Files:** Native return/own-world/cycle code in `bloodborne_re.cpp`, journal sources; existing join/journal/runtime tests.

- [ ] Connect explicit leave, confirmed disconnect, quit/restart and unsupported host destinations to one managed lifecycle.
- [ ] Close eligibility before teardown; preserve the original pre-party origin through deaths/follow loads.
- [ ] Return to the original point only with current safety evidence; otherwise use the lamp unlocked before joining. Never guess geometry or a newly unlocked host lamp.
- [ ] Reuse bounded native return retry and require own-world/destination/save confirmation before completing the journal.
- [ ] Keep deferred rewards and cycle identity isolated across slot reuse, native 6→7 and managed 7→7 transitions.
- [ ] Notify unsupported-area exit clearly; retain actionable pending recovery on failure.

**Pass:** Normal parties use the durable recovery path, not only process-local correction or offline fixtures.

### 7. Complete Windows feature parity

**Files:** `gpu/bbnet/bbnet_glue.cpp`, native recovery sections, journal platform branches and existing build configuration.

- [ ] Replace disabled Windows recovery/admission branches with equivalent validated native hook/persistence integration.
- [ ] Verify atomic replacement, locking, path handling, rollback and offline recovery on Windows.
- [ ] Identify and record the actual supported Windows build command/toolchain; compile/link the target and check required module/runtime files.

**Pass:** Windows contains the requested features and builds successfully. A Linux build or portable journal source alone is insufficient.

### 8. Final integration and before-packaging checkpoint

- [ ] Run the module build and six focused targets: `bbnet`, `bbsc-party-test`, `bbsc-join-test`, `bbsc-party-events-test`, `bbsc-party-transport-test`, `bbsc-http-poll-test`, `bbsc-matching2-admission-test` via Ninja in `bbport-dev`.
- [ ] Run all six executables and `out/online-test` cases `--bbsc-save-host`, `--bbsc-recovery-offline`, `--bbsc-recovery-module`.
- [ ] Review connected callers and fix reportable regressions; enable native admission only after steps 1–7 prerequisites pass.
- [ ] Record focused live safety evidence, outstanding broad gameplay/NG tests and Windows results honestly.
- [ ] Update plan/design/evidence/BBSC documentation and create the coherent `pre` checkpoint.
- [ ] STOP before running packaging scripts or producing/publishing AppImage/EXE. Packaging requires the user's next instruction.

**Ready-to-package condition:** All source feature paths are connected and focused safety/build checks pass. Untested broad gameplay scenarios are explicitly listed; disabled or missing functionality is not counted as complete.
