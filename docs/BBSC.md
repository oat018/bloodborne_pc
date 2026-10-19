# BBSC: Bloodborne seamless co-op in the port

BBSC (seamless co-op for Bloodborne, GPL-2.0-or-later; https://seamlesshunt.com) keeps a co-op
session alive through deaths, lamps, area changes, bosses and the Hunter's Dream, for up to five
hunters. It is written for shadp2p (a shadPS4 fork); the port carries it in its online module.

## Development checkpoint: 2026-10-19

**Current status:** This is a saved implementation checkpoint, not a feature-complete
pre-release. The party transport, identity-bound snapshot storage, managed restore
hooks and post-destination native-save fence are implemented in source. Normal joins
do not create managed home records, and the protocol's admission/event consumers are
not activated. Offline restart loading/warp, the explicit event/item catalog and exact
NG7 cycle isolation remain unfinished. The latest return-fence details are below.
The next development session must stop before AppImage/EXE packaging unless the user
requests otherwise. The subsequent push request pauses development at this checkpoint.

Native cross-map co-op has been confirmed with the BBSC Kit on the configured
private servers: one join load, original Parting Blank menu/animation, host and
guest departure, and return to the guest's captured pre-join position during the
same process. A host ending the party can produce the native disconnected-session
notice on the guest. All-map lighting and unsupported-area behavior remain open.

Selected-slot ownership and the native create, reload, Load Game deletion and
subsequent reuse of slot1 have been captured using disposable saves. Deletion
updates metadata while leaving the old character payload intact, so filename,
name and nonzero file contents do not identify a replacement character.

Runtime save receipts now use bbnet ABI3. Each successful observation binds the
canonical path, process-local generation, byte count and SHA256 of the actual
temporary copy committed by the runtime, including after unmount. Hash or durability
failure clears snapshot proof without advancing its generation; a diagnostic hash
failure does not prevent the game from saving. Receipts describe the committed
copy, not later external edits, stable character identity or newly earned progress.
Runtime and online module must be rebuilt together; ABI2 modules are rejected.
The runtime reuses the project's existing OpenSSL dependency (`libcrypto`).

The party policy and durable journal code remain foundations. Runtime integration
of the verified native character discriminator, uncapped cycle identity, durable
restart/disconnect recovery, safe
lamp fallback, admission gates and the joined-event/reward catalog are unfinished.
The legacy `SHADPS4_BBSC_SHARED_PROGRESS` whole-group merge remains present;
controlled checkpoints explicitly set it to0. The requested event-only sync is
not enabled. Do not treat this checkpoint as the completed anywhere-co-op feature.

Build and automated checks use distrobox `bbport-dev`. Gameplay uses copied
profiles and is run by the user. The verified create/load/delete/reuse sequence
needs no repeat; resume with the identity binding and remaining integration gates.
Further native inspection identifies persisted `ServerUserId`/`ServerChrId` and
the game's character-ID allocation request. The repository gameserver now issues
durably reserved IDs instead of an empty list; local server tests pass. The user
has installed the new Unraid image and its private allocation endpoint now returns
a positive ID. The first native BBSCTrace load now persists a positive ID in its
current and backup saves, and the same ID survives a new game process. Normal
deletion/recreation in the same slot now produces a distinct positive ID while
preserving the other character. These controlled identity checks pass.
The live reader also matches the native IDs and canonical selected file before
the first character save of a new process. Cycle binding and restart recovery
remain unfinished. Preserve the server's `<--data path>.characters.sqlite3` on
persistent storage.
The opt-in native identity trace also reports the saved capped NG count beside
the live value. A mismatch is diagnostic evidence; matching values, particularly
7, do not establish a committed new cycle or permit restart recovery.
The same diagnostic now records full snapshot SHA256 at native load, completed
serialization and copy input, and at the selected saved file. Three opt-in
pre-increment observers identify potential NG transitions without changing the
count. Three additional post-store observers validate the increment registers and
final capped value, rejecting a skipped Lua guard and unrelated event commands.
Fixture replay through the actual hook provider executes the captured 1.09
arithmetic for0→1,6→7 and7→7. These are diagnostic observations, not committed
cycle identities; the ending/save/restart flow still needs integration.
The copied-character provenance trial now confirms three native serialize/copy
hash matches and a final saved file matching the last snapshot. Atomic
generation/hash receipts now pass both automated runtime/module tests and the
live copied-character checkpoint: generation2 matches the second native
serialize/copy/commit, while the final file matches the later third save. Other
character slots remain unchanged. Do not repeat this checkpoint. Real NG
transition/replay and complete cycle identity remain unverified.
The new party protocol now has ordered admission and independent per-guest
baselines, account/connection-bound routing, bounded retry queues and cumulative
acknowledgement. Matching2 publishes copied room-owner/membership snapshots;
receive-thread messages enter a bounded game-thread inbox only for selected room
members (only the owner on a guest). Acknowledgement of earned events is produced
only after the intent journal has been flushed successfully. This acknowledges
recorded intent, not inventory delivery or a native character save.
Journal version2 preserves unfinished and committed claims with their original
party/admission across successive parties; version1 remains readable. A later
party cannot relabel or erase earlier claims. A native home-capture adapter now binds the selected canonical save path and native
character IDs to a leased durable origin/native-respawn record. Unresolved records
are preserved. On Linux the Lua join hook now refuses a new join for an existing
pending, locked or unreadable managed record, using the game's own null-name
failure path. Own-world capture and Blank leave handling run before trace sampling.
The normal join path does not yet create these new home records: durable own-world
snapshot/capability checks and the recovery coordinator must be connected first.
The game-thread coordinator still needs to connect these components to first-playable
admission, event capture and complete recovery. A managed return save fence is now
connected, as described below. No new event rewards are enabled yet.

Development now prioritizes completing the pre-release feature paths before the
broad gameplay matrix. Keep focused compilation, journal durability, wrong-peer
and duplicate-reward checks; defer repeated manual checkpoints and long sessions.
Linux AppImage and Windows EXE packaging are deferred. Windows journal I/O has a
guarded Win32 source implementation, but CMake still adds the journal source only
under `NOT WIN32`; the native managed adapters also retain platform guards. Windows
target integration, compilation and runtime validation remain unfinished. Neither
package is feature-complete today.
See the [plan](superpowers/plans/2026-10-18-bbsc-anywhere-coop.md),
[design](superpowers/specs/2026-10-18-bbsc-anywhere-coop-design.md) and
[evidence](superpowers/specs/2026-10-18-bbsc-integration-evidence.md).

## Switching it on

Launcher, Online page: "Play online" and "Seamless co-op (BBSC)". Every player needs it on and the
same server: a shadNet server with BBSC's game server next to it. Without the switch nothing of
BBSC is installed and the online module behaves as before.

The launcher's defaults are BBSC's public servers, Seamless Hunt (0.6): shadNet
`play.seamlesshunt.com:31313`, WebAPI `http://play.seamlesshunt.com:31315`, game server
`http://play.seamlesshunt.com` (plain http: BBSC's game server listens on 20443 without TLS). The
account (online id and password) is made on https://seamlesshunt.com; the launcher only takes it.
shadPS4's public shadNet server (`srv.shadps4.net`) accepts only shadPS4's own builds since
2026-10-12: it signs the port out (notification 19) and then refuses its logins for a while.

BBSC's game server on this PC: when the server's own game server does not answer (its
`:20443/bb-eu/ss.info` timed out for players while the rest of the public server worked,
2026-10-14), `run.sh` starts BBSC's `bbsc_gameserver.py` (`scripts/bbsc_gameserver`, from
BBSC-Server-Linux) on 127.0.0.1:20443 and points the game's server there; it sends the summon
calls to the shadNet server's WebAPI, accounts and bells go to the shadNet server as before.
Messages and bloodstains are then this PC's only. It stops with the game; its logs and data are in
`<user>/bbsc/`. `BB_BBSC_LOCAL_SERVER=1` always, `=0` never, `auto` (default) when the server's
does not answer within 5 s. (BBSC's own `BBSC_RELAY` is for the TCP relay in its Windows DLL; the
port's relay is the UDP one on port 3478 of the shadNet server.)

Environment: `BB_ONLINE=1 BB_BBSC=1` (what the launcher sets). BBSC's own switches
(`SHADPS4_BBSC_*`, `SHADPS4_BLOODBORNE_*`, see `gpu/bbnet/shadps4/core/bbsc_persistence.h`) work as
in BBSC: most features are on by default, `=0` turns one off.

When shadNet and the game/relay server run on different hosts, also set
`SHADPS4_P2P_RELAY=<game-relay-ip>:3478` for every player. The relay otherwise
defaults to the shadNet host, so HTTP matching may succeed while signaling times
out. The relay's UDP port must be reachable; TCP/WebAPI health alone is insufficient.

## Where it lives

| Part | Files |
|---|---|
| Game code (hooks, byte patches, session policy, world sync) | `gpu/bbnet/shadps4/core/bloodborne_re.cpp`, `bbsc_*.{h,cpp}`, `bbsc_messages.inc` (BBSC's; memory access validates range and protection with one query) |
| Network (signaling packets, relay fallback, room replay, placement header) | BBSC's changes merged into `gpu/bbnet/shadps4/core/libraries/network` and `np/` (three-way merge against shadp2p 6e97b40, the base of both); the relay as in BBSC's BBport build of 2026-10-14 (below) |
| NP lookup (`sceNpLookupNpId`, a third and later player) | `gpu/bbnet/shadps4/core/libraries/np/np_utility.{h,cpp}` (BBSC's), registered in `bbnet_register` |
| Hooks in the game's code (any game) | `gpu/shim/bbport_guest_code_hooks.{h,cpp}`: shadPS4's `Core::InstallGuestCodeHook` interface, the port's trampolines |
| What BBSC asks of shadPS4 | `gpu/bbnet/include/` (`core/cpu_patches.h`, `core/memory.h`, `core/file_format/psf.h`, `common/memory_patcher.h`), `gpu/bbnet/bbsc_glue.cpp` |
| Local game server | `scripts/bbsc_gameserver/` (BBSC's, unchanged; NoDreamForHunter format, MIT), chosen by `scripts/online.py bbsc-local`, started by `run.sh` |
| Start | `bbnet_image_ready` (`gpu/bbnet/bbnet_glue.cpp`), called by the runtime once the image is mapped and patched (`src/probe.c` -> `runtime_net_image_ready`) |

Differences from BBSC's own builds:
- The game's code stays writable while BBSC is on (`Core::KeepGuestCodeWritable`): BBSC writes
  byte patches into it and switches some while the game runs; the port's own code patches leave it
  so too.
- Any retail Bloodborne 1.09 (every region runs the same executable) counts as CUSA03173; every
  BBSC site is still byte-checked.
- Saves: `BBSC_SAVE_DIR` points at `<user>/savedata/<id>/CUSA00207/SPRJ0005` (the port's layout).
- Its loading screen is a line in the port's overlay (`bbgpu_overlay_banner`); the cinematic
  camera (F1, keyboard and mouse) gets the window's events through `bbgpu_add_event_filter`.
- Not ported: the animated title card (off by default in BBSC too), Discord status, its launcher's
  account creation and server cards (the port's launcher takes the server address and account).
- The "bell check" trace hook does not install (BBSC's own byte patch is at the same address); it
  only logs.

## Updates taken

- 2026-10-14, BBSC 0.0.1 "bug fixes" (its BBport build, `bbport-1.7.2-bbsc.patch`): the relay goes
  through the server for every known peer from the first frame (`SHADPS4_P2P_RELAY=auto` or
  `direct` = the old "direct first, relay after 2 s", `=0` off), peers are kept by online id with
  every endpoint as an alias (the one-way relay behind CGNAT that died after 30 s); the NP lookup
  above. Its signaling and matching files were already the same as here. The fixes inside
  `bbsc.dll` of that release (death and respawn, sign targeting, relay TCP framing) have no
  published source yet; they come in when BBSC publishes it.

## Checks

Checkpoint verification on 2026-10-19 passed inside `bbport-dev`: full
`BB_PGO=off BB_LTO=OFF bash build.sh --test`, plus `bbsc-memory-test`,
`bbsc-memscan-test`, the actual RX 9060 XT dma-buf check,
`bbsc-party-test` (policy/return/journal), `bbsc-join-test` and
`bbsc-http-poll-test`. These automated checks do not complete the remaining
live identity, recovery or event/reward gates.

`ninja -C out/gpu bbsc-memory-test && out/gpu/bbsc-memory-test` checks the native loader's image
mapping through BBSC's memory interface, including permission changes, mapping boundaries,
unmapping and overflowing sizes. The image and guest stacks are host-owned allocations, outside
the runtime's direct/flexible memory table: BBSC queries their Linux mappings when that table
does not cover an address. Without this fallback, reads of the image's global player/inventory
pointers return zero and the kit check cannot reach the inventory.

On Linux 6.11 and newer (when built with matching or newer Linux headers), host mappings use
`PROCMAP_QUERY` to query the current VMA without parsing the full maps file. Scanning it for
every BBSC read caused a CPU bottleneck after loading a character: the main thread spent about
3.1 s per 5 s in 84,000 queries, reducing a 75 FPS scene to 38 FPS on an RX 9060 XT. Permissions
are still queried each time, including after `mprotect` and `munmap`. Older kernels keep the text
fallback; the memory test also exercises that path by simulating an unsupported ioctl.

The param-table scanner has a separate Linux dma-buf read path. On the RX 9060 XT,
`process_vm_readv` returns `EFAULT` for exported device mappings even though CPU
loads work. It now falls back to bounded reads through `/proc/self/mem`, including
short/interrupted reads, while still selecting readable VMAs and keeping faults
inside the kernel copy. Without this path the Kit's table scan stays at zero and
never reaches inventory delivery, even with the mapping-query fix above.

`ninja -C out/gpu bbsc-memscan-test && out/gpu/bbsc-memscan-test` checks scan bounds,
chunk overlap, failed/partial reads, protection exclusions and disappearing mappings.
The explicit hardware target `bbsc-memscan-dmabuf-test` requires Vulkan cached
system-memory dma-buf export; it creates a 2 MiB buffer without a window or game
and checks scanning across the 1 MiB chunk boundary. It passed on the RX 9060 XT
in `bbport-dev`; it is not part of the default build.

`out/dev/verify/verify.sh NAME BB_ONLINE=1 BB_BBSC=1`: loads the save offline-capable, 14 self-tests
pass, no faults (2026-10-14). Real co-op needs two players on a BBSC server.


The explicit `bbsc-join-test` target exercises the actual placement cache and
native placement adapters with synthetic mapped state (no game or account). It
covers empty preparation ACKs, unchanged expiry, same-map target clearing, native
home preservation and co-op-only send-home restoration. Run builds/tests in
`bbport-dev`, for example `cmake --build out/gpu --target bbsc-join-test` followed
by `out/gpu/bbsc-join-test` inside that container.

Legacy per-slot storage now takes the selected slot from the native game's
load/save state instead of choosing the newest character file. An explicit
`BBSC_SAVE_DIR` restricts lookup to that profile; without it, ambiguous local
owners reject. The existing requirement for a write during this run remains.
The regression also covers newer files from another character and invalid or
missing selections. This does not identify a replacement character reusing a
deleted slot or implement durable position recovery after restarting the game.

Connect-first does not write host coordinates into the engine sign block, but
native loads also reuse that block. A separate process-local origin therefore
reads the guest's live native map/XYZ/heading at JoinSession_White before sampling,
retains it across guest rejoin, and restores sign+forced fields only at co-op
send-home. Quit to title clears this temporary copy. The user has confirmed original-point voluntary returns; this does not implement
durable restart recovery, character identity,
geometry checks or lamp fallback.

Password co-op host searches now share the bounded fast-sign poll loop (up to
five additional requests, three seconds apart). Initial mixed searches remain
intact; additional queries contain only co-op entries. Public/invasion-only
searches retain their native cadence. The explicit `bbsc-http-poll-test` target
checks eligibility and uses the real HTTP worker with an ephemeral loopback
server to verify candidate delivery, opt-out and cancellation. No game/account
is needed. Real end-to-end matchmaking time still requires gameplay testing.


Parting Blank keeps the game's native menu: stop the bell signal or end co-op.
The Kit row retains the original menu/dialog, leave effect7 and animation15;
the compatibility hook supplies the original Blank-use discriminator for11251.

Cross-map native summon already loads the host map. The old forced second join
reload has been removed; the post-summon tick only settles the guest beside the
host. An actual-adapter regression verifies no extra StageTransition, one placement,
and the invader exclusion. The unused host-effect fallback was removed after
live traces showed it never received a local Blank event. User gameplay now confirms
one cross-map load, correct guest Blank animation/messages and both host/guest
exit. Host-initiated exit reaches RemoveMember then guest OnRoomDisappeared; the
session-disconnected notice represents that host-initiated closure in this capture,
with no OnP2PTimeOut event. Lighting across all supported maps remains unverified.
Durable restart recovery and joined-event-only progress remain separate open gates.

### 2026-10-19 — Native return save fence checkpoint

Prepared-home journal v4 adds a monotone `native_return_selected` marker. The guest
send-home path persists it before selecting the saved native respawn point ID through
the native last-lamp branch. Off-map point maps come from the native point-to-map
resolver; zero XYZ values are unresolved metadata and are never written as warp
coordinates. The old raw `native-selector-1.09.txt` disassembly has an incorrect ELF
offset and is non-authoritative; use corrected PT_LOAD artifact `root-native-selector.txt`.

Return completion requires matching selected character/slot, the durable selector marker,
own-world/player readiness, live position equal to the resolved native point after its
map loads, and a SerializeBegin after destination confirmation whose successful
SerializeEnd hash matches a newer ABI3 selected-file receipt. First destination
confirmation discards earlier save evidence. A mismatched or unresolved destination
leaves recovery pending; pending/unverifiable managed state blocks the legacy
process-local raw-coordinate fallback. The focused join fixture verifies fail-closed
mismatch behavior; the party tracker fixture verifies a matching synthetic point.

Focused `bbport-dev` build plus party, join and transport checks pass; logs are
`checkpoint-v4-final-{build,party,join,transport}.log` under the ignored SDD evidence
directory. This checkpoint does not complete anywhere co-op: normal-join admission,
offline recovery-only module loading/warp, event catalog/application, NG7 isolation,
Windows compilation/runtime and broad gameplay validation remain unfinished. No
AppImage or EXE was packaged.
