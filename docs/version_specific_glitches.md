# Version-specific glitches suggested by the source

This is a catalog of bugs and glitch candidates inferred from the current
version conditionals, with the code taken as written. It is a set of leads for
testing, not a claim that every predicted symptom has been reproduced.

Audited on 2026-09-26 against staging commit
`f21cfbc660a7643fa030ae91df9d5ad3513f8aeb`. No game source was changed for this
audit.

The scan covered **190 `#if` / `#elif` / `#ifdef` / `#ifndef` directives naming
`VERSION_GSA*` in 59 files under `src/` and `include/`**. The coverage appendix
accounts for every file, including branches that do not suggest a bug. There
are **31 behavioral findings and three additional unresolved or latent issues**.
Several findings are likely fixes or presentation improvements rather than
exploitable glitches.

As requested, this trusts the reconstruction: EN and JP v1.0 are reported
complete, and the remaining ports are still in progress at high 99% completion.
Source intent, exact reproduction conditions, and the meaning of some game bits
remain less certain than the visible code differences. This audit did not run
the game, independently compare all retail binaries, or exhaustively compare
regional assets and scripts. A targeted follow-up traced the map-specific
branches through retail placement records and text banks; its scope and exact
record locations are in the location-evidence appendix. Other changes hidden
in assets, shared bugs with no version branch, and unported differences are
outside the completeness claim.

## Reading the version labels

| Label | Build macro | Release |
| --- | --- | --- |
| E0 | `VERSION_GSAE01` | EN v1.0 |
| J0 | `VERSION_GSAJ01` | JP v1.0 |
| P0 | `VERSION_GSAP01` | PAL v1.0 / revision 0 |
| E1 | `VERSION_GSAE01_rev1` | EN revision 1 |
| P1 | `VERSION_GSAP01_rev1` | PAL revision 1 |

These are branch memberships, not an assumed chronological ordering across
regions. In particular, J0 already has two fixes missing from E0; P0 has many
changes shared with E1 but lacks several revision-1 safeguards.

**High** means the source establishes a concrete failure mechanism or omitted
behavior. **Medium** means a plausible symptom follows from a targeted change,
but its practical trigger or intent needs testing. **Low** means the behavioral
difference is clear but calling it a bug is speculative. Even High does not
mean a retail gameplay reproduction was performed. “Changed in” means the
listed source contains the countermeasure, not that all related bugs are gone.

## Best places to start testing

| Finding | Suspect versions | Changed in | Expected issue | Confidence |
| --- | --- | --- | --- | --- |
| G01 | E0, J0, P0 | E1, P1 | Navigation patches remain stale after map changes | High |
| G02 | E0, J0, P0 | E1, P1 | Invalid enemy animation speed when a rate is zero | High |
| G03 | E0 | J0, P0, E1, P1 | Tricky retains a freed sequence target | High |
| G04 | E0, J0, P0 | E1, P1 | Tricky commands stay locked after an inconsistent Queen-return state | High |
| G07 | E0, J0, P0 | E1, P1 | Generic put-down code writes into Tricky ball collision state | High |
| G10 | E0, J0 | P0, E1, P1 | Volcano Force Point pressure plate remains latched down | High |
| G12 | E0, J0, P0, E1 | P1 | Krazoa Palace switch-camera flag stops updating when the player moves away | High |
| G18 | E0, J0 | P0, E1, P1 | Drakor missile launch produces invalid velocity from a zero vector | High |
| G21 | E0, J0 | P0, E1, P1 | Unloading the shop's Fuel Cell display leaks outstanding lightning allocations | High |
| G23 | E0, J0 | P0, E1, P1 | Asset table read races an unfinished load | Medium |

The remaining entries matter too, but often have narrower triggers, uncertain
player-facing effects, or concern UI rather than progression.

## Locations and recognizable encounters

These names come from the retail map-name table, object definitions, placement
parameters, and the source consumers of those parameters. A placed object
identifies a test location; it does not prove the problematic state is reachable
there in every act. Map IDs below are decimal; placement identities are hex.

| Where to look | Map / romlist | Findings and recognizable objects |
| --- | --- | --- |
| Thorntail Hollow | 7 / `hollow` | G04: returning Tricky to the Queen; `SH_tricky` controller `0x435F2` |
| Volcano Force Point Temple | 4 / `temple` | G10: Tricky-enabled `VFP_PuzzleP` pressure plate `0x41996` |
| DarkIce Mines, upper area | 19 / `snowmines` | G11: Dinosaur Horn interaction `DIMUseObjec` `0x4B13A`; G14: SnowHorn interaction during a hit reaction |
| Krazoa Palace | 11 / `warlock` | G12: pressure-switch door camera in act 1; the source prefix `WM` refers to this map |
| SnowHorn Wastes | 10 / `wastes` | G13: `NW_mammoth` family after a sequence; G25: the paid BribeClaw interaction |
| Ice Mountain | 23 / `newicemount` | G14: additional placements using the same `DIMSnowHorn` controller; reachability of its hit-reaction interaction needs checking |
| Dragon Rock, upper area | 2 / `dragrock` | G15/G16: HighTop rescue/escort and missile controller; G17: `DR_EarthWar` / EarthWalker riding action |
| Drakor boss arena | 44 / `finalboss` | G18: Drakor missile launch; the internal name `finalboss` does not mean Andross here |
| Cape Claw | 29 / `capeclaw` | G25: a second paid `GuardClaw` placement; the Cape Claw HighTop uses a different initial state from the Dragon Rock encounter |
| ThornTail Store / shop | 51 / `swapstore` | G21: `SPFuelCell` display `0x45B6F`, raw object ID `0x468` |
| CloudRunner Fortress | 12 / `fortress` | G26: the three `CFSunTemple` timer interactions, raw object ID `0x830` |
| Walled City | 13 / `wallcity` | G27: the introductory **"Walled City" area-name banner**, not tile-puzzle instructions |
| Several magic-cave entrances | `capeclaw`, `hollow`, `hollow2`, `moonpass`, `temple`, `wallcity`, `wastes` | G19: shared `MagicCaveTo` approach/glow cleanup; Cape Claw, Thorntail Hollow and its underground area, Moon Mountain Pass, Volcano Force Point, Walled City, SnowHorn Wastes |

## Gameplay, movement, and progression

### G01 — Loaded-map checksum can suppress navigation rebuilding

**Suspect:** E0, J0, P0. **Changed in:** E1, P1. **Confidence:** High.

