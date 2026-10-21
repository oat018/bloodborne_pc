# BBSC: password co-op across story maps, shared progress, and safe return

> **Windows direction amendment — 2026-10-21:** On branch `windows`, follow the [Windows-only migration plan](../plans/2026-10-21-bbsc-windows-only-migration.md) using Supermedo/bloodborne_pc. Earlier platform, upstream, distrobox, packaging and `origin/pre` instructions below describe the historical Linux baseline and are superseded on this branch. Product requirements remain in force. Existing test results do not establish Windows readiness; this amendment does not migrate runtime code.

Date: 2026-10-18

Status: written design approved by the user on 2026-10-18. Implementation planning
is authorized; the implementation plan requires review before execution.

## Implementation boundary and current status (2026-10-21)

Native admission activation remains disabled. The private-attempt lifetime and
Matching2 cleanup/shutdown races have source fixes, focused regression coverage,
and scoped re-review. The capability/HMAC/held-callback/coordinator/selected-save-
backup path remains preparatory. Native guest preparation/rollback has focused
tests and a clean scoped review; the module, six focused executables and three
runtime bridge checks pass in bbport-dev. Proof of cancellation before world
mutation still blocks enabling it. This status
does not revise the approved behavior requirements or claim feature completion.

First-playable type0x28 delivery now has source callers and a production-tick fixture,
including durable baseline/event Ack, death/loading retention and terminal local
exit. Root's final combined checks pass. Native activation, permanent event/item
application, complete return/unsupported-area integration and Windows remain open.

