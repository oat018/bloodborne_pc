# BBSC Windows integration base and history map

Updated: 2026-10-21 (Asia/Bangkok). W01 source-discovery record. This document records source ancestry and path ownership; it does not claim that BBSC has been ported or integrated.

## Pins and checkout facts

| Role | Commit / branch | Evidence |
|---|---|---|
| Historical BBSC implementation and current controller | `26d74ee802098928165387a40af87a04d03693c9` / `windows` | `git rev-parse HEAD`, `git branch --show-current` in `C:/Projects/bloodborne_pc`. This includes the historical runtime inherited from `ea8ac53801e49b5dc79c072971c0e05efcd7651f` and later documentation commits. |
| Historical source checkpoint | `ea8ac53801e49b5dc79c072971c0e05efcd7651f` | Handoff, direction plan, and migration manifest. |
| Pinned Supermedo upstream | `de2e38af055f09f819946951075c6bb119e62ae6` / `windows` upstream pin | Handoff and direction plan; separately registered baseline worktree is detached at this commit. |
| W01 implementation worktree | `C:/Users/numbe/.codex/worktrees/bbsc-windows-integration/bloodborne_pc`, on `codex/bbsc-windows-integration` at the upstream pin | Controller verified branch creation, clean `git status --short --branch --ignore-submodules`, and exact `HEAD` with approved worktree access. |

The controller `windows` and upstream trees share merge base `0afaaa264993b7afdde8e02a60fd57236234ef24`. The historical checkpoint is an ancestor of controller `HEAD`; controller `HEAD` is not an ancestor of the pinned upstream. The same repository object database has both commit histories, but the trees have substantially diverged. A normal Git merge can preserve both ancestries; it will not itself decide safe file ownership or complete the runtime port.

At the recorded pins, `git ls-tree -r --name-only` counted 1,008 tracked paths in the historical tree and 970 in upstream. A rename-aware tree comparison reported 1,210 path change records and 996 under `gpu/`. This count includes the extensive moved/removed/replaced emulator tree; it is not a count of BBSC files. Use per-file review and the manifest rather than treating these counts as a migration checklist.

## Path ownership map

The governing rule is: use the pinned upstream as the runtime base. Keep its emulator, renderer, launcher, platform, packaging, and vendor implementation. Reintroduce only specifically selected BBSC-owned code and the smallest required integration seams. Do not copy the historical tree wholesale, replace upstream files from the old fork, or treat networking support as BBSC seamless co-op.