[`Objfsa_UpdateWalkGroupPatches`](../src/dlls/engine/20_Hcurves/Hcurves.c#L1019)
multiplies the indices of all nonzero loaded-map flags and returns early if that
product has not changed. Loading index 0 forces the product to zero; index 1
does not affect it. Even without those indices, sets `{2, 6}` and `{3, 4}` both
produce 12. The old check also ignores changes between different nonzero flag
values. Revision 1 compares every flag byte with the previous array instead.

**Expected symptom:** actors using these walk groups can retain obsolete
connectivity, refuse a route, or navigate against the preceding map set.
**Test lead:** log loaded flags and patch rebuilding while crossing streaming
boundaries. The example sets demonstrate the algorithm's collisions; they are
not claimed to be reachable neighboring map combinations in normal play.

### G02 — Zero animation-rate inputs produce invalid playback speeds

**Suspect:** E0, J0, P0. **Changed in:** E1, P1. **Confidence:** High.

Four paths in [Baddie.c](../src/dlls/objects/201_Baddie/Baddie.c#L148), including
[`baddieSetMove`](../src/dlls/objects/201_Baddie/Baddie.c#L2027), calculate
`1.0f / (60.0f * rateScale)` without checking for zero. The additional sites are
at lines 224 and 1123. E1/P1 substitute `0.1f` when the rate is zero.

**Expected symptom:** non-finite animation speed, abrupt completion, or broken
animation/event progression when a script or state supplies zero. This does
not establish an inevitable crash or a specific enemy exploit.
**Test lead:** instrument zero-rate calls and control-state transitions; inspect
the resulting animation frame and event state before trying to force a route.

### G03 — Tricky does not recover when his sequence target is freed

**Suspect:** E0 only. **Changed in:** J0, P0, E1, P1. **Confidence:** High.

The [sequence-processing code](../src/dlls/objects/196_Tricky/tricky.c#L1912)
in other versions checks `TRICKY_STATE_FLAG_SEQUENCE_KEEP_STATE` together with
`followObj->objectFlags & OBJECT_OBJFLAG_FREED`. It resets Tricky's command state,
sets `TRICKY_MOVE_WALK_WAIT`, and clears current and previous speeds. E0 omits
this recovery.

**Expected symptom:** Tricky stays in a stale command or movement state after
the object he was following disappears; later stale-object access is also a
possibility, not a demonstrated crash.
**Test lead:** interrupt a target-following sequence by despawning or unloading
the target while the keep-state flag remains set. JP v1.0 is a useful control.

### G04 — Queen-return progress can leave Tricky commands unavailable

**Suspect:** E0, J0, P0. **Changed in:** E1, P1. **Confidence:** High.

[`shTricky_init`](../src/dlls/objects/422_SH_tricky/SH_tricky.c#L57) previously
treated `GAMEBIT_SH_ReturnedToQueen` alone as proof that the controller was
complete. E1/P1 also require `GAMEBIT_Tricky_Unlocked_Sidekick_Commands`.
If return is recorded but commands are still locked, they clear the return bit
so the sequence can recover. The update routine restores command and spawn bits
only in its wait-for-return phase.

**Expected symptom:** an inconsistent save or interrupted sequence leaves
Tricky's commands locked, with this controller doing nothing to restore them.
**Test lead:** reload during the interval between the two bits being updated,
or first test a controlled save with return=1 and commands=0. Whether a natural
save/reset timing can produce that combination still needs reproduction.

### G05 — Tricky's ball advances collision state while inactive

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** Medium.

Both [`SidekickBall_update`](../src/dlls/objects/245_SidekickBal/SidekickBal.c#L308)
and [`trickyBallMove`](../src/dlls/objects/245_SidekickBal/SidekickBal.c#L398)
unconditionally update, apply, and advance the path/collision state in E0/J0.
Later versions do so only when `hittableLatch == 1`; otherwise they reattach
the state to the object's current position.

**Expected symptom:** stale collision history or displacement while the ball is
inactive or being repositioned, followed by an unexpected bounce or position
correction. **Test lead:** repeatedly pick up, reposition, and release the ball
near walls, slopes, and changes in floor height; monitor the latch and trace
positions. This is distinct from the additional revision-1 change below.

### G06 — Ball throws reuse collision state from before the throw

**Suspect:** E0, J0, P0. **Changed in:** E1, P1. **Confidence:** Medium.

E1/P1 add `attachObject(obj, &state->pathControl)` immediately after setting
throw velocity and previous position in both
[`sidekickBall_throw`](../src/dlls/objects/245_SidekickBal/SidekickBal.c#L75)
and [`sidekickBall_launch`](../src/dlls/objects/245_SidekickBal/SidekickBal.c#L205).

**Expected symptom:** the first movement/collision sweep after a throw uses an
old origin or contact state, causing a snap, false collision, or odd trajectory.
**Test lead:** throw after carrying the ball a substantial distance or after
Tricky has moved it. P0 has G05's guard but lacks this explicit throw-time reset.

### G07 — Generic carryable cleanup can corrupt the ball's state

**Suspect:** E0, J0, P0. **Changed in:** E1, P1. **Confidence:** High for the
layout conflict; Medium for a naturally reachable crash.

[`Carryable_putDownAndSavePos`](../src/dlls/engine/47/47.c#L21) and the
[put-down completion path](../src/dlls/engine/47/47.c#L195) gain an exception for
`romDefNo == 0x112`, which [Tricky's spawn enum](../src/dlls/objects/196_Tricky/tricky.c#L759)
identifies as the sidekick ball. Without it, they cast `obj->extra` to
`CarryableState`, clear bytes at offsets 5 and 6, and may save its position.
The [ball state](../include/dlls/objects/245_SidekickBal.h#L20) actually begins
with a [collision state](../include/main/dll/curves_collision_state.h#L27),
whose pointer at offset 4 overlaps those writes.

**Expected symptom:** invalid ball collision data or inappropriate persisted
position after a generic put-down, potentially causing erratic movement or a
crash. **Test lead:** trace both carryable paths while putting down the ball,
and watch the first eight state bytes. The exception does not disable the
ball's own throw/idle behavior.

### G08 — Barrel ground-contact flicker can repeat the landing sound

**Suspect:** E0 only. **Changed in:** J0, P0, E1, P1. **Confidence:** High.

[Barrel landing detection](../src/dlls/objects/344/344.c#L441) in E0 remembers
only whether it was grounded on the preceding update. Other versions use a
three-update `groundGraceFrames` countdown, refreshed while grounded, before
allowing another landing sound. The corresponding state layout differs too.

**Expected symptom:** repeated put-down/landing noises when tiny bounces or
uneven contact alternate grounded and airborne states. **Test lead:** rest or
roll a barrel on a seam or moving surface. The branch directly changes sound
debouncing; it is not evidence of a changed explosion threshold.

### G09 — Barrel explosion may use stale hit-volume position data

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** Medium.

[`gunpowderBarrel_triggerExplosion`](../src/dlls/objects/344/344.c#L341) changes
the collision mask, capsule bounds, and blast slot. Later versions additionally
call `ObjHits_MarkObjectPositionDirty` when enabling the blast collision.

**Expected symptom:** the visible explosion and its collision effect can
disagree, or the newly configured blast can fail to refresh promptly.
**Test lead:** detonate a barrel immediately after carrying, moving, or throwing
it and compare hit-volume transforms against its position. The exact missed-hit
scenario depends on the shared collision update order.

### G10 — Volcano Force Point pressure plate becomes permanently latched on reload

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** High.

[`PressureSwitchFB_init`](../src/dlls/objects/251/251.c#L355) restores an already
pressed switch and generally sets `latched = 1`. Later versions exempt placement
identity **`0x41996`**. The update routine's ordinary release/movement paths are
gated by `latched == 0`.

**Location:** Volcano Force Point Temple (`temple`, map 4). The record is
`VFP_PuzzleP`, raw object ID `0x546`, at approximately
`(-318.25, 282.30, 2314.41)` in placement coordinates. Its
[parameters](../include/dlls/objects/251.h) specify pressed bit `0x4F6`,
enable bit `0x9FD`, and `drivesTricky = 1`. A nearby `HitAnimator` consumes
`0x4F6`, and a `VFPSeqObj` consumes `0x9FD`. This identifies a specific
Tricky-enabled puzzle plate, although the room's descriptive name and the
complete puzzle consequence remain unproven.

**Expected symptom:** this specific switch cannot release or cycle normally
after being loaded with its pressed bit set. That might obstruct a puzzle or
leave something open; the code alone does not establish which.
**Test lead:** press this plate, unload/reload or save/reload, then remove the
weight or move Tricky and compare. The placement identity is not a map ID.

### G11 — DarkIce Mines Dinosaur Horn interaction latches through the generic lock path

**Suspect:** E0, J0, P0, E1. **Changed in:** P1 only. **Confidence:** Medium.

[`DoorLock_update`](../src/dlls/objects/273/273.c#L127) in P1 excludes placement
identity **`0x4B13A`** from the trigger branch. The actual placement resolves to
**the Dinosaur Horn interaction in upper DarkIce Mines** (`snowmines`, map 19),
retail object `DIMUseObjec`, raw ID `0x860`, at approximately
`(788.68, -1042.00, 2254.41)`.

Its trigger and required-item fields both hold `0x1EE`,
[`GAMEBIT_ITEM_DinoHorn_Got`](../include/main/gamebit_ids.h#L492).
Crucially, its flags are zero and its unlock sequence is `-1`: this particular
record does **not** consume the horn or run an unlock sequence through the
excluded block. It sets its unlocked bit, marks the interaction started, and
disables the A button for that update. Subsequent updates disable interaction
while the unlocked bit remains set.

There is also a matching **asset change in P1**: the placement's
`unlockedGameBit` at `+0x1C` changes from `0x001A` (J0/P0/E1) to `0x03EF`.
Two nearby `DIMSeqObjec` records already use `0x3EF` as their open/completion
bit. The code exception and the changed bit therefore need testing together.

**Expected symptom:** the older interaction can become latched/inactive from
using the horn independently of that shared sequence-completion state.
**Test lead:** use the Dinosaur Horn at this placement, try interacting again,
then reload and compare bits `0x1A`/`0x3EF` and the neighboring sequence objects.
Whether the older behavior actually prevents progression is unconfirmed;
neither horn consumption nor a particular door softlock follows from this code.

### G12 — Krazoa Palace pressure-switch camera can retain stale state at a distance

**Suspect:** E0, J0, P0, E1. **Changed in:** P1 only. **Confidence:** High.

The [slot-510 pressure switch](../src/dlls/objects/510/510.c#L125) updates
`GAMEBIT_WM_SwitchCamActive` in map-event slot 11, act 1. Before P1, the entire
set/clear block requires the player to be within 100 units. Moving farther away skips
both activation and cleanup. P1 removes only this distance restriction.

**Location:** slot 11 is **Krazoa Palace**, romlist `warlock`. Its `WM_Pressure`
placements are `0x1F1A` and `0x47293`; the act-1 condition still governs whether
the changed code executes. A `WM_seqpoint` placement `0x42D45` takes `0x905`
as its sequence-start bit, independently connecting the source camera flag to
this map. The prefix should not be expanded as Walled City.

**Expected symptom:** the camera flag remains set after a remote release, or
does not activate for a switch pressed while the player is farther away.
**Test lead:** activate the switch camera, move beyond 100 units before the
hold expires, and compare the game bit and camera. The remaining distance
checks, including sound behavior, still exist.

### G13 — SnowHorn Wastes mammoth clears its reset request without resetting collision state

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** Medium.

[`NW_mammoth_processAnimEvents`](../src/dlls/objects/417/417.c#L166) requests a
path reset during sequence handling. In
[`NW_mammoth_update`](../src/dlls/objects/417/417.c#L659), E0/J0 simply clear that
request. Later versions first reattach the path state to the object.

**Location:** the placed DLL-417 actors are all in **SnowHorn Wastes**
(`wastes`, map 10), with retail names `NW_mammothh`, `NW_mammothw`,
`NW_mammothg`, and `NW_mammothb`. This is distinct from DLL 598 below.

**Expected symptom:** incorrect first movement or collision after a scripted
reposition, such as snapping back, getting stuck, or an obsolete contact.
**Test lead:** compare positions and collision trace origins immediately after
the mammoth's sequence callback. This is not a claim that every SnowHorn actor
uses the affected controller.

### G14 — DarkIce Mines SnowHorn remains interactable during hit reactions

**Suspect:** E0, J0, P0. **Changed in:** E1, P1. **Confidence:** High for the
interaction difference; Medium for an exploit.

[`DIMSnowHorn1_update`](../src/dlls/objects/598_DIMSnowHorn/DIMSnowHorn.c#L1046)
originally enables interaction before processing a hit reaction, which can
return early. E1/P1 disable interaction first and re-enable it only after the
reaction has finished.

**Location:** DLL 598's `DIMSnowHorn` placements occur in **upper DarkIce Mines**
and **Ice Mountain** (`snowmines` and `newicemount`). Their placement variants
differ, so Ice Mountain is an additional controller-use lead rather than a
claim that its actors expose the same mount/damage scenario. The similarly
named `DIMSnowHorn` lock prop in DLL 273 is a different object.

**Expected symptom:** an interaction or mount request can overlap a damage
reaction, potentially producing conflicting movement/animation states.
**Test lead:** press the interaction button while the rideable SnowHorn is
reacting to a hit. The branch concerns interaction flags, not general immunity
to damage.

### G15 — Dragon Rock HighTop's scripted transition can fail to reach its alternate state

**Suspect:** E0/J0 for the missing live-state check; E0/J0/P0 for the missing
creator event. **Changed in:** P0/E1/P1 and E1/P1 respectively. **Confidence:** Medium.

Later [`hightop_stateHandler02`](../src/dlls/objects/626/626.c#L546) immediately
returns state 8 when game bit `0x631` is set. E0/J0 check it in the initial
handler but not this active handler, and also retain a different
[motion event 7](../src/dlls/objects/626/626.c#L493) transition. Separately,
E1/P1's [`DR_Creator_SeqFn`](../src/dlls/objects/613_DR_Creator/DR_Creator.c#L39)
sets `0x631` on event 10 when its configured spawn bit is active.

**Location:** **Dragon Rock's HighTop rescue/escort**, `dragrock`, map 2.
`DR_HighTop` placement `0x3460C` has spawn variant 0. The `CC_HighTop` in
Cape Claw has variant 1, which instead initially selects state 10 in
[`hightop_stateHandler00`](../src/dlls/objects/626/626.c#L672).
Dragon Rock's `DR_Seqobj` `0x4555F` uses `0x631` as its trigger and
[`GAMEBIT_DR_RescuedHighTop` (`0x632`)](../include/main/gamebit_ids.h#L672)
as its completion bit, with sequence 11. This ties the formerly anonymous bit
to the rescue/escort sequence; the full event ordering still needs testing.

**Expected symptom:** HighTop remains in the preceding controlled/movement state
after a scripted event, or progression depends on receiving a different event.
**Test lead:** trace event 10, bit `0x631`, and handlers 0/2 around the relevant
Dragon Rock sequence. P0 has only part of this changed behavior.

### G16 — Dragon Rock HighTop death leaves a missile-sequence enable bit set

**Suspect:** E0, J0, P0. **Changed in:** E1, P1. **Confidence:** Medium.

When HighTop's air meter reaches zero, the
[death path](../src/dlls/objects/626/626.c#L892) in E1/P1 additionally clears
**`0xBF7`**, before shutting down the meter, clearing `0x634`, and spawning
the death object. Earlier versions leave `0xBF7` alone. Here the meter loses a
point on a hit; its API name does not establish an oxygen/drowning mechanic.

**Location and consumer:** Dragon Rock's `DR_Creator` placement `0x493D3`
uses **`0xBF7` as `spawnGameBit`**, with behavior mode 9.
[`DR_Creator_update`](../src/dlls/objects/613_DR_Creator/DR_Creator.c#L123)
starts sequence 4 while that bit is set, and its callback can spawn
`DRHomingMis` projectiles. Thus this is a concrete missile-sequence enable flag,
not merely an unnamed save bit.

**Expected symptom:** the missile sequence remains enabled after HighTop dies,
potentially interfering with encounter cleanup or retry. **Test lead:** let
HighTop's meter reach zero in control mode 2 or 8, then compare the creator's
sequence/spawn activity and subsequent retry. A specific softlock remains
unconfirmed.

### G17 — EarthWarrior zeroes movement state on every update of one action

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** Medium.

[`DR_EarthWarrior_stateHandler03`](../src/dlls/objects/599_DR_EarthWar/DR_EarthWar.c#L216)
clears `animSpeedA/B/C` and XYZ velocity every call in E0/J0. Later versions
perform those resets only when `moveJustStartedA` is true, alongside starting
move 7 or 8.

**Location:** `DR_EarthWar` placement `0x4C117` in **Dragon Rock, upper area**
(`dragrock`, map 2), the EarthWalker riding encounter. The filename's
"EarthWarrior" spelling is retained for locating the implementation.

**Expected symptom:** movement or accumulated state generated after entry is
repeatedly erased, making the action stall or behave differently under motion.
**Test lead:** compare these six fields through the full action, especially
while mounted. The names alone do not prove that the entire animation freezes;
the separately assigned `moveSpeed` is not zeroed by this branch.

### G18 — Drakor normalizes a zero missile-launch vector

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** High.

The [missile launch calculation](../src/dlls/objects/589_BossDrakor/BossDrakor.c#L308)
subtracts a projection and unconditionally normalizes the remainder in E0/J0.
Later versions normalize only if at least one component is nonzero. The
[SDK implementation](../src/dolphin/mtx/vec.c#L53) has no zero-vector guard.

**Location:** the retail map-name table calls map 44 **"BOSS Drakor"**. Its
romlist is misleadingly named `finalboss`; this finding concerns the Drakor
fight, not the later Andross fight.

**Expected symptom:** invalid/non-finite missile velocity when the remainder is
exactly zero, potentially producing a motionless, disappearing, or otherwise
broken projectile. **Test lead:** log the remainder during aligned launch
conditions, then compare the resulting velocity. It is not evidence that all
missiles, or ordinary near-zero vectors, fail.

### G19 — Staff glow can persist after leaving a magic cave's approach radius

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** High for the
missing cleanup; Medium for how long the glow survives other updates.

The [magic-cave-top proximity logic](../src/dlls/objects/287_MagicCaveTo/MagicCaveTo.c#L184)
resets the rumble timer and completion flag when the player moves outside the
approach radius. Later versions also obtain the staff and disable its glow.

**Expected symptom:** the cave-related staff glow remains after backing away.
**Test lead:** approach far enough to activate the effect, retreat without
entering, and watch until another action changes the staff's glow state.

## Loading, resources, and audio

### G20 — Timer sound continues while the timer's effective delta is zero

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** High.

[`gameTimerRun`](../src/main/modelEngine.c#L899) sets its local delta to zero
when the timer is paused or the HUD is hidden. E0/J0 still refresh the looped
timer sound. Later versions [guard that keepalive](../src/main/modelEngine.c#L959)
with `if (dt)`, while retaining volume and pan updates.

**Expected symptom:** timer beeping continues during a paused or hidden-HUD
interval. **Test lead:** pause the actual timer or enter a HUD-hidden sequence
while its loop sound is active. Opening the pause menu alone is not proof that
this function's pause flag is set. See also
[the existing timer audit](timer_controls_and_revisions.md).

### G21 — Shop Fuel Cell display destruction leaks live lightning effects

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** High.

[`shopitem_free`](../src/dlls/objects/644/644.c#L232) gains a loop freeing all
non-null entries in `lightningHandles[10]` for object type `0x468`. Its
[render routine](../src/dlls/objects/644/644.c#L127) allocates those effects and
normally frees them when their individual timers expire. Destroying the item
before expiry loses that ordinary cleanup opportunity in E0/J0.

**Location:** **ThornTail Store**, `swapstore` (map 51, retail map label
"Shop"). Raw ID `0x468` resolves to `SPFuelCell`, placed as `0x45B6F`.
This narrows the effect-bearing "sparkle item" to the Fuel Cell display;
the other shop items sharing DLL 644 are not all covered by this exception.

**Expected symptom:** memory loss whenever the item is unloaded with effects
still active; repeated visits could eventually increase allocation pressure.
**Test lead:** compare heap usage before and after repeated item load/unload
cycles. A crash threshold or amount leaked per visit has not been measured.

### G22 — Chapter-start save data is loaded and never freed

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** High for the leak,
Low for ordinary gameplay exposure.

In the [save-select launch path](../src/dlls/engine/53/53.c#L640), a chapter
value greater than 1 loads a savegame file and copies `0x6EC` bytes to the work
buffer. Only later versions free the loaded buffer afterward.

**Expected symptom:** leaked memory when launching through this chapter-start
path. **Test lead:** use the existing chapter-select flow if accessible and
compare allocations after repeated launches. This does not imply that loading
an ordinary player save leaks, or that chapter selection is exposed in every
retail menu configuration.

### G23 — Model/animation table lookups can race asynchronous loading

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** Medium.

[`getTableFileEntry`](../src/main/pi_dolphin.c#L1273) gains a wait on in-flight
load flags before dereferencing two tables: `MODELS.tab` (`0x2A`, mask `0xC`)
and `ANIMCURV.tab` (`0x0E`, mask `0xA0000000`). The wait services loads, reset
input, and disc errors. E0/J0 can read those tables immediately.

**Expected symptom:** stale/unready offsets cause the wrong model or animation
data to be requested, potentially leading to malformed objects or a load
failure. **Test lead:** log the pending flags at lookup, especially during fast
map transitions or interrupted disc reads. The table IDs are defined in
[mldf_fileid.h](../include/main/mldf_fileid.h#L22).

### G24 — Newly allocated model data lacks an explicit cache invalidation

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** Low to Medium.

[`ObjModel_LoadModelData`](../src/main/model.c#L2246) gains
`DCInvalidateRange(model, totalSize)` between allocation and loading/decompression.

**Expected symptom:** a possible hardware/cache-dependent stale-data issue when
memory is reused for model loading. The added cache operation is definite;
whether another layer already makes a particular transfer coherent is not
established by this audit. **Test lead:** inspect cache operations throughout
the actual loading path and test repeated model loads on hardware or with
appropriate cache emulation. Do not report generic model corruption as confirmed.

## Interaction prompts, text, and saving

### G25 — Missing explicit Hint prompt on a paid guard interaction

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** High for the
omitted icon request; Medium for the final visible icon.

The [guardclaw interaction](../src/dlls/objects/202/guardclaw.c#L163) becomes
enabled when `userData1 == 2` and the placement's game bit is clear. Later
versions explicitly request `A_BUTTON_ICON_HINT` when the player is in range.
E0/J0 still permit the paid trigger but omit that icon call.

**Locations:** the retail `GuardClaw` placements are the **SnowHorn Wastes
BribeClaw** (`wastes`, `0x305D4`, completion bit `0xD83`, named
[`GAMEBIT_NW_GotPastBribeClaw`](../include/main/gamebit_ids.h#L933)) and a
**Cape Claw guard** (`capeclaw`, `0x4B939`, completion bit `0xE00`). They
are registered under the shared Baddie DLL 201, which dispatches to this
handler, so looking only for object definitions with DLL 202 misses them.

**Expected symptom:** an absent or inappropriate A-button prompt even though
interaction works. **Test lead:** approach the unpaid/incomplete guard in that
state and compare the HUD; other UI code may supply a default icon.

### G26 — CloudRunner Fortress timer interactions lack an explicit idle prompt

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** High for the
omission; Medium for a visible glitch.

The [Sun Temple interaction](../src/dlls/objects/659/659.c#L157) suppresses the
prompt while the timer is running in all versions. Later code additionally
sets `A_BUTTON_ICON_CONTEXT_B` when that variant is in range and the timer is
not running.

**Location:** the checked raw object ID is **`0x830`**, which resolves to
`CFSunTemple` in **CloudRunner Fortress**, not a Walled City Sun Temple.
The three placements are `0x48186`, `0x4817C`, and `0x48188`; they use
activation bits `0xB1B`, `0xB1C`, and `0xB1D`, each also used by a nearby
`CNTstopwatc` timer. DLL 659 additionally serves `WCInvUseObj` in Walled
City, but that object has ID `0x526` and does not satisfy this version guard's
object-ID condition.

**Expected symptom:** an incorrect or missing prompt for starting the idle
interaction. **Test lead:** approach the timer-lockout variant before a timer
starts and after it finishes. This change does not establish broken timer logic.

### G27 — Walled City controller lacks its added area-name banner

**Absent in:** E0, J0. **Added in:** P0, E1, P1. **Confidence:** High for the
difference, Low that it is a bug rather than a usability addition.

[`wclevelcont_init`](../src/dlls/objects/653_WCLevelCont/WCLevelCont.c#L739)
sets a new `messageTimer` to 300 nominal frames. The
[update routine](../src/dlls/objects/653_WCLevelCont/WCLevelCont.c#L695) displays
text **1401** while the timer is positive, subtracting `timeDelta`. E0/J0 have
neither the field nor this display path.

**Recovered text:** in E1/P0/P1's `gametext/WallCity/English.bin`, entry 1401
is a formatting prefix followed by **"Walled City"**. Despite the current
constant name `WCLEVELCONT_TILE_MESSAGE_TEXT_ID`, this is the area-name banner,
not explanatory tile-puzzle text.

**Expected difference:** this controller does not request the area banner in
E0/J0. **Test lead:** enter/reload Walled City and compare text for the first
300 nominal frames. Other possible title-display paths have not been excluded;
this is a presentation addition, not evidence of a puzzle softlock.

### G28 — Memory-card messages use fixed spacing despite variable text height

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** Medium.

[`showMemCardError`](../src/track/intersect_memcard.c#L532) advances each message
string by a fixed 24 pixels in E0/J0. Later versions measure its height, take at
least the language's line height, then add 5 pixels.

**Expected symptom:** wrapped or tall strings crowd/overlap subsequent strings.
**Test lead:** compare multi-string memory-card errors and confirmation dialogs,
especially ones whose strings wrap. The change may primarily support longer
localized text; it does not prove stock E0/J0 messages necessarily overflow.

### G29 — Pause-menu hints do not round multi-line heights to full line spacing

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** Medium.

The [hint-text loop](../src/dlls/engine/0/0.c#L5005) previously advances by the
greater of measured glyph bounds and one font line height. Later code rounds
the measured span upward to a whole number of line heights before advancing.

**Expected symptom:** uneven or overly tight spacing between successive
multi-line hints, potentially overlap at particular glyph heights.
**Test lead:** compare a hint with multiple wrapped phrases and inspect the next
phrase's baseline. A short one-line hint may look identical in both versions.

### G30 — Card-image repair can write save blocks before the requested callback

**Suspect:** E0, J0. **Changed in:** P0, E1, P1. **Confidence:** Low for a
player-facing failure; High for the extra write path.

During image/checksum repair,
[`saveGame_prepareAndWrite`](../src/dlls/engine/2/maketex.c#L238) in E0/J0 can
write save blocks immediately, then later run the supplied callback if the
result permits it. Later versions perform those preliminary writes only when
`cb == NULL`, allowing a callback-driven operation to handle its own write.

**Expected symptom:** unnecessary writes, extra delay, or an additional failure
point before the intended callback when the image checksum requires repair.
The branch alone does **not** establish save corruption or lost progress.
**Test lead:** instrument write counts and callback entry with deliberately
mismatched card-image checksums, using disposable test saves.

### G31 — PAL title-screen file creation does not reload newly available options

**Primary comparison:** P0 versus P1. **Changed in:** P1 only. **Confidence:** Low.

[`TitleMenu_CreateSaveFile`](../src/dlls/engine/52_n_attractmode/n_attractmode.c#L338)
in P1 calls `loadGameOptions()` after successful `cardCreateSaveFile(1)`.
All other builds omit that step. Its PAL callers include the initial language
setup/restore flow, which preserves language and subtitle choices around card
handling.

**Expected symptom:** other option values in memory may remain stale after
successful creation/recovery of a save file. **Test lead:** on P0/P1, exercise
first-time language setup and card insertion/retry, then compare in-memory and
saved options. E0/J0/E1 also lack the reload, but do not expose the same PAL
language setup, so a universal settings-loss claim would overreach.

## Unresolved, latent, and later-version issues

### U01 — The added timer cancellation check is an ineffective fix

**Added in:** P0, E1, P1. **Underlying synchronization risk:** not shown fixed in
any version. **Confidence:** High that the check is inert as written.

[`timer_update`](../src/dlls/objects/693_Timer/Timer.c#L117) adds a condition
intended to set expiry, clear the start bit, and end a global-mode timer when
`isGameTimerDisabled() == 1`. But the
[accessor](../src/main/modelEngine.c#L1033) returns the raw mask `state & 2`:
its possible results are **0 and 2**, never 1.

Thus earlier versions lack the synchronization attempt, and later versions
still cannot enter that added branch. A globally stopped timer and the object's
local countdown/progress bits may disagree until another path resolves them.
This is stronger than a generic porting suspicion: the existing
[timer audit](timer_controls_and_revisions.md) records the same comparison and
raw return value in the checked retail binaries. **Test lead:** stop the global
timer externally while the Timer object remains active; compare its local
countdown, start/expiry bits, and ended latch. Do not label this “fixed in rev1.”

### U02 — Later audio allocation adds a prefix, but freeing still uses the interior pointer

**Changed in:** P0, E1, P1. **Earlier behavior:** E0/J0 allocate normally.
**Confidence:** High for the allocation/free mismatch; Low for lasting gameplay impact.

[`_audioAlloc`](../src/main/audio.c#L702) adds `0x100` bytes to a `0x2DC0` request
and returns allocation base plus `0x100`. That request is the 48-entry DSP voice
array. The [existing binary-backed audit](audio_voice_allocation_prefix.md)
leaves the purpose of the prefix unresolved; it cannot establish an earlier
audio glitch or prove this was an underrun workaround.

There is also a later-version risk visible as written:
[`salExitDspCtrl`](../src/musyx/runtime/sal_studio.c#L130) frees `dspVoice`
directly, [`audioFree`](../src/main/audio.c#L698) passes it on unchanged, and
[`mmFree`](../src/main/mm.c#L594) requires an exact allocation-base match.
That allocation therefore cannot be freed through this path. An allocation
failure also becomes an unchecked interior-pointer result in the special case.

**Test lead:** trace `audioReset` / `sndQuit` and heap reclamation in a later
build. The visible game caller runs during
[system reset](../src/main/gameloop.c#L775), so the failed free may have no
lasting player-visible effect on that path.
Do not turn the unexplained prefix into a claimed E0/J0 sound-corruption bug.

### U03 — The old Japanese system-font lookup omits the ideographic space

**Suspect:** E0/J0 when using the SJIS system-font path. **Changed in:** E1's
SJIS path. **PAL:** uses a different ANSI/localized setup. **Confidence:** High
for the lookup omission; Low for a visible spacing difference.

The resident [Japanese glyph list](../src/main/gametext.c#L621) contains U+3000,
also used by the wrong-disc message's spacer line. The old
[lookup table](../src/main/gametext_data.c#L378) does not map it;
[`lookupSjisGlyph`](../src/main/gametext.c#L1659) returns zero. E1's replacement
[table](../src/main/gametext.c#L767) maps it to SJIS `0x8140`.

The atlas builder passes the resulting empty string to
[`OSGetFontWidth` / `OSGetFontTexel`](../src/dolphin/os/OSFont.c#L336), both of
which return without updating the supplied width for a zero first byte.
Consequently that blank glyph inherits the preceding glyph's width. Its pixels
remain blank because the scratch image was cleared first.

**Expected symptom:** potentially incorrect blank/spacer metrics on the Japanese
disc-status screen. If both glyphs have the same width, nothing visible changes.
**Test lead:** compare U+3000's generated width and the wrong-disc screen under
SJIS. A normal ANSI-font E0 boot does not use this path. A small source-table
check found U+3000 was the only changed lookup result among the 85 resident
Japanese glyphs; the larger table is not evidence of 514 extra missing glyphs.

## Differences reviewed without calling them glitches

| ID | Difference | Why it is not counted as a demonstrated bug |
| --- | --- | --- |
| N01 | PAL display modes and viewport heights | PAL selects 50 Hz or EURGB60 instead of NTSC progressive scan. PAL's game-owned mode has EFB height 480 and XFB height 528, so later viewport code uses EFB height. E0/J0's normal modes use equal heights; their XFB-height spelling alone does not establish a retail viewport bug. See [video_viewport.h](../include/main/video_viewport.h#L6), [game UI](../src/dlls/engine/0/0.c#L4089), and [display-mode audit](gameloop_regional_display_mode.md). |
| N02 | PAL display-choice layout while a DVD error is shown | PAL calls `dvdCheckError` before drawing, moves the question from Y=110 to Y=190, and dims it by 64. Non-PAL draws the error later and ignores the newer return value even in E1. This suggests an overlap/readability test, but compares differently laid-out prompts, not proof of a shared earlier glitch. [gameloop.c](../src/main/gameloop.c#L328), [fileio.c](../src/main/fileio.c#L37). |
| N03 | PAL language options, defaults, resident messages, title art, and SRAM access | Expected regional functionality: five-language options, language-dependent resources, separate save-options calls, and PAL SRAM mode APIs. Dutch-console initial language setup also suppresses selected early card dialogs. EN/JP not exposing these is not a bug. [Options](../src/dlls/engine/55/55.c#L101), [save defaults](../src/dlls/engine/21/21.c#L1451), [language switching](../src/main/gametext.c#L1918), [card dialogs](../src/track/intersect_memcard.c#L503). |
| N04 | Dinosaur-language cheat availability | PAL deliberately hides the cheat entry and omits the well's fourth cheat reward. Non-PAL grants that reward only on the non-SJIS path. This looks like a regional feature decision, not a broken token; other reward/follow-up behavior remains. [Maze well](../src/dlls/objects/611_GM_MazeWell/GM_MazeWell.c#L131), [options](../src/dlls/engine/55/55.c#L319). |
| N05 | PAL50 movement tuning | Drakor, his hoverpad, and HighTop have render-mode-sensitive speed adjustments. These are candidates for timing/balance comparisons, but their absence in NTSC is not a defect. P0 and P1 share these branches. [Drakor](../src/dlls/objects/589_BossDrakor/BossDrakor.c#L718), [hoverpad](../src/dlls/objects/625/625.c#L674), [HighTop](../src/dlls/objects/626/626.c#L938). |
| N06 | UI layout and decoration | Later code adds task bullets, adjusts WarpStone art and a save-prompt Y coordinate, adds a four-pixel text-box border, and chooses narrow/wide NPC text boxes using an embedded marker. These are observable presentation differences; no particular stock text clipping failure was established. [NPC boxes](../src/dlls/engine/0/0.c#L2086), [text-box definitions](../src/main/gametext_data.c#L166), [border](../src/main/textrender_drawbox.c#L18), [WarpStone](../src/dlls/engine/65/65.c#L19), [menu line height](../src/dlls/engine/60/60.c#L24). |
| N07 | Diagnostics, storage, and matching-related source shape | Freed-object logging, assert line numbers, a removed unused constant, descriptor tail widths, local-variable aliases, and relocated/reshuffled string tables do not by themselves establish gameplay changes. The shrinking opaque `MldfNames.adjacency` array also has no identified glitch from this audit. [object logging](../src/main/object.c#L1287), [descriptor tail](../include/dlls/objects/358.h#L9), [adjacency](../src/main/pi_dolphin.c#L174). |
| N08 | Card-comment language constants and font setup refactoring | `OS_LANGUAGE_ITALIAN` in E1's comment builder numerically equals game `LANGUAGE_JAPANESE` (4); `getCurLanguage` returns the game's enum. The misleading constant name is not proof that Italian saves receive Japanese titles. Card-comment asset offsets move together with their backing arrays. The system-font resource-selection refactor has no demonstrated bug beyond U03. [comment builder](../src/dlls/engine/2/maketex.c#L394), [font setup](../src/main/gametext.c#L1271). |

## Location-evidence appendix

The follow-up scanned all **124 romlists in each of J0, E1, P0, and P1**.
Map labels were read from each extract's `MAPINFO.bin` and joined to the
romlist-name table in its DOL using
[map_catalog.py](../tools/orig/map_catalog.py). Object names and DLL numbers
were resolved through `OBJINDEX.bin`, `OBJECTS.tab`, and `OBJECTS.bin` using
[romlist_params.py](../tools/orig/romlist_params.py). The map/DLL associations
in the location index agree across these four extracts.

Each input DOL's SHA-1 was checked against its own `config/<version>/config.yml`
before using its tables. This validates the DOL input, not every extracted
asset or a complete regional disc. The local E0 `files/` directory does not
contain the romlists, `MAPINFO.bin`, or `OBJECTS.bin`; **E0 placement bytes were
not independently checked**. E0 locations are inferred from the source's same
identities/controller IDs and the four secondary extracts.

The records below give reproducible anchors without requiring guessed room
names. Offsets are into the **decompressed** `.romlist.zlb` stream and use E1
as the reference; identifiers remain the better cross-version key when records
move. Parameter offsets are relative to the record, after the
[24-byte common placement header](../include/game/objects/object_setup.h).
Coordinates elsewhere in this document are stored placement coordinates, not
a promise of the same numbers in every runtime world/local coordinate space.

| Finding | Romlist | E1 decoded offset | Placement identity / object | Evidence to inspect |
| --- | --- | --- | --- | --- |
| G04 | `hollow` | `0x4E90` | `0x435F2` / `SH_tricky` | DLL 422, raw ID `0x2EE` |
| G10 | `temple` | `0x799C` | `0x41996` / `VFP_PuzzleP` | `+0x1A = 0x4F6`, `+0x1E = 1`, `+0x20 = 0x9FD` |
| G10 | `temple` | `0x797C` | `0x4AC27` / `HitAnimator` | Target bit at `+0x18 = 0x4F6` |
| G10 | `temple` | `0x7A68` | `0x4C35F` / `VFPSeqObj` | Trigger at `+0x1A = 0x9FD`; open bit `0xCF4` |
| G11 | `snowmines` | `0x3FF8` | `0x4B13A` / `DIMUseObjec` | Unlocked bit `+0x1C`; horn bit `0x1EE` at `+0x1E` and `+0x22` |
| G11 | `snowmines` | `0x7914`, `0x796C` | `0x30329`, `0x30386` / `DIMSeqObjec` | Both open bits at `+0x18 = 0x3EF`; triggers `0x1FE`, `0x3F1` |
| G12 | `warlock` | `0x2CC8`, `0x4B6C` | `0x1F1A`, `0x47293` / `WM_Pressure` | DLL 510, raw ID `0x144` |
| G12 | `warlock` | `0x520` | `0x42D45` / `WM_seqpoint` | `+0x1E = 0x905`, the camera/sequence-start bit |
| G15/G16 | `dragrock` | `0x6F40` | `0x3460C` / `DR_HighTop` | DLL 626; variant at `+0x19 = 0` |
| G15 | `dragrock` | `0xA4C` | `0x4555F` / `DR_Seqobj` | Open bit `0x632`, trigger `0x631`, sequence 11, flags `0x08` |
| G16 | `dragrock` | `0xFE0` | `0x493D3` / `DR_Creator` | Spawn-enable bit `+0x18 = 0xBF7`; behavior `+0x1A = 9` |
| G17 | `dragrock` | `0x63F8` | `0x4C117` / `DR_EarthWar` | DLL 599, raw ID `0x416` |
| G21 | `swapstore` | `0x1D4` | `0x45B6F` / `SPFuelCell` | DLL 644, raw ID `0x468`, definition index `0x3D1` |
| G25 | `wastes` | `0x5214` | `0x305D4` / `GuardClaw` | `gameBitD` at `+0x1C = 0xD83` |
| G25 | `capeclaw` | `0x5F4C` | `0x4B939` / `GuardClaw` | `gameBitD` at `+0x1C = 0xE00` |
| G26 | `fortress` | `0x7970`, `0x7998`, `0x79C0` | `0x48186`, `0x4817C`, `0x48188` / `CFSunTemple` | Raw ID `0x830`; activation bits `0xB1B`/`0xB1C`/`0xB1D` |
| G26 | `fortress` | `0x7A0C`, `0x79E8`, `0x7A30` | `0x48187`, `0x4817F`, `0x48189` / `CNTstopwatc` | Timer-enable bits `0xB1B`/`0xB1C`/`0xB1D` respectively |

The source layouts that give these bytes meaning include
[PressureSwitchFB](../include/dlls/objects/251.h),
[DoorLock](../include/dlls/objects/273.h),
[SeqObject](../include/dlls/objects/274.h),
[DR_Creator](../include/main/dll/DR/dll_0265_drcreator.h),
[GroundBaddie](../include/main/dll/baddie_state.h#L225), and
[SunTemple](../include/main/dll/dll_0293_suntemple.h).
In particular, [SeqObject_update](../src/dlls/objects/274/274.c#L125) gives
the rescue object's `0x08` flag its set-open-bit-on-completion behavior.

### Targeted asset differences

The G10 pressure plate, G12 switches/camera sequence point, G16 creator,
G21 Fuel Cell, G25 guards, and G26 interaction records are byte-identical
across the four checked extracts, even where their stream offsets differ.
The following nearby differences should be preserved when testing:

| Record | J0 | E1 / P0 | P1 | Interpretation |
| --- | --- | --- | --- | --- |
| G11 `0x4B13A`, `+0x1C` | `0x001A` | `0x001A` | `0x03EF` | Concrete P1 asset correction paired with the code exception; changes which bit makes the horn interaction inactive |
| G10-associated `VFPSeqObj` `0x4C35F`, `+0x20` | `0x0095` | `0x008C` | `0x008C` | Different preempt-sequence ID; could be regional sequence numbering, so not independently labeled a fix |
| G15 `DR_Seqobj` `0x4555F`, header `+0x04` / `+0x06` | `0x04` / `0x6C` | `0x01` / `0xD4` | `0x01` / `0xD4` | Loading metadata differs; its rescue/trigger parameters are unchanged. Runtime impact not established |

For G11 the entire record is 40 bytes and only the two-byte unlocked-bit
field differs among these extracts. Both associated `DIMSeqObjec` records
are byte-identical across all four; their E1 offsets above shift to
`0x7804`/`0x785C` in J0/P0 and `0x79B4`/`0x7A0C` in P1.

For G27, parse `files/gametext/WallCity/English.bin` using
[`gameTextFinalizeLoad`'s table layout](../src/main/gametext.c#L1133):
a four-byte glyph count, 16 bytes per glyph, a four-byte entry header,
12 bytes per [GameTextDef](../include/main/gametext_lookup.h), then a counted
table of string offsets. In E1/P0/P1, definition 1401 is at `0xA4C`; its
single string is `EF A3 B4 01 B3` followed by `Walled City` and a null byte.
Those are formatting bytes plus a title, not a recovered puzzle explanation.

Example placement queries, run separately because search terms are combined:

```sh
python tools/orig/romlist_params.py --files-root orig/GSAE01_rev1/files --search dll:0x0111
python tools/orig/romlist_params.py --files-root orig/GSAE01_rev1/files --search dll:0x0272
python tools/orig/romlist_params.py --files-root orig/GSAE01_rev1/files --search dll:0x0293
```

The local Rena reference project's game-bit XML helped find candidate
relationships, especially `0x3EF` and `0x631`. The relationships reported above
were then checked against the actual placement bytes and current source;
reference annotations alone were not treated as proof of a map-specific fix.
Neither this follow-up nor the original audit compares every sequence, trigger,
collision mesh, map block, or object placement between releases. Untraced
script-only fixes remain an open area for further work.

## Coverage appendix

Counts include each version-bearing `#elif`, as well as nested conditions and
header declarations. Multiple guards implementing one behavior are grouped into
one finding. Paths link to the audited file; finding references give the useful
behavioral locations. This is a source-conditional inventory, not a count of
retail bugs.

| File | Directives | Disposition |
| --- | ---: | --- |
| [engine/0/0.c](../src/dlls/engine/0/0.c) | 16 | G29; N01, N06, N07 |
| [engine/2/maketex.c](../src/dlls/engine/2/maketex.c) | 11 | G30; N03, N08 |
| [engine/20_Hcurves/Hcurves.c](../src/dlls/engine/20_Hcurves/Hcurves.c) | 4 | G01; diagnostic/storage changes |
| [engine/21/21.c](../src/dlls/engine/21/21.c) | 2 | N03 |
| [engine/23/23.c](../src/dlls/engine/23/23.c) | 4 | N03 |
| [engine/47/47.c](../src/dlls/engine/47/47.c) | 2 | G07 |
| [engine/52_n_attractmode/n_attractmode.c](../src/dlls/engine/52_n_attractmode/n_attractmode.c) | 11 | G31; N03, N07 |
| [engine/53/53.c](../src/dlls/engine/53/53.c) | 3 | G22; task bullets in N06 |
| [engine/55/55.c](../src/dlls/engine/55/55.c) | 8 | N03, N04; descriptor padding in N07 |
| [engine/60/60.c](../src/dlls/engine/60/60.c) | 1 | N06 |
| [engine/65/65.c](../src/dlls/engine/65/65.c) | 1 | N06 |
| [objects/196_Tricky/tricky.c](../src/dlls/objects/196_Tricky/tricky.c) | 1 | G03 |
| [objects/201_Baddie/Baddie.c](../src/dlls/objects/201_Baddie/Baddie.c) | 5 | G02; unused constant in N07 |
| [objects/202/guardclaw.c](../src/dlls/objects/202/guardclaw.c) | 1 | G25 |
| [objects/245_SidekickBal/SidekickBal.c](../src/dlls/objects/245_SidekickBal/SidekickBal.c) | 4 | G05, G06 |
| [objects/251/251.c](../src/dlls/objects/251/251.c) | 1 | G10 |
| [objects/273/273.c](../src/dlls/objects/273/273.c) | 1 | G11 |
| [objects/287_MagicCaveTo/MagicCaveTo.c](../src/dlls/objects/287_MagicCaveTo/MagicCaveTo.c) | 1 | G19 |
| [objects/344/344.c](../src/dlls/objects/344/344.c) | 3 | G08, G09 |
| [objects/417/417.c](../src/dlls/objects/417/417.c) | 1 | G13 |
| [objects/422_SH_tricky/SH_tricky.c](../src/dlls/objects/422_SH_tricky/SH_tricky.c) | 1 | G04 |
| [objects/510/510.c](../src/dlls/objects/510/510.c) | 1 | G12 |
| [objects/589_BossDrakor/BossDrakor.c](../src/dlls/objects/589_BossDrakor/BossDrakor.c) | 5 | G18; N05 and logging |
| [objects/598_DIMSnowHorn/DIMSnowHorn.c](../src/dlls/objects/598_DIMSnowHorn/DIMSnowHorn.c) | 2 | G14 |
| [objects/599_DR_EarthWar/DR_EarthWar.c](../src/dlls/objects/599_DR_EarthWar/DR_EarthWar.c) | 1 | G17 |
| [objects/611_GM_MazeWell/GM_MazeWell.c](../src/dlls/objects/611_GM_MazeWell/GM_MazeWell.c) | 1 | N04 |
| [objects/613_DR_Creator/DR_Creator.c](../src/dlls/objects/613_DR_Creator/DR_Creator.c) | 2 | G15 |
| [objects/625/625.c](../src/dlls/objects/625/625.c) | 1 | N05 |
| [objects/626/626.c](../src/dlls/objects/626/626.c) | 4 | G15, G16; N05 |
| [objects/644/644.c](../src/dlls/objects/644/644.c) | 2 | G21 |
| [objects/653_WCLevelCont/WCLevelCont.c](../src/dlls/objects/653_WCLevelCont/WCLevelCont.c) | 2 | G27 |
| [objects/659/659.c](../src/dlls/objects/659/659.c) | 1 | G26 |
| [objects/693_Timer/Timer.c](../src/dlls/objects/693_Timer/Timer.c) | 1 | U01 |
| [objects/704/704.c](../src/dlls/objects/704/704.c) | 1 | N03: regional title texture |
| [dolphin/os/OSRtc.c](../src/dolphin/os/OSRtc.c) | 3 | N01, N03: language/display accessors |
| [main/audio.c](../src/main/audio.c) | 1 | U02 |
| [main/fileio.c](../src/main/fileio.c) | 3 | N02: DVD check result |
| [main/gameloop.c](../src/main/gameloop.c) | 18 | N01, N02 |
| [main/gametext.c](../src/main/gametext.c) | 16 | U03; N03, N08 |
| [main/gametext_data.c](../src/main/gametext_data.c) | 5 | U03; N03, N06, N08 |
| [main/mm.c](../src/main/mm.c) | 1 | N01: progressive flag storage absent in PAL |
| [main/model.c](../src/main/model.c) | 1 | G24 |
| [main/modelEngine.c](../src/main/modelEngine.c) | 2 | G20 |
| [main/object.c](../src/main/object.c) | 2 | N07 |
| [main/pi_dolphin.c](../src/main/pi_dolphin.c) | 7 | G23; N07 |
| [main/pi_videoinit.c](../src/main/pi_videoinit.c) | 1 | N01: PAL display-copy filtering |
| [main/textrender_drawbox.c](../src/main/textrender_drawbox.c) | 1 | N06 |
| [track/intersect_memcard.c](../src/track/intersect_memcard.c) | 6 | G28; N03 |
| [include/dlls/objects/344.h](../include/dlls/objects/344.h) | 3 | G08 state/bitfield layout |
| [include/dlls/objects/358.h](../include/dlls/objects/358.h) | 2 | N07 |
| [include/main/dll/dll_0015_save_settings.h](../include/main/dll/dll_0015_save_settings.h) | 1 | N03 declarations |
| [include/main/dll/savegame.h](../include/main/dll/savegame.h) | 1 | N03 declarations |
| [include/main/dll/WC/dll_028D_wclevelcont.h](../include/main/dll/WC/dll_028D_wclevelcont.h) | 2 | G27 state layout |
| [include/main/fileio.h](../include/main/fileio.h) | 1 | N02 declaration |
| [include/main/gameloop_internal.h](../include/main/gameloop_internal.h) | 3 | N01 declarations |
| [include/main/gametext_api.h](../include/main/gametext_api.h) | 1 | N03 declaration |
| [include/main/gametext_data.h](../include/main/gametext_data.h) | 1 | N03 declarations |
| [include/main/video_viewport.h](../include/main/video_viewport.h) | 1 | N01 |
| [include/track/intersect_card_api.h](../include/track/intersect_card_api.h) | 1 | N03 declaration |
| **Total** | **190** | **59 files** |

The broader directive scan also found inherited SDK/compiler switches such as
`SDK_REVISION`, `VERSION_GCCP01`, and `__MWERKS__`, plus generic legacy `VERSION`
macros in `global.h`. These are not automatically retail SFA release differences.
The current configuration supplies `VERSION_<target>` and applies the PAD donor
switch independently of the SFA target, so those switches were not counted as
additional game glitches.

To find newly added version branches for a future refresh:

```sh
rg -n '^\s*#\s*(if|ifdef|ifndef|elif)\b.*VERSION_GSA' src include
```

Validation for this document consisted of reviewing both sides of the guards
and relevant callers/types, checking the coverage counts and local links, and
small source-derived checks for G01's checksum collisions, U01's impossible
comparison, and U03's glyph mappings. The location follow-up additionally
checked four configured DOL hashes, scanned their romlists, compared the
targeted records, and decoded the Walled City text entry. No gameplay
reproduction or new full build is claimed by this documentation-only audit.
