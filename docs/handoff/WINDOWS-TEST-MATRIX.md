# BBSC Windows test matrix

Audit date: 2026-10-21. Historical source: `ea8ac53801e49b5dc79c072971c0e05efcd7651f`; inspected documentation tree: `781a02788e0408f07c2870f3cb1d18369a8c7b77`. Upstream comparison/build pin: `de2e38af055f09f819946951075c6bb119e62ae6` in `C:\Users\numbe\.codex\worktrees\bbsc-windows-baseline\bloodborne_pc`. The tables below began as static analysis. Subsequent actual Windows results are recorded in the execution section and [progress tracker](../WINDOWS-PROGRESS.md). No fixture or source-level claim establishes native hook validity.

The migration plan requires a Windows baseline first, then ported BBSC tests; Linux results remain historical evidence only (plan steps 1, 2, and 8). Upstream does not yet contain the historical BBSC source/test files or their CMake targets. The Windows build guide also explicitly says the existing `tests/` programs are not ported, with several using Linux-only APIs ([upstream Windows README](https://github.com/Supermedo/bloodborne_pc/blob/de2e38af055f09f819946951075c6bb119e62ae6/packaging/windows/README.md#L71-L77)).

## Executed Windows preflight (no game)

- Six upstream Python modules (`test_patches`, `test_prepare`, `test_link_libc`, `test_fov`, `test_input_defaults`, `test_trophies`): 52 total, 51 passed, one skipped real-ELF prerequisite, exit 0 with project-local TEMP/TMP.
- `command-buffer-test`, `motion-history-test`, `ui-composition-test`, `prompt-icons-test`: native build and run exit 0, assertions enabled.
- Four other renderer test targets fail to compile at shared `tests/test_gpu_runtime_stubs.cpp:9` (`sigjmp_buf` undefined); `upscaler-support-test` fails on `setenv`/`unsetenv` and also carries a `/tmp` fixture assumption. These five executions remain NOT RUN; W02 of the detailed plan owns adaptation.
- Historical `test_bbsc_character_ids.py`: eight passed, exit 0 with native loopback permitted. Initial sandbox-only failure was WinError 10013; no private/public network was used.
- Historical `test_bbsc_party_events.cpp` plus `bbsc_party.cpp`: direct native CLANG64 compile and run exit 0; output confirms 22-event catalog and unique-item/endgame fixture policies. This has not registered a target in the upstream tree.

Reproduction from PowerShell (use the explicit respective checkout as current directory):

```powershell
# Upstream checkout; TEMP/TMP must be a writable private test directory.
$env:PYTHONDONTWRITEBYTECODE='1'
$env:PYTHONPATH=(Join-Path (Get-Location) 'tests')
& C:\msys64\clang64\bin\python.exe -m unittest -v test_patches test_prepare test_link_libc test_fov test_input_defaults test_trophies

# Historical documentation/runtime checkout; use its own tests/paths.py.
$env:PYTHONPATH=(Join-Path (Get-Location) 'tests')
& C:\msys64\clang64\bin\python.exe -m unittest discover -s tests -p test_bbsc_character_ids.py -v
```

Reproduction of catalog fixture in CLANG64 at the historical checkout:

```bash
clang++ -std=c++23 -O0 -g -UNDEBUG -Wall -Wextra -Werror -Igpu/bbnet/include -Igpu/bbnet/shadps4 -Igpu tests/test_bbsc_party_events.cpp gpu/bbnet/shadps4/core/bbsc_party.cpp -o .superpowers/bbsc-party-events-windows.exe
.superpowers/bbsc-party-events-windows.exe
```

Private command logs and environment-failure details are listed in the tracker. These results qualify only their selected fixtures and pin; they do not establish that the BBSC runtime is integrated on Windows.

## Historical BBSC test sources

“Unchanged” means the test source itself has no OS-specific API preventing a Windows CLANG64 run; target integration may still be needed. “Portable, adapt” retains assertions but changes OS fixtures, wiring or seams. “Linux-only replacement” replaces platform-specific kernel/executable-memory tests while preserving product-level assertions. Unless noted below, historical tests remain NOT RUN.

| Source | Classification | Evidence and Windows implications | Status |
|---|---|---|---|
| `tests/test_bbsc_character_ids.py` | Unchanged (Python test source) | Uses Python stdlib temp directories, SQLite, subprocess, threads, and an ephemeral loopback HTTP server (imports at lines 1–17; setup at 20–29; HTTP coverage at 104–140). No game or Linux API is required. Requires `scripts/bbsc_gameserver/bbsc_gameserver.py`. | PASS: 8, native loopback |
| `tests/test_bbsc_party_events.cpp` | Unchanged (portable C++ test source) | Includes catalog API/std headers; entry at line 60. Historical target links `bbsc_party.cpp` and historical include roots (CMake 85–89); upstream registration still required. | Direct CLANG64 BUILD/RUN PASS |
| `tests/test_bbsc_party.cpp` | Portable, adapt | Domain assertions use `bbsc_party.h`; fixture identities hard-code `/tmp/...` at lines 49, 74, 76, 95, 102, and 150. Historical target combines it with the journal test in one executable (CMake lines 48–54), preventing a Windows run without porting the journal harness or splitting targets. Replace fixtures with temporary Windows paths and preserve path/selected-save assertions. | NOT RUN |
| `tests/test_bbsc_party_journal.cpp` | Portable, adapt (POSIX interruption/durability harness) | Includes `sys/syscall.h`, `sys/wait.h`, and `unistd.h` (lines 13–15); direct `fsync`, `write`, `renameat`, and `close` syscall wrappers at lines 32–47; fork-based lock/crash cases at 389–395; `mkdtemp` and `/tmp` at 809–810. It also links OpenSSL SHA and zlib (lines 12, 16; CMake 53). Port locking, flush/atomic-replace, process-interruption, and temp-directory mechanics to Win32/Windows filesystem semantics; keep the durability/recovery assertions. The historical implementation has some conditional Windows code, but that does not port this POSIX-only test harness. | NOT RUN |
| `tests/test_bbsc_join.cpp` | Linux-only replacement for native executable-hook cases; portable behavior assertions may be retained | Includes `sys/mman.h` and `unistd.h` (lines 6–7), uses `mkdtemp`/`/tmp` (113, 260–261, 1192–1193), and creates writable/executable mappings (479, 728, 905, 1858) plus anonymous mappings (2664). It includes the full historical `bloodborne_re.cpp` (line 3); its test description says it exercises actual placement cache/game-thread state (line 2). The target also links historical hook/decoder code and GNU section/linker flags (CMake lines 56–62). Split pure protocol/state checks from Windows-native integration. Replace mappings with Windows allocation/protection APIs for native code tests only after supported-build offsets/calling conventions are independently validated; a passing fixture cannot validate native hooks. | NOT RUN |
| `tests/test_bbsc_matching2_admission.cpp` | Portable, adapt (integration seam) | Uses C++ atomics, condition variables, mutexes, threads (lines 9–15), and includes the Matching2 implementation directly (lines 2–7); no direct POSIX calls appear in the test. Historical registration is Linux-only with GNU section/linker options and pthread/protobuf dependencies (CMake lines 72–83). Rewire to upstream's actual Matching2 sources, Windows-compatible thread/dependency targets, and callbacks, then preserve admission assertions. | NOT RUN |
| `tests/test_bbsc_http_poll.cpp` | Portable, adapt (path and source seam) | Includes the historical HTTP implementation directly (line 2) and provides fake services (lines 6–22); the selected profile fake is hard-coded to `/tmp/bbsc-http-poll-no-profile` (line 19). No direct Linux syscall is visible in this test. Port the path fixture and wire against the upstream HTTP implementation/callers; its historical target uses GNU dead-section flags and pthread/OpenSSL (CMake lines 91–96). | NOT RUN |
| `tests/test_bbsc_party_transport.cpp` | Portable, adapt (source seam) | Includes historical signaling helpers directly (line 2) and supplies stubs (lines 11–22); no direct Linux syscall appears in the test. Historical target uses GNU section/linker flags and pthread (CMake lines 64–70). Rewire against the upstream signaling implementation, Win32 socket dependencies, and actual static BBSC interface. | NOT RUN |
| `tests/test_bbsc_memory.cpp` | Linux-only replacement | Requires `<linux/fs.h>`, `sys/ioctl.h`, `sys/mman.h`, `sys/syscall.h`, and `unistd.h` (lines 8–12); probes `PROCMAP_QUERY`/`SYS_ioctl` (18–35, 78–89) and `/proc/self/maps` (80–85). Historical target is explicitly `if (NOT WIN32)` and links runtime memory code (CMake lines 241–249). Replace proc-map/ioctl assertions with Windows `VirtualQuery`/memory-query behavior and test Windows native allocation/read seams. | NOT RUN |
| `tests/test_bbsc_memscan.cpp` | Linux-only replacement for process-read/map enumeration | Requires `sys/mman.h`, `sys/uio.h`, and `unistd.h` (lines 8–10); wraps `process_vm_readv`/`pread` (18–25), maps test pages (45, 72), and names `/proc/self/maps` in a regression comment (75). Registration uses GNU `--wrap=process_vm_readv`/`--wrap=pread` (CMake lines 251–255). Replace with Windows `ReadProcessMemory`/`VirtualQuery` or corresponding adapted injection seams; retain scanner boundary/partial-read behavior. | NOT RUN |
| `tests/test_bbsc_memscan_dmabuf.cpp` | Linux-only replacement (hardware-specific) | Explicitly describes a no-game, hardware regression requiring cached dma-buf export (line 2); includes POSIX mmap/uio/unistd (10–12), maps an exported file descriptor (103), and reads `/proc/self/mem` (113–117). Historical target requires Vulkan and is Linux-only (CMake lines 257–263). Replace dma-buf and `/proc` expectations with the Windows Vulkan/shared-memory handle path; document hardware requirements separately. | NOT RUN |

## Historical BBSC build registration

The old project does not register BBSC tests in a root-level CMake file. `gpu/bbnet/CMakeLists.txt` wraps six BBSC executables in `if (NOT WIN32)` (lines 47–97):

- `bbsc-party-test`: combines party and journal tests with `bbsc_party.cpp` and `bbsc_party_journal.cpp`; links ZLIB and OpenSSL Crypto (lines 48–54).
- `bbsc-join-test`: includes join tests plus BBSC party/journal, guest hooks, and decoder; uses GNU section GC and links fmt, pthread, OpenSSL, Zydis, zlib (56–62).
- `bbsc-party-transport-test`: tests party transport with signaling helpers; uses GNU section GC and pthread (64–70).
- `bbsc-matching2-admission-test`: Matching2 test, internal source, generated protobuf; uses GNU section GC, pthread, fmt, protobuf (72–83).
- `bbsc-party-events-test`: event catalog test linked with `bbsc_party.cpp` (85–89).
- `bbsc-http-poll-test`: includes the historical HTTP source; uses GNU section GC, pthread, fmt, OpenSSL (91–96).

`gpu/CMakeLists.txt` separately registers `bbsc-memory-test`, `bbsc-memscan-test`, and `bbsc-memscan-dmabuf-test` inside another `if (NOT WIN32)` block (lines 241–263). These nine C++ target names are omitted on Windows. The Python test is not registered in CMake or the historical `build.sh`; invoke it explicitly if selected. These are not CTest registrations and no standard top-level BBSC test command exists.

## Pinned upstream test registrations and no-game candidates

At the inspected upstream pin, `gpu/CMakeLists.txt` registers nine explicit test executables with `EXCLUDE_FROM_ALL`, beginning at line 289. None launches the game. The nearby no-Vulkan-device comment is too broad: `scene-resolution-test`, `taa-shader-test` and `camera-motion-test` construct a headless `Vulkan::Instance` and require a compatible device/driver; `motion-shader-test` generates SPIR-V for external validation. Targets and source linkage are:

| Target | Source(s) | Notes |
|---|---|---|
| `command-buffer-test` | `tests/test_command_buffers.cpp` | Standalone test (lines 290–292). |
| `motion-history-test` | `tests/test_motion_history.cpp` | Standalone test (294–296). |
| `motion-shader-test` | `tests/test_motion_shaders.cpp`, `tests/test_gpu_runtime_stubs.cpp` | Links `bbgpu` (297–299). |
| `ui-composition-test` | `tests/test_ui_composition.cpp` | Standalone test (300–302). |
| `scene-resolution-test` | `tests/test_scene_resolution.cpp`, `tests/test_gpu_runtime_stubs.cpp` | Links `bbgpu` (304–311). |
| `taa-shader-test` | `tests/test_taa_shader.cpp`, `tests/test_gpu_runtime_stubs.cpp` | Links `bbgpu`, depends on generated host shaders (312–319). |
| `camera-motion-test` | `tests/test_camera_motion.cpp`, `tests/test_gpu_runtime_stubs.cpp` | Links `bbgpu`, depends on generated host shaders (320–324). |
| `prompt-icons-test` | `tests/test_prompt_icons.cpp`, `shim/bbport_prompt_icons.cpp` | Uses bundled font; optional folder argument writes rendered icon files (326–332). |
| `upscaler-support-test` | `tests/test_upscaler_support.cpp`, `shim/bbport_settings.cpp` | Standalone support policy test (334–338). |

These upstream tests are general BBPort coverage, not BBSC evidence. None runs merely because `bash build.sh` is invoked. The commands below describe the intended full group after W02 fixes the five known build failures; they are not a current passing suite. Use separate output folders for shader-generating programs and validate outputs with `spirv-val`.

```bash
ninja -C out/gpu command-buffer-test motion-history-test motion-shader-test ui-composition-test scene-resolution-test taa-shader-test camera-motion-test prompt-icons-test upscaler-support-test
./out/gpu/command-buffer-test.exe
./out/gpu/motion-history-test.exe
./out/gpu/motion-shader-test.exe
./out/gpu/ui-composition-test.exe
./out/gpu/scene-resolution-test.exe
./out/gpu/taa-shader-test.exe
./out/gpu/camera-motion-test.exe
./out/gpu/prompt-icons-test.exe
./out/gpu/upscaler-support-test.exe
```

The Python BBSC ID test is an additional no-game candidate (but not part of upstream):

```bash
python tests/test_bbsc_character_ids.py
```

Do not treat a successful upstream smoke test as BBSC migration or native-hook validation. The upstream Windows build is configured by the handoff's pinned MSYS2 CLANG64 flow; `packaging/windows/README.md` lines 8–26 documents the shell, dependencies, submodule initialization, and `bash build.sh`. The plan explicitly defers game launch/runtime checks to later stages and forbids claiming native hook/API validity from fixtures alone.

## Recommended Windows-port coverage order

1. Run the Python character-ID suite and add the catalog-only event executable first; they provide Windows-capable no-game checks with the least OS coupling.
2. Restore the protocol/session behavior tests against actual upstream callers (`http_poll`, `matching2_admission`, `party_transport`, `party`) as each integration seam is identified. Keep target wiring separate from assertions where useful.
3. Rebuild the journal and join tests around Windows file flush/replace, locking, process interruption, temporary directories, and executable memory. Treat these as native adapter work, not fixture-only validation.
4. Replace the Linux memory/procfs/dma-buf tests with Windows memory-query, remote-read, executable-protection, and Vulkan handle tests. Record the exact Windows API tested and keep hardware-dependent tests opt-in.

## W02 implementation evidence — 2026-10-21

**Current reverified result:** After an assertion-preserving test-only Hpp ABI shim, valid scene-fixture capacity/sample initialization and camera inline visibility parity, the final all-group run exits0 with28commandPASS/0FAIL/1PythonSKIP/0MISSING. All nine actual build/run pairs, four fresh exact SPIR-V validators and five BBSC fixtures pass; headless scene color/depth/stencil/mip, TAA and camera numerical checks ran with no native SKIP. Native target build and five runner regressions pass. Production flags/runtime source are unchanged. See current tracker for source pin, commands and private logs. The reopening note and earlier results below are historical; they are not used to replace this corrected execution evidence.

**Reopened during W05:** The original PowerShell5 invocation could return before GUI-subsystem test executables ended and reuse a stale exit code. The earlier nine-target RUN PASS claim below is invalid for the three headless executions. The corrected runner waits and has five passing runner regressions, including a delayed GUI exit7 case. At local integration checkpoint `a9a61df`, the combined run records actual `0xc0000005` exits for scene-resolution-test, taa-shader-test and camera-motion-test; baseline acceptance is reopened, not skipped. All five BBSC focused executables and exact four fresh SPIR-V validators pass with actual exits. See the tracker for ongoing diagnosis and subsequent evidence; native build remains a separate PASS.

The five preflight BUILD_FAIL results above remain historical at unmodified upstream pin. In the separate integration checkout, `d91f501` plus reviewed W02 test/runner diff fixes them. Full native build exit 0 (365 steps); all nine test targets BUILD/RUN PASS with assertions enabled; four fresh motion shader outputs pass spirv-val for Vulkan 1.3; three headless Vulkan tests pass without a game window. Baseline runner exit 0: 23 command PASS, 1 prerequisite SKIP, 0 FAIL/MISSING. Python: 52 cases, 51 PASS/1 SKIP real ELF prerequisite. Runner failure/missing/prerequisite tests: 3 PASS, exit 0. BBSC group explicitly MISSING/exit 1 until its actual targets are ported. This does not qualify BBSC policy, runtime hooks, persistence or gameplay. Exact commands, private evidence locations, review and next action are in the progress tracker.

## 2026-10-23 current W06 checkpoint recheck

At local request-fence2fd0a7b plus final Sync prerequisite diff, controller full Windows build exits0. Final BBSC group10PASS/0FAIL/0SKIP/0MISSING. Current all-group exits1:32commandPASS/0FAIL/2SKIP/0MISSING, because scene-resolution required format-blit features are unavailable and Python has one missing-real-ELF case (52total/51PASS/1SKIP). Four fresh SPIR-V validations and the other two headless tests pass. Earlier scene PASS is retained as historical evidence; current whole-suite verification is not PASS. See tracker for exact source checkpoint, private logs and open native/server gates. No renderer scope expansion or skipped requirement waiver.
