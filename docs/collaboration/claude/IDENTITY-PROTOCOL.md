# CL01-B: self-hosted route and identity at the changed join/progress boundaries

Owner: external Claude Code (CL01). Pins and short names (**B**, **S**, **N**, **Z**) are the same as in [BBSC-002-MAP.md](BBSC-002-MAP.md). Source audit only. No private server, account or service was contacted.

## 1. Self-hosted route: what 0.0.2 already supplies

Evidence classes:

- **REF**: the pinned reference release or source.
- **DEPLOYED**: the user's running server. Its version is unknown and was not contacted.

Everything below is REF.

| Service | Port | Component (REF) | Source available? | Client route on BBport |
|---|---|---|---|---|
| shadNet login + Matching2 rooms | TCP 31313 | `server/shadnet.exe` (Z Server) | Yes: N@5a849a1 (named in [B server/README.txt L56-61](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/server/README.txt#L56-L61)). The binary was not rebuilt or compared | `BB_SHADNET_SERVER` (Z `play-bbport.ps1`), read by S [emulator_settings.h L83](https://github.com/Supermedo/bloodborne_pc/blob/e5d04e2ba4864ccf40de6c3cf6aa532855320b0d/gpu/shim/core/emulator_settings.h#L83) |
| STUN reflector | UDP 31314 | shadnet.exe | as above | derived from the shadNet address |
| WebAPI + Bloodborne summon broker | TCP 31315 | shadnet.exe, with `BloodborneSeamlessCoop=true` ([B shadnet.cfg](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/server/server/shadnet.cfg)) and `SHADNET_BLOODBORNE_SEAMLESS_COOP=1` (Z Server `tools/start-host.ps1` L94-95) | N `src/bloodborne_summon_broker.cpp` | `host_overrides.custom.json` summon URLs → `http://<srv>:31315`; `BB_SHADNET_WEBAPI` |
| Game API (`ss.info`, login, messages, bloodstains, channels, notices) | TCP 20443 | `server/BBSC-GameServer.exe` (Z Server). Started with `--shadnet <ip> --host 0.0.0.0 --advertise <ip> --summon-interval 5 --no-playlog` (start-host.ps1 L97-98) | Yes: B `server/gameserver/bbsc_gameserver.py` ([README](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/server/gameserver/README.md)). The exe is presumably a packaged copy of this script; no build recipe is tracked, so equivalence is **not verified** | overrides `https://ss4.scej-network.jp:20443` → `http://<srv>:20443` |
| BBSC relay (follow, world flags, life, scaling, progress, aimed placement) | TCP 20444 | same game-server process ([L1033-1038](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/server/gameserver/bbsc_gameserver.py#L1033-L1038), `--relay-port` L1472) | yes (B) | the DLL derives it from the overrides file's game-server host, or from `BBSC_RELAY` ([B bbsc_relay.cpp L15-16](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/emulator/mod/bbsc_relay.cpp#L15-L16)). A **public** host is skipped, and BBSC traffic then goes over the P2P channel instead |
| P2P relay fallback (game UDP frames, by online id) | UDP 3478 | same process, bbhost module `Relay._shad` ([B bbhost_service.py L750-799](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/server/gameserver/bbhost_service.py#L750-L799)). It is on unless `--no-bbhost` (L1492) | yes (B) | patched `bb-probe.exe` uses UDP 3478 on the **shadNet** server address and forces it for every peer by default ([B patch L136-160](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/bbport/bbport-1.7.2-bbsc.patch#L136-L160)). `SHADPS4_P2P_RELAY=0/auto/host:port` changes this |
| bbhost sessions (for bbhost-emulator players) | TCP 18671 | same process | yes | not used by BBport/shadNet players |
| Account creation | TCP 31313 | `shadnet-sample.exe` via `CREATE ACCOUNT.cmd` / `create-account.ps1` (Z) | N | asks for a server: "Enter = card server, or type another". A typed private server drops the public card's key (Z `create-account.ps1` L10-18) |

**Answer for G3.** On a private address, every route that the join and progress paths need goes through these components: 31313, 31314, 31315, 20443, 20444 and UDP 3478. All of them are on the **same host address**, because the client derives 20443/20444/31315/3478 from the one account server (Z `play-bbport.ps1`; B patch L612 "port 3478 on the shadNet server's address"). No required route depends on a public BBSC or THD service.

Reuse the release's own pieces:

- `START-HOST.cmd` for the server.
- `CREATE ACCOUNT.cmd` with a typed private address.
- `PLAY BBPORT + BBSC.cmd` for the client.

Configuration notes that hold without any code change:

- The client's port numbers are fixed (20443/31315 written by `play-bbport.ps1`, 20444 by default, 3478 by default). A self-host with different ports needs either a hand-written `host_overrides.custom.json` plus `BBSC_RELAY` / `SHADPS4_P2P_RELAY_PORT`, or the same ports.
- `run.py` replaces the overrides **only** when no custom file exists ([S run.py L46-70](https://github.com/Supermedo/bloodborne_pc/blob/e5d04e2ba4864ccf40de6c3cf6aa532855320b0d/run.py#L46-L70)). After `play-bbport.ps1` has written `user\host_overrides.custom.json`, any later plain BBport run also uses the BBSC server.
- Launcher mode: BBport's Online page overrides the environment (S [bbport_launcher_win.py L460-475](https://github.com/Supermedo/bloodborne_pc/blob/e5d04e2ba4864ccf40de6c3cf6aa532855320b0d/launcher/bbport_launcher_win.py#L460-L475)). Keep the pool on shadNet; the `huntersdream` pool starts bbbridge instead.
- The public default is in Z `tools/BBSC-card-SeamlessBloodborne.json` (`play.seamlesshunt.com:31313`). It stays unused once account.dat names the private server.

Deployment, redeployment and contacting the user's service are out of scope. S-1..S-4 in [ACCEPTANCE-CASES.md](ACCEPTANCE-CASES.md) observe the route from the client side only.

## 2. Identity chain as implemented (REF)

| Identity | Producer | Consumer | Trust basis in the source |
|---|---|---|---|
| shadNet account (npid + password) | shadNet login, TCP 31313 (`BB_SHADNET_NPID` / `_PASSWORD`) | Matching2 rooms, signaling peers | Password login against the shadNet DB. Unencrypted TCP (B docs/KNOWN-LIMITATIONS.md L24) |
| Game-API `UserId` / `SessionId` | `BBSC-GameServer /basic_utils/login`: `user_id = 1000 + sha256(PlatformAccountId)[0:4] % 9000`; `SessionId` = new uuid4 ([B bbsc_gameserver.py L156-188](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/server/gameserver/bbsc_gameserver.py#L156-L188)) | broker sign/claim keys (`UserId`, `SessionId`, `TargetUserId`); the DLL's own user id for the aimed placement | `PlatformAccountId` is **client-asserted** and not checked against the shadNet login. Only 9,000 possible values |
| Summon sign / claim | game JSON to `/summon_messenger/create|get|request` | broker `MatchesSearch` / `Claim` (N [L76-110](https://github.com/Wozzardman/shadnet-p2p/blob/5a849a192fd4450593c57b10697316ac324c856e/src/bloodborne_summon_broker.cpp#L76-L110), [L495-516](https://github.com/Wozzardman/shadnet-p2p/blob/5a849a192fd4450593c57b10697316ac324c856e/src/bloodborne_summon_broker.cpp#L495-L516)) | Keys are the game's own fields. The password filter is `SummonWord` |
| Host placement (cross-map destination) | host DLL: on `/request`, it adds the header to the claim **and** sends relay frame `SHAD 0x31 "<TargetUserId>|<placement>"` ([B bbsc_http.cpp L507-524](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/emulator/mod/bbsc_http.cpp#L507-L524)) | guest DLL `OnAimedHostPlacement`: accepts when `target == own UserId` and the guest is not in state 3/4/6, then locks onto `sender` as its host ([L726-746](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/emulator/mod/bbsc_http.cpp#L726-L746)) | The `sender` id is a random u32 per process, **self-declared** in the envelope ([B bbsc_relay.cpp L500-504, L257-283](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/emulator/mod/bbsc_relay.cpp#L257-L283)). The server forwards every frame to everyone and says its session hints are "routing hints, not authentication" ([B bbsc_gameserver.py L1151-1157](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/server/gameserver/bbsc_gameserver.py#L1151-L1157)) |
| Host-only relay packets (follow, world flags, progress 0x26, scale, boss) | host DLL, about 2/s and every 5 s | guest `from_our_host()`: `sender == session == GetBbscRelayHost()` while the guest is in state 4/6 ([B bbsc_relay.cpp L281-335](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/emulator/mod/bbsc_relay.cpp#L281-L335)) | Bound to the host id learned from the aimed placement. The ids are self-declared |
| P2P relay registration | client `"BBR1" 0x01 id[16]` every 5 s | server `_shad`: last registration of an id wins; frames are accepted only from the registered address ([L757-768](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/server/gameserver/bbhost_service.py#L757-L768)) | Registration itself is unauthenticated |
| Character / save slot (progress, own world, loot, backups) | DLL `BbscActiveSaveSlot`: newest `userdataNNNN` written since run start, or the `BBSC_SAVE_DIR` slot ([B bloodborne_re.cpp L5905-5975](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/emulator/src/core/bloodborne_re.cpp#L5905-L5975)) | pending progress, own-world, loot files keyed `<user>-<slot>` | Uses file modification times. No character discriminator. `--user-id` is honored only for the shadPS4 `home` layout |

The only part with real authentication is shadNet login (account password). The game-API id, the relay sender and the P2P relay id are not bound to it. In the release's model that is acceptable: a private server is shared by friends who all hold the registration key. **No new HMAC or capability protocol is recommended.** The findings below are limited to the cases where these weak bindings actually decide the three goals.

## 3. Findings at the candidate changed boundaries

**F1. Cross-map join without a matching non-empty password (G1, SOURCE-GAP).**

*What the source does.*
- With `BloodborneSeamlessCoop=true`, `anywhereSummons` is on (N L355-362).
- `MatchesSearch` rejects a sign only when `!requestWord.isEmpty() && requestWord != signWord` (N L105-109).
- So a host with no password finds any guest sign, passworded or not, in any area.
- Then `Search` (N L417-431) marks a sign in another map `Preparing` and sends the guest a placement, which leads to the cross-map load.
- Neither the BBSC DLL nor the bbsc_gameserver.py summon path sees the password. bbhost's own `_matches` requires equality ([B bbhost_service.py L425-428](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/server/gameserver/bbhost_service.py#L425-L428)), but that path is not used by shadNet/BBport players.

*To reproduce.* Run A-J4. The host has no password, the guest has "X", in different maps, on the private server. Expected baseline: the join happens. That would contradict requirement 2.

*Smallest candidate fixes (choose after reproduction):*

- (a) **Server, one function.** In `MatchesSearch`, when anywhere mode would match a sign whose `AreaId` differs from the request's, require `requestWord` non-empty and equal to `signWord`. Same-map behavior stays unchanged.
  - Producer: the game's `SummonWord`. Consumer: the broker.
  - The trust basis is unchanged, because the broker already owns matching.
  - Cost: rebuilding the self-hosted shadnet.exe, which is outside the current no-redeploy boundary.
- (b) **Client only, within the existing consumer.**
  - The guest DLL remembers the `SummonWord` of its last `/summon_messenger/create` body.
  - The host DLL appends its own search word to the aimed frame (`"<target>|<word>|<placement>"`).
  - `OnAimedHostPlacement` refuses a cross-map placement unless both words are non-empty and equal.
  - The server needs no change, but both clients must run the changed DLL.
  - The relay is unauthenticated, so this protects honest clients, not against a modified one. That matches the release's trust model.

*Not recommended:* any new crypto/HMAC, or replacing the broker.

**F2. The aimed placement is bound to the broker claim by `UserId` only (G1, stale/wrong candidate).**

*What the source does.*
- The guest takes the first `0x31` frame naming its `UserId` and makes that sender its host. Nothing checks that this sender is the host whose claim the broker delivers.
- Two hosts ringing the same sign within the 90 s window, or a stale frame, could make the guest place itself at host A while the broker delivers host B's claim.
- Mitigations already in the source:
  - the broker gives one claim per sign (`Conflict` for a second claimant, N L500-504)
  - the host sends the frame only together with an actual claim request
  - the aimed placement expires after 90 s (B bbsc_http.cpp L85-88)
  - the placement is cleared on state 4/6 (L16975-16993)

*Status.* PRESENT-NOT-TESTED. It becomes a gap only if A-J5 shows a wrong-host placement on the private server.

*Minimal fix if needed (consumer side, no protocol).* After the claim is delivered, the guest compares the claim's requester `UserId` (from the delivered `SummonDataCreateResponse` body, which the guest's `ReadData` wrapper already sees) with the `UserId` the host sent along. This needs the host's game-API `UserId` added to the `0x31` text. Do not infer identity from equal endpoints.

**F3. Progress eligibility has no admission baseline (G2, SOURCE-GAP).** The facts are in [BBSC-002-MAP.md §2](BBSC-002-MAP.md#2-three-goal-reuse-matrix).

*Identity seam.* Progress arrives only through host-only relay packets (`0x26`, world flags) from the locked host id. The guest side therefore already ties "which host" to the session. The missing piece is **when**, not **who**. The smallest delta needs no new identity:

- On the guest, record the first-seen value of each boss flag and each kind-1 group after reaching state 6.
- Collect only the bits that change from 0 to 1 afterwards.

This reuses the existing `TickBbscSharedProgress` / `OnBbscProgressPacket` producer and consumer and the existing pending file. A guest that re-joins the same host in a later session starts a new baseline, so events from between sessions are never imported.

**F4. Game-API `UserId` collisions (G1 matching, deterministic).** `user_id = 1000 + (sha256(PlatformAccountId)[0:4] mod 9000)` (B bbsc_gameserver.py L159-163). Two accounts that land on the same value cannot see each other's signs, because `MatchesSearch` drops `sign.UserId == requester` (N L79-81), and the aimed placement would target both. The probability is low (about 1/9000 per pair) but **permanent** for that pair of names. *Check:* compute both test accounts' ids before A-J1, and pick different names if they collide. This is a configuration step, not a code change.

**F5. Character identity uses file modification times (G2 persistence; affects test setup).** See BBSC-002-MAP.md C3/C4. For the R03 copied profiles, set `BBSC_SAVE_DIR` per client and use separate package copies. The source's own 2026-10-12 incident is why this matters: a guest wrote its journal under the host's newer save ([B bloodborne_re.cpp L5861-5865](https://github.com/CLSAMG22/BBSC/blob/4f11faf88b6ba658ebde6fe5fbefdadedba4fa41/emulator/src/core/bloodborne_re.cpp#L5861-L5865)). If the F3 delta adds persistent writes, P-8 (wrong character) becomes a required case.

## 4. Not pursued (recorded, by scope)

- Unencrypted shadNet TCP and an unauthenticated relay: release limitations, private-network use.
- THD/bbhost compatibility, public-server enforcement and cross-emulator play.
- A new account service.
- Rebuilding or redeploying the user's server.
- The deployed server's source and version: unknown, and not asked again here.
