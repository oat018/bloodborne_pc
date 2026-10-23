# BBport 1.7.2 + BBSC 0.0.2 alignment plan

> Execute inline with superpowers:executing-plans. The external Claude Code collaborator follows the separate handoff; do not spawn agents on their behalf.

**Goal:** Use BBSC0.0.2 on BBport1.7.2 for cross-map join anywhere in supported story maps/Dream, earned progress sync, and self-hosted servers. Keep all other systems at their selected upstream behavior.
**Architecture:** Assemble and reproduce the complete reference client/mod/server configuration first. Reuse its join, transport, world/progress and recovery code; change only a demonstrated gap in the three goals. Existing local legacy work is optional reference, not a migration backlog.
**Tech Stack:** Pinned Windows BBport and BBSC dependencies/build/load paths; existing self-hosted service. No replacement backend or launcher.
**Spec:** [Product requirements and latest scope amendment](../specs/2026-10-18-bbsc-anywhere-coop-design.md). Latest scope clarification2026-10-23 supersedes earlier mechanism-heavy implementation proposals.

## Scope contract — latest user instruction

| Work | Decision / limit |
|---|---|
| Join anywhere | In scope: existing BBSC bells/join and host placement across supported story maps and Hunter's Dream, preserving same-nonempty-password policy. Use existing native transitions. Chalice remains unsupported as in0.0.2; do not claim literal every engine map. |
| Progress sync | In scope: jointly earned boss/story progress persists for participants after save/reload; preserve own older progress/quest choices and avoid duplicate event rewards. Reuse BBSC first; add only missing verified event/item handling. No whole-save/world-flag copying. |
| Self-hosted | In scope: configure and verify the existing BBSC/shadNet/game-service components against the user's own host. All required join/progress services must have a self-hosted path; no required public BBSC/THD backend. Keep existing accounts/ports/relay behavior where usable. |
| Game and other systems | Preserve BBport1.7.2/BBSC0.0.2 renderer, graphics/FSR, input, launcher/UI, combat, balance/scaling, spectating, world time, NPC/quest mechanics and unrelated online features. Existing features remain; reimplementation/improvement is out of scope. |
| Save/return safety | Necessary correctness for these goals, not a separate platform project. Reuse baseline backup/own-world/return/recovery; test the changed seam. Add minimal persistence only for a demonstrated missing guarantee. |

**Change rule:** Every proposed code change must name (1) one of the three goals, (2) pinned baseline source/reproduction showing the gap, (3) why existing behavior/configuration cannot supply it, (4) the smallest affected producer/consumer and (5) a focused acceptance check. If any item is absent, keep it as a finding, not implementation work. A build/ABI correction is allowed only when required to build or run this exact combination and verified separately.

**Not scheduled:** wholesale legacy W01–W11 port; THD/bbhost or multi-backend qualification; public-server/cross-emulator compatibility development; new account/OAuth/email service; custom HMAC/capability protocol; generic journal/save codec/NG epoch framework; whole-game NPC/item catalogue; renderer/input/launcher upgrades; packaging/redeployment. Adjusting existing test-profile settings to operate the baseline is allowed; redesigning input is not. Existing baseline authentication, validation and save protection must be retained. A concrete failure in a scoped change can justify a minimal fix; an optional mechanism's absence alone is not a failure.

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

- Pins below remain fixed. Preserve old code/history/FSR work and local-only source branches; no reset/rebase/force push or blanket cherry-pick.
- The complete BBSC mod is required; applying its BBport transport patch alone is not full baseline composition. Missing gameplay DLL/load setup is a real dependency, not a reason to rebuild BBSC inside the emulator.
- Preserve existing baseline features. Our custom managed admission/progress application remains OFF until its focused native gates pass. Baseline-as-shipped observations on copied profiles are separate from acceptance of new custom behavior.
- No new protocol/HMAC framework is a prerequisite for reproducing the unchanged baseline. If changed join/progress code needs identity correlation, prove the actual selected peer/character and stale-attempt behavior at that seam using existing authenticated/session mechanisms first; never guess a crosswalk or weaken validation.
- User permits copied-profile diagnostics. Distinct clients require distinct accounts/profiles/ports. Never replace an in-use binary/save, upload credentials or count a duplicate login as two peers.
- Self-hosted capability is required, but this task does not authorize redeployment of the user's live server. Use existing reference configuration and authorized existing test service; record missing source/version honestly.
- Native fixture, baseline gameplay and custom-change gameplay results stay separate. Current build/Python/native checks apply only to their recorded source; full target gameplay still NOT RUN.
- Codex owns source/build/shared documents; external Claude owns four audit documents. No subagents or external messaging. Test only affected behavior plus baseline smoke; no general graphics/input regression project.

