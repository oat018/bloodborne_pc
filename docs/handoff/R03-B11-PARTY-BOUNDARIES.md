# R03 B11: party boundaries, dead guest, pre-placement rewards

2026-10-26, Claude (R03 takeover). Base: seamless `4492dd5` (B09/B10 merged). Source change in the tracked patch (`bloodborne_re.cpp` only). Same native setup as B09/B10:
- two online copied clients on the owner's self-hosted server;
- fresh copies of the protected Kaori host and U-I-AHH guest saves;
- shared progress ON;
- the disclosed debug aids (position write, HP-0 after the native boss bar, and now the guest's own HP 0).

## Changes

1. **Leaving ends the party.**
   - Native evidence: B08 showed session state 6 holding through the guest's death and the whole wipe reload, and changing only at a real departure (6→7→0).
   - `TickBbscSharedProgress` now raises the existing party-reset counter on any 6→not-6 transition.
   - This covers what the 5 s relay-unlock rule did not separate (CL03 C4 / G2-22): a rejoin to the same host before the unlock, and a host that quits and comes back with another character under the same relay id.
2. **Reward item only after placement** (CL02 A-27 / G2-03 contract). `NoteBbscBossRewardEvent` now also requires `bbsc_join_teleport_done`. A boss whose wait reaches the guest during its join load is logged `before we were placed in the host's world - not ours` and not queued. The flag holds through death and loads in the session, and is set again after a rejoin.

## Fixtures

- `test_bbsc_party_progress.py` adds a scenario: the guest leaves and rejoins the same host while the relay lock is still held. Flags the host gained in between are history, and earlier collected progress is kept. **RED** on the B10 source (`!has(2601)` fails). **GREEN** now.
- `test_bbsc_reward_delivery.py` adds a scenario: a reward event before placement is not queued. **RED** on the source before this gate (`queue.empty()` fails). **GREEN** now.
- `test_bbsc_*`: 4 OK. `test_native_bbsc`: 2 OK.
- DLL builds: departure rule `539b96a7…`; with the reward gate `89b4043e…`. Both exit 0.

## Native evidence

**Dead guest at the kill** (B11 DLL, 23:52-00:00):
- After the boss bar, the guest's HP was set to 0. `PartyGhostDeath` at 23:56:41, then spectating (`watch camera`).
- Amelia was killed while the guest was dead. At 23:56:55 the guest logged `boss event 12401800 earned in this party`, and the reward was queued.
- At 23:57:00 `the host set 2 boss flag(s) during the party`; boss clear with `the spectating player respawns`.
- Death penalty: the 15000 kill echoes were given back, and only the 535 held at death went into the bloodstain.
- Respawn in place at 23:57:03. The reward (`lot 50000001 finished`) was delivered **after** the respawn: the B02 delivery waits for a living character.

**Leave and rejoin the same host** (same session):
- Parting Blank, then merge at home (2 boss flags, 20 world flags in 3 groups). B10 finished it as soon as the game saved (23:58:37).
- Kin Bell again. At 23:59:00 `new party (host cf224863) - the baseline starts again`. Placement baseline: **14 flags** (the 12 older ones plus Amelia's 2, now history).
- Second Parting Blank at 00:00:16: `home again, nothing from the host to merge`.
- The rejoin took about 11 s, so the relay lock had also been released. The under-5 s case is covered by the fixture only.

**Host quits and returns with another character** (G2-22 g, reward-gate DLL, 00:03-00:16):
- The host quit to title at 00:07:57. The guest was sent home (state 6→0 at 00:10:30) to its **exact origin**; this is also a host-loss return. Its merge (Amelia) was finished once the game saved (00:13:32).
- The host loaded **EiEi (level 55) in the same process**. No GPU crash this time; B05 had hit PM4 errors here.
- The host rang again and the guest rejoined. The relay id was unchanged (95a5a909, as CL03 B-31 predicted), but the guest logged `new party`, and EiEi's history was the baseline (16 flags).
- Leaving: `nothing from the host to merge`.

**G2-03 attempts (kill before placement): not reproduced natively.**
- In both attempts the external HP-0 write landed during Amelia's opening sequence, and the host only processed the death after the joining guest had arrived.
- Second attempt: guest placed at 00:20:59; host `ForceSummonSuccess` trace hit=1 at 00:21:04.6; guest reward event at 00:21:04.7; host `BlockClear2_1` at 00:21:09. In game order the kill came after placement, so the admission was correct.
- The pre-placement window was not hit. The gate and the latch are covered by fixtures; B05's pre-party negative stands.

**CL03 B-30 settled:** the trace site labelled `ForceSummonSuccess` (`0x013849A0`) fired at the boss kill (hit=1 at 00:21:04.6, just before the guest's reward event and the host's `BlockClear2_1`). It is the BlockClear2 entry, as the source comment says.

**G2-17 / C3 (stale or reordered host data):**
- The collector admits only 0→1 bits against its party baseline.
- Host flag packets are whole-state, so a delayed older packet is a subset and admits nothing.
- The only harmful input would be an older snapshot carrying a since-cleared bit. B06's receiver rejects older completed and partial snapshots per map (actual-function fixture), and the collector reads only that receiver's cache.
- New `tests/test_bbsc_reorder_collect.py` compiles the **actual B06 receiver together with the actual collector**. History snapshots arrive (one with a transient bit), the guest is placed, the host sets a bit, and then delayed older snapshots arrive: one with an old bit, and a stale copy of the transient one. Only the party bit is collected.
  - **RED** with the B06 ordering guard removed: `collected() == 0x80` fails, because the old bit was collected.
  - **GREEN** with it.
- **Not done:** the default-off native injection seam (CL04 A-37), so there is still no native injection proof.

All protected source saves are unchanged. All test processes were closed.

## Review fix: placement without the spectate option (PR #7 review, P2)

The review found that the reward gate was never satisfied with `SHADPS4_BBSC_SPECTATE` off ("Respawn in place"). Both `bbsc_join_teleport_done = true` assignments are in `TickBbscJoinTeleport`, and `TraceEntry` calls it only when spectate is on. So with spectate off and guest boss items on, every valid reward was rejected. The shared-progress collector had the same dependency: it starts collecting at the same latch.

- New latch `bbsc_party_placed`, kept by `TickBbscPartyPlacement`, which runs every tick whatever the spectate option.
  - **Spectate on:** it follows the join teleport (`bbsc_join_teleport_done`), as before.
  - **Spectate off:** there is no join teleport; the game's own summon load places the guest. The latch is set on the first tick when all of these hold:
    - a guest in the host's world (`BbscGuestInHostWorld`: session state 6, type not 0/8);
    - not loading (`global+0x08` and `+0x1592` zero);
    - the local character alive.
  - It holds through deaths and loads in the session and is cleared when session state leaves 6, as the join teleport latch is.
- `NoteBbscBossRewardEvent` and the shared-progress eligibility now read `bbsc_party_placed`. `TickBbscFollowHost` still waits for the join teleport itself.
- **Fixture:** `test_bbsc_reward_delivery.py` compiles the actual `BbscGuestInHostWorld` and `TickBbscPartyPlacement` with the reward producer/consumer, spectate off, the join teleport latch false throughout:
  - before placement: no reward during the summon load, while dead, while not a guest, or before the first placement tick;
  - after placement, two kill events give exactly one queued and one delivered reward;
  - a death plus a load keeps the latch;
  - a new session starts unplaced;
  - with spectate on, placement still waits for the join teleport.
  - **RED** with the reviewed gate (`bbsc_join_teleport_done`) restored: `bbsc_boss_reward_queue.size() == 1` fails. **GREEN** now.
- The other fixtures set the renamed latch. `test_bbsc_*` 5 OK, `test_native_bbsc` 2 OK, `git diff --check` clean.
- DLL build `d5250903…`, exit 0.
- **Native, spectate off** (`SHADPS4_BBSC_SPECTATE=0` on both clients, fresh save copies, same server):
  - Join 1:
    - 12:59:09 `connect-first: summoned reload finished in host map 0x18000000`;
    - 12:59:19.996 `party: placed in the host's world by the summon load (no join teleport)`;
    - 12:59:21 shared progress became eligible (baseline 12 flags, 160 groups).
  - The guest then fell to its death twice (a debug teleport into an unloaded spot). The first death kept the session and respawned at the host's lamp (12:59:40 → 13:00:05, `v8 respawned`). The latch stayed set: no second placement line.
  - Rejoin after the guest and host processes were restarted:
    - 13:24:27 `new party`;
    - 13:24:43.883 placed again by the summon load;
    - 13:24:44 eligible.
  - In those first runs the reward after placement was not reached: the host died to Amelia before the debug HP-0 found her, and her fight never came back without a load.
- **Native reward, spectate off: PASS** (follow-up run, after PR #7 was merged).
  - Setup: fresh copies of both protected saves, the same DLL `d5250903…`, spectate off on both clients. The guest joined first; then the host was warped once to (51.9, 35.71, 312.3) and walked to the arena trigger.
  - Timeline:
    - 14:29:06 the summon load ended; 14:29:17.118 the guest was placed by the summon load (no join teleport); 14:29:18 shared progress became eligible.
    - 14:31:20 Amelia's bar was up and her HP was set to 0 (max 8051 this time).
    - 14:31:24.461 `boss event 12401800 earned in this party`. Then `boss drop: event 12401800 queued for user sd1 slot 3`, goods 4001 x1 into the carried items, the reward notice, and `lot 50000001 finished for user sd1 slot 3`.
    - 14:31:26 two boss flags were collected.
  - So with spectate off, the reward was rejected before placement (fixture) and given exactly once after it (native and fixture).
- **Rig findings (not caused by the PR #7/#8 changes, open):** in an earlier attempt the host died four times in one party (debug teleports, a pad walk off a ledge).
  - Afterwards the host showed only the guest's name tag and HP bar, not its body. The two were about 4 m apart in the same map, and each respawn logged `v8 restarted 1 remote player insert task(s)` and `hunters: 2`.
  - The host process stopped twice during a load (no crash line kept, no Windows error record). The rig's PowerShell launcher (`$ErrorActionPreference='Stop'` with `*>`) died on the game's first stderr line and so lost the output. A later run with the fixed launcher (`cmd`, one log per launch) caught the same stop at the same load point: `Unimplemented PM4 type 0` / `STOP: GPU library assertion failed`, exit code 23. These stops are most likely that emulator GPU crash, not a mod change. The launcher fix only keeps the evidence.
  - Teleports the host had survived earlier then killed it at once. A fresh start fixed this.

## Status after B11

| Case | Result |
|---|---|
| G2-01 history excluded (B09) | PASS (native) |
| G2-02 joint boss kept after own reloads (B09/B10) | PASS (native, Amelia) |
| G2-04/G2-18 death in the same party | PASS (native, dead at the kill; earlier wipe kept the session) |
| G2-22 (d) rejoin same host | PASS (native, about 11 s; under 5 s by fixture) |
| G2-22 (g) host character change | PASS (native) |
| G3 host-loss return | PASS (native, exact origin) |
| G2-03 kill before placement | Gate added and fixture GREEN; native window not reproducible with this rig |
| G2-17 / C3 | Actual receiver + collector fixture RED/GREEN; native injection not done |
| Guest **loading** (follow load) at the kill | Not tested |
| Progress beyond bosses (doors, lamps, NPCs) | Kind-1 bits are collected only when they turn on in the party; kinds 5/6/7 (items, NPC states) are never merged, so guest NPC choices stay its own. Not exercised natively beyond the boss area |
