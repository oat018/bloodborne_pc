# BBSC Windows migration manifest

Draft read-only comparison for migration-plan step 1; it is not yet exit-reviewed or complete.

## Pins and scope

| Tree | Commit | Finding |
|---|---|---|
| Historical BBSC runtime | `ea8ac53801e49b5dc79c072971c0e05efcd7651f` | BBSC is integrated under `gpu/bbnet/`, with Linux-port runtime glue and tests. |
| Supermedo Windows upstream | `de2e38af055f09f819946951075c6bb119e62ae6` | Static `bbnet` sources are in `gpu/bbnet/vendor/shadps4/`; integration is registered in `gpu/CMakeLists.txt`. |

Compared using `git ls-tree`, `git show`, and `git grep` at the two pins. The worktree's `git status` cannot complete because Git attempts to enter the FSR submodule and receives Permission denied; this does not affect the pinned-tree comparison. No build was run. The only file changed by this step is this manifest.

## What upstream has and does not have

Upstream has general PSN/networking, Matching2, signaling, parties and P2P transport. Its `gpu/bbnet/SHADNET-PORT.md`, vendor tree, `thd/` implementation and `bbnet_glue.cpp` concern ShadNet/ShadPS4 integration. The pinned tree has no BBSC symbols in `gpu/CMakeLists.txt`, `gpu/bbnet/`, or `tests/`; it has no BBSC game hooks, policy, party persistence, journal, save receipts, Bloodborne-specific catalogs, or BBSC tests. This is not evidence that networking supplies seamless-co-op behavior.

The old BBSC runtime calls into its own forked ShadPS4 code and Linux bbport shim. Preserve BBSC behavior as portable policy where possible, but rebind it to upstream's actual network/runtime callers. Do not transplant the old vendor subtree or `gpu/bbnet/CMakeLists.txt` wholesale.

## File and component disposition

