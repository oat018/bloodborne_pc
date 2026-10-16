# Seamless co-op for bbport

This branch adds a seamless co-op mode to the Windows version of bbport 1.7
(`Supermedo/bloodborne_pc`, itself based on `deadinside28/bloodborne_pc`). It builds on the
online play of 1.6 (the network and PSN libraries of `Wozzardman/shadp2p`) and on that
project's experimental seamless mode, and adds session rules in the spirit of the Seamless
Co-op mods for the Souls games.

It is experimental. Most of what is below was tested with two copies of the game running on
one PC against a private shadNet server. The first session between two machines (a Windows
host and a Linux guest over a VPN) was played with v2.3 and showed the problems listed under
"Fixed after v2.3".

## What it does

With `SHADPS4_BLOODBORNE_SEAMLESS_COOP=1` (cross-area summoning, from shadp2p) and
`SHADPS4_BLOODBORNE_STAY_IN_SESSION=1` (the rules of this branch):

- The session survives a death. The dead player respawns at its own last lantern; the other
  player keeps playing without a loading screen. A guest that respawned in another area is
  taken back to the host's area a few seconds later.
- In a boss fight a death waits (new after v2.5, as in Elden Ring Seamless Co-op and BBSC):
  a player who dies while another is still alive stays down and watches. When the boss dies
  the player gets up beside a living partner without a loading screen; when every player is
  down, everybody's death runs and each one respawns at their lantern. The leave key (Home)
  also ends the wait.
  With a lobby the games tell each other who is alive (the copy of a dead player in the other
  games still has its HP). `SHADPS4_BLOODBORNE_SPECTATE=0` keeps the old rule,
  `SHADPS4_BLOODBORNE_SHARED_DEATH=1` makes every death wait, not only in a boss fight.
- The session survives a boss kill. Nobody is sent home.
- The host can use lanterns inside the session. When the host travels, guests are taken to the
  same lantern, and from there to the spot where the host stands.
- Guests keep the host's progress (new in v2.2; `SHADPS4_BLOODBORNE_SHARE_PROGRESS=0` turns it
  off). When the host's world kills a boss during the session, the guest's own save gets the
  same boss flags and the boss's item (the game gives that to the host only), so the boss is
  dead in the guest's world too. Lanterns the host has lit and comes near are lit for the
  guest as well.
- Experimental, off by default: with `SHADPS4_BLOODBORNE_GUEST_ITEMS=1` (needs the shared
  progress) every player has its own items lying in the world. A guest sees the host world's
  items, picks them up for itself and keeps them; what one player takes stays there for the
  other. An item the guest already took in its own world is not there again.
- Experimental, off by default: with `SHADPS4_BLOODBORNE_SHARE_DOORS=1` (needs the shared
  progress) a guest also keeps what the host's world changes for good during the session:
  doors and gates opened, shortcuts, lifts. See the limits below.
- A guest can talk to the characters of the host's world and use its lanterns
  (`SHADPS4_BLOODBORNE_GUEST_TALK=0` turns it off).
- A lobby with a password instead of the bells (new after v2.5): with
  `SHADPS4_BLOODBORNE_LOBBY=host` on one player and `=join` plus the same
  `SHADPS4_BLOODBORNE_LOBBY_PASSWORD` on the others, nobody rings anything. While a joining
  game is in the lobby, the host's game opens its room by itself; the joining game travels
  alone to the host's map and joins the room directly. After a death, a disconnection or a
  restarted game they find each other again the same way. Home leaves for good (until the game
  is started again). See "The lobby" below.
- Bell hotkeys (without a lobby), so no bell items are needed: PgUp = Beckoning Bell (host),
  PgDn = Small Resonant Bell (guest), Home = leave the session, End = leave and ring again.
- Ringing the Beckoning Bell costs no Insight, by hotkey or from the inventory: the game took one
  per ring, also for the rings of the automatic re-join (`SHADPS4_BLOODBORNE_BELL_INSIGHT=1`
  keeps the cost).
