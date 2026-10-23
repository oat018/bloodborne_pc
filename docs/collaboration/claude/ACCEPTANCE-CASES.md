# CL01-C: acceptance cases for the three goals

Owner: external Claude Code (CL01). These are executable test **specifications**. None has been run, and none may be reported as PASS from this document. Source citations are in [BBSC-002-MAP.md](BBSC-002-MAP.md) and [IDENTITY-PROTOCOL.md](IDENTITY-PROTOCOL.md).

## Common setup for every case

**Clients.** Two distinct clients, H (host) and G (guest). Each one has:
- its own copy of `BBSC-0.0.2-BBport.zip` (MAP C4)
- its own copied `BB_USER_DIR`, with `BBSC_SAVE_DIR=<BB_USER_DIR>\savedata\<id>\CUSA00207\SPRJ0005` (MAP C3)
- its own shadNet account. Before starting, check that the two game-API `UserId`s differ (IDENTITY F4).

**Builds.**
- Baseline runs use the unchanged 0.0.2 artifacts. Record SHA256 for `bb-probe.exe`, `bbsc.dll`, `bbsc-launch.exe`, `shadnet.exe` and `BBSC-GameServer.exe`.
- Delta runs name the changed source commit.

**Server.** The user's self-hosted address only. No redeployment. Record which build is deployed if it is known; otherwise mark it "unknown".

**Before each run.**
- Copy both save folders aside.
- Check that the DLL log shows `Image verified ... byte patch sites written` and `HTTP: game's HTTP calls wrapped`.

**Cleanup / retry.** If a run fails at startup with a `bbsc.dll+0x1933` fault, it is a startup failure (MAP C2), not a goal result. Retry it, and count those retries separately. After every case, restore the copied saves before the next one.

