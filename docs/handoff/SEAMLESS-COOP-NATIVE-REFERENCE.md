# Native facts from the bbport seamless co-op fork (reference, not evidence)

Added: 2026-10-23 by oat018. Task: none (reference input for W03 blocked facts, W07, W09, W10). Documentation only; no runtime, plan checkbox or tracker status changes.

**Evidence class.** Everything below comes from a *different fork*: `oat018/bloodborne_pc`, branch `seamless-coop`, commit [`264ca43`](https://github.com/oat018/bloodborne_pc/tree/264ca43c6dba98a47e5cc41e07b0820daa015dac). That branch sits on Supermedo `windows` at `652f9a4` (bbport 1.9), the merge base with this plan's pin `de2e38a` (1.9.2, 32 commits later). Its tests ran on **Windows**, with two copies of the game on one PC, against a shadp2p server, using a debug-command rig (`SHADPS4_BLOODBORNE_DEBUG_FILE`). None of it was run at `de2e38a` or with the historical BBSC runtime. Under AGENTS.md this is a lead for W07/G1–G3 diagnostics, **not** native qualification. Do not mark a gate or checkbox from it.

Offsets are offsets into the CUSA03173 v1.09 eboot image (`image_base + offset`). They do not depend on the port version, but the bytes each hook checks were only verified on the 1.9 build of that fork.

Links use `SC:` for `https://github.com/oat018/bloodborne_pc/blob/264ca43c6dba98a47e5cc41e07b0820daa015dac/`.

## 1. Overlap with historical BBSC

Both trees derive `bloodborne_re.cpp` from shadp2p. Of the 81 named hex constants in the fork (79 image offsets, 2 wire magics), 64 also appear in the historical BBSC file at `ea8ac53` (`gpu/bbnet/shadps4/core/bloodborne_re.cpp`). The other 17 (15 offsets, 2 magics) are listed in §4. The useful new information is therefore mostly (a) how the fork hooks into the **static Windows runtime of Supermedo 1.9**, and (b) which of the shared sites ran on Windows.

## 2. Integration with the static runtime (W03 "no invented interface", W07)

The fork needed exactly one new lifecycle call. Everything else hooks game code.

| Point | Fork implementation | Relevance |
|---|---|---|
| Install time | `bbnet_install_coop(image, size)` declared in `gpu/bbgpu.h`, defined in `gpu/bbnet/bbnet_glue.cpp`. `src/probe.c` calls it after relocation and the Windows TCB rewrite, **before** `protect(image, …, 0)`, so the image is still writable. Non-zero return makes the loader `fail()`. | A concrete candidate for the "hook install/restore/protection" diagnostic in G1. It is an added declaration. W03 says no BBSC declaration is approved until owner, thread, cancellation and lifetime are source-backed. |
| All-or-nothing | `InstallSeamlessCoopPatches()` (`SC:gpu/bbnet/bloodborne_re.cpp#L8029`) checks every site's expected bytes before writing the first one. `SeamlessCoopInstallFailure()` names the part that failed. | Matches "zero side effect on rejection" at install level. |
| Code hooks | `Core::InstallGuestCodeHook(address, expected_bytes, tag, handler)` (`SC:gpu/bbnet/guest_hook.cpp#L110`). Trampolines live in one 1 MiB block allocated within near-jump reach of the image (`AllocateNear`, `VirtualAlloc` on Windows, L37). Handlers receive a `GuestRegisterSnapshot`. | Upstream has no shadPS4 per-module trampoline area, so W07 needs some equivalent. |
| Memory-access checks | `HasMemoryAccess` (`SC:…/bloodborne_re.cpp#L966`). Image: `bbport_image_access` in `src/probe.c`, all writable until `image_sealed`, then per segment flags. Game mappings: `runtime_memory_has_access` in `src/runtime_memory.c` (VMA walk, `KIND_RESERVED` refused). | Satisfies "no write without verified access" for game data writes. |
| Image globals | `MemoryPatcher::g_eboot_address` / `g_eboot_image_size` / `g_game_serial` defined in `bbnet_glue.cpp`, because the port does not link shadPS4's patcher. | Small, but it will come up again. |

## 3. Shared sites seen running on Windows (leads for blocked contract rows)

| Blocked row (WINDOWS-ADAPTER-CONTRACT.md) | Fork observation | Where |
|---|---|---|
| Game-thread tick ("do not add an invented game tick") | The fork's only per-frame driver is a code hook on **`SosStatusUpdate` `0x01872360`**. Its handler (`TraceEntry`) runs 18 steps in order: placement, debug commands, reinsertion, shared progress, death rules, bell and stop requests, and so on. The enemy-sync state is commented "game thread only". Historical BBSC hooks the same offset (`ea8ac53` L18340). | `SC:…/bloodborne_re.cpp#L4867-L4887`, install at L8413 |
| Earliest game-world consumer (W08/G1) | Not resolved by the fork either. The session entry points it hooks: `CSMultiPlayerIns` ctor `0x01E4F9F0`, `CSMultiPlayMan::Stop` `0x01ED07A0`, stage transition request `0x013CDE30`, summon build entry `0x01874710`, cross-map guest handoff `0x01E4FF3A`. These are places to timestamp in the G1 trace, not proven cancellation points. | L6741-6772, L8384-8403 |
| Event flags (G2 shared progress) | Get `0x01399BF0` `(unused, flag)`; set `0x013CFCC0` `(EventFlagMan*, flag, state)`; manager pointer `image+0x0553B100`. The setter hook prologue is `41 89 D0 8B 4F 1C`. | L1925-1945, L6804-6809 |
| Native item grant (G2) | `ItemLotAward` `0x017DDC50` `(ItemLotMan* @ image+0x0553D6E0, lot id, host_only=0)` awards a lot to the local player, guest included. In the fork it is **not** linked to a save receipt. Only the in-memory grant was observed. | L1947-1954 |
| Serializer / selected slot | **Not found in the fork.** It never needed a save hook. No lead. | — |
| Lanterns / return point (W10/G3) | Registration `0x0133B030` (hooked). Lit flag = registration flag + 10 (`LanternLitFlagOffset`). Headstone warp `0x013CDF30 (return point id)`. 85 known return-point ids with area/block are in `ReturnPoints`. Same-map placement helper `0x0154B110` (behind debug `RemoteWarpPlayer`). | L5574, L5637, L5980-6020, L2872 |
| Current map | `GetCurrentPackedMap()`: `[image+0x0553B148]` → index at `+0x20`, array at `+0x10`, with access checks. | L1252-1270 |
| Guest return home / Blank-like exit (G3) | `GuestReturnHome` `0x01381A60 (context=0)` is called by the fork to send a guest home. Before the call it writes the player's `+0x78` into `[image+0x0553D6D0]+0xF0`. | L3605-3613 |
| Talk/NPC behaviour for guests | Talk function 52 ("multiplayer in progress") and 19 ("on a guest's side") are answered *no*. Guest talk site `0x0203BA78`. | L6936, doc §How it works |

## 4. Offsets present only in the fork (not in historical BBSC `ea8ac53`)

| Name | Offset | Use in the fork |
|---|---|---|
| GuestReturnHome | `0x01381A60` | send a guest home (see §3) |
| SoloDeathRespawnReturn | `0x01381EE3` | death respawn path |
| EventCommandHandler | `0x017C0420` | collect flags and item lots from "Handle Boss Defeat" onward (shared progress) |
| MultiPlayerInsRequestGuest / Host | `0x01E54B60` / `0x01E54A90` | `(manager, handle)` re-request a player's character after a reload |
| MultiPlayerInsStart | `0x01E55610` | `(manager, handle, enter type, flag)` |
| SessionHandleCopy / Destroy | `0x00CA1500` / `0x00CA1570` | keep session handles across reloads |
| SessionEventHandler | `0x0178B5E0` | session events |
| MapItemRemove | `0x017D6780` | `(MapItemMan*, &list id, picked up)`: record a guest's pick-up flag |
| TalkForGuest | `0x0203BA78` | talk answers above |
| LeaveLimitStart | `0x0138A4A6` | leave timer |
| GuestTreasureTest | `0x017DC3CF` (with 3 related sites in `TreasureTests`) | patches so guests get treasure/item nodes |
| InsightCost | `0x018C92BB` | bell cost |
| BlockChangeStop | `0x0193C657` | no session stop on block change |

Wire magics `"BBS1"` (`0x32534242`, placement note) and `"SYNC"` (enemy sync) are fork-only protocol and are not proposed for BBSC.

## 5. What the fork's Windows tests did cover (for context only)

From the fork's v3.3 notes and later commits: session formed in 24-27 s. Lobby scenario passed (deaths, Hunter's Dream round trip, leave keys). Enemy sync: 185 enemies, 0 differing after two guest deaths. Guest death respawn: one load, 0.9 m from the host. Rejoin and four restarts on either side passed. Bell meeting passed. Guest item pick-up was tested by debug command; button pick-up was only checked up to the prompt. **Not covered:** two machines, frame generation together with co-op, Linux, save/serializer durability, NG+ identity. The last fork commit (dead host respawns at its guest) has a test script, but no result is recorded.

## Suggested use

- W07: compare §2 against the historical hook layer before choosing an install point. If the THD/static owner agrees, consider `SosStatusUpdate` as the game-thread tick candidate to *verify* in G1 (thread id, frequency, lifetime across loads). Do not just assume it.
- G2 diagnostics: add `ItemLotAward` entry/exit and `EventFlagSet` callers to the planned trace, next to the still-unknown serializer.
- G3 diagnostics: `ReturnPointWarp`, lantern lit flags and `GuestReturnHome` are concrete hooks to log for "exact point/fallback" and "both Blank paths".