| Path/component | Owner in integration tree | W01/W02–W11 handling |
|---|---|---|
| Upstream emulator/runtime (`src/`, `gpu/` except selected BBSC additions, common/core, renderer, Vulkan, shaders, tools) | Supermedo upstream | Retain upstream. All non-BBSC renderer, GPU, emulator, memory, thread, OS, and build changes in the historical tree are out of BBSC migration scope. No bulk cherry-pick or directory overlay. |
| Upstream network implementation (`gpu/bbnet/vendor/shadps4/`, `gpu/bbnet/thd/`, upstream `gpu/bbnet/bbnet_glue.cpp`, `gpu/CMakeLists.txt`) | Supermedo upstream | Retain the static `bbnet` integration and its vendor/runtime contracts. Add BBSC only through narrow, reviewed changes to callers/build registration. Do not transplant old `gpu/bbnet/CMakeLists.txt`, forked ShadPS4 network/NP/Matching2/P2P implementation, or duplicate vendor trees. |
| Portable BBSC protocol/policy and party state (`gpu/bbnet/include/core/bbsc_party*.h`, historical `gpu/bbnet/shadps4/core/bbsc_party*.cpp`) | BBSC addition | Selectively adapt under the plan's `gpu/bbnet/bbsc/` implementation layout and `gpu/bbnet/include/core/` declarations. Retain policy/protocol semantics; reconcile upstream types, C++ conventions, callback order, and static target ownership. |
| BBSC persistence, journal, backup, receipt, identity (`bbsc_persistence.*`, party journal sources) | BBSC addition over upstream platform APIs | Adapt algorithms; bind to verified Windows profile/save identity, locking, flush, atomic replace, and save APIs. Historical POSIX paths and `BBSC_SAVE_DIR` derivation are not portable contracts. |
| BBSC game hooks and scanner (`bloodborne_re.*`, `bbsc_memscan.*`, `bbsc_patch_tables.h`, messages/cinematic) | BBSC-specific source; upstream owns runtime interface | Reintroduce only under W07 after W03 defines exact interfaces. Historical signatures, offsets, ABI, hooks, and thread assumptions are unverified against the pinned upstream and CUSA03173 1.09. Native managed admission remains OFF. |
| BBSC Matching2/HTTP/signaling hunks | Upstream owns the files and APIs; BBSC owns only selected behavioral additions | Reapply individual BBSC hunks after caller and broker-field discovery. Relevant upstream locations include `gpu/bbnet/vendor/shadps4/core/libraries/{net,network,np}/` and `gpu/bbnet/thd/`. Preserve upstream behavior and avoid duplicate declarations. W03/W06 own exact seams. |
| Historical `gpu/bbnet/shadps4/core/libraries/network/p2p_sockets.cpp`, `sockets.h` | Upstream runtime owns its network stack | No same-name upstream counterpart at the pinned tree. Do not restore the old files as a bridge; identify actual upstream transport callers and implement only a necessary BBSC seam. |
| BBSC intro/loading UI headers and old UI calls | Upstream owns UI and lifecycle | Defer until actual caller/lifetime is identified. Preserve the approved Blank/menu/animation/text behavior through upstream interfaces; historical ImGui headers do not establish those interfaces. |
| BBSC tests (`tests/test_bbsc_*.cpp`, `tests/test_bbsc_character_ids.py`) | BBSC addition, tied to actual implementation | Port selectively with real targets/dependencies. Fixtures do not prove native ABI, admission, persistence, rewards, or gameplay. The gameserver identity test remains separate from client save identity. |
| Gameserver (`scripts/bbsc_gameserver/`) | BBSC service source | Retain only if required by the approved service path; no server redeploy or reset in this work. Broker/server changes require evidence the pinned client protocol cannot meet requirements and a separately reviewed scope. |
| Historical documentation, migration plans, test matrix, handoff, setup evidence | Project documentation | Preserve and update only by explicit plan/task ownership. Keep product specification, upstream/platform direction, detailed execution plan, and factual progress tracker roles distinct. |
| Historical launcher, `run.py`/`run.sh`, Linux/AppImage/Nix build and packaging, broad runtime/shim changes | Upstream owns replacement runtime/release paths | Do not restore these as BBSC port work. Packaging remains out of scope; no EXE/ZIP packaging. |
| `gpu/third_party/fsr-vulkan` and historical local FSR patch | Existing separate local/submodule work | Preserve its pinned submodule state and archival patch. Do not apply the local FSR patch or change its submodule while integrating BBSC. |

The detailed execution plan's prospective implementation filenames are not assertions that equivalent files already exist upstream. Historical source inventory and caller comparison are in [the migration manifest](WINDOWS-MIGRATION-MANIFEST.md); approved product behavior is in [the design specification](../superpowers/specs/2026-10-18-bbsc-anywhere-coop-design.md). W03 remains responsible for native caller, thread, lifetime, admission, and save contracts; this W01 map must not preempt that work.

## Submodule pins

`git ls-tree -r <commit>` reports gitlinks as mode `160000`. Common pins at both recorded trees are:

| Path | Gitlink commit |
|---|---|
| `gpu/third_party/fsr-vulkan` | `c64f093404125e960813f95f1996c14780f0dd69` |
| `gpu/third_party/imgui` | `f1cc2ae15e53a861a874c3034aae6798fde194ab` |
| `third_party/LibAtrac9` | `efca2e3af35562a09a9bb6deed90e45b4b824dc4` |

Upstream-only gitlinks are `gpu/third_party/magic_enum` (`e046b69a3736d314fad813e159b1c192eaef92cd`), `gpu/third_party/miniz` (`2948eebafc5b9f1c69bebed80333e403a09bd62d`), and `gpu/third_party/xbyak` (`cd6bad3cad2e07a3e277c23d2b1f065320024717`). Retain these upstream dependencies; they are absent from the historical tree. Use upstream's dependency graph. Do not update the common gitlinks as part of BBSC migration without a separately evidenced dependency need. The unrelated local FSR patch is not the FSR submodule pin.

## History-preserving route back to `windows`

