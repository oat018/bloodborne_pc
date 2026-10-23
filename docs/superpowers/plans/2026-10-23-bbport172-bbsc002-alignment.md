# BBport 1.7.2 + BBSC 0.0.2 alignment plan

> Execute inline with superpowers:executing-plans. The external Claude Code collaborator follows the separate handoff; do not spawn agents on their behalf.

**Goal:** Establish the exact BBSC 0.0.2 BBport baseline, then port the approved project additions with separate ownership and fresh Windows evidence.
**Architecture:** Preserve the current integration checkout as history. Establish a separate BBport 1.7.2 baseline with the pinned BBSC patch; map existing policy/storage/native work onto its actual source layout. Codex owns runtime integration; Claude owns an independent reference/protocol audit and test specification.
**Tech Stack:** Windows x64, MSYS2 CLANG64, the dependencies/build scripts of the pinned release; record actual versions when rebuilding.
**Spec:** [Approved product requirements](../specs/2026-10-18-bbsc-anywhere-coop-design.md), [Windows direction](2026-10-21-bbsc-windows-only-migration.md), latest user decision 2026-10-23: BBport1.7.2 replaces the briefly proposed1.9.4 target; BBSCv0.0.2 remains selected.

## Authoritative pins

| Role | Exact reference | Commit / digest |
|---|---|---|
| Game/runtime baseline | [Supermedo windows-v1.7.2](https://github.com/Supermedo/bloodborne_pc/releases/tag/windows-v1.7.2) | `e5d04e2ba4864ccf40de6c3cf6aa532855320b0d` |
| BBSC implementation reference | [CLSAMG22 v0.0.2](https://github.com/CLSAMG22/BBSC/releases/tag/v0.0.2) | `4f11faf88b6ba658ebde6fe5fbefdadedba4fa41` |
| BBSC patch input | [bbport/bbport-1.7.2-bbsc.patch](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/bbport/bbport-1.7.2-bbsc.patch) | SHA256 `e34c79b388605998b430ec4eed556fc2cb8c8928f1d3e9f26bf5d2df29732aa3` |
| Previous integration reference | LOCAL `codex/bbsc-windows-integration`, runtime checkpoint `e6670fd7f3fcf40b1051766cb245a99e9b4af0db` | Old upstream `de2e38af055f09f819946951075c6bb119e62ae6`; later commits are documents only |
| Historical product implementation | `ea8ac53801e49b5dc79c072971c0e05efcd7651f` | Comparison/history only |

Verification: `git ls-remote` resolved both release tags to the commits above; BBSC pinned `bbport/README.txt` independently names e5d04e2 + the patch. Its patch has15 destination-file sections. The release is marked prerelease; its notes require all session participants on0.0.2. This does not establish the version of the user's deployed private service. No release EXE/ZIP was installed or built this turn.

## Constraints and evidence boundary

- This amendment supersedes old active source pins and source-layout assumptions in the October21 execution plan; that plan's product gates and ordered W tasks remain, subject to requalification below. Version1.9.4 is superseded, not an active alternative.
- BBSC0.0.2 is the implementation/compatibility reference; retain approved custom persistence/admission/recovery requirements. Audit mismatches explicitly rather than silently dropping requirements or assuming every new reference feature is already integrated.
- Preserve `windows`, `pre`, old integration branch, old saves and unrelated FSR dirt. No reset/rebase/force push. New integration branch must remain LOCAL unless the user changes that instruction.
- No packaging or server redeployment. Building development executables for verification is permitted. Managed admission remains OFF until the relevant native gate passes; do not silently run a reference build's default-on gameplay hooks against original saves.
- User permits game diagnostics on copied profiles. A currently running old-build game is historical diagnostic activity; never call it1.7.2 evidence. Do not terminate a user-controlled in-world session or replace an in-use binary/profile to switch bases.
- Main historical checkout is the documentation controller, not the new runtime tree. Never compile it as the target baseline.
- Source audit confirms1.7.2 owns network libraries at `gpu/shadps4/core/libraries/`, static glue at `gpu/bbnet/bbnet_glue.cpp`; old `gpu/bbnet/vendor/shadps4/` and `thd/` contracts cannot simply be transplanted. Remap actual native owners first.
- Existing33PASS/native build/private-login evidence is valid only for its recorded old tree. It is NOT RUN on the new target. Existing W06 queue/verification2-of4 is historical, not a fresh target acceptance count.
- One owner for runtime/CMake/test-runner/shared plans: Codex. External collaborator writes only their owned audit files; no shared runtime edits.

## Tasks and completion criteria

### R01 — Release pins and ownership (Codex)

- [x] Verify tag commits and BBSC source recipe, record patch digest and old evidence boundary.
- [x] Publish this plan, [Claude handoff](../../handoff/CLAUDE-CODE-WINDOWS.md), root Claude entry point and child tracker.
Publication: commit/push documentation checkpoints to origin/windows and report the actual Git outcome; code branches remain LOCAL.

### R02 — Separate source baseline and patch qualification (Codex; READY after R01)

Files: new isolated integration checkout; upstream `gpu/bbnet/bbnet_glue.cpp`, `gpu/shadps4/core/libraries/network/`, `gpu/shadps4/core/libraries/np/`, `gpu/shim/core/bloodborne_re.h` only as changed by the pinned patch.

- [x] Read using-git-worktrees and list existing attachments; create a separate LOCAL integration checkout at exact e5d04e2. Proposed branch `codex/bbsc-bbport172`; do not reuse/reset the old integration tree. Record actual branch/path/SHA.
- [x] Obtain the patch from pinned4f11faf, verify SHA256, inspect all15 destinations, run `git apply --check` on the clean1.7.2 tree and retain actual exit/output. Stop for a mismatch; never declare patch compatibility from its filename.
- [ ] Review potentially default-on hooks; qualify a diagnostic-safe path with admission OFF before any game launch. Preserve the original patch as an immutable input; record every additional guard/delta separately.
- [x] Apply the reviewed patch in the new checkout and record diff/source/submodule pins. Native/client compatibility is still NOT RUN. Do not copy packaged executables over source outputs.

### R03 — Fresh Windows build and baseline qualification (Codex; scoped build/tests PASS, WAIT_DEP for full qualification)

Files: pinned `build.sh`, `gpu/CMakeLists.txt`, actual baseline tests; isolated build/log/profile outputs. New runner changes only if required by real failures.

- [ ] Inventory target test registrations and native ABI before adapting old runner/stubs; retain assertions. Use native CLANG64, record tools, commands, source SHA and diff.
- [x] Build with `bash build.sh`; classify BUILD_FAIL/PASS from actual waited exit. Initialize only pinned needed dependencies; preserve their licenses and Gitlinks.
- [ ] Run applicable baseline/patch tests; unavailable targets are MISSING/NOT RUN, never carry forward33PASS. Use RED/GREEN for any corrective behavior change.
- [ ] After safe guard review, launch a fresh copied profile; record title/world/account/peer observations separately. A new source/profile requires fresh evidence; no checkpoint closes from old login logs.

### R04 — Reconcile old work and resume W tasks (Codex; WAIT_DEP R02/R03/CL01)

- [ ] Remap W01/W03 source/registration/tick/HTTP/transport/serializer owners onto1.7.2+patch. Review Claude's source-cited findings and evidence gaps.
- [ ] Compare W04/W05 portable helpers/tests and custom product requirements against BBSC0.0.2; selectively port useful deltas. Do not blanket cherry-pick old vendor/THD code or duplicate already provided BBSC behavior.
- [ ] Update the detailed W task file lists/interfaces/tests before implementing revised native code. Re-run affected tests, then requalify W06 guarded consumers and G1–G3 on the new tree. No guessed identity crosswalk or offsets.
- [ ] Keep every task/status/checkbox current; preserve old evidence as scoped history. Only then continue W07–W11 in dependency order.

### CL01 — BBSC reference/protocol audit (Claude Code; READY, parallel with R02/R03)

Parent: R01/R04 and W03/W06. Exact instructions, file allowlist, source refs, deliverable schema and handback checks are in the [Claude handoff](../../handoff/CLAUDE-CODE-WINDOWS.md). Child status is in [Claude progress](../../collaboration/claude/PROGRESS.md); Codex mirrors significant results into the shared tracker.

- [ ] Map reference patch/hook/protocol behavior and gaps against approved requirements.
- [ ] Trace selected HTTP user/character to native account/room/peer, or report absence with exact searched source/commit. A reference source is not proof of deployed server equivalence.
- [ ] Deliver source-cited acceptance cases, findings and local document commits within the allowlist. No runtime edits, deployment or invented server identity mapping.

## Review focus

- Mixed release files or a moved tag: exact commits/digest and recorded provenance must match.
- Default-on reference hooks bypassing managed admission OFF: audit before game launch, prove the guard at actual native side-effect boundaries.
- Old source paths/thread contracts applied to1.7.2: re-derive owners and cancellation before porting.
- Identical-looking account/character numbers confused with authenticated binding: require actual provenance and current attempt/room correlation.
- Two writers editing native/progress files or testing shared saves: separate checkouts/profile/build dirs and exclusive allowlists; reject out-of-scope patches at handback.


**R02/R03 source checkpoint amendment:** Pinned patch check/apply/reverse-check all0 in new LOCAL codex/bbsc-bbport172 at e5d04e2,15 destinations. Review finds BBSC gameplay callback no-ops explicitly delegated to an external mod DLL; patch is insufficient for full BBSC integration. R03 build-only phase may proceed after source/patch checks; diagnostic game launch remains dependent on actual module/admission guard review. No new game launched until that remaining R02 checkbox has evidence. Old loaded e6670fd session stays separate.


**R03 measured checkpoint:** Initial build1 due CRLF parent patch vs LF MSYS submodule, reproduced by exact native Git apply check; two FSR patch working files normalized to LF with tracked content unchanged, check0. Repeated full bash build.sh exit0 (336 stages);3 native command-buffer/motion-history/ui-composition tests actual waited exits0. Python five baseline modules39tests/38PASS/1missing-real-ELF SKIP. Full graphics/ABI/BBSC fixture inventory and target gameplay not qualified, remaining boxes open. Evidence in controller .superpowers/bbport172. No upstream input changes: copied old-profile mouse config enabled with backup, physical input still unverified; two-player test awaits distinct second account and complete compatible guarded clients. User permits unattended handling but no duplicate identity or account creation invented. R02 remaining guard/module item open; R03 full qualification WAIT_DEP.
