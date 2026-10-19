# BBSC game server

What makes Bloodborne believe it is online, without depending on anybody else's machine.

> Need help with any of this, or with shadPS4 and Bloodborne on PC generally?
> Join the **Bloodborne PC** Discord — it is the main community for running this game on PC.

## Why this exists

Before Bloodborne enables a single online feature it fetches:

```
https://ss4.scej-network.jp:20443/bb-eu/ss.info
```

That file is **base64-encoded XML**, and it is the **server list**. A `<gameurl2>` block names one
URL per game API — login, notices, blood messages, ghosts, summoning, play logs — plus the timing
and matching parameters the online systems run on. Everything the game does afterwards goes
wherever that file says.

Private servers work by redirecting that one URL. Previously it pointed at `thehuntersdream.com`,
so their `ss.info` decided where *our* co-op went. On 2026-09-21 their backend stopped responding
(TCP connected, no HTTP reply, on both 18671 and 20443) and co-op stopped working for us with it.

Serving our own removes that dependency for good.

## What it does

| Endpoints | Where they go |
| --- | --- |
| the four `api_SummonData*` | the real shadnet server — this is where co-op actually happens |
| everything else | this process, which answers empty-but-valid JSON so the game stops asking |

Blood messages, ghosts, notices and channels are **not implemented**. They get well-formed empty
replies, which is enough to keep the game happy; this does not pretend to provide them.

Each player runs their own copy on `127.0.0.1`. Only the summon endpoints are shared, which is why
the shadnet address is the one thing you have to set.

## Running it

```
python bbsc_gameserver.py --shadnet 100.101.102.103
```

Then in `%APPDATA%\shadPS4\host_overrides.json`:

```json
{ "https://ss4.scej-network.jp:20443": "http://127.0.0.1:20443" }
```

Standard library only — no Flask — so a stock Python runs it. Leave the window open while playing;
it logs every request it answers, which is the fastest way to see whether the game is really
talking to us.

| Option | Effect |
| --- | --- |
| `--shadnet ADDR` | **required** — the machine running `shadnet.exe` |
| `--summon-interval N` | seconds between summon-sign checks (retail 35 / 30) |
| `--level-range N` | co-op level window (retail ±20) |
| `--host` / `--port` | where to listen (default `127.0.0.1:20443`) |
| `--advertise ADDR` | address to write into `ss.info` when it differs from `--host` |
| `--verbose` | log request headers and bodies |

Defaults change nothing except the addresses. Retuning is opt-in.

## Retail tuning values

Taken from the real `ss.info`, recorded here because they are not visible anywhere in the game and
several of them explain behaviour we spent a long time blaming on the emulator.

| Tag | Retail | What it controls |
| --- | --- | --- |
| `SummonDataGetListInterval2` | 35 | seconds between checks for a summon sign |
| `SummonDataCreateInterval2` | 30 | how often a host re-posts its sign |
| `SummonDataGetListGuardTimer2` | 60 | guard timer on the sign list |
| `SummonDataGetListGetMaxCount2` | 20 | most signs returned at once |
| `SummonDataCoopMatchingLevelUpperAbs2` | 20 | co-op level window, upper |
| `SummonDataCoopMatchingLevelLowerAbs2` | −20 | co-op level window, lower |
| `SummonDataBloodMadHunterPercent2` | 5 | blood-mad hunter chance |
| `ReloadServerStatusInfoInterval2` | 300 | how often the game re-reads `ss.info` |

**`SummonDataGetListInterval2` is what "quick summoning" means.** A sign can take over half a
minute to appear on retail settings, and no amount of work inside the game could have changed it —
the value lives in this file.

## Native character IDs

`/basic_utils/sync_chara_id` now reserves IDs before replying with
`PublishCharacterIdList: [{"PublishCharaId": <integer>}]`. The retail 1.09 client
requests `CharaIdNum: 1` when its current character has no server ID and persists
the result through its existing serializer. This replaces the empty-list stub.
It does not enable the unfinished BBSC recovery or progress-sync coordinator.

IDs are positive integers at most 2^53-1 and are reserved in SQLite using full
synchronous commits. The database is `<--data path>.characters.sqlite3`; keep it
on persistent storage alongside `online-data.json`, including in containers and
backups. Request retries may reserve additional unused IDs. Storage errors return
`ResKind: 1` and no IDs; corrupt databases are not silently reset at startup.

Verification: `python3 -m unittest discover -s tests -p test_bbsc_character_ids.py`
from the repository root, inside `bbport-dev`. Controlled gameplay against the
updated private server confirms native allocation, persistence across a new
process, and a distinct ID after deletion/recreation in the same slot. An older
character may already
contain the native unset sentinel from the empty-list reply; this change does not
rewrite existing character IDs or repair those saves automatically.

### Unraid image checkpoint

`Containerfile.native-id` layers only the updated server script over the locally
loaded `localhost/bbsc-gameserver:0.6-pre1`. Its entrypoint, arguments and image
runtime configuration are inherited. Build from the repository root in
`bbport-dev`, using the host's existing Podman:

```sh
distrobox-host-exec podman build --pull=never --network=none \
  -f scripts/bbsc_gameserver/Containerfile.native-id \
  -t localhost/bbsc-gameserver:0.6-pre1-native-id1 scripts/bbsc_gameserver
```

The prepared Docker archive is
`out/containers/bbsc-gameserver-0.6-pre1-native-id1.tar.gz`, with a companion
`.sha256` file. Copy both to the same folder on Unraid, verify with
`sha256sum -c bbsc-gameserver-0.6-pre1-native-id1.tar.gz.sha256`, then run
`docker load -i bbsc-gameserver-0.6-pre1-native-id1.tar.gz` from that folder.

Change only the gameserver template Repository to
`localhost/bbsc-gameserver:0.6-pre1-native-id1`. Keep `--init --pull=never`,
the existing network/environment settings, and the confirmed mapping
`/mnt/app-data/appdata/bbsc/gameserver` → `/data`. Apply the template. The
shadNet image remains `localhost/bbsc-shadnet:0.6-pre1`.

The new registry will be
`/mnt/app-data/appdata/bbsc/gameserver/online-data.json.characters.sqlite3`.
Do not remove it when updating or rolling back the image. Rollback uses the
original gameserver tag with the same data mapping. Local isolated-container
checks verify HTTP allocation, HTTP/STUN health, UID99 writes and retention of
IDs after container recreation using the same volume. The user reports installing
this image on Unraid, and the private allocation endpoint returns a positive ID.
Native allocation, save/reload and same-slot replacement checkpoints now pass.
The selected-character reader and native snapshot/file hash correlation also pass
their copied-profile trials. Atomic generation/hash receipts, actual NG transition
tracking and the durable recovery/progress consumers remain unfinished; see the
[integration evidence](../../docs/superpowers/specs/2026-10-18-bbsc-integration-evidence.md).

## Credit

The `ss.info` format, the endpoint list and the JSON response shapes come from
[NoDreamForHunter](https://github.com/fhody125-web) by **fhody125**, MIT licensed — see
`LICENSE.NoDreamForHunter`. He handed over the project and a real `ss.info` when thehuntersdream
went down, which is the only reason any of this was possible. `ss_info_template.xml` is his file
with the shadnet address left as a placeholder.

This script is a standalone reimplementation rather than a fork: no Flask dependency, and it can
retune the summon parameters on the way out.
