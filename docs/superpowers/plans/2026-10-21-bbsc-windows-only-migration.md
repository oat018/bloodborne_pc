# Windows direction — BBport1.7.2 + BBSC0.0.2

Updated2026-10-23 after explicit user scope review. This document controls platform and preservation constraints; execute the [active alignment/delta plan](2026-10-23-bbport172-bbsc002-alignment.md).

## Goals and baseline

- Exactly three goals: seamless cross-map join in supported story maps/Hunter's Dream; jointly earned boss/story progress sync; self-hosted service support.
- Preserve everything else from Supermedo windows-v1.7.2 (`e5d04e2ba4864ccf40de6c3cf6aa532855320b0d`) and CLSAMG22/BBSCv0.0.2 (`4f11faf88b6ba658ebde6fe5fbefdadedba4fa41`). Use the complete BBSC mod and its existing load path, not only its BBport network patch.
- First reproduce baseline behavior on the user's host. New code needs a demonstrated gap, existing owner, minimal fix and focused test. No blanket port of the historical implementation.
- Product correctness follows the [spec's latest scope amendment](../specs/2026-10-18-bbsc-anywhere-coop-design.md): protect own saves/progress, avoid duplicate rewards and preserve safe return. These are outcomes; old custom mechanisms are not required wholesale.
- Chalice remains unsupported. Existing spectating/scaling/world-time/combat/NPC/UI features stay baseline; no independent upgrades or feature rebuilds.

## Execution and preservation

- Windows client development only; keep upstream and BBSC licenses/credits/dependency pins. Do not upgrade pins implicitly.
- Codex owns runtime/build/CMake/runner/shared plans. External Claude follows the [four-file audit allowlist](../../handoff/CLAUDE-CODE-WINDOWS.md), in a separate clone. No automatic subagents.
- Preserve windows/pre history, old local integration, assets/saves and unrelated dirty FSR work. No reset/rebase/force push. Checkpoint documents to origin/windows; source branches remain LOCAL as instructed.
- Copied-profile diagnostics are authorized. Preserve user-controlled sessions and never replace in-use binaries/profiles. Distinct clients require distinct identities/profiles/ports.
- Custom managed admission/progress application stays OFF until its required native gate passes. Baseline testing is separate evidence; ordinary network success does not qualify custom progress or recovery.
- No EXE/ZIP release packaging or server redeployment. Self-hosted support uses existing compatible components/configuration and authorized test service first; not a new server/account system.
- Check usage; below10% remaining stop new work, record and commit/checkpoint. Do not fabricate missing native or runtime evidence.

## Scope audit result

Necessary: complete pinned client/mod composition, minimal build compatibility, self-host configuration, join/progress/exit verification and demonstrated goal fixes.
Unscheduled: public server/THD/multi-backend or cross-play development, custom HMAC/capability as an automatic prerequisite, general journal/save/NG/quest frameworks, renderer/FSR/input/launcher improvements. Retain existing authentication/validation and safety; a failing scoped case can require a minimal corrective change.

The earlier detailed migration checklist is [archived at the reviewed documentation checkpoint](https://github.com/kaoitp/bloodborne_pc/blob/8cae08bc1884136e493fc107a64b1244794ec597/docs/superpowers/plans/2026-10-21-bbsc-windows-only-migration.md). Its completed work/evidence remains history, not automatic new work. Current tasks/statuses are in the [tracker](../../WINDOWS-PROGRESS.md).