- A guest is a full player (new after v2.5; found by
  [BBSC](https://github.com/Classmg22/BBSC), GPL-2.0-or-later): full blood echoes instead of
  half, Insight for a boss kill, no 30% cut of maximum HP, and the host world's NPCs look as
  they do to the host instead of grey ghosts. `SHADPS4_BLOODBORNE_GUEST_HALF_REWARDS=1`,
  `SHADPS4_BLOODBORNE_GUEST_HP_CUT=1` and `SHADPS4_BLOODBORNE_GHOST_NPCS=1` keep the game's
  rules. A member that stops answering no longer ends the session after about 6 minutes of
  undelivered packets (the game's limit of 1001 is raised to 30000).
- Enemies get stronger for every guest (new after v2.5, as in BBSC and Elden Ring Seamless
  Co-op): per guest +40% HP for normal enemies, +100% for bosses, +15% damage, +15% / +20%
  stagger resistance. A boss is what the game itself marks in a co-op fight: its fight scripts
  give the boss the vanilla buff (SpEffect 7500 / 7501, x1.5 / x2.0 HP), whose rate is turned
  into the rest of the boss share, so the boss ends at the boss rate (the Cleric Beast with one
  guest: 3015 -> 6031 HP, checked in a real fight). The param flag isSoulGetByBoss is not used
  for that: the Cleric Beast does not have it, and a flagged boss used to get the boss rate
  times the vanilla buff (x3.0).
  Percentages: `SHADPS4_BLOODBORNE_SCALE_ENEMY_HP`, `_SCALE_BOSS_HP`, `_SCALE_DAMAGE`,
  `_SCALE_ENEMY_STAGGER`, `_SCALE_BOSS_STAGGER` (set the same on every player);
  `SHADPS4_BLOODBORNE_SCALING=0` keeps the game's rule. The enemy tables are found in memory
  about 6 s after the game starts. Hostile NPC hunters are not scaled.
- No co-op fog walls (new after v2.5, as in BBSC): the walls the game raises around an area
  while a session is up stay down, so the players can walk on into the next area. Boss fog
  walls stay. A floor guard keeps a character standing on a part of the map one game has
  already unloaded from crashing it. `SHADPS4_BLOODBORNE_FOG_WALLS=1` keeps the walls.
- With `SHADPS4_BLOODBORNE_AUTO_REJOIN=1` the same bell is rung again when the players really
  lose each other (network trouble).
- Mouse and keyboard: bbport 1.7's own (`input.ini`, the launcher's Controls page,
  `docs/INPUT.md`). The mouse mode this branch had before 1.7 (`BB_MOUSE_KEYBOARD`, the
  "Mouse & keyboard" section of the Insert menu) is gone.

Shared progress is limited to bosses and lanterns: doors and shortcuts, items lying in the
world (each without its experimental switch above) and NPC quests still belong to the host
alone, and chalice dungeons were not tried. A boss killed before the guest joined is not
handed over, nor is a door opened before that.

The doors switch hands over every map event flag of the range 1AAB0xxx that the host's world
sets during the session. That range holds doors, gates, shortcuts and lifts, but also other
one-time events of a map (an ambush that has happened, a scene that has played), which are
then done in the guest's world too. It was tested with a flag set by debug command, not with a
door in the game.

Talking as a guest: what a character says and sells follows the host's world (a guest's flags
are the host's while it is in the session), and what the conversation changes is changed in
the host's world only. Tested with one character (Gilbert) and the lanterns. A guest that uses
a lantern to leave the area is taken back to the host a few seconds later. The game puts a guest's own flags back
when it leaves a session, so the mod writes them again in the guest's own world (and keeps
them in `user/seamless_progress.txt` until they have stayed, which also covers closing the
game from inside the host's world).

Known limits: a guest that respawned or travelled looks
like an ordinary player on its own screen; three or more players were never tried; enemy scaling and the other extras of the
Souls mods do not exist.

Matching takes about 10 s when both players are in the same area and about 30 s when the guest
first has to load the host's area (measured with two copies of the game on one PC; it was one
to two minutes). The game itself looks for hosts every 35 s, repeats a guest's advertisement
every 30 s and leaves a guest alone for a minute after a summon that did not complete; these
waits are capped at 5 s (`SHADPS4_BLOODBORNE_MATCH_INTERVAL=<seconds>`, 0 for the game's own
timing).

Known problem: in roughly one cross-area match out of eight the first summon does not complete.
The two games connect and the host sends its summon request, but the guest's game does not act
on it and closes the connection 7 s later (no packet is lost; the cause is not found yet). The
host tries again by itself about a minute later, and a guest still waiting after 100 s rings
its bell anew; most of these matches form within two to three minutes. If one does not,
restarting both games has always cleared it.

Fixed in v2.1: the second time the same two players matched from different areas the guest's
game crashed in the join (the server keeps the earlier claim, so the host summoned the guest
across maps directly), and a guest that had left a session was pulled back to the host's map
when it went somewhere else. A guest now travels to the host's map only while its Small
Resonant Bell is ringing, and does so again when the server repeats a claim.

Fixed after v2.3 (first session between two machines):

- Linux: the mod could not read the game's thread stacks (a hook's return address, arguments
  passed by address), so most of the session rules silently did nothing for a Linux player: no
  following the host, the host's character not put back after the guest's own reload (enemies
  then stand still), a guest's picked-up items not noted, and a respawned guest lost its team
  type, which made the host's game treat that guest's leaving as its own and reload into an
  empty grey screen. Small reads now fall back to the kernel (`process_vm_readv`).
