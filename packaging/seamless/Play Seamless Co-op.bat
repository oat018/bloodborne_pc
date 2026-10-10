@echo off
rem Bloodborne (bbport + seamless co-op) on a private co-op server (shadNet).
rem Copy this file into the bbport folder (next to "Play Bloodborne.exe") and fill in the three
rem values below. In game: PgUp = Beckoning Bell (host), PgDn = Small Resonant Bell (guest),
rem Home = leave the session, End = leave and ring again, Insert = bbport menu.

set SERVER=CHANGE_ME_SERVER_ADDRESS
set BB_SHADNET_NPID=CHANGE_ME_ACCOUNT
set BB_SHADNET_PASSWORD=CHANGE_ME_PASSWORD

set BB_ONLINE=1
set BB_SHADNET_SERVER=%SERVER%:31313
set BB_SHADNET_WEBAPI=http://%SERVER%:31315
set BB_COMMUNITY_SERVER=https://thehuntersdream.com
set BB_UPNP=0
set SHADPS4_BLOODBORNE_SEAMLESS_COOP=1
rem Rings the same bell again when the players really lose each other. Home stops it.
set SHADPS4_BLOODBORNE_AUTO_REJOIN=1
rem Seamless rules: the session survives deaths and boss kills, lanterns work inside the session
rem and guests follow the host when it travels. Every player needs the same setting.
set SHADPS4_BLOODBORNE_STAY_IN_SESSION=1
rem Built-in mouse + keyboard controls: remove "rem " from the next line.
rem set BB_MOUSE_KEYBOARD=1

start "" "%~dp0Play Bloodborne.exe"