**Evidence types.**
- `SRC`: source or fixture inspection.
- `NAT`: native log line (the DLL `logs\bbsc-mod-*.log` / `bbsc-timeline-*.txt`, the BBport `user\last_run.log`, or the server `logs\`).
- `GP`: user gameplay observation, with a screenshot when useful.
- `SAVE`: comparison of save or flag state after reload.

Keep these separate. Do not mix baseline and delta results.

## G1: cross-map join anywhere (parent R03/R04, gate G1, W06)

| ID | Setup / input | Production boundary exercised | Expected | No-side-effect assertions | Evidence | Blocker |
|---|---|---|---|---|---|---|
| A-J1 | Both use the same non-empty password "P". H is in Cathedral Ward with a lamp; G is in Central Yharnam. H rings the Hunt Bell, G rings the Kin Bell | broker `Search` Preparing → `Advertise` delivery → guest `RewriteAreaId` → `ApplyCrossMapSummonGuestPlacement` (native summoned reload) → join teleport | G loads once into H's map. The scene, floor and lighting are complete. G ends within a few meters of H. The DLL log shows `connect-first: placement written` and `summoned reload finished in host map` | No second forced `StageTransition` during the join. H's world is unchanged. G's own save is untouched until a normal save | NAT + GP | R02 composition |
| A-J2 | As A-J1, with the directions reversed (host in Old Yharnam or Forbidden Woods, guest in Cathedral Ward) | same | same | same | NAT + GP | R02 |
| A-J3 | Password "P". (a) H in the Hunter's Dream, G in a story map. (b) G in the Dream, H in a story map | the same path plus the Dream present/restore (§"The Hunter's Dream together") | Join succeeds both ways. After G goes home, G's own Dream state (Doll, workshop, messengers' starter items) is restored | G gains no starter weapons and no Dream progress from H (B bloodborne_re.cpp L11081-11087 rule) | NAT + GP + SAVE | R02 |
| A-J4 | **Password mismatch.** (a) H has no password, G has "P". (b) H has "P", G has "Q". (c) Neither has a password. Different maps each time | broker `MatchesSearch` (N L76-110), plus any F1 delta | **Required:** (a), (b) and (c) give no cross-map join and no warp. Baseline prediction from source: (b) refused; (a) and (c) **joined** (SOURCE-GAP F1). If confirmed, that makes it a REPRODUCED-GAP | G never loads into H's map. G's sign may stay listed. G's position and world are unchanged | NAT (broker log `preparing cross-map`) + GP | R02; F1 delta for re-test |
| A-J5 | **Stale or wrong candidate.** (a) H rings G's sign and then quits before the join; within 90 s a second host H2 (third client, password "P") rings. (b) G restarts its game mid-handshake and rings again | `OnAimedHostPlacement` freshness (90 s), `GetBbscConnectFirstAreaId` clearing on state 4/6, broker one-claim rule | G joins only the host whose claim was delivered, or no host. It never loads into H's map and then sits beside H2, or the reverse. A restarted G starts clean (the forced delete of its stale record) | No placement older than the current claim is applied. G's world is unchanged after a failed join | NAT (`host … rang our sign`, `placement written`, the delivered claim) + GP | needs a third distinct account for (a); otherwise WAIT_DEP |
| A-J6 | **Same-map rules.** Same map, no password, level difference beyond 10 + level/5 | broker anywhere mode skipping the level and distance filters | Record the baseline behavior (source predicts a match). Codex or the user decides whether this is acceptable; it is not a goal blocker | — | NAT + GP | decision |
| A-J7 | **Unsupported area (observe only).** H in a Chalice Dungeon rings; G in a story map. Separately: H takes a lamp into a Chalice Dungeon while G is a guest | placement and follow, which have no area-29 filter | Record what happens. The release lists this as unsupported. The required outcome if a delta is made: refusal or a safe send-home, never a stranded guest | G's save is intact after reload | NAT + GP + SAVE | none; a delta only if the behavior is unsafe |

## G2: earned progress sync (parent R03/R04, gate G2)

Before each case, list the selected boss and kind-1 flags of both saves. A save-flag dump is out of scope for CL01; Codex owns any tool for it.

| ID | Setup / input | Production boundary | Expected | No-side-effect assertions | Evidence | Blocker |
|---|---|---|---|---|---|---|
| P-1 | **Host history.** H has already killed Cleric Beast and Father Gascoigne and opened the Cathedral Ward shortcuts. G has done none of these. G joins and they walk around without killing a boss. G goes home with Silencing Blank, saves, quits and reloads | `TickBbscSharedProgress` collection → `MergeBbscSharedProgress` at home | **Required:** G's world does **not** gain H's earlier boss flags or shortcuts. Source prediction for the baseline: they **are** gained (SOURCE-GAP F3) | G's stats, inventory, echoes and Insight are unchanged | SAVE + NAT (`shared progress: … boss flag(s)`) | R02; F3 delta for re-test |
| P-2 | **Jointly earned boss.** Neither has killed the target boss. G joins, both kill it, and G goes home | boss-drop hook (19-event table), then collection and merge | G's world has that boss's completion flag(s) after reload. G received the event's lot exactly once | No other boss or shortcut flags from H's history (with F3). No change to NPC choices | SAVE + GP | R02 |
| P-3 | **Admission boundary.** H kills a boss while G is still loading the first join (before first playable). Separately: H kills a boss after G has left | collector start (state 6, player type 1) and end (left + 180 s) | Neither kill counts for G. A kill between first playable and departure does count (P-2) | — | SAVE + NAT | F3 delta defines "first seen" |
| P-4 | **Own progress kept.** G is ahead (it has a boss H lacks, and a lamp H lacks). G joins H and goes home | own-world journal restore → merge (add-only) | After reload, G still has its own boss and lamp. Nothing was removed | No flag goes from 1 to 0 in G's save | SAVE | R02 |
| P-5 | **NPC/quest phase.** H and G have different choices for one NPC quest (e.g. sending survivors to different places). G joins and goes home | kind-1 merge (kinds 6/7 are excluded) | G's NPC state is unchanged | No quest-phase flag set from H | SAVE | needs a per-flag check of the merged kind-1 bits |
| P-6 | **Reward replay or missing reward.** (a) G has already taken a boss's lot, and they kill that boss again together (NG or a repeat event): G gets the repeat lot or nothing, never a duplicate key item. (b) Where a boss flag reaches G through the merge (baseline P-1 path), check that its required key item is not now unobtainable | boss-reward table first/repeat lot; loot view; merge | (a) no duplicate. (b) Record it. If it is unobtainable, that is a further gap tied to F3 | — | SAVE + GP | R02 |
| P-7 | **Crash while a guest.** After P-2's kill, kill G's process (Task Manager) within 10 s, and again after more than 10 s. Restart G alone (offline is fine) and load the same character | pending file written every 10 s (temp + rename); recovery `RecoverBbscOwnWorldFile` → `LoadBbscPendingProgress` once home | After more than 10 s, the kill is merged at the next home load. Under 10 s, it may be lost; report it honestly. G's own world groups come back from the journal before the merge | No host flags remain in G's own world beyond the eligible ones. No double merge on a second reload | SAVE + NAT (`own world restored after a crash`) | R02 |
| P-8 | **Wrong character** (required if a delta adds persistent writes). G has two characters (slots 0 and 1). G joins with slot 0, the process is killed, then G loads slot 1 first | `BbscActiveSaveSlot` (newest write) keys the pending and journal files | Slot 1 receives nothing from slot 0's files. Slot 0 is recovered when it is next loaded | No cross-slot writes | SAVE + NAT | F5; a delta only if it fails |
| P-9 | **Backup failure** (required if a delta touches the merge). Make the save-backups folder unwritable | `BackupBbscSaves` → merge proceeds anyway (MAP G2 row) | Record the baseline. For a delta that changes persistence: no merge without a successful backup, or an explicit user-visible warning | Original saves stay intact | NAT + SAVE | delta-dependent |

## G3: self-hosted services (parent R03, W06)

| ID | Setup / input | Production boundary | Expected | No-side-effect assertions | Evidence | Blocker |
|---|---|---|---|---|---|---|
| S-1 | Client account created with `CREATE ACCOUNT.cmd` against the private address (typed, not the card) | shadNet 31313 registration and login | Login succeeds. `tools\account.dat` names the private server. `host_overrides.custom.json` points 20443 and 31315 at it | The public card server is never contacted (check the BBport log and the overrides file) | NAT | user account on the private server |
| S-2 | Run A-J1 while watching the server logs | 20443 `ss.info` and login; 31315 broker; 20444 relay connect (`relay: <name> connected (2 player(s))`) | All join traffic appears in the private server's logs. The DLL log shows the relay connected (not "public game server … P2P channel") | No request to thehuntersdream.com or play.seamlesshunt.com | NAT (client + server logs) | server log access |
| S-3 | Force the P2P relay: both clients on one PC, or `SHADPS4_P2P_RELAY` left at its default | UDP 3478 `Relay._shad` | Session established. The server logs `shadPS4 relay: <npid> registered` | — | NAT | — |
| S-4 | Stop the game server (20443/20444) for 2 minutes mid-session, then start it again. shadNet keeps running | relay reconnect loop; game API retries | Record the baseline. The required minimum: G does not lose its saves. Afterwards a normal departure still returns G home | Saves intact | NAT + GP + SAVE | user permission to restart their own service; else WAIT_DEP |

## G1/G3 safe exit and recovery (gate G3)

| ID | Setup / input | Production boundary | Expected | No-side-effect assertions | Evidence | Blocker |
|---|---|---|---|---|---|---|
| R-1 | Normal departure: G uses Silencing Blank. Repeat with H using a Blank on G | §"Back home where we were" (spot kept at `JoinSession_*`) | G loads at its pre-join spot in its own world with its own world flags (plus eligible progress) | G does not end up in the area H last took it to | GP + SAVE | R02 |
| R-2 | Disconnect: kill H's process during a session | stranded-guest path (`TickBbscStrandedGuest`), own-world restore | G gets home within the baseline grace period. Its saves and world are its own | No H flags remain in G's world after reload | NAT + GP + SAVE | R02 |
| R-3 | Interrupted session: kill G's process while visiting H in a map G has not unlocked. Restart G and load the character | save position plus own-world journal recovery. The kept spot is memory-only | Record where G loads. Required: G is in its own world at a valid spot, and the journal is restored. If G loads into a map it has not unlocked, or is stuck, that is a REPRODUCED-GAP against requirement 4/7 | Original saves (copied aside) are not needed to recover | NAT + GP + SAVE | R02 |

## Order for R03 baseline reproduction

1. S-1, S-2: the route.
2. A-J1, A-J3a: the basic join.
3. A-J4: the F1 check.
4. P-1, P-2, P-4: the F3 check.
5. R-1, R-2, R-3.
6. P-7.
7. The rest as the inputs become available.

Only failures reproduced here feed R04 deltas, following the alignment plan's five-part change rule.
