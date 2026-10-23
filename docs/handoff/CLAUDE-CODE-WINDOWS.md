# Claude Code handoff — BBSC 0.0.2 audit

Updated2026-10-23. This is a ready work package for the user's external collaborator, not permission for Codex to spawn agents. Start with [AGENTS.md](../../AGENTS.md), [Windows resume](WINDOWS-RESUME.md), the [alignment plan](../superpowers/plans/2026-10-23-bbport172-bbsc002-alignment.md), then [approved product requirements](../superpowers/specs/2026-10-18-bbsc-anywhere-coop-design.md).

## Ready-to-paste prompt

> Work on task CL01 as the external Claude Code collaborator. Use BBport windows-v1.7.2 at e5d04e2ba4864ccf40de6c3cf6aa532855320b0d and CLSAMG22/BBSC v0.0.2 at4f11faf88b6ba658ebde6fe5fbefdadedba4fa41. Read this handoff and linked constraints. Audit BBSC features, native hooks and authenticated identity/protocol continuity against our approved requirements; produce the three source-cited documents below. Work in your own clone/branch, modify only docs/collaboration/claude/ allowlisted files, keep progress in its PROGRESS.md. Do not modify runtime/CMake/tests/shared plans, deploy servers, launch games, activate managed admission or push windows/integration branches. Return local commit SHA, changed paths, checks, findings and missing evidence. The primary Codex owner integrates results and keeps central progress synchronized.

## Split and ownership

| Owner | Work | Exclusive write scope |
|---|---|---|
| Codex | R01–R04; source baseline/build, W native integration, live diagnostics | All runtime/test/CMake/runner/build/config/entry-point files; shared handoff/plan/progress docs |
| Claude | CL01 reference/protocol/acceptance audit, independently rejectable deliverable | Exactly the four files below |

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

### CL01-A: feature and source map

Write `BBSC-002-MAP.md` with one row per relevant behavior: source commit/path/symbol/line, shipped implementation, correspondence to our requirement, missing delta, actual native owner/thread/side effects, confidence and candidate test.

Required coverage: cross-map same-nonempty-password admission; selected host/current attempt; Kit/Blank normal UI/actions/departures; first playable entry; death/load continuity; boss/story eligibility; additive permanent items/exactly-once receipts; origin/fallback return; disconnect/crash recovery; unsupported/Chalice areas. Include reference spectating/world-time/scaling behavior as reference facts and flag spec differences rather than silently expanding scope.

Read pinned `bbport/README.txt`, the complete `bbport/bbport-1.7.2-bbsc.patch`, relevant `emulator/src/`, `emulator/scripts/` and `docs/` source; enumerate patch destinations and generated/missing dependencies. Inspect registration/runtime enable defaults. Do not assume the BBport patch contains every emulator feature.

Done: every required row has source evidence or explicit NOT_FOUND/NOT_QUALIFIED plus searched paths. Do not use release notes as sole implementation proof.

### CL01-B: identity/protocol audit (highest integration value)

Write `IDENTITY-PROTOCOL.md`. Trace HTTP login/sign-create/search/claim/delete identities (`UserId`, `CharaId`, `SessionId`, selected tuple), Matching2 account/OnlineId/room/member, signaling connection and endpoint. Identify exact producers/consumers and authentication/trust boundary for each conversion.

Search the pinned repository for actual broker/Matching2 server implementation. Initial tree inspection found `server/gameserver/` Python sources but no server C++/proto paths there; verify what is present, externally referenced, binary-only or absent. A documented `cmd_matching.cpp` name or old research note does not mean the deployed source is available. BBSC0.0.2 source, reference Python stub and user's deployed private services are distinct evidence classes.

Cover guest invite vs claim direction, private proof/HMAC support, expiration/retry, host/password change, stale replies, cleanup/shutdown, relay/direct endpoint identity and cross-play version requirements. State exactly whether a selected-character-to-native-owner binding exists; equal values/name/endpoint are observations, not authenticated equivalence. If absent, provide minimum required evidence/fields and trust properties; do not fabricate a bridge or propose deployment as already authorized.

Done: source-cited positive chain or precise gap report, negative cases and implementation boundaries; no server URL/version question to user (already unavailable). Do not contact the user's private services/accounts.

### CL01-C: acceptance cases and handback

Write `ACCEPTANCE-CASES.md`: ID, parent W/G gate, setup/input, real production boundary to exercise, expected allowed/rejected behavior, no-side-effect assertions, cleanup/retry condition, evidence type and blocker. Cover both success and stale/ambiguous/failure cases. Separate source/fixtures/native/gameplay; do not invent stable offsets/APIs or report unrun tests as PASS. These are executable-test specifications, not permission to modify tests.

Review your own documents once for unsupported claims and scope. Check local Markdown links, UTF-8 and `git diff --check`; commit only the four allowlisted paths. Mark child VERIFIED only when documents/checks are complete, not when the game feature is qualified; use WAIT_DEP for unavailable inputs and record useful completed findings. Main owner decides source integration and gate status.

Return: branch/base/commit SHAs; exact changed paths; checks/actual results; five most useful findings with source links; unverified assumptions; next integration action. Local commits or a patch are enough. Do not push `windows`, `pre` or either integration branch. Do not cherry-pick or merge your own work into the user's shared runtime. If remote publication of your personal branch is needed, coordinate with the human.

## Stop and ownership changes

Need another file? Record the requested path/change/reason in your progress file and hand it back; keep auditing within scope. A future implementation task needs an updated explicit ownership contract before touching source. No concurrent local game/server/account activity, EXE/ZIP packaging, secrets/saves in Git, history rewrite or auto-enable admission. If usage can be inspected and either remaining window is below10%, stop at a coherent checkpoint, update child tracker and commit owned files; report unavailable usage honestly.
