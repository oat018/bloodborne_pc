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
- Native unmodified upstream baseline build passed: `bash build.sh` exited 0, Ninja completed successfully and produced `out/bb-probe.exe`, `out/bb-gpu-capabilities.exe` and `out/bb-play.exe`. CMake policy/deprecation warnings were non-fatal. No game launch or copied-save compatibility is claimed.
- Private archive locations, external SHA-256 files, original saves and game-data path have not yet been provided on Windows. Do not infer they were backed up from Git, which excludes them.
- No test profile has been restored, no save has been written, and private server connectivity has not been established.
- Managed admission stays disabled. No runtime code migration, server redeployment or EXE/ZIP packaging has been performed.

Next: review the migration manifest and validate copied-profile launch when private files are available. Step 2 still requires launch and portable-test inventory evidence. Follow the ordered migration plan for implementation.

## Baseline invocation

From the separate baseline checkout, with PowerShell environment `MSYSTEM=CLANG64` and `CHERE_INVOKING=1`:

```powershell
& C:\msys64\usr\bin\bash.exe -lc 'bash build.sh > baseline-build-console.log 2>&1'
```

Console evidence: `baseline-build-console.log`; compiler evidence: `out/gpu-build.log`, both private build artifacts in that checkout. The upstream build script applies its own tracked FSR patches to its clean submodule as expected; this is distinct from the unrelated archival patch.