| Historical path/component | Disposition | Existing callers / build / tests | Migration note |
|---|---|---|---|
| `gpu/bbnet/bbsc_glue.cpp`, BBSC additions in `bbnet_glue.cpp` | **Adapt** | Called from old bbnet registration/startup; target linked by `gpu/CMakeLists.txt` in upstream. | Split portable BBSC initialization from OS, renderer/UI, save-root and old-emulator glue. Add a narrow static interface described below. Rebind lifecycle to upstream static `bbnet`; no `dlopen`/module loader. |
| `gpu/bbnet/include/core/bbsc_party.h`, `gpu/bbnet/include/core/bbsc_party_events.h`, `gpu/bbnet/shadps4/core/bbsc_party.cpp`, `gpu/bbnet/shadps4/core/bbsc_party_journal.cpp` | **Reuse/adapt** | Runtime callers in old `shadps4/core/bloodborne_re.cpp` and network hooks; tests `test_bbsc_party.cpp`, `test_bbsc_party_events.cpp`, `test_bbsc_party_journal.cpp`, `test_bbsc_party_transport.cpp`; old `gpu/bbnet/CMakeLists.txt` creates EXCLUDE_FROM_ALL targets. | Header declarations live under `include/core/`; implementations live under `shadps4/core/`. Preserve protocol/state-machine and durable journal semantics as portable units; recompile against upstream C++/filesystem/thread conventions and verify callback ordering. |
| `bbsc_persistence.{h,cpp}` | **Adapt** | Called by old runtime glue and Bloodborne hooks; exercised extensively by `test_bbsc_join.cpp`, `test_bbsc_character_ids.py`; built into old bbnet library. | Keep identity, receipts, backup and journal algorithms, but replace POSIX save discovery/locking/flush/atomic replacement and old `BBSC_SAVE_DIR` derivation with verified Windows profile/save APIs. |
| `bbsc_memscan.{h,cpp}`, `bbsc_patch_tables.h`, `bloodborne_re.cpp`, `bbsc_messages.inc`, `bbsc_cinematic.{h,cpp}` | **Adapt; native offsets deferred** | Reached through old startup/image hooks in `bbnet_glue.cpp`; `test_bbsc_memscan.cpp`, `test_bbsc_memscan_dmabuf.cpp`, `test_bbsc_memory.cpp`, and join tests cover fixtures. | Port scanners/policy only after comparing image/memory abstractions. Every game offset, signature and calling convention needs evidence against CUSA03173 1.09 on the pinned runtime; do not assume historical addresses remain valid. Keep native managed admission off. |
| `gpu/bbnet/include/core/bbsc_party.h`, `bbsc_party_events.h`, `bloodborne_re.h`, `common/memory_patcher.h`, `core/memory.h`, `core/file_format/psf.h`, `core/cpu_patches.h` | **Adapt selectively** | Included by old game/network code; fixture tests compile some headers with shim sources. Upstream owns analogous `gpu/bbnet/include/core/*` types and memory/runtime APIs. | Translate narrow dependencies to upstream types. Avoid replacing upstream compatibility headers; retain only BBSC-owned declarations or minimal adapters. |
| `gpu/bbnet/include/imgui/bbsc_intro_layer.h`, `gpu/bbnet/include/imgui/bbsc_loading_layer.h` | **Defer/adapt** | Included by `bbsc_glue.cpp`; loading layer also included by old `shadps4/core/bloodborne_re.cpp`. UI implementation is supplied by old port's ImGui/UI seam, not a standalone BBSC source in upstream. | Preserve requested Blank/menu/animation/text behavior, but rebind to the pinned upstream UI ownership and lifecycle after identifying its caller. Do not copy Linux-port UI headers as if upstream implemented their interfaces. |
| Historical `gpu/bbnet/shadps4/core/libraries/network/**`, `np/**`, `np_signaling_bbsc.h`, Matching2 and P2P edits | **Superseded as base; selectively adapt BBSC hunks** | Old `bbnet_register` installs NP lookup and network hooks; `test_bbsc_matching2_admission.cpp`, `test_bbsc_http_poll.cpp`, `test_bbsc_join.cpp`, `test_bbsc_party_transport.cpp`. | Upstream owns these APIs under `gpu/bbnet/vendor/shadps4/core/libraries/{net,network,np}/` plus `thd/`. Reapply only BBSC-specific admission/protocol/relay behavior after per-file comparison. Preserve upstream's vendor and source registration. |
| Historical `gpu/bbnet/CMakeLists.txt` | **Superseded** | Registered old shared/dynamic module and test executables; targets are referenced only within this old CMake file. | Upstream has no `gpu/bbnet/CMakeLists.txt`; static source list/dependencies are in `gpu/CMakeLists.txt`. Add selected BBSC sources and opt-in test targets there. |
| `tests/test_bbsc_party.cpp`, `test_bbsc_party_events.cpp`, `test_bbsc_party_journal.cpp`, `test_bbsc_party_transport.cpp` | **Reuse/adapt** | Old EXCLUDE_FROM_ALL CMake targets. | Keep as portable policy/protocol regression fixtures, updating fake transport and compile registration. They do not qualify native runtime behavior. |
| `tests/test_bbsc_join.cpp`, `test_bbsc_matching2_admission.cpp`, `test_bbsc_http_poll.cpp` | **Adapt** | Old `gpu/bbnet/CMakeLists.txt` registration; exercise many old ShadPS4 test seams and environment variables. | Repoint to upstream caller seams; retain rejection-before-entry, correlated cleanup, peer/session binding, expiry/retry and receipt assertions. POSIX environment/filesystem setup needs Windows implementation. |
| `tests/test_bbsc_memory.cpp`, `test_bbsc_memscan.cpp`, `test_bbsc_memscan_dmabuf.cpp`, `tests/test_bbsc_character_ids.py` | **Adapt/defer** | Old CMake registration for C++ tests; Python identity test has no registration in `gpu/bbnet/CMakeLists.txt` (inspect runner before reuse). | Pure buffer/identity fixtures can remain portable. DMA-BUF/Linux mapping assumptions are not Windows evidence; replace with Windows mapping fixtures. Character identity fixtures do not validate live offsets or slot identity. |
| `src/runtime_net.c` | **Superseded as loader; adapt narrow responsibilities** | Old `probe.c` calls `runtime_net_configure` and `runtime_net_image_ready`; `runtime_net.c` uses `dlopen`/`dlsym` for `bbnet_init`, `bbnet_resolve`, recovery entrypoints and network symbol dispatch (e.g. lines 46-121 at historical pin). | Upstream is statically linked and exposes `bbnet_configure`, `bbnet_set_image`, `bbnet_initialize`, `bbnet_register` from `gpu/bbnet/bbnet_glue.cpp`; callers are `src/probe.c` (`bbnet_configure` line 509, `bbnet_set_image` line 681, `bbnet_initialize` line 717) and `gpu/shim/bbgpu.cpp` (`bbnet_register` lines 267-273) at upstream pin. Reuse explicit opt-in/image/profile intent only if supported by upstream configuration; no dynamic-loader transplant. |
| `src/runtime_file.c` | **Adapt save receipt bridge; supersede Linux file semantics** | Old BBSC bridge includes `bbnet.h`; `runtime_file_save_receipt` and receipt snapshots are defined in this file (historical lines 53-96, 150-167), passed into `bbnet_init`/recovery host in `runtime_net.c` lines 81, 121. | This is a concrete runtime-to-BBSC durable-save seam, not just generic file I/O. Map selected-save identity, flush/snapshot receipt and callback into upstream Windows save path only after its concrete writer/close caller is identified; never infer durability from a live-memory grant. |
| `docs/BBSC.md`, historical design/spec and integration evidence | **Reuse as historical reference; update later** | Documentation only. | Product behavior/spec stays authoritative per migration plan. Rewrite platform/build claims only when Windows evidence exists; do not relabel old Linux results. |
| `scripts/bbsc_gameserver/**` | **Reuse/adapt later** | External service; old local-server launcher scripts started it. | Server protocol may remain portable, but Windows launch/configuration is outside step 1. Keep private credentials out of source. |
| Old launcher, `run.sh`, online scripts and Linux shims | **Superseded/deferred** | Old launch scripts selected BBSC and derived profile paths. | Upstream Windows launch/build layout owns configuration. Reintroduce only user-facing BBSC toggles/profile setup that can be mapped to supported Windows paths. |