Use the BBSC already carried in BBPORT (`gpu/bbnet/`) as the primary
implementation and reference base. The inspected [bbport revision
`8185b88d80573fa300595d5c54450183b4109935`](https://github.com/deadinside28/bloodborne_pc/tree/8185b88d80573fa300595d5c54450183b4109935)
is the current comparison baseline. The inspected external [CLSAMG22/BBSC revision
`315f76c38b1ca5843b082f93f47927a9193ad94a`](https://github.com/CLSAMG22/BBSC/tree/315f76c38b1ca5843b082f93f47927a9193ad94a)
is supplementary only for behavior missing from BBPORT's existing BBSC, not an
equal or replacement base. These inspected versions are not a freeze on later
upstream changes. Do not wholesale replace or merge either repository.

Two small runtime bridges are required because the module cannot observe these
runtime-owned facts itself: `src/runtime_net.c` loads only recovery hooks when
`BB_ONLINE=0` (the offline runtime otherwise never opens the module), and
`src/runtime_file.c` supplies the canonical selected `/savedata0` mount path before
the first dirty save close (the module cannot resolve the runtime-private mount).
The path-only response is ENOENT with generation, bytes and SHA256 empty; it is
strictly path provenance, not a committed-save receipt. Focused tests are
`out/online-test --bbsc-recovery-offline`, `--bbsc-recovery-module`, and
`--bbsc-save-host`, covering offline imports/workers, invalid-image rejection, and
path-only lookup followed by a real committed receipt.

Necessary bbport changes are allowed when a concrete runtime boundary requires
them; document the evidence that BBSC cannot supply, keep the change minimal, and
add a focused test. The two bridges above are a scope limit, not a blanket ban on
future core fixes or a renewed approval gate. At this status point, offline managed
recovery/return, managed capped-7 cycle proof/rollover and catalog data have source
and synthetic tests. Native pre-world admission/capability, catalog event application
and native-save confirmation for earned rewards are not enabled; Windows integration
and live-game recovery remain unverified. This status does not change the eleven
approved behavior requirements below.

The current source also observes the native flag serializer's dynamic span and
catalog group slots, exposes an immutable receipt/identity-gated optional flag
reader, and records bounded native sign/search/claim observations from the final
HTTP response delivered to the game. These have synthetic tests, not live-game
proof. Windows now includes the journal source and native path/reparse checks;
Windows build/runtime and native recovery hooks remain unverified/disabled.
The Lua null-name guard proves skipped dispatch only: its native caller ignores
the return value. Native join cancellation therefore remains an integration gate.

The focused cycle caller fixture now runs the real observers through fresh native
6→7 and saturated 7→7 transitions, ordinary same-epoch saves, and restart recovery.
Only a recognized native-6 preimage can cancel an uncommitted increment; a recognized
postimage needs fresh SerializeEnd and receipt evidence in the new process. Equal-hash
7→7 restart state remains ambiguous and blocked. These are synthetic file/observer
tests, not live native gameplay proof.

## Intent and agreed behavior

Friends should be able to ring their co-op bells in different story maps or the
Hunter's Dream, connect, and have the guest arrive beside the host. A guest may
visit a map they have not unlocked. On leaving, losing the connection, or loading
the character after a crash or quit, they return to their own pre-party location
without losing progress or items earned together.

The eleven interview decisions establish these requirements:

1. Support main-story maps and the Hunter's Dream. Chalice Dungeons are excluded.
2. Cross-map matching requires the same nonempty co-op password. Preserve existing
   same-map matching rules and unrelated level/channel restrictions.
3. Unlocked-map status does not prevent a guest from visiting the host's world.
4. Persist the guest's pre-party map, position, heading, and previously unlocked
   fallback lamp before entering the host's world. Recover after process exit.
5. Show the host's world while visiting, but permanently share only events earned
   together. Never import the host's older progress just because the guest joined.
6. Add verified boss/story progress without undoing the guest's existing progress
   or overwriting conflicting NPC quest choices. Do not copy whole flag groups.
7. Return to the original point only when it can be validated in the restored
   world. Otherwise use the lamp recorded before joining.
8. Give missing progression items tied to shared events from an explicit verified
   list. Do not duplicate items already owned or re-grant consumed event rewards.
9. Eligibility starts after the guest first finishes joining and is ready to play.
   It continues through death and loading within that party, and ends on departure.
10. If the host enters an unsupported area, end co-op, notify the guest, and return
    them safely rather than following into that area.
11. Target the existing BBSC server first. A compatible private shadNet server is
    acceptable if the existing broker cannot meet the requirements.

## Existing implementation and the gaps

Paths below are relative to the repository root. Locations refer to the code
examined at commit `31a2e53`; they are evidence, not new interfaces.

| Existing seam | Reuse and required change |
| --- | --- |
| `gpu/bbnet/shadps4/core/bloodborne_re.cpp`, `InstallSeamlessCoopPatches` | Already relaxes bell/area checks. Restrict the new supported-map policy explicitly; do not treat every usable map as supported. |
| `BbscConnectFirstHost`, `BbscWriteConnectFirstPlacement`, `ApplyCrossMapSummonGuestPlacement` | Already transports a destination into the native summon reload. Bind it to the selected host and join attempt, and enforce eligibility before applying it. |
| `RequestBbscSessionWarp`, `TickBbscJoinTeleport`, follow-host handling | Reuse native map loading and later in-map placement. Existing cross-map lighting recovery must survive; never substitute a coordinate-only teleport across maps. |
| `gpu/bbnet/shadps4/core/libraries/network/http.cpp`, `ApplyBloodborneSeamlessRoute`, `RewriteConnectFirstAreaId` | Routes requests to shadNet and rewrites advertisement `AreaId`. It does not currently enforce password-only cross-map eligibility or filter returned candidates. |
| `CaptureBbscOwnWorldBeforeJoin`, own-world and Dream restoration | Already retains the guest's map flags and restores them after returning or recovering. Extend the lifecycle rather than building another world snapshot system. |
| `TickBbscProgressBroadcast`, `OnBbscProgressPacket`, `TickBbscSharedProgress` | Currently sends existing boss flags and collects broad kind-1 world groups. Replace permanent collection with verified events after admission. Host presentation snapshots remain separate. |
| `WriteBbscPendingProgress`, `LoadBbscPendingProgress` | Existing per-slot progress journal and temporary-file/rename pattern are useful. Ten-second polling and rename alone are insufficient for the new recovery guarantees. |
| `TickBbscStrandedGuest`, explicit leave hooks | Existing disconnect recovery uses a generic home respawn after 15 seconds. Route completed session teardown through the stored return destination. |
| `BbscActiveSaveSlot`, inventory helpers, `BackupBbscSaves` | Reuse save layout and native inventory operations. Active-slot selection currently uses recent file writes; establish reliable character ownership before accepting a protected join. |

The launcher uses shadNet; its defaults are in `scripts/online.py`. The optional
`scripts/bbsc_gameserver/bbhost_service.py` implements a different, incompatible
bbhost transport. Changing that service is not the implementation of this design.
The shadNet summon broker source is not present in this repository.

## Architecture and ownership

Keep this in the existing BBSC game, HTTP, and signaling paths. Add only the small
policy and durable party state needed by those callers; no second networking
stack, general-purpose quest engine, new dependency, or new launcher page.

- The game thread owns admission, world changes, event application, and return.
  Network callbacks validate and enqueue messages; they never walk live map data
  or mutate character/world memory.
- HTTP matchmaking validates candidate/password/map eligibility and retains the
  identity of the selected host and join attempt. Placement headers alone are
  not proof of a password match or of host identity.
- The existing peer signaling path carries a versioned admission baseline and
  shared-event updates, scoped to the current host, party, and guest admission.
  Extend its packet definitions and mirrored declarations together.
- The existing own-world journal and progress journal share a durable party
  identity and recovery order. Do not introduce an independent competing restore
  timer. A compact return record may reference these existing payloads.

## Admission and placement

Use one explicit allowlist of verified main-story map identifiers and the Dream.
Validate both the source and target; reject unknown maps and Chalice instances.
Use game map data to produce this list, not a numeric range guessed from observed
maps. Do not allow bells during a loading screen, an uninitialized character,
or another engine state where a native join is unsafe.

For a cross-map candidate, require a nonempty password with the game's existing
comparison semantics, co-op role, supported maps, and the selected host's identity.
Apply these checks before advertisement rewriting, claim acceptance, placement
consumption, and the actual load. Expired preparation or a changed host/password
invalidates the attempt; an old placement must never drive a later join.

Before accepting a join that can change the world, establish the active local
user, save path, slot, and a stable character discriminator. A recent file-write
timestamp alone is not sufficient when several characters or processes exist.
Bind the journal to that identity and validate it again on recovery. Refuse the
join with an actionable message if identity, backup, or durable recording fails.

Capture the return point and own-world state immediately before the first
host-world mutation, and commit them to disk before allowing that mutation.
The current `JoinSession_White` logging hook is a useful lifecycle observation,
but the implementation must locate an interception point that can actually stop
the join on failure; a void trace callback is not an error-handling mechanism.

Load the target through the existing native summon/warp path, then place the guest
near the actual host in the loaded map. Preserve the known lighting reload where
required. Do not fall back to another guest's position when the host cannot be
identified. Wait or fail the join safely instead. Once joining is complete, admit
the guest for shared progress as described below.

## Progress: presentation versus permanent rewards

Host world snapshots are temporary presentation state. They may show old boss
deaths, opened doors, and the host's Dream, but they are never themselves an
instruction to change the guest's permanent world.

Replace the permanent raw-group OR merge with an explicit event catalog. Each
entry identifies a boss/story event, its verified completion evidence, additive
guest flags, prerequisite/conflict checks, and any required progression items.
Derive entries from the game's event and parameter data and verify them in game.
An entry does not ship until that mapping has evidence. NPC quest alternatives,
exclusive phase flags, inventory pickup bitmaps, and arbitrary kind-1 bits are
not automatically admitted. Dream presentation must not grant starter weapons,
change Doll/workshop state permanently, or import old host progress.

The release catalog must cover the agreed boss/story progression throughout the
supported maps. A verified subset is useful for development, but does not satisfy
the full release scope; do not silently omit bosses or required progression items.

The admission boundary must be ordered on the host's game thread:

1. The guest completes its initial map load and placement and requests admission.
2. The host captures a complete baseline of the catalog's completion states and
   admits that ready guest at the same sequence boundary on its game thread.
3. The guest receives the complete baseline and acknowledgement of that active
   boundary. Events already completed at the boundary are presentation-only.
4. New catalog events after that boundary are sequenced, retained for delivery,
   and attributed to active admissions. Events during baseline transfer remain
   ordered after the boundary; do not lose them between snapshot and subscription.

The initial join cover remains until admission is established, so "ready to play"
has an observable boundary. Baseline all catalog events, including events in maps
not currently loaded. A first snapshot of a later map must not masquerade as new
progress. Guests retain eligibility through subsequent deaths, spectating, and
supported-map loads; those operations do not start a new baseline.

Accept events only from the selected host for the current party/admission, with
validated sizes, catalog IDs, and sequence numbers. Ignore duplicates and stale
packets. Close the admission boundary at departure. A retained event demonstrably
earned before that boundary may finish recovery afterward; events earned after
departure do not become eligible. Do not promise recovery of events that neither
peer recorded before a crash.

A guest acknowledges an event only after its earned-event record is durable.
The host retains and retries unacknowledged events while the admission exists;
the guest deduplicates all retries. A confirmed reward must never depend solely
on the existing five-second broadcast or ten-second journal timer. Report an
interrupted, unconfirmed delivery honestly rather than announcing it as saved.

On returning, restore the guest's own world first, then apply eligible events
using additive and conflict checks. If a catalog event requires an unavailable
prerequisite, retain it pending rather than manufacture unrelated story/quest
completion. It must not mark a boss complete while leaving its required reward
unrecoverable. Preserve the existing guest's stats, equipment, ordinary earned
loot, echoes, and unrelated state; this is not a whole-save rollback.

## Progression items and replay safety

Use the existing native inventory operations. For each eligible event, check
carried items, storage, the guest's existing completion/claim evidence, and its
durable event ledger before giving a reward. Current possession is not enough:
a legitimately consumed key must not be reissued after a retry or later party.
Bind event completion to the character's game cycle where appropriate so a new
cycle is not mistaken for a duplicate from an old one.

Persist event intent before applying it. If inventory is not ready or cannot
accept the item, keep the reward pending, notify the player, and retry safely.
Do not silently finish a transaction or remove its journal because an operation
was attempted. A crash between an item grant and ledger completion must reconcile
against the actual loaded save before reapplying anything. Durable completion
requires evidence that the game's relevant save write succeeded.

## Return and crash recovery

Persist a versioned, bounded record with character identity, game cycle, party and
admission identity, original map/position/heading, pre-party fallback lamp, durable
phase, and references to own-world and earned-event data. Do not persist passwords
or stale runtime pointers. Reject malformed, truncated, incompatible, or
wrong-character records without applying their coordinates or flags.

Write through a temporary file, check every operation, flush to durable storage,
atomically replace the destination, and persist the directory update where the
platform requires it. Serialise updates and prevent concurrent sessions from
owning the same character record. Preserve existing backup rules and protected
user exclusions. Never overwrite an unresolved recovery record with a new party.

| Phase | Meaning and recovery action |
| --- | --- |
| Prepared | Original world and return destination are durable; a join may proceed. If it never proceeds, verify the character is still home before finishing the record. |
| Visiting | The guest may be in host state. An interrupted process must recover home on the next load of this character before accepting another join. |
| Returning | Eligibility is closed and a native return is in progress. Resume it safely after interruption; retain earned events and unclaimed rewards. |
| Complete | The character is in its own world, the chosen destination and return save are confirmed, and outstanding event/reward intents are durable in the progress journal. Archive the return record only now. |

Explicit leave, confirmed session loss, host shutdown, unsupported destination,
and restart recovery converge on the same return operation. Preserve the existing
disconnect grace while checking for actual session termination; missed packets
alone are not an immediate reason to eject a guest. Reuse BBSC teardown to release
session locks and stop following the old host.

Return in this order: close admission, stop applying incoming host presentation,
restore the character's own world, incorporate eligible progress at the correct
native load boundary, select a safe destination, perform the native load, and
confirm the final world/position and successful save. Align the existing own-world
and progress recovery timers with this coordinator so they cannot race it.

A deferred event or reward does not keep a successfully restored character in
the Visiting/Returning phase. Keep those intents in the progress journal after
return completion, keyed by their original admission, and permit a later party
without overwriting them. This allows unmet prerequisites to be completed later
without losing credit or trapping the character in recovery.

Validate the original point against the restored world *including* newly earned
progress, using verified engine collision/placement evidence. Finite coordinates
and a valid map ID alone do not prove a point is safe. If validation is unavailable
or fails, use the saved pre-party lamp through the native respawn mechanism.
If no pre-party lamp was unlocked, retain the game's verified own-world initial
respawn destination as the fallback; never invent coordinates in the Dream. If no
safe native fallback can be established, refuse admission before leaving home.

Recovery belongs to the recorded character only and runs on its next load, even
offline; do not automatically reconnect to the old host. A corrupt record or
storage error must leave the originals intact and explain the failed recovery.
Use a verified native home respawn if the character is stranded and a trustworthy
destination record cannot be read. Never apply an unrelated character's backup.

## Unsupported transitions and compatibility

Validate follow-host destinations before requesting a warp. A transition to
Chalice or another unsupported map closes guest admission, announces that the
destination is unsupported, and runs the normal return operation. The host may
continue solo. The pre-party return point does not change as the party travels.

Both participants need the feature-capable build for the new progress/admission
semantics. Negotiate capability before host-world mutation; incompatible peers
must not silently receive the old broad progress behavior under this feature.
Do not change invasion placement/progress or enable cross-map public matching.

Do not reinterpret a legacy raw-group progress journal as events earned under the
new admission rules. Preserve existing recovery files and resolve an interrupted
legacy visit before admitting that character to the new flow. Legacy data lacks
the baseline needed to reconstruct which events were earned together; never
guess that baseline, delete the file silently, or mix it into a new party record.

Use the existing public broker as the first integration target. Verify that
candidate and claim data permit binding the selected host, password match, and
placement. Client gating protects patched clients but is not proof of broker-wide
password enforcement. If the exchange cannot establish the required binding,
disable the unsupported path with a clear message and implement/test a compatible
private shadNet broker. Do not weaken the password requirement or substitute the
bbhost service to make the test pass. No server deployment is part of this design
document task.

## Change surface for the implementation plan

- `bloodborne_re.cpp`: map eligibility, reject-capable admission interception,
  accurate host placement, presentation/progress separation, event catalog,
  inventory reward application, and coordinated return/recovery.
- `gpu/bbnet/include/core/bloodborne_re.h`: the minimal shared declarations needed
  by the HTTP/signaling callers.
- `gpu/bbnet/shadps4/core/libraries/network/http.cpp`: candidate/claim eligibility,
  placement binding, and stale-attempt invalidation.
- `gpu/bbnet/shadps4/core/libraries/np/np_signaling/`: peer capability, ordered
  baseline/event messages, validation, and corresponding relay handling if used.
- `bbsc_persistence.*` and `bbsc_patch_tables.h`: only lifecycle/byte-patch changes
  shown necessary by inspection; retain byte verification and save safety.
- Focused tests in `tests/`, registration in `gpu/CMakeLists.txt` as needed, and
  `docs/BBSC.md` for final user-facing behavior and recovery limitations.
- Broker sources/configuration only if existing-protocol integration fails; obtain
  the actual compatible source before planning server edits. No such source is
  currently included in this repository.

## Verification and release gates

Keep pure policy, catalog validation, message parsing, and journal recovery
testable without booting the game. Use small focused tests and fault injection;
unit success does not replace actual co-op verification.

| Scenario | Required result |
| --- | --- |
| Same nonempty password, different story maps; both directions with Dream | Join through native loading and arrive near the actual host with complete scene/floor/lighting. |
| Empty/different password, stale placement, wrong host, unknown map | No cross-map join or unintended warp; stale messages cannot affect the next attempt. |
| Guest has not unlocked host map | Visit succeeds without importing old progress or losing a safe return. |
| Host has old boss/story completion before admission | Its world is displayed correctly; that completion and its rewards are not permanently imported. |
| Event occurs at baseline/admission boundary or during later map transfer | Exactly the events after the admission boundary are retained; no snapshot race imports old flags or loses ordered new ones. |
| Guest dead, spectating, or loading after initial admission | New eligible events remain earned once; first-join loading does not grant earlier events. |
| Guest already ahead or has conflicting NPC choices | No progress rollback, NPC-choice overwrite, or unsafe raw flag-group merge. |
| Required item already carried, stored, previously consumed, or reward retried | No duplicate; incomplete inventory operations remain pending and recoverable. |
| Explicit leave, host quit/crash, or sustained connection loss | Guest returns to original safe point, or pre-party native fallback, keeping earned progress and loot. |
| Original point invalid after progress; no lamp on a new character | Validated native fallback is used; admission is blocked if none can be established. |
| Process killed before join, during loading, while visiting, during return, or around a save/reward commit | Reloading the recorded character recovers consistently without erasing earned data or reissuing rewards. |
| Wrong character, changed cycle, two local users/processes, corrupt/truncated journal, disk-full/rename/flush failure | No cross-character writes, destructive overwrite, silent journal completion, or unsafe join. |
| Host enters Chalice | Guest receives the unsupported-area notice and returns; no unsupported follow warp. |
| More than one guest and a late joiner | Every guest uses its own admission boundary and return record; a late joiner never receives earlier party progress. |
| Existing public broker, then private broker only if required | All password/identity checks pass; server choice cannot bypass acceptance criteria. |
| Invasion, same-map matching, lamp warp, existing memory interface | Existing unrelated behavior remains correct; run the BBSC memory regression check where the build is available. |

Before implementation estimates are committed, verify four integration facts:
the reject-capable join point and stable save identity; broker candidate/claim
fields and capability exchange timing; the native safe-return/save-completion
signals; and the event-to-flag/item catalog coverage. These are engineering
verification gates, not unanswered product preferences. A failed gate must be
reported explicitly rather than replaced with a broader flag copy, guessed
destination, or unverified server guarantee.

## Review and next step

This document introduces no gameplay changes. Review it for agreement with the
interview, then prepare the implementation plan after the user approves this
written design. That plan must sequence the integration gates and concrete tests
before changing the existing join, progress, or return behavior.
