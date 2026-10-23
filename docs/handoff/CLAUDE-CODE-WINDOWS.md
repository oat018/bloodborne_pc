# Claude Code handoff — BBSC 0.0.2 audit

Updated2026-10-23. This is a ready work package for the user's external collaborator, not permission for Codex to spawn agents. Start with [AGENTS.md](../../AGENTS.md), [Windows resume](WINDOWS-RESUME.md), the [alignment plan](../superpowers/plans/2026-10-23-bbport172-bbsc002-alignment.md), then [approved product requirements](../superpowers/specs/2026-10-18-bbsc-anywhere-coop-design.md).

## Clone and start

Run in a new directory on the collaborator's machine:

```powershell
git clone --single-branch --branch windows https://github.com/kaoitp/bloodborne_pc.git bloodborne-cl01
cd bloodborne-cl01
git switch -c claude/bbsc002-reference-audit
```

Open this folder in Claude Code. Its root `CLAUDE.md` points here; paste the prompt below. Read the current tracker before editing, then mark CL01 IN_PROGRESS only in the owned child PROGRESS.md.

Everything required for CL01 instructions, product spec, ownership and progress is tracked on origin/windows. Public reference repositories/commits are listed below. No local-only integration commit, game installation, private account, save, raw diagnostic log, submodule initialization or native build is needed for this audit. The clone's runtime is historical: do not build it as the selected1.7.2 baseline or treat its source as the BBSC0.0.2 reference. Codex's local source baseline is8fdd41b (documentation bced828); those code commits remain LOCAL under the user's existing instruction and are not prerequisites for CL01.

Return the audit commit/patch and allowlisted paths to the human/Codex owner. Do not push this task into windows or the integration branches. Implementation work outside the allowlist requires a separate ownership assignment.

## Ready-to-paste prompt

> Work on task CL01 as the external Claude Code collaborator. Use BBport windows-v1.7.2 at e5d04e2ba4864ccf40de6c3cf6aa532855320b0d and CLSAMG22/BBSC v0.0.2 at4f11faf88b6ba658ebde6fe5fbefdadedba4fa41. Read this handoff and linked constraints. Audit complete baseline composition and existing support for only three goals: cross-map join anywhere supported, earned progress sync and self-hosted services. Identify reuse first and only demonstrated gaps; produce the three source-cited documents below. Do not propose a whole legacy port, new HMAC/backend/save framework or unrelated game-system work. Work in your own clone/branch, modify only docs/collaboration/claude/ allowlisted files, keep progress in its PROGRESS.md. Do not modify runtime/CMake/tests/shared plans, deploy servers, launch games, activate managed admission or push windows/integration branches. Return local commit SHA, changed paths, checks, findings and missing evidence. The primary Codex owner integrates results and keeps central progress synchronized.

## Split and ownership

| Owner | Work | Exclusive write scope |
|---|---|---|
| Codex | R01–R04; source baseline/build, W native integration, live diagnostics | All runtime/test/CMake/runner/build/config/entry-point files; shared handoff/plan/progress docs |
| Claude | CL01 complete-baseline/reuse/three-goal gap audit | Exactly the four files below |

Claude may create/edit only:

- `docs/collaboration/claude/PROGRESS.md`
- `docs/collaboration/claude/BBSC-002-MAP.md`
- `docs/collaboration/claude/IDENTITY-PROTOCOL.md`
- `docs/collaboration/claude/ACCEPTANCE-CASES.md`

Everything else is read-only, including AGENTS.md, CLAUDE.md, WINDOWS-PROGRESS.md, all shared plans, source, tests and current local integration. Proposed runtime/test changes belong as a documented finding in the owned files. Codex must not modify these four files after this initial scaffold until handback. Child tracker is the collaboration-specific progress record; Codex alone mirrors shared status/checkboxes to avoid merge collisions.

## Checkout and inputs