- A host is no longer put through the forced reload of the "OnLeave_Limit" session script,
  whatever a guest's game reports (`SHADPS4_BLOODBORNE_HOST_LEAVE_RELOAD=1` for the game's own
  behaviour).
- Linux: the launch script now writes the host overrides (the Windows launcher does that by
  itself); without them the game could not go online at all. It also keeps the game's output
  in `last_run.log` next to the script.
- Windows: the hooks cost several milliseconds per frame (45 instead of 68 FPS in a light
  scene), because every memory read asked `VirtualQuery` first.

Still open from that session: a guest's old advertisement stays on the shadNet server after
its game died (a host that claims it gets "conflicting claim"; the guest's game rings anew by
itself after 100 s without a session); one crash of the Linux renderer during a reload
(`Vulkan::Scheduler::ReserveRecordData`, radv).

One player restarting its game (fixed after v2.5). The player that kept running reused its
NpSignaling connection to the other one: `ProcessPendingActivations` took the new address and
reported ESTABLISHED and MUTUAL_ACTIVATED at once, without a handshake with the new process,
and the game went on with its summon exchange (P2P virtual port 40) from the middle; the two
did not meet again until both had restarted. Now a peer that comes back on another port is a
new process: the old connection is reported DEAD (the game drops what it had going), and the
next attempt opens a new one. Measured with two games on one PC, one of them killed during a
session and started again: before, a restarted guest found no session within 4 minutes (2 of
2); after, it was back in the session 78 to 93 s after its ring 5 of 6 times. A restarted
host was back after 15 s before and after the change. Do not restart the shadNet server under a running game: the game logs in
again by itself, but a host in that state does not learn of room joins (every join ends after
30 s) until its game is started again.

Notes from the second two-machine evening (v2.5):

- The bells do nothing inside Oedon Chapel (the Cathedral Ward lantern): the bell icon shows a
  cross and no request reaches the server. Step outside, or ring somewhere else.
- A host uses PgUp only. PgDn is the guest's bell; End or Home right after a guest has joined
  ends that session.
- A guest that joins is put where the host stands (the game summons at a summon point of the
  area, which can be far away).
- The scripts that tell why a session or a life ended (names with Death, Leave, TimeOut,
  Recall, BlockClear, Join) are now logged without the debug file, so a log sent in by a
  player shows a death from a leave.
