@echo off
rem Bloodborne (bbport + seamless co-op) on a private co-op server (shadNet).
rem Copy this file into the bbport folder (next to "Play Bloodborne.exe") and fill in the three
rem values below. In game: PgUp = Beckoning Bell (host), PgDn = Small Resonant Bell (guest)
rem (or the bells from the items), Home = leave the session, Insert = bbport menu; without a
rem lobby: End = leave and ring again.

set SERVER=CHANGE_ME_SERVER_ADDRESS
set BB_SHADNET_NPID=CHANGE_ME_ACCOUNT
set BB_SHADNET_PASSWORD=CHANGE_ME_PASSWORD

set BB_ONLINE=1
set BB_SHADNET_SERVER=%SERVER%:31313
set BB_SHADNET_WEBAPI=http://%SERVER%:31315
set BB_COMMUNITY_SERVER=https://thehuntersdream.com
set BB_UPNP=0
set SHADPS4_BLOODBORNE_SEAMLESS_COOP=1
rem Co-op lobby: one player hosts, the others join with the same password. The players meet once
rem the host has rung the Beckoning Bell and a joining player the Small Resonant Bell; the
rem joining game then goes into the host's world by itself (it travels to the host's map
rem first). Deaths keep the session; after Home or a restarted game, ring again. The host's game listens on TCP port 31320
rem (SHADPS4_BLOODBORNE_LOBBY_PORT): on the host's PC allow "bb-probe.exe" when Windows asks.
rem The others reach it at the server address unless SHADPS4_BLOODBORNE_LOBBY_ADDRESS names the
rem host's address. Put "rem " in front of the two lines to play with the bells instead.
set SHADPS4_BLOODBORNE_LOBBY=host
set SHADPS4_BLOODBORNE_LOBBY_PASSWORD=CHANGE_ME_LOBBY_PASSWORD
rem Without a lobby: rings the same bell again when the players really lose each other. Home
rem stops it.
set SHADPS4_BLOODBORNE_AUTO_REJOIN=1
rem Seamless rules: the session survives deaths and boss kills, lanterns work inside the session
rem and guests follow the host when it travels. Every player needs the same setting.
set SHADPS4_BLOODBORNE_STAY_IN_SESSION=1
rem Experimental: every player has its own items lying in the world (a guest picks up
rem the host world's items for itself). Every player needs the same setting; remove "rem " from
rem the next line.
rem set SHADPS4_BLOODBORNE_GUEST_ITEMS=1
rem Experimental: a guest also keeps the doors, gates, shortcuts and lifts the host's world
rem opens during the session. Every player needs the same setting; remove "rem " from the
rem next line.
rem set SHADPS4_BLOODBORNE_SHARE_DOORS=1
rem Mouse + keyboard: the Controls page of Bloodborne.exe, or input.ini (docs/INPUT.md).

start "" "%~dp0Play Bloodborne.exe"
