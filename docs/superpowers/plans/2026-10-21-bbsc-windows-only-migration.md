# BBSC Windows-only Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans. Track completion with the checkboxes below.

**Goal:** Move BBSC development to Windows only using Supermedo/bloodborne_pc, complete the agreed features, and stop before EXE/ZIP packaging.

**Architecture:** Preserve the existing project history and adapt only BBSC and necessary integration seams to the new upstream. Reuse BBSC already present in the chosen base where verified; online networking alone is not evidence that its seamless co-op features exist. Avoid unrelated renderer, launcher and input changes.

**Tech Stack:** Native Windows 10 1903+/11 x64, MSYS2 CLANG64, upstream CMake/Ninja/C++ toolchain and existing network dependencies. Linux distrobox `bbport-dev` remains optional for source inspection and portable checks; it does not establish Windows readiness.

**Spec:** Product requirements in the [approved design](../specs/2026-10-18-bbsc-anywhere-coop-design.md) remain in force. This plan supersedes earlier upstream, platform, build-environment, release-target and checkpoint-branch instructions on `windows`.

## Global constraints

- Direction approved on 2026-10-21: Windows only. AppImage and Linux release parity leave the active release scope; existing Linux code and historical evidence are retained.
- Primary upstream: [Supermedo/bloodborne_pc](https://github.com/Supermedo/bloodborne_pc), default branch `windows`. Inspected HEAD: `de2e38af055f09f819946951075c6bb119e62ae6`. Record a new pin explicitly if changed later.
- This documentation branch starts at `a37b419ec03b1286c3d7accd954487de6d6d6429`. Its runtime is still the previous codebase: selecting a new upstream does not mean the code has been migrated or built on Windows.
- Keep `pre` and `origin/pre` as the historical checkpoint. Work and usage checkpoints now go to `windows` / `origin/windows`. Below 10% remaining in the 300-minute window, stop expanding scope, document, commit and push that branch.
- `origin` remains the user's repository. Use Supermedo as a separately named source remote when migration begins; changing GitHub fork parent metadata is unnecessary for this workflow. Do not delete/refork, rewrite history or force-push as part of migration. See [GitHub's remote guidance](https://docs.github.com/en/pull-requests/how-tos/work-with-forks/configuring-a-remote-repository-for-a-fork).
- Use Luna subagents for substantial independent work with disjoint ownership. Small documentation and integration edits may be handled by the controller.
- Preserve the unrelated dirty `gpu/third_party/fsr-vulkan` work. Use a clean checkout for upstream baseline builds: its build script initializes submodules and applies patches.
- No game data, accounts, tokens or original saves in Git. Runtime tests use copied profiles. Do not deploy servers or run packaging as part of this documentation change.
- Keep native managed admission OFF until the new Windows integration proves rejection before world entry. Removing platform guards is not a port.

## Requirements carried forward

- Support main story areas and Hunter's Dream, with cross-map matching only for the same nonempty password. Join as a guest and place the guest with the host.
- Display the host's temporary world state during the party. Persist only jointly earned boss/shared-story progress and explicitly enumerated necessary items, without duplicate grants, progress rollback or conflicting quest-choice overwrites. Review NPC quests individually; never copy the complete flag set.
- Eligibility starts when that guest first becomes playable in the host world. It survives death and loading within that party and ends on actual departure. Exclude events before entry and after exit.
- Persist the pre-join map/position and a previously unlocked fallback lamp before joining. Restore the original point only when verified safe; otherwise use that captured fallback. Recovery must survive disconnects and closing the game.
- On unsupported destinations, end co-op safely, return the guest and explain the unsupported area. Preserve the requested Blank menu, animations and normal departure behavior.
- Private matchmaking/server changes remain available if required. Existing test servers are not proof of Windows client compatibility.

## Inspected upstream seams

The [Windows build guide](https://github.com/Supermedo/bloodborne_pc/blob/de2e38af055f09f819946951075c6bb119e62ae6/packaging/windows/README.md), [build script](https://github.com/Supermedo/bloodborne_pc/blob/de2e38af055f09f819946951075c6bb119e62ae6/build.sh) and [GPU CMake](https://github.com/Supermedo/bloodborne_pc/blob/de2e38af055f09f819946951075c6bb119e62ae6/gpu/CMakeLists.txt) are the starting references.

- `bbnet` is static and linked into `bbgpu`; network sources live under `gpu/bbnet/vendor/shadps4/`, with `bbnet_glue.cpp`, `include/` and `thd/` beside them.
- Its build registration is in `gpu/CMakeLists.txt`; the inspected base has no standalone `gpu/bbnet/CMakeLists.txt`. Adapt the existing integration instead of transplanting the old dynamic-module loader.
- `bash build.sh` builds `bb-probe`, `bb-gpu-capabilities` and `bb-play`, with Windows executable outputs copied to `out/`. These are build outputs, not proof that the distributable launcher has been packaged.
- The upstream packaging script is `packaging/windows/package.sh`. It is a later reference, not an action authorized by this before-packaging plan.

## Review focus

A failed join must not mutate the guest world. Pending recovery must remain usable across interruption. Character/slot reuse and NG epochs must not inherit another identity's progress or return point. Native callback enqueue is not consumption, Lua skipping is not proven cancellation, and live inventory is not a persisted save receipt. Old Linux test results are historical evidence only.

## Ordered work and exit checks

### 0. Record the direction

- [x] Create `windows` from the existing checkpoint, preserving `pre`.
- [x] Record the new upstream pin, Windows-only scope and documentation precedence.
- Publication: commit these documents and verify the resulting commit on `origin/windows`; record publication evidence in the handoff.

### 1. Inventory and define the migration patch set

**Inspect:** old `gpu/bbnet/`, `tests/test_bbsc*`, runtime bridges and new `gpu/bbnet/vendor/shadps4/`, `gpu/CMakeLists.txt`, launcher/build scripts.

- [ ] Compare a clean pinned upstream checkout with the old BBSC checkpoint; record actual existing BBSC components separately from general network support.
- [ ] Write a file-by-file migration manifest: reuse, adapt, superseded or deferred, including callers, tests, build registration and fixtures.
- [ ] Define the static runtime/BBSC interface and a selective patch sequence. Preserve history when integrating; do not blindly cherry-pick the entire Linux integration or replace unrelated upstream code.
- [ ] Record Windows paths, profile layout and available native machine/runner. Availability has not yet been established.

**Exit:** reviewed migration manifest and exact source/build pins, with no assumed native offsets or unverified APIs.

### 2. Establish the unmodified Windows baseline

**Files:** upstream `build.sh`, `msys2-env.sh`, `gpu/CMakeLists.txt`; baseline evidence document.

- [ ] In a clean Windows checkout, follow the pinned MSYS2 CLANG64 guide and run `bash build.sh` before introducing BBSC changes.
- [ ] Record tool versions, command, output paths and baseline launch result using a copied test profile. Separate upstream failures from migration failures.
- [ ] Identify which existing portable tests build on Windows; Linux-only test programs need adaptation or an explicit gap.

**Exit:** reproducible native Windows baseline build and launch evidence. No packaging yet.

### 3. Adapt BBSC networking and session controls

**Files:** actual network vendor paths identified in step 1, `bbnet_glue.cpp`, `gpu/CMakeLists.txt`, corresponding migrated tests.

- [ ] Reuse private-attempt/HMAC proof, correlated Matching2 cleanup, party transport and protocol validation in the existing static module without duplicating vendors.
- [ ] Integrate Kit distribution, password matching, single-load cross-map join and Blank controls through the new base's actual callers.
- [ ] Verify peer/session binding, expiry, retry, stale callbacks and both host/guest departure paths. Keep the requested Blank menu and normal animation/text paths.

**Exit:** Windows build plus focused network/session tests; runtime join behavior remains unproven until native checks.

### 4. Port native identity, receipts and durable recovery primitives

**Files:** native adapter and runtime bridge paths established in step 1; journal/backup/receipt helpers and tests.

- [ ] Adapt native hooks for the supported CUSA03173 1.09 revision. Validate Windows calling conventions, executable memory/cache handling and game-thread dispatch against this base.
- [ ] Port stable character identity, slot deletion/reuse and NG epoch handling, including capped NG+ cycle ambiguity.
- [ ] Preserve immutable generation-plus-content backups, atomic journal replacement, rollback and selected-save receipts using verified Windows file/locking/flush behavior.
- [ ] Verify recovery discovery before online startup and across crashes/restarts. Keep pending data when evidence is insufficient.

**Exit:** native Windows adapter evidence and interruption/identity tests; fixture success alone cannot qualify native hooks.

### 5. Prove admission and first-playable boundaries

**Files:** native join/Matching2 consumers, admission coordinator, first-playable session tracking and migrated tests.

- [ ] Prove forced rejection occurs before Lua join, multiplayer initialization, placement and reload, including alternate room/signaling paths; then verify a permitted retry.
- [ ] Port per-guest challenge, party identity, baseline, durable intent acknowledgement and departure closure.
- [ ] Verify first playable entry, death/loading continuity and exclusion of pre-entry/post-exit events on Windows copied profiles.

**Exit:** positive native rejection/acceptance evidence and regression coverage before enabling managed admission.

### 6. Connect additive progress and necessary-item application

**Files:** persisted inventory/save codec, event/reward callers, catalog, apply/receipt coordinator and migrated tests.

- [ ] Resolve the persisted inventory codec and native grant/save completion path; live memory changes are insufficient.
- [ ] Reuse the existing boss/key catalog as a candidate list, validating each event/item mapping against the new integration. Document exact grants and no-duplicate behavior.
- [ ] Apply only eligible shared boss/story progress, preserving guest progress and conflicting choices. Review NPC cases separately.
- [ ] Verify selected-save durable receipts and interrupted replay before retiring pending grants.

**Exit:** Windows-backed application and recovery evidence; catalog fixtures alone do not prove permanent progress works.

### 7. Complete safe return and transitions

**Files:** native return/warp hooks, recovery journal, area support policy and corresponding tests.

- [ ] Capture origin plus previously unlocked fallback before admission. Verify safe exact-point return, fallback and failed-validation behavior.
- [ ] Cover guest Blank, host Blank, disconnect, process termination/restart and unsupported destination handling.
- [ ] Verify character/slot changes and NG transitions cannot consume a foreign return record. Keep recovery fences until durable completion.

**Exit:** native Windows return/recovery evidence on copied saves. Higher NG testing remains an explicit gap until a suitable test profile exists.

### 8. Review readiness and stop before packaging

**Files:** Windows build registration, migrated tests, BBSC documentation, licensing/credits and release checklist.

- [ ] Build the integrated Windows tree and run focused migrated module/runtime tests. Record actual results and remaining gameplay gaps without importing Linux PASS labels.
- [ ] Review the patch against upstream: only BBSC and necessary integration changes; retain upstream credits and applicable licenses.
- [ ] Check future launcher/data/DLL/config paths against upstream packaging layout without producing a package.
- [ ] Update feature status, user test instructions and recovery limitations. Confirm required features are wired into active paths, not only present behind disabled hooks.
- [ ] Commit and push a reviewed before-packaging checkpoint to `origin/windows`.

**Exit:** a documented Windows build with complete required integration and explicit test gaps, ready for a separately authorized packaging step. Do not run `packaging/windows/package.sh`, create AppImages, or claim an EXE release in this plan.

## Dependencies and evidence policy

`1 → 2 → 3 → 4 → 5`; `4 + 5 → 6`; `4 + 5 + 6 → 7`; all preceding steps → `8`.
Read-only catalog investigation can run independently, but native adapter work must have one owner at a time. Broad gameplay coverage may follow later; save integrity, admission and recovery checks remain completion gates. Preserve historical Linux evidence with its original platform and pin instead of relabeling it as Windows validation.