## Tasks and completion criteria

### R01 — Release pins and ownership (Codex)

- [x] Verify tag commits and BBSC source recipe, record patch digest and old evidence boundary.
- [x] Publish this plan, [Claude handoff](../../handoff/CLAUDE-CODE-WINDOWS.md), root Claude entry point and child tracker.
Publication: commit/push documentation checkpoints to origin/windows and report the actual Git outcome; code branches remain LOCAL.

### R02 — Complete reference composition (Codex; transport/build portion passed, mod composition WAIT_DEP)

Files: isolated integration checkout; pinned transport patch owns the15 recorded paths under gpu/bbnet/gpu/shadps4/gpu/shim. Identify exact existing mod/load/configuration artifacts from the pinned release before assembly; no new loader or arbitrary native hooks.

- [x] Read using-git-worktrees and list existing attachments; create a separate LOCAL integration checkout at exact e5d04e2. Proposed branch `codex/bbsc-bbport172`; do not reuse/reset the old integration tree. Record actual branch/path/SHA.
- [x] Obtain the patch from pinned4f11faf, verify SHA256, inspect all15 destinations, run `git apply --check` on the clean1.7.2 tree and retain actual exit/output. Stop for a mismatch; never declare patch compatibility from its filename.
- [ ] Identify and assemble the pinned BBSC gameplay mod DLL, its existing loader/configuration and required data with the1.7.2 client. Preserve release behavior; keep our custom extension OFF. Establish copied-profile test boundaries and exact artifact/source provenance. Do not substitute no-op transport callbacks or implement a new mod loader.
- [x] Apply the reviewed patch in the new checkout and record diff/source/submodule pins. Native/client compatibility is still NOT RUN. Do not copy packaged executables over source outputs.

### R03 — Reproduce the complete baseline on self-hosted services (Codex; build/scoped tests passed, gameplay WAIT_DEP R02)

Files: pinned `build.sh`, `gpu/CMakeLists.txt`, actual baseline tests; isolated build/log/profile outputs. New runner changes only if required by real failures.

- [ ] Verify only the native ABI/load/registration boundaries needed by the complete release combination. Record source/artifact hashes and configuration. Do not port the old runner or all old native adapters by default.
- [x] Build with `bash build.sh`; classify BUILD_FAIL/PASS from actual waited exit. Initialize only pinned needed dependencies; preserve their licenses and Gitlinks.
- [ ] Run relevant reference/patch checks and baseline startup. Expand tests only for touched code or a concrete failure; retain assertions and truthful skips. Existing scoped3native/38Python passes are evidence, not full gameplay acceptance.
- [ ] On two distinct copied-profile clients/accounts, verify existing join/placement, disconnect/return, interrupted-session exit/reload and boss/story save/reload on self-hosted gameplay/API/matchmaking/summon/relay services as needed. Record baseline PASS/failure before designing custom changes; no public-server/cross-play matrix required.

### R04 — Implement only demonstrated goal gaps (Codex; WAIT_DEP R02/R03 and relevant CL01 findings)

- [ ] Produce a three-goal delta table: EXISTING/PASS, PRESENT-NOT-TESTED, REPRODUCED-GAP or UNSUPPORTED. For every gap name actual baseline owner, reproduction and smallest change; for existing behavior schedule verification only.
- [ ] For cross-map/password/placement gaps, extend the existing BBSC consumer. Test valid join, wrong/stale candidate/password and failed join without guest-world mutation. Do not add alternate transport or a custom crypto protocol merely because old code has one.
- [ ] For earned boss/story progress gaps, reuse the existing progress/event/save path. Test before-entry/after-exit exclusion, own-progress preservation, save/reload and no duplicate reward; crash/retry/wrong-character cases are mandatory if our change introduces persistent writes/recovery. No general inventory/quest/NG framework.
- [ ] For self-host configuration gaps, reuse the existing server components and client settings. Verify required service routing and peer exchange on the user's host; do not build an account backend or redeploy production as part of this scope.
- [ ] Run focused regression/native/gameplay checks for the actual changes, then repeat the three-goal end-to-end scenario. Preserve baseline Kit/Blank, departure/return and save correctness. Scope-review every changed path before completion.

