# BBSC development entry point

For development, setup, migration or resuming this repository, read [the Windows handoff](docs/handoff/WINDOWS-RESUME.md) first, then the authoritative plan linked there.

Active direction: Windows only, Supermedo/bloodborne_pc upstream, BBSC-focused changes. The `windows` branch currently contains the historical implementation plus migration documentation; it is not yet a port to that upstream. Preserve `pre` as history. Commit/push checkpoints to `origin/windows`.

## Required plan and progress workflow

These rules apply to every development, migration, setup, bug-fix, test and review task in this repository, including work delegated to subagents and work resumed in a new session.

Before making changes:

1. Read the [Windows handoff](docs/handoff/WINDOWS-RESUME.md), [Windows direction plan](docs/superpowers/plans/2026-10-21-bbsc-windows-only-migration.md), [detailed execution plan](docs/superpowers/plans/2026-10-21-bbsc-windows-execution.md) and [progress tracker](docs/WINDOWS-PROGRESS.md).
2. Identify the task ID, dependencies, acceptance checks and relevant approved specification. Resume from recorded evidence and commits; do not repeat completed work or assume an unchecked task is complete.
3. Mark the active task `IN_PROGRESS` in the tracker. Delegated work must name its parent task ID, file ownership and expected evidence. Keep one owner for shared native-adapter changes.

During work and at every meaningful checkpoint:

- Follow the plan's order, scope, pinned upstream and acceptance criteria. Product behavior follows the approved specification; platform/scope follows the Windows direction plan; the detailed plan defines execution tasks. The tracker records facts, not replacement requirements.
- Update `docs/WINDOWS-PROGRESS.md` as work advances, fails, changes direction or reaches a dependency/gameplay gate. Keep the corresponding plan checkboxes synchronized. Do not defer all progress recording until the end of a session.
- Record the task ID, concrete change, tested source commit (and uncommitted diff state when relevant), actual commands and outcomes, test counts/skips, evidence location, review findings, remaining limitations and next action. Keep private logs, saves, assets and credentials out of tracked documents.
- Use explicit statuses: `READY`, `IN_PROGRESS`, `WAIT_DEP`, `WAIT_GAME`, `VERIFIED` or `BLOCKED`. Distinguish `PASS`, `SKIP`, `NOT RUN` and `BUILD_FAIL`; mark a checkbox complete only after its acceptance checks have evidence.
- If the plan needs adjustment, record the reason, affected task/dependencies and revised checks in the plan and tracker before relying on the change. Routine implementation choices may be resolved and documented within the user's authorized scope; never silently expand product scope or waive a required gate.
- Keep fixture, native Windows and user gameplay evidence separate. Never infer native admission, permanent rewards or recovery correctness from fixtures or old Linux results. Behavioral changes invalidate affected gameplay evidence until retested.

Before ending a development turn, handing off, committing or pushing:

1. Bring the tracker and affected plan checkboxes up to date, even when work is incomplete. Leave a precise resume point and blockers; do not leave a task falsely marked active or verified.
2. Run the checks appropriate to the changes and report actual results. Documentation-only maintenance needs link/content/whitespace verification, not an unrelated game launch.
3. Include the updated plan/progress documents in the relevant checkpoint and commit/push to `origin/windows` as authorized. Preserve `origin/pre`; never force-push or rewrite history. Record any failed publication and distinguish a local checkpoint from a pushed one.
4. Tell the user what changed, what was verified, what remains and where to track it. Do not claim the Windows port or a feature is complete while required tasks or gates remain open.

Current constraints remain in force: managed admission OFF until the required native gate passes; no game launch while the user's no-game instruction is active; no EXE/ZIP packaging or server redeployment in the current development scope. Follow later explicit user instructions when they change that scope or the waiting checkpoint.