## Upstream registration and callers

At the upstream pin, `gpu/CMakeLists.txt:189` adds `bbnet/bbnet_glue.cpp` to the static target, and `target_link_libraries(bbgpu PUBLIC bbnet)` links the target (around lines 210-240). `gpu/bbgpu.h:20-24` declares the C entrypoints; `src/probe.c:509,681,717` calls `bbnet_configure`, `bbnet_set_image`, and `bbnet_initialize`; `gpu/shim/bbgpu.cpp:267-273` declares and calls `bbnet_register`. Implementations are in `gpu/bbnet/bbnet_glue.cpp:440-464`. These symbols establish upstream's static integration lifecycle. In particular, `bbnet_initialize` initializes ShadNet runtime/NP state and `bbnet_set_image` forwards the image to `BbThd::SetImage`; neither is a BBSC hook or evidence of admission timing. Upstream's build entry point is root `build.sh`; Windows instructions are in `packaging/windows/README.md`. Its test tree contains generic runtime/GPU tests, but none of the old BBSC tests or targets. Old tests were intentionally excluded from default builds and only selected test targets were registered through old bbnet CMake.

Verified lifecycle mapping: upstream GPU initialization calls `bbnet_register` (`gpu/shim/bbgpu.cpp:267-273`); probe setup calls `bbnet_configure`, then passes the loaded image through `bbnet_set_image`, then calls `bbnet_initialize` (`src/probe.c:509,681,717`). Exact BBSC placement within this lifecycle is still a proposal. Party events/transport may map to upstream `thd/` and P2P transport, but no BBSC callers exist there. Admission must be attached to the concrete Matching2/signaling consumer only after call order proves it can reject before Lua join, multiplayer initialization, placement, or reload. Game/save hooks require supported-image, memory, and Windows save callback evidence. Merely finding analogous upstream APIs is not proof of call order.

## Proposed static interface

Keep BBSC independent of upstream internals behind a small statically linked adapter, tentatively `bbsc::Runtime` (names are a proposal, not existing API):

```cpp
struct Runtime {
    Network& network();       // session-bound send/receive and peer lifecycle
    Game& game();             // verified image identity, game-thread dispatch, safe memory view
    SaveStore& saves();       // selected-profile/save identity and durable atomic operations
    Ui& ui();                 // optional menu/text/animation requests
    Config config();          // opt-in and supported title/version policy
};
void Initialize(Runtime&);
void Shutdown();
AdmissionResult BeforeJoin(const JoinRequest&); // synchronous decision before side effects
```

Portable policy owns party/session state, protocol validation, eligibility, additive progress selection, journal/receipt decisions and recovery intent. Upstream adapter owns transport callbacks, game-thread scheduling, image/memory services, UI request forwarding and save I/O. `BeforeJoin` must be called by the actual admission consumer before Lua join, multiplayer initialization, placement, or reload; the new caller and all alternate paths remain to be proven. Keep APIs narrow and avoid exporting internal upstream C++ types across BBSC policy.

## Selective patch order

1. Establish and record pinned clean Windows upstream baseline per plan step 2; do not mix BBSC code into it.
2. Add portable BBSC policy units (party/session/events, protocol checks, journal/receipt decisions) and migrate their tests; no native hook registration.
3. Add upstream static CMake registration and the BBSC adapter interface; initialize only under explicit BBSC opt-in, preserving default network behavior.
4. Adapt networking in small files/hunks: authenticated private attempt and peer binding; Matching2 admission before side effects; correlated cleanup; existing party transport; departure paths. Retain upstream vendor files and compare every touched hunk against the pinned revision.
5. Port selected-profile durable save primitives and receipts with Windows semantics/tests; keep recovery pending when profile or identity is ambiguous.
6. Add image/memory adapter after native version evidence; separately establish admission rejection and first-playable boundary before enabling managed joins.
7. Integrate additive progress and safe return behind durable receipts/recovery fences; verify before broad runtime enablement.

## Evidence gaps and limits

- No native MSYS2 CLANG64 build or launch has been performed; Windows machine/runner availability, tool versions, dependency installation and profile path selection are not established here.
- No current upstream headers/call graph prove admission ordering or every alternate join path. Confirm directly in pinned source before wiring.
- No upstream image offsets, Bloodborne 1.09 native callbacks, Windows calling convention, executable page-write/cache behavior, game-thread guarantees or save codec/grant completion interface were validated by this comparison. This manifest assigns no offsets.
- Historical Linux fixture/test results and gameplay reports are not Windows evidence. Tests must be ported and run on Windows; live copied-profile behavior still requires user gameplay checks.
- FSR submodule access prevented `git status` from completing. No submodule, runtime, plan or Git metadata was modified.
