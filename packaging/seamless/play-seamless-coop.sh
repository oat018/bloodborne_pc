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
# Experimental: a guest also keeps the doors, gates, shortcuts and lifts the host's world opens
# during the session. Every player needs the same setting; remove "# " from the next line.
# export SHADPS4_BLOODBORNE_SHARE_DOORS=1
# export BB_MOUSE_KEYBOARD=1   # built-in mouse + keyboard controls

here=$(cd -- "$(dirname -- "$0")" && pwd)
# Where the game's own server addresses go: sign-in and messages to the community server,
# summon signs to the shadNet server. Without this file the game cannot go online.
community_host=${BB_COMMUNITY_SERVER#*://}
export SHADPS4_HTTP_HOST_OVERRIDES_JSON="$here/host_overrides.json"
cat > "$SHADPS4_HTTP_HOST_OVERRIDES_JSON" <<EOF
{
  "https://ss4.scej-network.jp:20443": "$BB_COMMUNITY_SERVER",
  "https://$community_host:18671/summon_messenger": "$BB_SHADNET_WEBAPI",
  "http://$community_host:18671/summon_messenger": "$BB_SHADNET_WEBAPI"
}
EOF
# Everything the game prints is kept in last_run.log next to this script (the run before it
# in last_run.prev.log): send both files when something goes wrong.
log=$here/last_run.log
[[ -f $log ]] && mv -f "$log" "$here/last_run.prev.log"
if [[ -x $here/Bloodborne-bbport-x86_64.AppImage ]]; then
    "$here/Bloodborne-bbport-x86_64.AppImage" --play "$@" 2>&1 | tee "$log"
else
    bash "$here/run.sh" "$@" 2>&1 | tee "$log"
fi
