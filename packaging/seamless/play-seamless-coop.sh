#!/usr/bin/env bash
# Bloodborne (bbport + seamless co-op) on a private co-op server (shadNet), Linux.
# Put this file next to Bloodborne-bbport-x86_64.AppImage, fill in the three values below,
# then: chmod +x play-seamless-coop.sh Bloodborne-bbport-x86_64.AppImage && ./play-seamless-coop.sh
# Run the AppImage once without this script first to choose the game folder in the launcher.
# Keys in game: PgUp = Beckoning Bell (host), PgDn = Small Resonant Bell (guest),
# Home = leave the session, End = leave and ring again, Insert = bbport menu.
set -euo pipefail
SERVER=CHANGE_ME_SERVER_ADDRESS          # the server PC's address (Tailscale or LAN)
export BB_SHADNET_NPID=CHANGE_ME_ACCOUNT # your account on that server
export BB_SHADNET_PASSWORD=CHANGE_ME_PASSWORD

export BB_ONLINE=1
export BB_SHADNET_SERVER="$SERVER:31313"
export BB_SHADNET_WEBAPI="http://$SERVER:31315"
export BB_COMMUNITY_SERVER=https://thehuntersdream.com
export BB_UPNP=0
export SHADPS4_BLOODBORNE_SEAMLESS_COOP=1
# Rings the same bell again when the players really lose each other. Home stops it.
export SHADPS4_BLOODBORNE_AUTO_REJOIN=1
# Seamless rules: the session survives deaths and boss kills, lanterns work inside the session
# and guests follow the host when it travels. Every player needs the same setting.
export SHADPS4_BLOODBORNE_STAY_IN_SESSION=1
# Experimental: every player has its own items lying in the world (a guest picks up
# the host world's items for itself). Every player needs the same setting; remove "# " from
# the next line.
# export SHADPS4_BLOODBORNE_GUEST_ITEMS=1
# export BB_MOUSE_KEYBOARD=1   # built-in mouse + keyboard controls

here=$(cd -- "$(dirname -- "$0")" && pwd)
if [[ -x $here/Bloodborne-bbport-x86_64.AppImage ]]; then
    exec "$here/Bloodborne-bbport-x86_64.AppImage" --play "$@"
fi
exec bash "$here/run.sh" "$@"