Legacy W task IDs remain evidence cross-references. They are not an instruction to port all former modules or finish unrelated acceptance matrices. G1 join, G2 earned progress and G3 exit/recovery apply to the behavior actually delivered; no safety acceptance is waived.

### CL01 — Baseline reuse/self-host/three-goal gap audit (Claude Code; READY, parallel with R02/R03)

Parent: R01/R04 and W03/W06. Exact instructions, file allowlist, source refs, deliverable schema and handback checks are in the [Claude handoff](../../handoff/CLAUDE-CODE-WINDOWS.md). Child status is in [Claude progress](../../collaboration/claude/PROGRESS.md); Codex mirrors significant results into the shared tracker.

- [ ] Map complete mod composition and the three goals to existing BBSC implementation; identify what to reuse and what truly fails/misses.
- [ ] Map the existing self-hosted client/service route and identity/session boundaries needed by candidate changed join/progress seams. Do not require new HMAC/public-server/cross-play work; unknown deployed source does not block unrelated baseline composition.
- [ ] Deliver source-cited acceptance cases, findings and local document commits within the allowlist. No runtime edits, deployment or invented server identity mapping.

## Completion checks

1. Two distinct accounts on complete pinned clients can join across supported maps/Dream and place the guest beside the selected host, using the user's self-hosted service path.
2. Jointly earned boss/story progress survives each participant's save/reload; no host's pre-party history import, guest progress rollback or duplicate relevant rewards.
3. Existing departure/disconnect/return and interrupted-session reload remain safe and original saves remain protected; deeper wrong-character/write-failure/retry tests cover custom persistence changes.
4. Other game systems remain baseline behavior. Every changed source path has the five-part goal/gap/minimality justification above. Reused baseline features are not labelled newly implemented.
5. Unimplemented/unsupported areas and unrun tests remain explicit. No claim that1.7.2 transport patch alone delivers the mod.

Historical measured checkpoints follow; their old mechanism/dependency proposals do not override the scope contract above.

**R02/R03 source checkpoint amendment:** Pinned patch check/apply/reverse-check all0 in new LOCAL codex/bbsc-bbport172 at e5d04e2,15 destinations. Review finds BBSC gameplay callback no-ops explicitly delegated to an external mod DLL; patch is insufficient for full BBSC integration. R03 build-only phase may proceed after source/patch checks; diagnostic game launch remains dependent on actual module/admission guard review. No new game launched until that remaining R02 checkbox has evidence. Old loaded e6670fd session stays separate.


**R03 measured checkpoint:** Initial build1 due CRLF parent patch vs LF MSYS submodule, reproduced by exact native Git apply check; two FSR patch working files normalized to LF with tracked content unchanged, check0. Repeated full bash build.sh exit0 (336 stages);3 native command-buffer/motion-history/ui-composition tests actual waited exits0. Python five baseline modules39tests/38PASS/1missing-real-ELF SKIP. Full graphics/ABI/BBSC fixture inventory and target gameplay not qualified, remaining boxes open. Evidence in controller .superpowers/bbport172. No upstream input changes: copied old-profile mouse config enabled with backup, physical input still unverified; two-player test awaits distinct second account and complete compatible guarded clients. User permits unattended handling but no duplicate identity or account creation invented. R02 remaining guard/module item open; R03 full qualification WAIT_DEP.


**SCOPE01 review2026-10-23:** User reconfirmed three goals and baseline preservation. Replaced blanket legacy migration with full reference composition → self-hosted baseline reproduction → demonstrated deltas. Custom HMAC, multi-backend/THD, general storage/NG/quest and unrelated game-system work are unscheduled unless a scoped failing case justifies the smallest fix. No native/source/runtime behavior changed by this review.
