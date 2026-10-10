# Seamless co-op for bbport

This branch adds a seamless co-op mode to the Windows version of bbport 1.6
(`Supermedo/bloodborne_pc`, itself based on `deadinside28/bloodborne_pc`). It builds on the
online play of 1.6 (the network and PSN libraries of `Wozzardman/shadp2p`) and on that
project's experimental seamless mode, and adds session rules in the spirit of the Seamless
Co-op mods for the Souls games.

It is experimental. Everything below was tested with two copies of the game running on one
PC against a private shadNet server; a session over the internet between two machines has not
been tested with this build yet.

## What it does

With `SHADPS4_BLOODBORNE_SEAMLESS_COOP=1` (cross-area summoning, from shadp2p) and
`SHADPS4_BLOODBORNE_STAY_IN_SESSION=1` (the rules of this branch):

- The session survives a death. The dead player respawns at its own last lantern; the other
  player keeps playing without a loading screen. A guest that respawned in another area is
  taken back to the host's area a few seconds later.
- The session survives a boss kill. Nobody is sent home.
- The host can use lanterns inside the session. When the host travels, guests are taken to the
  same lantern.
- Bell hotkeys, so no Insight or bell items are needed: PgUp = Beckoning Bell (host),
  PgDn = Small Resonant Bell (guest), Home = leave the session, End = leave and ring again.
- With `SHADPS4_BLOODBORNE_AUTO_REJOIN=1` the same bell is rung again when the players really
  lose each other (network trouble).
- Built-in mouse and keyboard controls (`BB_MOUSE_KEYBOARD=1`, or the "Mouse & keyboard"
  section at the bottom of the Insert menu). The relative-mouse plumbing comes from
  `Mush-iii/bloodborne_pc`.

Known limits: guests cannot use lanterns themselves and arrive at the host's lantern, not at
the host's position; a guest that respawned or travelled looks like an ordinary player on its
own screen; three or more players were never tried; enemy scaling and the other extras of the
Souls mods do not exist.

Every player of a session needs the same build and the same switches.

## Using it

1. One player runs a shadNet server (`Wozzardman/shadnet-p2p`) with
   `BloodborneSeamlessCoop=true` and creates an account for each player. The players must be
   able to reach the server and each other (same LAN, or a VPN such as Tailscale).
2. Install bbport as usual and choose the game folder in the launcher (Bloodborne CUSA03173,
   version 1.09, your own copy).
3. Copy the launch script from `packaging/seamless/` next to the game launcher, put the
   server address and your account in it, and start the game with it:
   `Play Seamless Co-op.bat` on Windows, `play-seamless-coop.sh` on Linux.
4. Choose "Play Online", then the host presses PgUp and the guest PgDn. The first match takes
   up to a minute.

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

- Death and boss kill: the session scripts that send a guest home ("HostDead", "BlockClear",
  "PartyGhostDeath") are not scheduled, and a stage transition no longer stops the session.
- After a reload the game does not create the other players' characters again. Each client
  keeps the session handles of the players it has seen and asks the game's own insertion task
  manager for them again.
- Lanterns: the lantern's talk script asks whether a multiplayer session is in progress; it is
  told no.
- Following the host: each client tells its session peers its map and the lantern it last
  travelled to, once a second, on a spare virtual port of the P2P transport
  (`gpu/shadps4/core/libraries/network/p2p_sockets.cpp`). A guest in another map than the host
  uses the game's own headstone warp to that lantern.

`SHADPS4_BLOODBORNE_DEBUG_FILE=<file>` turns on a test aid: the session state is logged when it
changes and one command line written to that file is run (see `ApplyDebugCommands`).