- Not solved: twice a guest's death in the host's world ended with the guest out of the room
  and, once, a loading screen that did not finish; in the test rig a guest's death keeps the
  session. A community server (thehuntersdream.com) that times out is not the cause: with a
  guest's requests to it failing for five minutes the session stayed up, also through a death.
Since v2.5 a guest also follows the host to another lantern of the same area: it is taken to
the host when the host's character leaves and comes back, or jumps, and the host's notes name
a lantern (not after the host's death respawn).

The Linux build is compiled by the workflow of this branch and cannot be run by the author
(no Linux machine); the Linux fixes above are compiled but were not run when this was written.

Every player of a session needs the same build and the same switches.

## The lobby

The host's game holds the lobby: an HTTP endpoint on TCP port 31320
(`SHADPS4_BLOODBORNE_LOBBY_PORT`). A joining game asks it once a second with the password and
gets the host's room id, online id and position. The joining game reaches the host at the
shadNet server's address unless `SHADPS4_BLOODBORNE_LOBBY_ADDRESS` names the host's address, so
the simplest setup is the host running the server. The host's PC must accept that port (on
Windows, allow bb-probe.exe when the firewall asks).

How the meeting works without a summon:

- Host: the game's "create session" step that a claimed summon sign runs (0x01788ED0 with the
  summon object, then `SprjSessionManager` 0x01ECFCD0) is called directly while a joining game
  is in the lobby. Calling it again while the room is open only extends its keep-open time. Not
  in the Hunter's Dream, where the game never hosts.
- Guest: the game's join of a summon request (the part of `SosStatus.Update` that reads the
  host's serialised session info, 0x18739CB..0x1873B38) is run with session info made from the
  room id and the host's online id: the data a host sends is the same every time apart from
  those. A guest in another map first travels there alone (the forced placement the summon
  travel uses): the game's own cross-map join hangs in the loading screen. A join that has not
  brought the host's world after 25 s is stopped and started again.
- Both games open their session connection (P2P virtual port 30) on the same signaling event.
  A game that received the other's opening packet before it had sent its own never answered
  it, and that join ran into the room's time-out. The P2P transport therefore holds packets
  from a peer until the game has sent something to that peer (at most a second), and keeps
  packets for a virtual port the game has not opened yet for two seconds.

Tested with two games on one PC: first meeting from the Hunter's Dream about 25 s after the
host arrives; after the joining game was killed and started again, 5 of 6 met at the first
attempt (about 18 s after the room opened) and the sixth at the second; after the host's game
was killed and started again, 3 of 3 at the first attempt; deaths of either
player, the host travelling to the Dream and back, and leaving with Home all behave as with the
bells. Not yet tested between two machines.

## Using it

1. One player runs a shadNet server (`Wozzardman/shadnet-p2p`) with
   `BloodborneSeamlessCoop=true` and creates an account for each player. The players must be
   able to reach the server and each other (same LAN, or a VPN such as Tailscale).
2. Install bbport as usual and choose the game folder in the launcher (Bloodborne CUSA03173,
   version 1.09, your own copy).
3. Copy the launch script from `packaging/seamless/` next to the game launcher, put the
   server address and your account in it, and start the game with it:
   `Play Seamless Co-op.bat` on Windows, `play-seamless-coop.sh` on Linux.
4. Choose "Play Online". With the lobby set in the script the players meet by themselves;
   without it the host presses PgUp and the guest PgDn.

## Building

Windows: MSYS2 CLANG64 as described in `packaging/windows/README.md`, then `bash build.sh`.
With a recent clang the CMake cache may need
`-DCMAKE_CXX_FLAGS="-include cstdlib -include new -include cmath -include cwchar"` and
`-DCMAKE_C_FLAGS=-Wno-error=unused-but-set-global`.

Linux: `nix-shell shell.nix --run "bash build.sh"`, then `bash packaging/appimage.sh` for the
AppImage. The workflow in `.github/workflows/build-linux.yml` does the same.

## How it works