1. Clone the user's `kaoitp/bloodborne_pc` repository on branch `windows` into a separate directory, fetch current documentation, and create your own local branch (suggested `claude/bbsc002-reference-audit`). Do not switch the user's active checkout or use the Codex worktree/build/profile directory.
2. Read all four required Windows documents and this amendment. Old W01–W06 facts refer to another base. Current target is1.7.2, NOT1.9.4; current documentation checkout still contains historical runtime.
3. Obtain independent read-only reference clones of [Supermedo](https://github.com/Supermedo/bloodborne_pc) and [BBSC](https://github.com/CLSAMG22/BBSC), checkout the exact commits above, and record `git rev-parse HEAD`. Do not use moving branch HEAD. A full dependency/submodule installation is unnecessary for this source audit.
4. Reference source/checkouts/logs may live in ignored `.superpowers/references/` in your own clone; never stage them. Existing private logs/accounts/saves from Codex are not an input you need.
5. Set CL01 `IN_PROGRESS` in your PROGRESS.md before work. Parent R01/R04/W03/W06, owner Claude, evidence and next action must remain explicit at each checkpoint.

## Work in order

### CL01-A: complete baseline and three-goal reuse map

Write `BBSC-002-MAP.md`. First identify the complete0.0.2 BBport client/mod DLL, actual load/configuration path and required source/data/artifacts. The1.7.2 network patch explicitly has no-op gameplay callbacks; explain what supplies gameplay in the unchanged release before proposing new code. Source/load instructions must come from pinned source/package references; do not invent a new loader.

Then give one row for each goal: cross-map join/host placement on supported maps/Dream, eligible earned boss/story progress and save/reload, and self-hosted service configuration. Columns: source commit/path/symbol/line, existing behavior, EXISTING/PASS vs PRESENT-NOT-TESTED vs REPRODUCED-GAP vs UNSUPPORTED, required delta if any, responsible existing caller, smallest relevant test. Source presence alone is PRESENT-NOT-TESTED, not runtime PASS.

Include only correctness needed for those goals: password/current selected peer; first playable/exit eligibility; own-progress and relevant rewards non-duplication; safe return/disconnect. Preserve baseline Kit/Blank/scaling/spectate/world-time/UI/input/NPC systems; no independent upgrade/audit of their internals unless a concrete three-goal gap points there. Chalice remains unsupported. No whole-game quest/item catalogue or generic persistence/NG project.

Read pinned `bbport/README.txt`, the full BBport patch, relevant mod source/load configuration and only the relevant release docs. Use existing implementation first. Done: actual composition requirements plus three-goal source/reuse matrix and explicit missing/unrun evidence. Release notes alone are not implementation proof.

### CL01-B: self-host route and changed-boundary protocol findings

Write `IDENTITY-PROTOCOL.md`. Map existing self-hosted game API, account/Matching2, summon broker and optional relay/client configuration; identify what components the release already supplies and whether a required goal route depends on a public service. Recommend reuse/configuration first. No public-server, THD/bbhost or cross-emulator compatibility development; no new account/OAuth/backend design or deployment.

Trace selected user/character/session/room/peer identity and stale-attempt handling only as needed to explain a candidate changed join/progress boundary. Preserve baseline validation and authentication. If an actual gap makes a new association necessary, show producer/consumer/trust evidence or state its absence; never infer authentication from equal numbers/endpoints. Optional HMAC/capability support is a fact to record, not a prerequisite for unchanged baseline reproduction or an automatic new protocol task.

Check whether pinned server source/configuration exists or is referenced/binary-only. Reference release and user-deployed service are distinct evidence classes. Unknown deployed source/version is already recorded; do not repeat the user question or let it block unrelated composition findings. Do not contact private services/accounts. Done: existing self-host path and only relevant, source-cited boundary gaps with minimum evidence needed to resolve them.

### CL01-C: acceptance cases and handback

Write `ACCEPTANCE-CASES.md`: ID, parent W/G gate, setup/input, real production boundary to exercise, expected allowed/rejected behavior, no-side-effect assertions, cleanup/retry condition, evidence type and blocker. Limit cases to the three goals and changed save/return/peer boundaries: valid cross-map/Dream join, wrong/stale candidate/password if affected, earned-event/save-reload/no-duplicate/own-progress, self-host routing, normal exit/disconnect and interrupted-session reload. Add deeper wrong-character/write-failure/retry cases when a proposed delta changes persistence/recovery. Do not prescribe new HMAC, general journal/codec, NG/quest or renderer/input suites. Cover both success and relevant failure cases. Separate source/fixtures/native/gameplay; do not invent stable offsets/APIs or report unrun tests as PASS. These are executable-test specifications, not permission to modify tests.

Review your own documents once for unsupported claims and scope. Check local Markdown links, UTF-8 and `git diff --check`; commit only the four allowlisted paths. Mark child VERIFIED only when documents/checks are complete, not when the game feature is qualified; use WAIT_DEP for unavailable inputs and record useful completed findings. Main owner decides source integration and gate status.

Return: branch/base/commit SHAs; exact changed paths; checks/actual results; five most useful findings with source links; unverified assumptions; next integration action. Local commits or a patch are enough. Do not push `windows`, `pre` or either integration branch. Do not cherry-pick or merge your own work into the user's shared runtime. If remote publication of your personal branch is needed, coordinate with the human.

## Stop and ownership changes

Need another file? Record the requested path/change/reason in your progress file and hand it back; keep auditing within scope. A future implementation task needs an updated explicit ownership contract before touching source. No concurrent local game/server/account activity, EXE/ZIP packaging, secrets/saves in Git, history rewrite or auto-enable admission. If usage can be inspected and either remaining window is below10%, stop at a coherent checkpoint, update child tracker and commit owned files; report unavailable usage honestly.
