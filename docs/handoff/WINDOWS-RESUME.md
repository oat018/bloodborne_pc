# Resume BBSC development on Windows

Updated: 2026-10-21 (Asia/Bangkok). This file replaces dependence on the old Codex session, subagents, distrobox and Linux absolute paths.

## Windows checkpoint after migration setup

The Windows machine, pinned upstream build and copied-profile basic gameplay are verified; the historical BBSC runtime is still not integrated on that upstream. Read the [progress tracker](../WINDOWS-PROGRESS.md), [detailed execution plan](../superpowers/plans/2026-10-21-bbsc-windows-execution.md), [test matrix](WINDOWS-TEST-MATRIX.md) and [setup evidence](WINDOWS-SETUP-2026-10-21.md) before work. W01 workspace/history, W03 source map and W04 portable policy remain verified in their stated scope. W02 is reverified after correcting PowerShell GUI waiting and a measured Vulkan-Hpp NDEBUG ABI split, preserving test assertions and fixing invalid scene-fixture setup. Final combined runner exits0:28 command PASS,0FAIL,1Python prerequisite SKIP; all three headless and five BBSC fixtures actually pass. W05 is VERIFIED for filesystem/process/synthetic identity/API observations only; its initial RED is implementation-absence/link evidence, not a retained behavioral RED. Native selected-slot/serializer/grant and G1–G3 gameplay gates remain unpassed. W06 is IN_PROGRESS, with one native/static adapter owner and a separate service-reference fixture owner; actual THD and vendor route coverage is required. Follow the tracker for source commits and exact evidence. User authorized continued offline execution; no game launch, packaging or server redeployment. Managed admission stays OFF and the integration branch stays LOCAL.
The older new-machine instructions below are retained for repeat setup/history; their statements that build/profile availability are unverified describe the original handoff, not the current evidence. Source pins and preservation/activation/packaging constraints still apply.

## Current state and source of truth

- Active branch: `windows`; push target: `origin/windows` in `kaoitp/bloodborne_pc`.
- Primary code upstream: `https://github.com/Supermedo/bloodborne_pc.git`, branch `windows`, inspected pin `de2e38af055f09f819946951075c6bb119e62ae6`.
- Historical runtime checkpoint: `ea8ac53801e49b5dc79c072971c0e05efcd7651f`; historical documentation checkpoint: `a37b419ec03b1286c3d7accd954487de6d6d6429` on `pre`. Direction document first published in `263df6d`.
- Portable policy is ported only in the local `codex/bbsc-windows-integration` checkout; native/runtime integration remains incomplete. Native managed admission is OFF. Permanent progress/item application and Windows native recovery remain incomplete. Old Linux tests and user gameplay confirmations are not Windows validation.
- Follow [the ordered Windows migration plan](../superpowers/plans/2026-10-21-bbsc-windows-only-migration.md). Its platform/upstream rules supersede the older plans; approved product behavior remains unchanged.
- Existing implementation, tests, catalogs, journal/backup/receipt helpers and gameserver sources are tracked. No outstanding subagent result blocks resumption. Source/test inventory is now recorded; W01 has established the separate integration checkout; W03 records verified source owners and blocked native observations without inventing adapter interfaces. Reconcile any later-discovered notes with source and committed evidence.

## Before erasing Linux

Copy both private archives prepared in `/tmp` to an external drive or another machine and verify their SHA-256 against the adjacent `.sha256` files:

- `bbsc-os-migration-private-2026-10-21.tar.gz`: ignored `.superpowers` evidence, Linux test scripts, copied profiles and analysis artifacts, plus the preserved FSR patch.
- `bbsc-original-savedata-private-2026-10-21.tar.gz`: snapshot of `/home/kao/.local/share/bbport/user/savedata/`.

These archives are NOT on GitHub. `/tmp` will not survive replacing the OS. Keep them private; they contain saves and may contain session/network data. Close any game before the final save backup; the prepared snapshot is not a guarantee of a consistent live save. The original Bloodborne CUSA03173 1.09 game directory at `/run/media/system/storage/Bloodborne/CUSA03173/` must also be preserved separately if that disk will be erased. Game assets are not included in the archives or repository.

The old `.superpowers` directory is approximately 2.9 GiB before compression. Restore it privately for historical investigations; its Linux scripts require path/tool adaptation and are not Windows setup scripts. `out/`, caches and installed dependencies are rebuildable and excluded. Credentials, Git authentication, Codex session history and distrobox are not migrated.

## Fresh Windows checkout

Install Git for Windows and MSYS2. In a shell with Git available, choose a short checkout path such as `C:/dev/bloodborne_pc`:

```sh
git clone --branch windows https://github.com/kaoitp/bloodborne_pc.git C:/dev/bloodborne_pc
cd C:/dev/bloodborne_pc
git remote add upstream https://github.com/Supermedo/bloodborne_pc.git
git fetch upstream
git remote set-head upstream -a
git status --short --branch
git log -3 --oneline
```

Remote configuration is local Git metadata: a clone only creates `origin`, so the `upstream` commands above must be repeated on Windows. Configure your Git authentication there; use author `kaoitp <itpanw@gmail.com>` for project commits as already authorized, without assuming global config exists.

Open this checkout as a new Codex project/session. `AGENTS.md` points here automatically. Personal skills and Luna model availability depend on that installation; absent skills or old subagents do not replace the committed plan. Use Luna subagents for substantial independent tasks where available, with disjoint ownership; small edits can be handled by the controller. Check the 5-hour usage window periodically and checkpoint/push `origin/windows` below 10% remaining if that metric is available.

Do not run the historical tree's build script as if migration were complete. First inspect the pinned upstream in a separate clean worktree:

```sh
git worktree add --detach ../bloodborne_pc-upstream-windows de2e38af055f09f819946951075c6bb119e62ae6
```

Use the pinned `packaging/windows/README.md` in that worktree for dependency commands. Native Windows MSYS2 **CLANG64** is the baseline environment. Build/profile evidence and the initial old-to-new file/caller/test/build inventory now exist; do not repeat setup automatically. Keep build scripts away from unrelated FSR work. Use the W01 integration checkout and follow the detailed dependencies; W03 owns unresolved adapter declarations and native caller discovery. Runtime migration and packaging require their own later evidence; do not automatically merge all upstream changes or run `package.sh` now.

## Saves, test profiles and servers

- Use copied saves only. Set an explicit new Windows `BB_USER_DIR` for each test profile; retain the `savedata` directory beneath that user directory. Verify actual selected-save identity and compatibility before allowing writes. Do not copy Linux cache/output directories into Windows.
- Previous tests used EiEi at Cathedral Ward; later identity tests also created/deleted BBSCTrace from Load Game. There is no near-ending/NG+7 test save. Treat profile selection as a fresh check.
- The user can test two game windows on one machine with separate copied profiles/accounts. User performs necessary gameplay tests; provide exact steps and expected outputs at a checkpoint.
- Private servers: shadNet `100.119.93.51`, gameserver `100.86.192.108` (private network connectivity must be established on Windows).
- Unraid templates reported `localhost/bbsc-gameserver:0.6-pre1`, `localhost/bbsc-shadnet:0.6-pre1`, extra parameters `--init --pull=never`; gameserver `/data` maps to `/mnt/app-data/appdata/bbsc/gameserver`. These are historical deployment inputs, not verification of the currently running images. Do not redeploy/reset persistent data during setup.
- Server sources and instructions: [gameserver README](../../scripts/bbsc_gameserver/README.md). Authentication/account tokens must be configured privately on Windows, never pasted into tracked files.

## Unrelated local FSR work preserved separately

The Linux checkout retains a 119-line uncommitted change in `gpu/third_party/fsr-vulkan/src/ffx_vk_fsr4_v07.c`. Its exact diff is preserved as [an archival patch](fsr-vulkan-local-work.patch), with base commit and SHA-256 in [metadata](fsr-vulkan-local-work.json). This patch is not part of BBSC or the Windows baseline. To recover it, use a separate checkout at the recorded submodule base, verify its checksum and `git apply --check` before applying. Do not replace the Windows upstream submodule with it automatically.

## New-session prompt

> Read AGENTS.md, docs/handoff/WINDOWS-RESUME.md and docs/WINDOWS-PROGRESS.md. Resume the detailed Windows execution plan at its first incomplete task (currently W05) when implementation is requested. Use the verified pinned upstream baseline; do not redo completed setup. W03 owns unresolved native caller/interface discovery. Preserve origin/pre and unrelated FSR work. Use gpt-6-luna for substantial independent tasks and stop before packaging. Do not open the game while the user's no-game instruction remains active; G1–G3 are future user gameplay gates. Keep source/evidence gaps explicit and checkpoint/push origin/windows below 10% remaining in the 5-hour window. Do not treat historical BBSC as already migrated.

## Handoff acceptance

Publication is complete only when the handoff, root entry point, migration plan and FSR archival patch are committed and the local HEAD equals `origin/windows` after push. Verify private archive integrity separately and copy those archives outside the machine before OS replacement. Windows tool installation, build, save compatibility and game testing remain unverified until performed on Windows.