The game is the original PS4 executable, so the mode is a set of byte patches (each verified
against the expected bytes of version 1.09) and code hooks, in `gpu/bbnet/bloodborne_re.cpp`.
That file is generated from shadp2p's `src/core/bloodborne_re.cpp` by
`tools/port_seamless.py`; `gpu/bbnet/guest_hook.cpp` provides the code hooks.

Safety rules of the installation and of every write into the game:

- All or nothing. Every part that is switched on checks all of its sites before it changes the
  first one. If a part cannot be installed the loader stops with `Co-op: NOT STARTED: <part>`
  (and an `ERROR:` line) instead of starting a game that plays by other rules than the
  partner's. The message names the switch that plays without that part.
- The game's code is written only while the loader still has the image writable, and only over
  bytes that were checked (`PatchImage` / `WriteImageAt`). Game data is written only where
  `HasMemoryAccess` says it is writable (`WriteValue` / `WriteGuestBytes`); on Linux that asks
  the loader for the image (`bbport_image_access`, by segment protection) and the runtime for
  the game's mappings (`runtime_memory_has_access`), on Windows `VirtualQuery`. Param rows
  (enemy scaling) are written only inside tables that were found writable as a whole.
- A host placement from the other game (lobby or summon header) is used only when it parses
  whole: all seven fields, a map the game has, finite coordinates within 100 km.
  `SHADPS4_BLOODBORNE_TEST_INSTALL_FAILURE=<text>` makes the installation fail on purpose
  (test aid).

- Death and boss kill: the session scripts that send a guest home ("HostDead", "BlockClear",
  "PartyGhostDeath") are not scheduled, and a stage transition no longer stops the session.
- After a reload the game does not create the other players' characters again. Each client
  keeps the session handles of the players it has seen and asks the game's own insertion task
  manager for them again.
- Lanterns and talking: the talk scripts ask whether a multiplayer session is in progress
  (talk function 52) and whether they run on a guest's side (function 19); they are told no.
- Following the host: each client tells its session peers its map and the lantern it last
  travelled to, once a second, on a spare virtual port of the P2P transport
  (`gpu/shadps4/core/libraries/network/p2p_sockets.cpp`). A guest in another map than the host
  uses the game's own headstone warp to that lantern.
  Once the host's character is in the guest's world, the guest is placed at its position with
  the game's own same-map placement helper (the one behind the debug command
  `RemoteWarpPlayer`).
- Shared progress: the host watches its event scripts. From an event's "Handle Boss Defeat"
  instruction on, the flags that event sets and the item lots it awards are collected, with the
  event's own flag; for lanterns the "lit" flag (registration flag + 10) of every lantern the
  game has registered. The entries ride in the once-a-second note, four at a time, and the
  guest sets the flags with the game's own flag setter and awards itself the item lots.
- Doors (experimental): a hook on the game's event flag setter notes, on a host with a
  session, every flag 1AAB0xxx that goes from off to on; the entries ride in the same note.
- Items for guests (experimental): the game sets up an object's treasure only when the player
  is not a guest, numbers the item nodes of a guest all alike (a pick-up then takes the first
  node of the list, also for what enemies drop) and empties the guest's item list when the host
  says "forget the shared items"; three patches undo that. A hook on the node removal notes the
  event flag of what a guest picks up; the flag is set and kept like a boss flag. Joining
  replaces a guest's flags with the host's, so the "taken" flags of the item lot table are read
  at home, and in a session the nodes of those items are removed. Tested with two copies of
  the game by debug command (item given, the other player's item untouched, still gone after a
  death and after leaving); walking up to an item and pressing the button was only checked up
  to the prompt.
- Matching speed: the timing values of the game's network settings are lowered in memory, and
  the HTTP layer repeats the guest's last advertisement between the game's own repeats (the
  answer to it is what sends a guest to a host in another area).

`SHADPS4_P2P_LOG=1` logs the first 3000 P2P datagrams of the game (needs `BB_GPU_LOG=info`).
`SHADPS4_BLOODBORNE_DEBUG_FILE=<file>` turns on a test aid: the session state is logged when it
changes and one command line written to that file is run (see `ApplyDebugCommands`).
