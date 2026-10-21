# Windows preparation checkpoint — 2026-10-21

This records fresh-machine preparation, not a completed BBSC port.

- Active checkout: `C:/Projects/bloodborne_pc`, branch `windows`, tracking `origin/windows` at preparation start `30be002`.
- Historical runtime: `ea8ac53801e49b5dc79c072971c0e05efcd7651f`; preserved `origin/pre`.
- Configured source remote: `upstream=https://github.com/Supermedo/bloodborne_pc.git`. Fetched `upstream/windows` equals approved pin `de2e38af055f09f819946951075c6bb119e62ae6`.
- Configured repository-local author: `kaoitp <itpanw@gmail.com>`.
- Separate app-managed baseline checkout: `C:/Users/numbe/.codex/worktrees/bbsc-windows-baseline/bloodborne_pc`, detached at the approved upstream pin. All six upstream submodules initialized successfully. No historical FSR work was applied there.
- MSYS2: `C:/msys64`; CLANG64 shell verified. Clang 23.1.3, CMake 4.4.4, Ninja 1.13.2, Python 3.14.8, Vulkan pkg-config 1.4.363, SDL3 3.4.18.
- Plain PowerShell PATH does not expose the CLANG64 tools. Invoke through `MSYSTEM=CLANG64` and MSYS2 login bash. The sandbox prevents MSYS2 process initialization; the same check succeeds with approved execution outside the sandbox. This is separate from a compiler failure.
- Dependency installation followed the pinned `packaging/windows/README.md`; `pacman -S --needed --noconfirm ...` completed with exit 0 (95 additional packages). An initial mirror 404 recovered through the package mirrors. Baseline build results must be recorded below.

## Source preservation

Git's archived FSR patch bytes hash to `025696793a698f2a6f5915314d269c8b77dd4d9301121933c840596bbf41e6f4`, matching its metadata. Initial Windows working-tree bytes had CRLF conversion and a different checksum. Added an exact-path `-text` attribute and restored the file from the committed blob. The archival content is unchanged.

## Remaining evidence

- [Draft migration manifest](WINDOWS-MIGRATION-MANIFEST.md) captures the initial comparison. Step 1 remains open until its concrete caller/registration mapping is reviewed.
- Native unmodified upstream baseline build passed: `bash build.sh` exited 0, Ninja completed successfully and produced `out/bb-probe.exe`, `out/bb-gpu-capabilities.exe` and `out/bb-play.exe`. CMake policy/deprecation warnings were non-fatal. First launch now reaches the title screen; selected-character and in-world copied-save compatibility remain open.
- Private archives located in `D:/bbport-bbsc-backup`. Both archives' member tables read successfully (savedata: 23 members; migration evidence: 16,860 members). Adjacent external `.sha256` files are absent, so comparison with the pre-migration checksum remains unavailable. Current SHA-256 values are recorded below; they are not proof of agreement with the original machine.
- Restored 18 save/metadata files into `C:/Projects/bloodborne_pc/.superpowers/windows-profiles/baseline-offline`; every copied file's size and SHA-256 matched its archived member. Original archive untouched. Per-file manifest is private beside the profile. Private server connectivity has not been established.
- Managed admission stays disabled. No runtime code migration, server redeployment or EXE/ZIP packaging has been performed.

Next: follow W01 in the [detailed execution plan](../superpowers/plans/2026-10-21-bbsc-windows-execution.md) after the user's next instruction. Source/test inventory and no-game preflight are recorded in the [progress tracker](../WINDOWS-PROGRESS.md) and [test matrix](WINDOWS-TEST-MATRIX.md). Basic copied-profile gameplay is user-confirmed; post-write reload and recovery evidence remain open. No game was opened during the planning pass.

## Baseline invocation

From the separate baseline checkout, with PowerShell environment `MSYSTEM=CLANG64` and `CHERE_INVOKING=1`:

```powershell
& C:\msys64\usr\bin\bash.exe -lc 'bash build.sh > baseline-build-console.log 2>&1'
```

Console evidence: `baseline-build-console.log`; compiler evidence: `out/gpu-build.log`, both private build artifacts in that checkout. The upstream build script applies its own tracked FSR patches to its clean submodule as expected; this is distinct from the unrelated archival patch.

## Game and first launch verification

The user extracted the base game and applied the supplied update using their PKG tool. Confirmed game root: `D:/Games/Bloodborne/CUSA03173`. Its SFO reports `TITLE_ID=CUSA03173`, `APP_VER=01.09` and `INSTALL_DIR_SAVEDATA=CUSA00207`; this last value matches the archived save layout, so no save-directory renaming is required. Upstream `scripts/patches.py:eboot_matches_patches` returned true for the reconstructed executable. Upstream `prepare.py` and module linking completed successfully; game source files were not patched by these preparation scripts.

The native `bb-probe.exe --vulkan-only` smoke test returned exit 0. First game launch reached the rendered Play Online / Play Offline title screen, observed directly. Log confirms `SPRJ0005` mounted as an existing read-only save. GPU reported AMD Radeon RX 9060 XT, 16,304 MiB VRAM, Vulkan 1.4.359. Startup includes non-fatal Vulkan format/extension warnings and missing optional-file probes; reaching the title screen does not establish in-world graphics or save correctness.

Private reproducible launcher: `C:/Projects/bloodborne_pc/.superpowers/launch-windows-baseline.ps1`. It sets explicit game/data/profile/probe paths, `BB_ONLINE=0`, `BB_SOUND_HACK=0`, mods disabled and FPS 30. Generated data lives in `.superpowers/windows-baseline-data`, separately from both the original game and the save archive. Private launch log: `.superpowers/windows-baseline-launch.log`.

User validation (2026-10-21): the user confirmed that all characters appeared. After reopening the same offline copied profile, the user reported normal operation and exited the game. A subsequent process check found no running `bb-probe`. The second launch log contains in-world `m24` asset activity, memory statistics and repeated mounts of the existing `SPRJ0005` save. This establishes user-confirmed basic Windows gameplay with the restored profile. Exact character/location and a post-write reload were not independently captured; interrupted-save recovery and BBSC remain unverified. Do not claim BBSC has been ported from this upstream launch.

Current archive SHA-256 (2026-10-21):

After the user exited, copied-profile files `backup0001`, `userdata0001`, `backup0010` and `userdata0010` differed from their restoration hashes, confirming writes occurred in the test copy. The original savedata archive was rehashed and still matches the value below. A changed file alone does not prove a durable/reloadable save receipt.

```text
ed2e8aefa728bb837cdc8d09f02d4ee5439b5ca02b1a25a2c290be3811089ead  bbsc-original-savedata-private-2026-10-21.tar.gz
af4201622c2e75127b471b3cae6adf3648916ed015fe65741afe97a52b595d96  bbsc-os-migration-private-2026-10-21.tar.gz
```