1. Keep the managed baseline checkout detached and unchanged at `de2e38af055f09f819946951075c6bb119e62ae6` as the comparison reference. Develop in the separate managed `codex/bbsc-windows-integration` worktree based on that exact commit.
2. Commit implementation work on the integration branch with focused path selection and the existing task checkpoints. Keep upstream runtime ownership and review every changed upstream file for the BBSC-only delta. No blanket merge, checkout, or directory copy from `windows`.
3. Before final integration, refresh `windows` from its normal remote checkpoint without rewriting history. Merge the completed integration branch into `windows` with a normal, non-fast-forward-capable merge (for example `git merge --no-ff codex/bbsc-windows-integration` when both refs are available). Resolve conflicts by retaining pinned-upstream runtime behavior and explicitly reapplying only reviewed BBSC/documentation content. The merge result must have both parent histories and must be inspected as a complete tree/diff.
4. Run required integration checks on the resolved merge commit, record exact source SHA and results, and review path ownership, submodule gitlinks, licensing, and unrelated deltas. If the merge contains broad upstream changes, review those as part of this integration, not as an implied acceptance by ancestry.
5. Commit and push `windows` normally to `origin/windows` only at the checkpoint authorized by the main task. Preserve `origin/pre`; never force-push or rewrite either history. Verify local `windows` equals `origin/windows` after publication.

An ancestry-only merge that leaves the historical tree as its resolved content is not a code migration and must not be marked as such. Conversely, choosing upstream as the integration branch base does not authorize blindly replacing all of `windows` without a complete tree review. The final merge is a later W11 acceptance action, not part of this source-discovery document.

## Commands and outcomes recorded for W01 discovery

Commands ran from `C:/Projects/bloodborne_pc` unless `-C` is shown. They read Git metadata/source trees only; no build, game, package, deployment, or test command was run for this discovery.

| Command / check | Result |
|---|---|
| `git status --ignore-submodules=all --short --branch` | `## windows...origin/windows`; an existing modified `docs/WINDOWS-PROGRESS.md` was present in the controller at inspection. It is outside this file's ownership and was not edited by this work. |
| `git rev-parse HEAD`; `git branch --show-current` | `26d74ee802098928165387a40af87a04d03693c9`; `windows`. |
| `git worktree list --porcelain` | Controller, detached baseline at upstream pin, and the managed integration path were listed. The integration worktree status could not be directly read by this sandbox; controller should verify it. |
| `git merge-base --is-ancestor ea8ac538... 26d74ee...` | Exit 0: historical runtime checkpoint is in controller ancestry. |
| `git merge-base --is-ancestor 26d74ee... de2e38af...` | Exit 1: controller is not an ancestor of upstream. |
| `git merge-base 26d74ee... de2e38af...` | `0afaaa264993b7afdde8e02a60fd57236234ef24`. |
| `git diff --name-status --find-renames=50% de2e38af... 26d74ee...`; `git diff --stat ...` | Large divergence: 1,210 name-status records with rename detection and 996 records under `gpu/`; includes extensive renderer/runtime/network moves and historical-only BBSC. See the pinned migration manifest for reviewed component classifications. |
| `git ls-tree -r --name-only <pin>`; `git ls-tree -r <pin>` | 1,008 historical and 970 upstream path entries; confirmed common and historical-only submodule gitlinks listed above. |
| `git status --ignore-submodules=all --short --branch` on baseline (from worktree listing) | Detached baseline shown at upstream pin; recursive submodule status was intentionally avoided due sandbox metadata access limits. |

The broad two-tree `git diff --check` produced many pre-existing whitespace findings across imported historical/upstream source and the archived FSR patch. It is not a check of this newly authored document and is not evidence that the integration diff is clean. The appropriate documentation check is to validate this file's links/content/whitespace after it is copied into the integration branch. Do not silently normalize unrelated historical files to make that broad comparison pass.

## Unresolved facts and next action

- Parent/controller must verify the managed integration worktree branch, `HEAD`, status, and submodule initialization state directly, then copy this record into it; branch and HEAD have now been verified by the controller.
- W03 must establish exact upstream native callers, reject-capable admission point, game-thread/tick and callback lifetime, selected-save identity, native return/save completion signals, and HTTP/signaling contracts. W01 has not resolved these.
- Broker capability/claim binding, complete event/flag/item mapping, native offsets and signatures, and UI lifecycle remain unverified. No fixtures or historical hooks qualify these facts.
- There is no demonstrated clean, conflict-free merge result yet. The route above is a normal-history proposal derived from the graph and path ownership; W11 must perform full conflict/diff review and checks before publishing.

**W01 discovery status:** source ancestry and ownership map recorded; branch/HEAD verified by controller. Documentation copy, review and checkpoint remain pending. The final reviewed merge belongs to W11. This document does not claim W01 verified, any BBSC runtime migrated, or any native/game gate passed.
