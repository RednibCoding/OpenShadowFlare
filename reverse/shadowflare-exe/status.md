# Reconstruction status labels

- `identified`: the address and broad purpose are known.
- `analyzed`: control flow and important side effects are understood.
- `partial`: some corresponding portable behavior is implemented, but it is
  not complete enough for a fidelity claim.
- `implemented`: the portable behavior is complete enough for focused tests.
- `verified`: behavior has passed an original-versus-reconstruction test with
  representative retail data or event sequences.

Addresses describe the retail executable only. The portable executable does
not try to preserve internal addresses, compiler-generated class layouts, or
operating-system handles.

## Enemy effect runtime

The effect path has two distinct retail owners. `0x0042fdc0` first copies the
22 constructor arguments into a `0x3b0`-byte controller node. That node keeps
the effect number, source and target identities, direction, optional explicit
origin, source judgement, delay, complete 77-word packet, packet kind, and the
remaining authored constructor values. `0x0042fd60` updates this controller
list and removes a node only when its selected handler returns zero.

`0x00429ec0` is the controller dispatch, not a visual-resource lookup table.
The twelve effect numbers present in shipped enemy profiles dispatch as
follows:

| Effect | Controller |
|---:|---:|
| 10001 | `0x0042ae40` |
| 10002 | `0x0042b1c0` |
| 10003 | `0x0042b540` |
| 10004 | `0x0042a860` |
| 10005 | `0x0042cd70` |
| 10010 | `0x0042e7e0` |
| 10011 | `0x0042d6e0` |
| 10012 | `0x0042db10` |
| 10013 | `0x0042e240` |
| 10014 | `0x0042e5c0` |
| 10016 | `0x0042ea50` |
| 10021 | `0x0042eeb0` |

The first two branches prove why a portable effect cannot be represented as
one animation with a damage callback. On controller update zero, type 1 creates
resource `10000012` at the current source actor and type 2 creates resource
`11000027`. Both are ordinary source animations whose positive judgement
edges come from the source bounds plus one. When the controller counter
reaches constructor argument 12, it resolves the source position again,
projects a point exactly 180 world units along the stored angle for nonzero
owners, and creates a second runtime actor there. Owner kind zero leaves that
child at the supplied explicit origin. Type 1 uses resource `10000010` and positional
sample 19; type 2 uses resource `10000040` and positional sample 94. The
second actor has `[-50,-50,50,50]` bounds, chart-zero timing, the copied combat
packet, and contact expiry. Only then does the controller return zero.

The third branch is table-driven. `0x0042b540` reads Table 205 row zero at
`subtype - 1`; Plasma Bat subtype 20 yields five waves. From the stored impact
origin it attempts a wave every four updates at radii 250, 450, 650, 850, and
1050. The placement query uses `[-100,-100,100,100]`, ordinary map collision,
and dynamic type-zero scenario objects. A failed query permanently suppresses
that and every later wave.

A clear wave consumes one `rand() % 4` and creates resources `10000030`,
`10000031`, and `10000032` together. Only the first actor uses the random
chart and an update-zero collision window; it processes every overlapping
target with the copied packet. The other two are chart-zero visual layers.
Sample 21 plays once at the wave position. The controller expires at
`delay + wave count * 4`, separately from all three actors' CAF lifetimes.

Type 4 at `0x0042a860` re-resolves its source twice. Update three creates
resource `10000002` at the current source with the complete source judgement,
chart zero, direction eight, and additional display status `0x80`. At the
authored delay it resolves the source again and creates resource `10000000`
twice at the upper and lower opposite judgement corners. The first draws
chart one. The second draws chart zero but still derives its lifetime from
chart one. Both use display height 200.

The same delayed update plays samples 29 and 23 and creates a third, invisible
actor with the source judgement expanded by 150 on all sides. That actor has
a one-update lifetime, an update-zero collision window, processes every
eligible target, carries the copied packet, and uses bank-zero contact sample
20. A local player at a strict distance below 3001 also starts camera shake
mode zero for eight updates at magnitude six. `0x00412720` alternates vertical
offsets zero and six before clearing the request at counter eight.

Type 5 at `0x0042cd70` captures its source on update three and creates
resource `10000051`. That captured position is retained even if the source
moves afterward. The maximum frame count of chart zero, direction eight in
that resource becomes the controller's clock; constructor delay ten is not
used by this handler.

At the frame count, resource `10000050` appears with display height 200. At
frame-count plus four, an invisible one-update packet actor expands the
original source judgement by 150, plays contact sample 20, and requests the
same nearby eight-by-six camera shake as type 4. Resource `10000052` follows
at frame-count plus 15 with display height 200 and additional status `0x80`.
All three visible actors use a lower-right-plus-one point judgement, chart
zero, direction eight, and their own complete CAF lifetime. Sample 22 plays
at offsets 6, 9, 12, 15, 18, and 21. The controller expires at frame-count
plus 22.

Type 10 at `0x0042e7e0` reads Table 206 row zero at `subtype - 1` and attempts
one wave every eight updates after the authored delay. Wave positions advance
from the fixed impact origin at radii `wave * 300 + 250`, with
`[-150,-150,150,150]` placement bounds. The first failed placement
permanently suppresses that and every later wave.

Each clear wave creates resource `10000060` with chart zero, direction eight,
and a complete animation lifetime. Its update-zero collision processes every
eligible target with the copied packet, while sample 22 and the nearby
eight-update, magnitude-six camera shake originate at the wave position. The
controller expires at `delay + Table206Value * 8`. Passive coverage proves
the complete unobstructed and blocked timelines; enemy 26 in Devil's Castle
2F supplies the shipped live render, damage, audio, camera, blocked-tail, and
cleanup case.

Type 11 at `0x0042d6e0` creates resource `10000012` at the source on update
zero. At the authored delay, Table 204 row zero at `subtype - 1` supplies two
through eight radial children. Their angles are
`stored_angle - index * (6.283184 / count)`. Nonzero owners re-resolve the
source and start each child 180 units along its angle; owner kind zero starts
every child at the stored explicit origin.

The children use resource `10000010`, homing mode one, turn value 20,
`[-80,-80,79,79]` bounds, a 90-update lifetime, chart zero, direction derived
from travel, static-contact expiry, target-contact expiry, the copied packet,
and bank-zero sample 20. Sample 19 plays once at the final child's spawn
position, then the controller expires. The shared runtime actor now preserves
current-position homing, the retail shortest-angle turn, passed-target
behavior, and permanent straight travel after a missing or dead target.
Tower of Ordeal scenario 15 supplies the shipped subtype-ten live case.

Type 12 at `0x0042db10` creates resource `11000027` at the source plus a
Table-204-sized resource-`10000080` warning fan on update zero. Column 29
supplies the divisor for retail's `count * 2.5132736 / divisor` spread, with
its extra half-spacing correction for even counts. Live owners project the
warning actors 150 units; fixed-origin owners leave them at the supplied
point. Their subtype is also their explicit lifetime.

At the authored delay, the controller re-resolves the source and creates the
same fan from resource `10000081`, projected 180 units for live owners. These
actors move straight rather than home, use 50-unit bounds, a 90-update
lifetime, static and first-target expiry, optional hit memory, and sample 20.
Each copied packet replaces words 34/35 with `21021` and its direction and
words 74/75 with `21022` and the same direction. Sample 94 plays at the last
projectile before immediate controller expiry. Dread Wisp 24 in `North of The
Remains of The Dead` (`03010003`) supplies the shipped subtype-ten live case.

Type 13 at `0x0042e240` reads Table 204 at `subtype - 1` and attempts four
radial shells, four updates apart, at radii 350, 550, 750, and 950 from the
fixed impact origin. Angles advance positively through retail's `6.283184`
full circle. Each of the table's at most eight rays has an independent
permanent placement-obstruction flag, so one blocked direction does not stop
the other directions.

Every clear point consumes one random chart and creates resources `10000030`,
`10000031`, and `10000032`. As in type 3, only the first layer processes every
overlapping target on update zero with the copied packet. Sample 21 plays once
per attempted shell at its final radial position, including when that final
ray is blocked. The controller expires at the authored delay plus 16.
Lightning Gargoyle 11 in `03140000` supplies the shipped subtype-20 live case.

Type 14 at `0x0042e5c0` has no source visual. At the authored delay it resolves
a nonzero owner, projects 180 units along the stored angle, and creates
resource `10000070`. Owner kind zero uses the explicit origin without that
projection. The actor moves at constructor value six, uses constructor value
seven as display height, and keeps 50-unit bounds, chart-zero directional
drawing, static and first-target expiry, optional target memory, the copied
packet, and contact sample 20. Sample 22 plays at launch and the controller
expires immediately. Stone Wisp 2 in `03140000` supplies the shipped
subtype-one live case.

Type 16 at `0x0042ea50` launches resource `10000110` at the authored delay.
It uses the same live-owner 180-unit projection and fixed-origin exception as
type 14, with 80-unit bounds, authored speed and display height, directional
chart zero, static and first-target expiry, optional target memory, the copied
packet, contact sample 20, and launch sample 19.

The controller stores the returned runtime actor identity and refreshes its
saved x/y position on every later update. After that actor disappears,
resource `10000111` is created at the last saved position. Its 240-unit
bounds process every eligible target with the copied packet only on update
five, while the visual lives for its complete chart-zero animation. Sample 22
plays at creation. A player no farther than 3000 units receives the familiar
eight-update, magnitude-six camera shake, then the controller expires.
Goliate's second effect variant in `04050002` supplies the shipped subtype-ten
live case.

Type 21 at `0x0042eeb0` reads one, three, or five radial rays from Table 207.
Update zero creates source resource `11000210`. At the authored delay it
launches resource `10000100` at evenly spaced subtractive angles, projecting
180 units from a live owner or retaining an owner-kind-zero explicit origin.
The rays use 80-unit bounds, authored speed and height, twenty-degree homing,
speed-scaled animation, a full chart-zero lifetime, static and first-target
expiry, the copied packet, contact sample 20, and one launch sample 19.

Every returned actor identity and last position is tracked separately. After
a ray disappears, four stages appear at four-update intervals with 240-unit
bounds. Resources `12000000`, `11000033`, `10000030`, and `10000060` rewrite
packet words 32/34 to `0/20000`, `1/21013`, `2/20005`, and `3/21000`.
Stages two and four process every overlapping target on update zero; stages
one and three are visual. Stage three adds resources `10000031` and
`10000032`. Every primary stage plays sample 19, while the final stage also
plays sample 22 and requests the nearby eight-update, magnitude-six camera
shake. The controller ends only after all rays finish. Arc Angel's third
attack in `99000036` supplies the shipped subtype-30 five-ray live case.

### Player Land Mine controller

Effect 1000 dispatches to `0x0042bd40`. `0x00441c00` and the HUD rectangle
`x=496..511, y=424..439` both require a nonzero player `+0x328` count and a
zero global placement lockout. They set that lockout to ten, spend one mine,
and enqueue target mask 20 at the hero's exact current position. The HUD path
also plays sample 58.

Controller update zero creates static OPTION resource 1000 with
`[-150,-150,150,150]` judgement and a 300-update actor lifetime. Counter 40
arms target collision. From then on every twentieth update plays positional
sample 54. Enemy or active scenario-object contact removes the mine actor; its
natural lifetime does the same.

The missing actor advances the controller into resource 1001 with
`[-600,-600,600,600]` judgement, sample 29, and an every-target physical
packet. Its damage is Table 23 row `placed level - 1`, column zero, plus player
runtime `+0x2c4`, clamped to at least one. Rebuilding that runtime value adds
equipped instance word 81; instance word 84 instead raises maximum mines at
`+0x2c0` from its base ten. Even counters 12 through 40 expand a radius by 50
and place one randomly angled 1002, 1003, and 1004 visual. Counter 12 also
creates the four 1005..1008 pieces with paired 1004 actors, initial vertical
velocity 1500, acceleration -100, and the retail bounce. The controller ends
at counter 80.

Runtime actors are a separate category. `0x00429dd0` creates identity
`50000000 + local ID`, while `0x0045e1a0` copies a 126-word descriptor into
the actor. `0x0045e1e0` owns homing, free, or owner-attached movement; static
environment collision; target masks; exact-target filtering; living and
scenario checks; optional one-hit bookkeeping for up to 500 identities;
physical-versus-magical hit chance; receiver dispatch; descriptor-driven
static/CAF drawing; and lifetime. Target collision is active only from
descriptor word 35 through word 36 inclusive. Bits one, two, and four select
player, owned-companion, and enemy families independently.

The category-40000000 action dispatcher at `0x0045f960` and its chart-five
branch at `0x0045fff0` are adjacent in the executable but belong to a different
actor class. They must not be used to interpret category-50000000 descriptor
word 17; that word controls expiry after an environment collision.

The category-40000000 owner is now live. `0x004501c0` creates character
`16000000 + player slot` at the player's position, takes the PARTNER resource
and draw strengths from Table 60, and builds the level profile by summing
Table `800 + companion type`. The portable actor carries all six shipped
types and all three PARTNER resources instead of borrowing the matching
PEOPLE actor.

The ordinary follow and combat halves of `0x004622b0` and `0x00462610` are
reconstructed. Judgement distance below
160 selects idle and refreshes a five-update linger; 160 through 599 walks at
parameter row one divided by five; 600 and above runs at row two divided by
five; and 4000 or above snaps to player position plus `(200,200)`. Those
states use charts zero, one, and two and share normal movement, collision,
interpolation, and display ordering. Scenario changes relocate the companion
with its owner. While the owner is within 1200, the actor searches for the
nearest living type-two actor in that range. Attack mode disengages beyond
1499 owner units, approaches the repeated target at run speed until its
159-unit action range, and faces it before requesting action one.

Action one at `0x0045fff0` uses chart five and derives its timing tier from
companion parameter row zero divided by 32. The ten factors are 0.2 through
1.1. Newly crossed part-zero markers preserve sample 95 at bit `0x400` and
the impact at bit `0x40`; the impact repeats the retail exact-facing search
inside 150 units. It checks row-six hit rate against enemy physical evasion,
creates MISS on failure, or sends the family-one companion packet with row
five attack, owner companion level, native element, random effect
21000..21003, and sample 44. A shipped live regression covers acquisition,
approach, both markers, the enemy receiver, and actual damage outside Remote
Town.

Owned-companion activity follows the player runtime flag at `+0x15a0`.
`0x00440f70` initializes it to one (`INACTIVE`). Both the Space command in
`0x004429b0` and the x `0..111`, y `393..408` strip in `0x00445bd0` toggle it
with `1 - current`; the pointer strip consumes its click before world movement.
Switching inactive clears pending combat intent without interrupting a locked
attack presentation. Inactive companions still run the normal follow bands
and remain visible and solid, but `0x004622b0` skips autonomous acquisition
and enemy/effect target selection rejects owner mode one.

The matching HUD path in `0x004039f0` draws `Bar.njp` pattern 30, clips
pattern 29 from the right to `life * 109 / maximum`, and uses patterns 31 and
32 for `ACTIVE` and `INACTIVE`. A sub-30-percent fill pulses at RGB strength
1500 for two of every four updates. The mode survives scenario transitions
inside a play session but is deliberately absent from the save stream, as in
retail.

The rest of the owned-companion combat lifecycle is live too. `0x004616d0`
scales PARTNER chart three across the receiver duration and applies its
collision-aware diminishing 120-unit hit impulse. `0x00461990` locks chart
four direction eight, creates effect 21010, holds the final frame, fades over
60 updates, and writes the saved 900-update respawn countdown. A category-four
definition `98000002` anywhere in the backpack shortens that countdown to 600
without being consumed. At zero, life is restored to the table maximum at the
player position and `0x004610b0` plays chart seven direction eight before
returning to ordinary owner AI.

`0x004134a0` awards one companion point for a local-slot owner or companion
kill while the companion is alive below its cap. The player threshold is
processed between that award and `0x00412e20`, so companion leveling sees the
new player level from the same kill. Table `800 + type` row 18 supplies each
threshold, the cap is `player level / 3 + 2` up to 35, and a level rebuilds
the summed profile and restores full life. The 0x160-byte player record keeps
the companion type, level, experience, and defeated countdown.

The record's level and experience are the active row, not the whole catalog.
`0x00440f70` allocates level and experience arrays at `+0x1590` and `+0x1594`
with the Table 60 row count at `+0x158c`; every level starts at one and every
experience starts at zero. Opcode 45 at `0x004336a9` evaluates one companion
type and calls `0x00450500`. That function stores the current row, restores the
new row, clears `+0x15c`, replaces the owned companion actor at the player, and
sets its life to maximum. All six shipped types are selected by six calls in
three scenarios.

Opcode 3 at `0x0043167d` supplies the companion `Check Status` branch. It reads
one constant companion type, uses that type's saved array level for
`0x004136f0`, and formats the result into the normal actor speech buffer. The
printed level and experience still come from the active record fields at
`+0x154` and `+0x158`. Its cap is player level divided by three plus two,
clamped to 35, with separate limit and maximum strings. The original display
also reads profile `+0x54` under `M Defense` and `+0x44` under `M Evasion
Rate`; these are magical hit rate and physical defense, not the fields the
labels imply. All six shipped calls across three scenarios are covered by a
catalog audit and live Remote Town interaction.

The save path stores the current row before writing count, all levels, and all
experiences, followed by player Land Mines at `+0x328`. The portable player and
save owners now restore and rewrite this exact sequence, including progression
for companions which are not currently selected.

The portable `EnemyEffectController` now covers the complete controller half
of types 1 through 5, types 10 through 14, type 16, and type 21. Focused tests
cover zero, positive, and negative delays,
source re-resolution, missing and fixed owners, exact resources and bounds,
packet copying, projection, positional samples, Table 205 wave timing, the
random chart, and persistent obstruction. Its actor outputs are
paired with a passive `RuntimeEffectActor` that now covers source-animation
lifetime, free forward movement, static collision and special-ground
filtering, contact expiry, chart-zero frame timing, and the inclusive target
window.

The common target path is portable too. It queries overlap at the current
position before movement, filters the five target bits and exact identities,
keeps living/current/local/active/display state, and preserves strict
first-nearest or query-order multi-target processing. The optional 500-entry
identity list records player, companion, and enemy contacts before the hit
roll, so misses cannot be retried. Packet word 1 selects physical or magical
evasion, word 36 supplies hit rating, and the Visual C++ random stream feeds
the `20..98` check. Typed receiver/miss requests and the two configured audio
pairs preserve the retail one-sound guard and NPC multi-target mode.

The world owns and renders these controllers and actors, builds their live
target snapshots, and applies their receiver requests. Shipped regressions
cover type 2 in `03000507` and the type-3 Plasma Bat in `00010001`, including
resources, audio, damage, cleanup, and unchanged item ownership. Scenario
`01000004` covers type 4's warning, paired burst charts, invisible damage,
two launch sounds, contact sample 20, camera shake, and item ownership. The
type-5 sequence is covered by enemy 48 in `04060004`, including its
resource-driven timing, three visuals, six pulses, area damage, camera shake,
cleanup, and item ownership. Arc Angel in `99000036` covers type 21's source,
five tracked rays, four stages, two packet windows, layered render, audio,
camera shake, cleanup, and item ownership. Mapping
`type + 10000` directly to one OPTION resource would still lose retail timing,
targeting, audio, and often an entire intermediate actor.

## Current portable slices

The first game-core slice covers:

- the constructor defaults used when `SFlare.Cfg` is absent
- the exact 16-value, 64-byte config load/save layout
- retail validation, clamping, and partial updates on failed reads
- Shift-JIS-aware `/w` and `/f` command-line handling
- the title, character-selection, and gameplay transition dispatcher
- the title enter/leave lifecycle and its exact resource manifest
- the title fade, menu navigation, hover regions, delayed actions, audio cues,
  smoke-animation scheduling, CAF decoding, and steam drawing
- portable V001/V003 VOC decoding and LAL playback for title and menu audio
- new-character and saved-game selection enter/leave behavior
- the character-selection fade and top-level per-frame screen dispatcher
- saved-game list navigation, slot hit regions, double-click timing, and the
  Continue, Delete, Back, and Exit decisions
- new-character gender selection and portable 15-byte name entry
- Online/Single and New/Join mode menus plus portable host-address entry
- saved-game summary parsing, BMP previews, and Delete confirmation
- complete saved-game row text from the player record: Level, Job, Sex, Name,
  HP, MP, and EXP as the original separate same-origin label and padded-value
  packets, including gold labels, pale-white values, and half-intensity idle
  rows
- software drawing with the original Select and bitmap-font pattern sheets
- the six-slot save-name search used by both menu states
- both retail menu input-binding tables
- the statically linked Visual C++ random-number generator
- the shared 77-word combat packet damage formula, including its table,
  element, defense, minimum-damage, source-lookup, and random-order rules
- the passive enemy damage receiver, including local and network ownership,
  reaction tables, life attribution, reflection, packet effects, audio and
  status requests, kill metadata, and death presentation selection
- the live enemy hit and death actions, including chart-two reaction timing,
  collision-aware displacement, chart-three direction selection, marker and
  death audio, fade timing, and removal from live presentation while retaining
  the inactive scenario slot
- lethal kill accounting, proportional experience, novice level growth,
  weapon/effect counters, authored Table 30/31 item rolls, Gold Find, and
  complete ground-item instance ownership
- common `Character/OPTION` combat effects 21000 through 21014, including
  owner-position snapshots, display ordering, one-pass CAF lifetime, and the
  fixed death-effect timing
- the player combat defense profile, including the two-dimensional elemental
  anchors, equipment and rolled-item contributions, identified backpack
  passives, two-handed off-hand suppression, and final affinity clamps
- the passive player damage receiver, including local life and mana ownership,
  three shield paths, revival, equipment durability, reflection, hit reaction,
  configured effects, audio requests, training, and death presentation
- the live owned-companion damage lifecycle, including its family-one
  receiver, player-slot ownership, excluded actions, reaction tables and
  stages, effect-owner distinctions, sample 119, chart-three hit reaction,
  chart-four death fade, saved countdown, chart-seven revival, and progression
- gameplay entry and its retail loading-screen sub-state
- portable RCLIB-L decoding shared by NJP and ground-map data
- the initial `00000000` scenario's fixed MCT header and entry-point table
- its leading ID lists, common variable entity record, and seven-record
  `PEOPLE` block
- its data-selected ground and object maps, NJP/SDW pattern list, new-player
  spawn, facing direction, title, music, and player CAF/NJP/SDW drawing
- Ostare's retail people resource, placement, part mask, shadow, idle pause,
  and bounded walk
- the complete Remote Town SCS container and the first status-triggered
  conversation, including asynchronous message-close callbacks, native actor
  hooks, four script-created ground items, and the two-message player-level
  branch used on a repeat interaction
- Ostare's pointer hover, pale selection tint, nameplate, and actor-anchored
  `Hukidasi.njp` speech bubble
- judgement-rectangle interaction range and retail-style auto-approach before
  an NPC status starts
- one shared executable-owned movement controller for the player and PEOPLE
  actors, including persistent cardinal obstacle-edge steering and the retail
  ownership later enemies will also use
- smooth 60 Hz camera and actor presentation interpolated from unchanged
  30 Hz gameplay updates
- companion choice messages with hidden `~` range markup, pointer hit testing,
  persistent red hover selection, gray inactive choices, script result
  write-back, initial-minus-one informational follow-ups, and status-one
  callbacks, verified through Kerberos and Harley's live Remote Town
  interactions
- opaque-pixel world selection for PEOPLE actors and ground items, including
  configured click priority, the retail range square, item nameplates and
  pale tint, shared auto-approach, and the first inventory-owned pickup

These pieces live in `OpenShadowFlare::GameCore` and have no dependency on
LWL, LGL, LAL, Win32, or another platform API. The executable runtime loads
the config before creating its LWL window, just as the retail entry point does.

The first gameplay HUD layer now follows `0x004039f0`. `Bar.njp` supplies the
fixed lower interface, walk/run marker, level digits, life and mana fills, and
the Table 13-driven 109-pixel experience fill and frame. The owned companion
uses the original bottom-left frame, reverse life fill, low-life pulse, and
active/inactive marker. GAPI has a general
destination clip for the live fills, so the original artwork is revealed
rather than stretched. The HUD
is a screen-space renderer outside the world camera and owns the lower input
band. Retail registers the standard Windows arrow once and never calls
`SetCursor`; LWL's native platform arrow is therefore the portable equivalent.
The Menu, Status, and Item labels now use `0x00445bd0`'s exact inclusive
rectangles and open their real owners. Interface clicks are consumed before
world commands, including every held update through release after dropping an
inventory item onto the map.

World feedback now follows `0x004165d0`, `0x0040ee70`, and `0x00416bb0`.
Portable `RKC_RPGSCRN` tests the configured inclusive square against the
opaque pixels of each current visible CAF/NJP cell. `WorldPointer` prefers an
exact tip hit, otherwise the nearest candidate in a priority group, while
retaining retail display depth and the five priorities from `SFlare.Cfg`.
Disabling the range reduces selection to the exact cursor tip. The default
priority selects an item over a person. The range renderer uses the same exact
half-sizes 0, 12, 16, 24, and 48, with strength 100 over empty ground and 300
over a target. PEOPLE targets are white and item targets are yellow.

Ground items use their `Item.Ibn` name, while money shows its quantity followed
by `Gold`. Their visible part receives the same +300 pale tint as a selected
person. Type-three interaction in `0x00449240` shares the 159-unit rectangle
range and movement-controller approach. Once close, the first single-player
`0x004526a0` path transfers the concrete item to `PlayerInventory` and erases
the stable ground entity only after acceptance. Gold stacking to 10,000 is
covered. Definition offsets `0x1c`, `0x20`, and `0x24` now provide the
inventory width, height, and item weight. Backpack insertion scans the retail
9-by-4 grid in row-major order, retains each multi-cell footprint, and leaves
ownership unchanged when no complete placement is available.

The first inventory screen follows `0x00404760`, `0x00407170`, and
`0x00408a80`. `I` and the lower ITEM button toggle the live right-hand panel,
move the camera anchor to x=160, and restrict world picking to x=0..319.
`Status.njp` patterns 2, 3, 0/1, and 74/75 supply the original frame, gender
silhouette, and Close tab. Gold is summed from the owned stacks; equipped
weight comes from the equipment owner. Backpack
items draw from the separate `Item0000.njp` through `Item0013.njp` groups at
`(336 + grid_x*32, 264 + grid_y*32)`. The inventory and lower HUD regions are
cleared before their transparent authored layers are composed, matching
retail's reserved UI surfaces instead of exposing world pixels through slots.
Moving and dropping use the retail held-item path. All nine equipment
boxes now follow the `0x00447290` regions: helmet `560..623,16..79`, body
`560..623,88..183`, boots `560..623,192..255`, main hand
`480..543,16..143`, off hand `480..543,160..255`, and the four accessory
cells at `(400,143)`, `(400,183)`, `(440,143)`, and `(440,183)`. Category zero
belongs in the main hand. Category one's first serialized field classifies
helmet, body, off hand, and boots as subtypes zero through three. Accessories
accept category two with a one-cell width and use serialized offset 100 for
their required level. Every box applies that check and performs the same
pointer swap. Equipped weight and the ten base contribution fields are summed
over all nine objects.

The HUD pockets follow `0x00445bd0` and the item tail of `0x00407170`. They
form a separate 4-by-2 category-three owner, with row-zero origin `(357,413)`
and row-one origin `(405,445)`. Pointer hit testing uses the exact staggered
rectangles, complete item footprints are retained, and placing over one item
swaps it onto the shared pointer. `0x0044a5f0` maps keys `1` through `8` to
the four row-zero cells followed by the four row-one cells. `0x0044a240`
applies the decoded flat and maximum-percent player life/mana fields and
removes the item only when a value changes. The same path handles a secondary
click on medicine in either the backpack or belt, so a Tablet at full life or
a Capsule at full mana stays in its owner and produces no use sound. Its
player restore amounts are multiplied by 100 plus the equipped definition
bonuses at runtime definition offsets `+0x108` for life and `+0x114` for mana;
both the flat amount and maximum-pool percentage use that multiplier.

Only when neither player pool changes does `0x0044a240` resolve the owned
companion as character `16000000 + local player slot`. A companion whose
current life is positive and below maximum receives medicine definition
offset `+0x14` plus offset `+0x18` percent of maximum, clamped to maximum. This
is the Meat family in `Item.Ibn`. A full, absent, or defeated companion does
not consume the item. Any successful player or companion restoration jumps
to the common sample-16/consume result before the later condition branch.
The remaining condition branch is persistent, not timed. Definition effect
`-2` clears player offsets `+0x74/+0x78`. Effects zero through seven call
`FUN_0044fd10` at `0x0044fd10` with the definition value (4,000 for the shipped
medicines). Its anchors are `(0,20000)`, `(0,-20000)`, `(-20000,0)`,
`(20000,0)`, `(14140,-14140)`, `(-14140,14140)`, `(-14140,-14140)`, and
`(14140,14140)`. It snaps when the truncated Euclidean distance is no greater
than the step; otherwise it adds the truncated cosine/sine projection. The
runtime offsets correspond to player-record offsets `0x64/0x68`, so normal
retail saves preserve the result. An unchanged point does not consume the
medicine.

New-character equipment and owned items now follow `0x00440f70` as well.
Category one definition zero is equipped in the body slot. Category-three
definition zero fills backpack column zero and belt row zero four times;
definition `10000000` does the same for backpack column one and belt row one.
The separate mine counter starts at five. It is restored from the field after
the magic block and its counted two-array history owner; older sparse portable
saves retain the five-mine default unless their versioned tail includes a
count.

Mine pickup keeps that count completely outside the ordinary item owners.
The category-four, definition-one branch of `0x00449ef0` frees the concrete
item and increments player `+0x328` only while it is below runtime maximum
`+0x2c0`. When full, it returns the still-live item to `0x004526a0`; the
single-player failure tail restarts its mode-zero drop animation rather than
placing it in the 9-by-4 backpack. `0x00408a80` draws `Status.njp` pattern 67
for a nonzero count and prints `current / maximum` at right edges 446 and 471,
with the maximum colored against base `+0x160` (ten).

Mine's category record contains generic weight value one, but the live weight
owner `0x00445630` only sums the nine equipment pointers at `+0x4e8..+0x50c`.
No use of player `+0x328` feeds that calculation, its inventory display, or
the attack-speed tier.

Inventory movement sounds follow `0x00466110`: category-two items use sample
93, Gold uses 85, ordinary items below weight 60 use 48, and heavier ones use
47. Equipment placement uses sample 49 except for category two, while a
successful belt effect uses sample 16. Sounds are emitted only after the
corresponding ownership change succeeds. The success tail of
`0x00449ef0` also calls selector zero after a picked-up world item has entered
the player owner, so ground pickup uses the same category-and-weight sample
instead of being silent.

If that owner rejects the item because no backpack footprint is available,
the single-player tail of `0x004526a0` recreates the same concrete instance as
a mode-zero world drop. The portable ground actor keeps its position and item
state while resetting height, velocity, gravity, and bounce state. It then
plays selector two at first impact instead of pretending the click was lost.

Ground drops use selector two from the same routine. Their first contact with
the ground plays sample 15 for an ordinary item, 85 for Gold, or 93 for a
category-two item. The smaller second bounce is silent. This lives in the
shared ground-item update path, so script-created drops and player-dropped
items behave alike.

The `X` panel follows `0x00404760`, `0x00409000`, and `0x00447970`. It owns a
separate 9-by-10 grid whose first item cell is `(16,72)`, composes Status
patterns 14 and 15 over a reserved left-hand surface, moves the camera anchor
to x=480, and keeps the right-hand world viewport live. It shares pointer
ownership, centered placement, single-item displacement, Gold stacking, and
the three-update information delay with the ordinary inventory. Opening
Inventory or Special Item closes the other. Opcode 41 argument zero reaches
this same owner from the Warehouse; it does not create another container.

Opcode 41's nonzero branch is no longer unnamed. A scan of every shipped SCS
found its only use in scenario `99000013`, sentence 10. The matching Tower of
Ordeal 12F MCT calls object `10000900` `Giant Warehouse`. At `0x004335ac`,
argument zero toggles `0x0048ce48`; a nonzero argument toggles `0x0048ce4c`,
and each branch clears the other flag.

The Giant Warehouse has ten 9-by-10 containers at player `+0x520` through
`+0x544`, a selected page at `+0x558`, and ten page-unlock values from `+0x55c`
through `+0x580`. New-character initialization enables page zero only. The
selected page is transient. `0x00404760` draws pattern 73 and the page strip:
74 means disabled, 75..84 mean enabled, and 85..94 mean selected. Tabs begin
at `(24,41)` and advance by 24 pixels. `0x00447ca0` owns the tab and item
input, the close cell at x272..295/y40..55, and sample 58. All ten unlock
values and item containers follow the Land Mine field in retail saves and are
now restored and rewritten; sparse portable saves use the backward-compatible
version-four tail.
The owner now restores and rewrites its exact save-payload container.

The four containers at player `+0x548..+0x554` are a separate automatic item
owner. Category-four `Item.Ibn` records supply page `-1..2` and a fixed cell in
their last three words; the executable reserves four pages even though the
shipped data does not currently select page three. `0x00466480` returns the
page and `0x00466490` returns the cell. Ground pickup sends a non-negative-page
item there instead of to backpack `+0x514`, and an existing matching item
causes the ordinary failed-pickup drop response.

Opcode 58 at `0x00433b33` searches all four automatic pages, the backpack,
active main hand, body, active off hand, head, legs, and four accessories, in
that order. Opcode 59 at `0x00433ced` removes the first match in the same
order, refreshing equipment-derived state when needed. Opcode 75 at
`0x0043443c` creates the requested definition, uses its authored page and
cell, and inserts it only when absent. The belt, alternate arms, one-page
Warehouse, and Giant Warehouse are intentionally excluded. The save writer
places all four automatic pages immediately after the ten Giant Warehouse
containers; both retail fixtures and the backward-compatible portable tail
now round-trip them byte-for-byte.

The following script reward is mapped too. Opcode 68 at `0x004342de` reads
Table 13 for the local player's current level, calculates the evaluated
percentage with a signed 64-bit multiply and divide, and adds it to player
experience. A threshold crossing calls the ordinary `0x00412fb0` growth path,
clears overflow, restores resources, and produces the same level-up notice and
samples as combat. Shipped scenario `04900001`, sentence 30 uses 50 percent
immediately after granting the Spirit Stone through opcode 75.

The inventory-panel transfer button at classifier case 10 remains pending.

Category-one records place the requirement at serialized offset 148, CAF part
at 152, and default RGB strengths at 156, 160, and 164. The Short Sword
therefore enables part 12, the Round Shield enables part 9 with strengths
900/800/500, and body armor follows the same decoded path. Retail
`0x00444ca0` only adds the body, main-hand, and off-hand objects to the CAF
mask; helmet and boots remain stat-bearing slots without their own enabled
layer. A weapon's optional second part and the retail off-hand suppression
rule are decoded too.

The shared item information path at `0x00409160` and `0x00409a60` is now used
by backpack, equipment, and special-item hovers. The pointer must remain on
the item for three
updates. Font01 uses its native 6-by-12 cells with no extra letter spacing;
the widest elemental row controls the centered x position. Retail flag `0x20`
adds four pixels of padding around the text, fills that rectangle with the
600/1000 black fade, and the four `0x204` packets draw its 500/1000 white
one-pixel frame. The padded rectangle starts eight pixels below the pointer,
and both axes clamp to the 640-by-480 frame. Ordinary items list only non-zero
values from the ten-field contribution vector, then
durability, weight, required level, the condition-adjusted quarter-price, and
the two four-element rows. Serialized offset 20 supplies the base price,
offset 100 supplies maximum durability, and category-zero/category-one
element arrays begin at offsets 208 and 168. Quality zero through three uses
the retail gray, muted red, pale blue, and blue text colors. Category-four
definition zero takes the executable's shorter branch: Gold shows its stack
amount in the exact `Price                     :%9d` row, producing the wide
three-line panel instead of a name-only box. The other formatter branches are
preserved too: category two shows weight, required level, and sale price;
category three consumables such as Tablet show sale price; and non-Gold
category-four items show sale price. One-cell items therefore keep the
authored-width information panel rather than collapsing to a name-only box.

The condition corner in `0x00465cb0` is reconstructed for backpack, equipped,
and pointer-held gear. Categories zero and one compare current and maximum
durability using the executable's integer percentage. Values below ten
percent draw `Status.njp` pattern 16 at
`(x + width*32 - 16, y + height*32 - 16)`. A nonzero value blinks for eight
game updates and hides for eight; zero durability stays visible. The adjacent
pattern-17 state has not yet been given a trustworthy gameplay owner.
Special-item script operations, consumable effects, dynamic dyed colors, and
network replication remain.

The in-game settings panel now follows `0x004103c0`. Escape opens the original
two-layer `Status.njp` panel and suspends world input. Boolean options use the
retail ON/OFF cells, click range uses its five 24-pixel cells, clicking a
priority class moves it to the right-hand end exactly as the executable does,
and both volume sliders use the original mute sentinel and `-3000` through
zero scale. Pointer, shadow, occluding-object, effect-volume, and BGM-volume
changes apply live; all changed configuration fields are written back through
the reconstructed 64-byte `SFlare.Cfg` writer when the game exits. The
screen-mode row is hidden and the LWL window stays windowed, but y=86 remains
empty so every later row keeps its retail coordinate. Mission and Map now
open their own screens from the original rows: Mission is modal, while Map
leaves the right-hand world viewport live.

The two confirmed save actions defer their final transition for the retail
saving frame, then process it before any independently open Map, Warehouse,
Special Item, or Inventory panel. Those panels can no longer consume every
following UI update and strand the saving confirmation on screen.

The Help row and `H` shortcut now open the screen drawn by `0x0040e710`.
Status patterns 10 and 66 provide the authored 640-by-415 frame and the
230-by-128 action preview. Font01 text keeps the retail coordinates, colors,
shadows, row spacing, and original wording. The player uses CAF chart 7 at the
preview anchor. Help entered through Settings also runs the shared
`0x004088b0` `CLOSE` animation with Status patterns 27 through 30; Escape or
any click above the HUD dismisses it. The conditional companion branch draws
the real owned PARTNER actor at `(212,158)` with chart seven and the shared
preview animation counter.

The Mission List follows `0x0040cea0`. Status pattern 10 supplies the authored
640-by-415 frame, patterns 110 through 113 select either 24-entry page, and
patterns 25 and 26 provide the unfinished and cleared locks. Titles come from
`Table.Tbd` table 41; mission `n` gets its description lines from table
`700+n`. Script state zero hides a row, one draws the bright unfinished row,
and two draws the gray cleared row. The list keeps the retail 12 rows per
column, 27-pixel row cells, page-tab hit boxes, `Q` shortcut, and Settings
entry. Page-tab selection uses retail sample 58. Clicking a row opens the same
tinted patterns 59 and 58 detail panel with the bracketed title and 16-pixel
description lines. A click on the detail returns to the list, while Escape,
`Q`, or a click outside the list but above the HUD closes it.

The live half-width Map follows `0x0040d4d0`. It loads the current scenario's
`Scenario.Njp`, clips it to x=32..318 and y=40..374, and covers unexplored
territory with the same map-sized mask used by retail. Movement reveals the
retail 68-by-46 rectangle around each visited position. The player stays at
the original 160,210 map anchor while arrow keys scroll by 16 horizontal or
10 vertical pixels, Enter recenters, and the marker from `MapIcon.njp` uses
the 15-of-20 blink. Status patterns 71 and 118 provide the authored frame and
pulse. Its `0x11` UI mode leaves simulation and world input running in the
right half, moves the camera anchor from x=320 to x=480, and prevents Map-panel
clicks from leaking into the world. `N`, the Settings row, Escape, and a
secondary click above the HUD open or close it through the matching retail
paths. Loading and writing the per-save `Save/M%08d%02d.msk` history,
other online-player markers, and later scenario transitions remain with their
future owners.

The two save rows now use the same secondary states as `0x004103c0`. Their
prompts replace the settings text without replacing or re-fading the panel;
YES and NO retain their retail coordinates, hitboxes, hover color, and samples
56 and 55. YES calls the portable `0x0044b580` path before either returning to
the title or ending the process. A successful write shows the retail
`Now saving the data ` stage for one update; a failed write leaves the
confirmation open.

The writer reproduces the `ShadowFlare0005` envelope: plain 0x160-byte menu
record, payload size, one-byte Visual C++ `rand()` XOR key, signed-byte
checksum, and the inverse of the executable's 256-byte substitution table.
When rewriting an original save, it validates and decodes the payload, updates
its repeated player record, then parses the exact item prefix used by
`0x0044b580`: eleven optional equipment records followed by backpack, belt,
and special-item containers. The nine player equipment slots, 9-by-4
backpack, 4-by-2 belt, and 9-by-10 Special Item container now save and load
their category, definition, grid position, Gold quantity, durability, identified state,
and category-sized instance state. The two still-unnamed equipment records
and all trailing payload bytes remain byte-for-byte unchanged. The loader also
skips the first counted flag array after the items and restores the following
51 transport flags against Table 40. Tests cover a new world save/load round
trip and unchanged re-encoding of an original retail save. Scenario, position,
and the remaining dynamic payload still need owners. The mine count after the
magic block's counted two-array history section is now restored and rewritten
independently. Writes go
through a sibling temporary file
and protected replacement so a corrupt source or failed write does not
silently destroy the slot.

The next reconstructed save boundary is the complete magic block.
`0x0044b580` writes a fixed count of 22, the availability array at player
`+0x1440`, level array at `+0x1498`, experience array at `+0x14f0`, and eight
magic-bar IDs from `0x0048d508`. `0x00440f70` seeds the arrays with `0`, `1`,
and `0`, while all bar slots begin at `-1`. The portable `PlayerMagic` owner
and `restoreRetailMagic`/`replaceRetailMagic` preserve this block independently
of actor, panel, and effect state.

The matching script ownership is confirmed. Opcode 67 at `0x004340e7` writes
availability value three to the evaluated spell index. Opcode 69 at
`0x0043412b` tests that stored value for exact equality with three and writes a
boolean operand. Scenario `04100000` contains both the query branch and later
reward. `PlayerMagic` now performs those operations on its persistent array,
so the debug-only All Spells view cannot leak into scenario progression and a
normal save keeps the granted spell.

With `Save Image at Game End` enabled, the world-only software surface is also
captured before the HUD, conversation overlay, or Escape panel is drawn.
Portable `RKC_DIB` writes the retail 391-by-114, 24-bit `Save\%04d.Bmp` partner
from the player-centered crop. The saved-game screen reads that file through
the same portable BMP boundary and displays it at the original coordinates.

Portable behavior originating in the DLLs is kept under `SF_EXE/libs`, with a
directory for each of the fourteen original boundaries. `RK_FUNCTION`,
`RKC_DBFCONTROL`, `RKC_DIB`, `RKC_DSOUND`, `RKC_UPDIB`, `RKC_RPGSCRN`, and
`RKC_RPG_SCRIPT` currently build as separate static libraries. Each has one
public API header and small implementation files; future executable slices
should port proven behavior from the corresponding Win32 reconstruction into
the matching library instead of adding it directly to `GameCore`.

The menu lifecycle code emits resource, input, cursor, and audio work through
portable callbacks. Those callbacks deliberately describe what the game needs,
not how a particular operating system provides it.

The title screen's game decisions from `0x00420e60` are now reconstructed.
Its original background, menu entries, selection brightness, and fades are
drawn by the portable runtime. All ten smoke animations use their original CAF
frames, random delays, pipe positions, and scene brightness. Version/copyright
text and network messages are the remaining title presentation work. The
runtime also loads the retail common-effects and title-music VOC containers,
plays voice 62 for the title cue, voice 58 for navigation, voice 56 for title
confirmation, and starts the looping BGM after the original 60-frame delay.

The top-level character-selection dispatcher at `0x00421c50` is reconstructed
through its fade, mode dispatch, sub-screen dispatch, and delayed gameplay
transition. New-character creation, the saved-game list, Delete confirmation,
and all shared mode/host screens are implemented and drawn with the original
assets. LWL supplies portable UTF-8 text and clipboard input; the game state
keeps the retail 15-byte field limit and does not depend on a native edit
control. Selecting a new character now follows the executable's exact
20-frame, 97-pixel center slide while the opposite portrait fades. The reverse
animation, 130-by-20 name field, and 6-by-12 block caret mirror the retail draw
packets as well. Shared mode dialogs are drawn as full-bright overlays after
their dimmed backdrop, and returning from the saved-game dialog restores both
the list brightness and its interaction state. Save-slot numerals now remain
static unless pointed at; only the hovered numeral follows the retail
3-2-1-0-0-1-2-3 pulse.

Character selection keeps the title BGM playing when it is already active,
starts it on entry when necessary, uses common voice 55 for its selection
actions, and releases the music when the menu flow ends. All playback uses the
retail effect and BGM volume values through LAL.

Gameplay entry at `0x0041d3f0` now prepares the initial single-player world.
The initial loading presentation follows the cached `Waiting.njp` path in
`0x00402920`: pattern 0 is the Episode 1 background, pattern 3 is the
bottom-right loading label, and pattern 2 becomes the horizontally moving
confirmation arrow after world setup completes. Return or a click inside the
retail arrow rectangle enters the world. The 120-frame `VisualNN.njp` fade in
`0x00417bd0` is a separate loading presenter used later in gameplay and is not
part of this initial transition.

`ScenarioData` follows the fixed part of `0x00427b50`: it validates
`MCED DATA v0000\x1a`, reads the two 260-byte paths, music index, and 256-byte
area title. At `0x324`, the first three counted lists preload object, PEOPLE,
and enemy resources. The object, PEOPLE, enemy, and item groups then use a
shared variable entity record before tails of `0x34`, `0x2c`, `0x13c`, and
`0x10` bytes. The portable decoder walks that sequence exactly, reads the
entry table and three-word footer in forward file order, and rejects truncated
groups or trailing bytes. All 209 shipped MCT files pass this path, covering
5,203 objects, 163 PEOPLE records, 18,788 enemies, and 84 placed items with no
mismatch between a nonnegative object, PEOPLE, or enemy resource and its
preload list. Enemy tails retain their fixed AI-controller name separately
from the surrounding parameter blocks.

The first enemy runtime follows the common half of that loader and
`0x00458f40`. Enemy character numbers use `14000000 + local ID`; the actor
keeps its MCT state, position, judgement, direction, name, optional part
table, resolved `Character\ENEMY` animation, and AI-control name. The
Wasteland of Pillars fixture creates all 66 records transactionally. The
constructor-selected action seven reaches `0x0045b600`, which draws CAF chart
zero and advances one frame per active-map update. Enemies join the ordinary
shadow/visible depth passes and their enabled judgement rectangles block the
player. The catalog also contains 34 invisible, non-colliding `Enemy Hole`
actors with resource `-1`; these remain live script/AI identities without a
fabricated visual. Live AI selection, movement actions, hit/death
presentation, experience, and drops remain; the later player-impact boundary
now commits receiver life and reaction state.

The loader rearrangement now has three more named consumers. Pre-AI MCT values
1 through 4 become the spawn-relative patrol rectangle read by `0x0045c3c0`.
Pre-AI value 8 is copied twice, initializing both current life at runtime
`+0xd4` and maximum life at `+0xe4`. Post-AI value 54 becomes the
thousandths-based movement speed scale at `+0x1dc`. `EnemyActor` owns those
typed values while preserving both complete raw blocks for later work.

The global AI catalog now has its portable owner too. `0x004127d0` loads
`Control.aid` before scenario actors are created; the preserved version-one
file contains 64 exact-name lists, 18 event buckets per list, and 1,338 action
candidates. `AiControlDatabase` retains the nine parameter and six condition
integers without speculative field names. All 18,788 MCT enemy records resolve
their controller name, and each live enemy stores the resulting stable list
index.

The evaluator at `0x0045c9f0` is now reproduced as an executable-owned,
deterministic unit. Condition values zero through two gate inclusive current
life percentage, values three through five request a target inside inclusive
distance limits, parameter zero supplies priority, and parameter two supplies
weight. Its temporary linked list reverses file order and retains a later
lower-priority candidate until another new maximum clears the list; the
portable path preserves that retail quirk. Events 1 through 10, 16, and 17
fall back to event zero when nothing is selected. Enemy life fields, target
lookup, and the native action dispatcher are now reconstructed behind this
boundary. Live attachment still waits for selected-action storage and
complete movement/presentation consumers so it cannot expose a partial
behavior path.

Target lookup itself is pinned at both native entry points. `0x00459500`
checks the four player slots first, accepting active state exactly one and
choosing the first nearest same-scenario actor inside inclusive bounds. Only
when no player qualifies does it inspect companion character numbers
`16000000` through `16000003`, requiring the script-active bit, owner mode
zero, and optional positive life. `0x004593f0` is the default path: it accepts
any nonzero player active state and its type-five companion fallback does not
repeat the script-active test. Both measure judgement-rectangle distance,
prefer every player over every companion, and preserve first-entry ties.

The MCT-to-runtime rearrangement for all six attack presentations is pinned
too. Direct variants take target limits from post-AI values 3 through 5,
chart offsets from 41 through 43, and speed indices from 47 through 49.
Effect variants take type/subtype/parameter/additive triples from values
9, 15, 12, and 18 respectively, chart offsets from 44 through 46, and speed
indices from 50 through 52. Retail adds chart bases four and seven. Every
visual enemy in all 209 shipped scenarios has a speed index inside the
ten-value table and a resulting chart present in its CAF.

Presentation dispatch at `0x00459290` sends actions one through three to
`0x0045a2f0` and four through six to `0x0045ac90`. Entry performs the matching
ranged or default target lookup and faces it. Continuing updates calculate
the frame from elapsed updates times `{0.3, 0.4, 0.6, 0.8, 1.0, 1.5, 2.0,
2.5, 3.0, 4.0}`, truncating toward zero. Every newly crossed part-zero CAF
cell is scanned for impact bit `0x40` and sound slots `0x400`, `0x800`, and
`0x1000`; a jump beyond the chart end deliberately skips those frames before
clamping. The last frame restores idle action seven and emits events two
through seven only if the current event is minus one. The portable controller
now returns the exact frame, facing, target, marker mask, typed effect fields,
resolved samples, and completion event. Native damage, effect creation, audio
playback, and live update attachment are still kept outside this timing owner.

`0x0045a2a0` indexes a 25-resource by three-marker by ten-chart override table
at `0x00480a20`, then falls back to the three ten-chart rows at `0x004809a8`.
The override table has 59 populated cells. Only chart three has a fallback,
sample 86 in all three rows. The complete 750-cell override table is pinned by
count, sum, and byte hash, and the portable presentation result now carries
the final sample number for each marker rather than making animation code own
audio playback.

The effect-side impact path at `0x0045ac90` is reconstructed through its
enqueue boundary. `0x00417410` reads table 19 for selectors five and above,
table 18 for the packet value with the MCT additive, table 35 for constructor
argument six, and table 21 for the final argument. `0x004174b0` expands one
three-column group from each of tables 70 through 78 into the executable's
three separate nine-word packet banks. The portable request preserves the
77-word packet indices, the fields written by `0x00417e70`, the twelve
nonnegative type cases, type 10's one random visual draw, explicit origins
for types 3, 10, and 13, and type 12's second default-target lookup at impact.
All shipped nonnegative type/subtype pairs fit those tables; type `-1` remains
disabled. The shared runtime effect owner and the direct damage path are not
attached yet.

The direct impact half of `0x0045a2f0` is reconstructed up to its target
damage callbacks. `0x00453d50` accepts living same-scenario players with any
nonzero active state, inclusive judgement distance, and the attacker's exact
or neighboring facing sector. Only if no player qualifies does `0x00453fa0`
scan type-five actors, requiring the active-status high bit, exact facing,
positive life, and owner mode other than one. Both keep strict first-entry
ties. The common 77-word packet now carries post-AI values 0 through 2, 6
through 8, 29 through 37, plus pre-AI values 6 and 7 at their exact indices.
Its visual draw happens before `0x00413e00` clamps attack minus defense to
20 through 98 and consumes the hit roll. Misses emit the native miss request;
hits emit a damage request with the attacker's current position, followed by
sample 6 and event 17 only over event minus one. Player damage may switch the
attacker to presentation 11 and abort those post-hit effects, so that decision
remains explicit for the future live owner.

The alternative direct branch is preserved separately. When post-AI value 21
is not minus one and value 25 matches the attack variant, it skips normal
impact targeting and hit chance, consumes two visual draws, applies the exact
effect-number 0, 4, 5, 7, or default packet switch, and emits the 22-argument
effect request using post-AI values 23, 22, and 24. The shipped catalog
contains pairs `-1/0`, `0/0`, `1/0`, `4/0`, `5/0`, `6/1`, and `7/0`.
Actual player/type-five health mutation, miss presentation, effect-list
ownership, audio playback, and live enemy attachment are still outside this
passive boundary.

The first native dispatcher pair is reconstructed separately. `0x0045c350`
implements action zero as a timed idle: AID parameter one is its duration,
event 11 holds it active, and event zero is restored when the counter reaches
that duration. `0x0045c3c0` implements action one as a patrol inside the four
spawn-relative MCT bounds. AID parameter three is multiplied by the MCT speed
scale and divided by 1,000; parameters four and five control moving and idle
updates, and parameter six chooses the walking chart. Event 12 holds the
action and event one ends it.

Mode three in `0x00454310` draws the patrol X and Y independently from
inclusive ranges on its first active update. A zero movement limit returns
before either draw. The portable action controller emits the rectangle and
duration without consuming random state; the separate destination selector
owns those draws. All 61 shipped wait actions, all 92 patrol actions, and the
six zero-duration patrol cases are pinned, but stay dormant while later
presentation-side behavior remains incomplete.

The three short dispatch handlers after patrol are pinned too. `0x0045c560`
maps AI actions two through four onto presentation actions one through three,
and `0x0045c5a0` maps actions five through seven onto presentation actions
four through six. They clear the current presentation and reset the elapsed
action counter on entry, then do nothing on later updates while the
presentation routine owns completion. `0x0045c5e0` handles action eight by
resetting the counter on entry without changing the current presentation.

The shipped catalog uses action two 450 times, action three 158 times, action
five 178 times, action six 91 times, and action seven 42 times. It contains no
action-four or action-eight records. Their native handlers are still covered,
and malformed action numbers remain unable to alter controller state.

The remaining native dispatcher handlers are reconstructed behind the same
boundary. Action nine at `0x0045c600` retreats from its selected target using
movement mode five for a player or two for a scenario actor. It stops at
bounds distance 10,000, reports event 14 while active, and returns event nine
when its inclusive duration expires, target lookup fails, or walking stops.
Action ten at `0x0045c780` approaches to bounds contact with player mode four
or actor mode one, using event 15 before event ten. Both scale AID parameter
three by the MCT speed field; parameters seven and eight become target-refresh
cadence and random-turn chance.

Action eleven at `0x0045c900` starts mode-zero movement toward the cached walk
point, using the AI list's scaled `WalkPointSpeed` and stop distance 150. It
holds event minus one through counter 90, then returns event zero. The shipped
catalog contains 61 retreat actions, 205 approach actions, and no action
eleven. Forty-four retreats use authored target ranges and 17 use default
target selection; every approach uses a range, with 30 enabling refresh and
random turning. All twelve native dispatcher actions now emit one typed
portable movement/presentation result, but live consumption still waits for
the corresponding presentation and movement-mode reconstruction.

Remote Town's object group contains local IDs `0`, `200` through `204`, and
`300`, using `Character\OBJECT` resources 8, 15, and 14. Record 300 is named
`Warehouse`. `0x0045dd00` maps the 13-value tail into the type-zero runtime
object: its first three values select static-pattern or CAF-chart drawing,
while later confirmed values supply status bit `0x80`, height, draw flags and
strength, and RGB strengths. The remaining raw values are preserved without
guessed names. Remote Town's enemy and item counts are both zero. Entry key
zero supplies (`89898`, `2811`, direction `3`), the MCT map path selects
`f00_01`, and its footer is `{0, 0, 2000}`.

The first scenario then draws the decoded `f00_01.Gnd` cells at that entry and
places the selected male or female animation at the camera center. Its 279
`f00_01.Obl` records now supply
Remote Town's gates, walls, trees, rocks, and other static scenery. Pattern
bounds provide view culling. Static scenery, people, ground items, and the
player now enter combined shadow and visible lists. The initial
`InsertSort` order uses status class and the projected left/top judgement
corner; `SortDisplayObject` then reproduces the retail strict comparison of
all four absolute rectangle edges. This full pass fixes long Remote Town wall
segments and both city houses occluding an actor behind them. Paired
`ShadowLowPat` SDW assets use the same ordering and the configured opaque or
50-percent shadow mode through GAPI's general opacity support.

The first decoded person is Ostare: local ID `0`, people resource `13`, name
color `0x00e0e0e0`, label height `80`, position (`91467`, `1532`), judgement
`[-80, -80, 79, 79]`, and direction `7`. The custom 256-entry part table leaves
CAF parts 0, 1, 2, 3, and 6 enabled while disabling 4 and 5. Portable loading
resolves that resource to `Character/PEOPLE/00000013`, loads its NJP, SDW, and
CAF, and runs it at the 30 Hz game cadence. The people tail supplies speed 10,
a 30-update walk limit, a 30-update idle pause, and a spawn-relative rectangle
from (`-437`, `-223`) to (`269`, `231`). The type-one update at `0x0045d150`
starts movement after that pause; destination-selector mode three chooses an
inclusive random point in the rectangle, and `0x0045d9f0` selects CAF chart
one until arrival or the walk limit. The first tested draw uses shadow pattern
280 and visible patterns 1744 and 1784 at the retail starting camera anchor.
Actor shadows and visible cells share the depth-sorted world passes with the
player and static OBL scenery.

The selector at `0x00454310` is covered independently for all seven retail
modes. It preserves fixed points, inclusive patrol draws and exact duration,
player/scenario-actor approach and retreat bounds, refresh cadence and random
turn consumption, rectangle-edge projection, signed midpoint rounding, and
the native mode-two no-step quirk. It only selects destinations; the movement
controller at `0x00454930` still owns collision and stepping.

The same scenario's `Scenario.Scs` is now decoded in full: version `000`, 66
temporary flags, no network flags, 61 bitwise-inverted messages, 23 status
triggers, 220 sentences, and 608 commands. The interpreter entry at
`0x00430f80` is reconstructed for the commands exercised by Ostare's complete
opening interaction: conditional sentence calls, assignments, messages,
addition, subtraction, item creation, and three native actor actions. Clicking
local person zero derives character number `12000000`, resolves status kind
zero to sentence four, and shows retail message `1000000`. Opcode 2 finishes
the rest of its sentence before returning a message wait. Closing the actor
bubble follows `0x00453220` into status kind one at `0x00430940`; repeated
callbacks show messages `1000001` through `1000004` and finally release the
actor through opcode 19.

Scenario startup now instantiates all seven records in the Remote Town PEOPLE
group instead of stopping after the first. Their resource IDs resolve through
the same zero-padded `Character/PEOPLE` catalog, including the shared
`01000000` and `01000001` animal resources. Malse's new-game status follows
messages `1000019` and `1000020`; the later quest offer stays behind the
retail Red Goblin progression check.

After quest zero completes, Malse's message `1000013` exposes Trade, Identify
Items, Repair Items, and QUIT through the normal choice callback. Identify is
not the spell's one-item cursor mode. Sentence 114 reaches opcode 55 at
`0x0043397d`, which tests the five ordinary equipment pointers, four accessory
pointers, and the two item containers at player offsets `+0x514` and `+0x51c`
for an unidentified instance. With a match, opcode 51 at `0x00432fed` stores
twenty evaluated message parameters; message `1000017` consumes the first one
as the flat 100-Gold price and initially selects `NO`.

The confirmation callback reaches opcode 53 at `0x00433923` for total Gold.
Less than 100 selects message `1000015` and performs no mutation. Otherwise
opcode 54 at `0x00433940` removes 100 Gold from the backpack owner and opcode 4
at `0x00432296` sets identified one on all five equipment slots, four
accessories, backpack items, and belt items. The portable owners mirror that
change into raw word 48 for categories zero and one and word 47 for category
two, preserving it through the existing save writer. If opcode 55 finds no
unknown item, message `1000018` is shown and the service releases Malse without
charging Gold.

Repair starts at sentence 117. Opcode 52 at `0x004310d7` maps selector zero to
player pointers `+0x4f4` and `+0x50c` (active and alternate arms), one to
`+0x4e8` (head), two to `+0x4ec` (body), three to `+0x4f8` and `+0x510`
(active and alternate shields), four to `+0x4f0` (legs), and `-1` to the
backpack at `+0x514`. Sentence 117 sums selectors zero through four for All
Equipped, uses opcode 51 to substitute all seven values into message
`1000014`, and initially selects `QUIT`. Sentence 75 maps choices to the
individual repair branches; zero cost shows message `1000016`, insufficient
Gold shows `1000015`, and success invokes opcode 9 at `0x0043234a` before
opcode 54 spends the quoted amount. All Equipped invokes selectors zero
through four; Non-Equipped invokes `-1`.

`FUN_004667a0` computes one damaged item's price as
`((maximum-current) * (FUN_004661c0(item, 0) / 10)) / maximum`, returning one
when that division rounds to zero and returning zero for full durability.
`FUN_00466800` restores current durability to maximum. `FUN_00467140` and
`FUN_00467180` mutate and price only category-zero and category-one backpack
items. The item-value path begins with the definition base price, applies
Table 34 rows zero through seven to the absolute rolled elements and rows
eight through 46 to the absolute 39 instance parameters, with weapon parameter
16 using `abs(1-value)`, and preserves 32-bit wrapped arithmetic. The two
alternate equipment pointers are part of the eleven-pointer retail save
stream and are now retained as explicit equipment slots rather than opaque
bytes.

All seven records use the same type-one PEOPLE updater. Their final tail
values account for the visible differences: runtime offset `+0xd4` permits
native action 21 to face its evaluated target, while the preceding loader
conversion at `+0xd0` enables autonomous wandering only when the raw value is
zero. Malse disables both, so his opening action 21 intentionally leaves his
direction alone. Syria and the four animals can turn but do not wander;
Ostare enables both. Native action 18 only suspends movement and starts the
interaction, and action 19 releases it. The remaining tail value is `-65` for
all seven and is still unnamed.

Syria's new-game status follows messages `1000040` and `1000041`. Opcode 2 now
passes the current script character through the message event, which anchors
her bubble without inventing an actor command absent from this branch.
`0x00433f29` evaluates three operands and updates quest state. Its ordinary
path writes the requested value and sends event `0x41`; its state-two path
latches completion, requires old state one, writes state two, and sends event
`0x42`. The optional server broadcast remains outside the current
single-player slice. Syria exercises `{quest 0, state 1, no broadcast}`.
`0x00433868` then stores quest zero as the selected notice and writes `600` to
the adjacent counter. `0x004050f0` consumes that state: it gets the title from
Table 41, wraps it in the Shift-JIS corner brackets at `0x0047d530`, and draws
it in strength 224 at y 368 with a one-pixel black shadow. The encoded text is
right-aligned to x 612 in the normal HUD layout. Its exact six-pixel-per-byte
by twelve-pixel rectangle is clickable and opens the Mission List, while the
counter is decremented once per interface update. The two script cues call the
ordinary sound owner with samples 65 for an update and 66 for completion.

The same function scans the type-12 quest array for its first state-one row
and draws `StatusIcon.njp` pattern zero at `(616,360)`. Its exact
`[616,640) x [368,384)` hit box opens the Mission List independently of the
timed title, so the shortcut remains until no active quest exists.

The operand arrays have now been separated at their actual retail owners.
Type 12 is quest state, type 10 is the Table 40 transport array, and type 11
is broader script progress such as Ostare's index-four conversation flag.
Syria's repeat branch confirms the distinction by reading type-12 index zero.
It uses opcodes 42 and 43 to compare current/maximum life and mana, and opcode
63 for the optional player-condition pair, before choosing her ordinary
healing or blessing response instead of restarting the quest.

Scenario `00000001` owns the completion side. Red Goblin MCT ID `10000` maps
to script character `14010000`; its status-kind-four sentence 12 reaches
opcode 62 with `{0,2,1}` in single-player mode. The retail enemy lifecycle
calls `0x004309a0` only after the death presentation and fade expire. The
portable enemy owner now does the same, keeping the quest consequence in the
authored SCS rather than attaching it to an enemy name.

The scenario enemy registry now covers scripted reactivation as well. Operand
type 3 adds `14000000` to its local enemy number and reads the same entry used
by opcodes 31 and 32; a missing entry returns `-1`. All 160 shipped type-three
uses are opcode-0 reads. Opcode 28 at `0x00433022` calls `0x004309f0` to run a
target's status-kind-six sentence inline, preserving that target's character
context and succeeding when no such status exists. The corpus contains 181
calls across 32 scenarios and every shipped target has kind six.

Opcode 25 at `0x004326c9` evaluates enemy, X, Y, and direction before calling
`0x0045a140`. A living enemy is an ignored successful command. An inactive one
has its life, action, animation, movement, AI, reaction, attribution, death,
and opacity state reset, then moves to the supplied point without changing its
authored spawn rectangle. All 34 calls across 13 scenarios use operand shape
`{4,6,7,1}`. Expired portable enemies therefore remain in stable MCT slots.
Scenario `04000003` now proves the complete shipped flow from inactive search,
through kind-six effects 20007/20008 and sample 27, to delayed full-life
activation.

The callback for message `1000003` reads Ostare's live position through
operand types 6 and 7. Opcodes 11 and 12 form the offsets, and four opcode-10
calls create three ordinary ground-item records plus 200 money. The portable
records preserve category, definition ID, quantity, and retail position.
`0x00462f80` is reconstructed as an executable-owned item database loader:
the 256-byte substitution table, RCLIB-L block, signed-byte checksum, five
category counts, inverted names and descriptions, and category-specific
record sizes are all checked against the retail file. The `Item0000` group
and patterns 0, 45, 279, and 270 are inventory artwork only. The ground-item
path at `0x00458930` instead reads definition offsets `0x30` and `0x34`,
loading Character/ITEM resource zero and CAF charts 0, 5, 36, and 30. Those
charts produce NJP patterns 77, 82, 113, and 107 with their SDW shadows. The
CAF's mode-one palette rule selects `chart * 2 + cell priority`, and
definition offsets `0x3c`, `0x40`, and `0x44` provide the visible part's
default red, green, and blue strengths. The
portable entities also reproduce the 1600 initial vertical velocity, 280
gravity, 700 rebound, and two-bounce settle state before joining the shared
dynamic depth pass. The first impact emits the retail selector-two item sound;
the second impact is silent. A pickup rejected by the full backpack restarts
this same mode-zero presentation and sound. Unnamed definition fields remain
preserved as raw bytes.

The loader's separate MCT item branch sets initialization mode one. Those
actors use script character number `18000000 + local ID`, copy the three
common state channels, replace the common judgement with
`[-20,-20,19,19]`, and start at height zero in bounce state two. They
therefore do not run the drop arc or emit either impact sound. One MCT record
creates one actor; only category-four definition zero receives an inclusive
random quantity, without the script drop path's 10,000-Gold splitting.
Ground resources zero through six use the animated `Animation.*` layout,
while resources seven through eleven use static `Pattern.njp` and
`Pattern.sdw`; definition offset `0x34` is the CAF chart or direct pattern
index respectively.

On the next click, the retained type-11 flag enters sentence six. Opcode 61 at
`0x00433f16` reads offset `0x34` from the local player, writes level one to the
script destination, and selects the under-level-five message `1000005`. Its
callback shows `1000006` before releasing Ostare. Unknown opcodes stop with an
explicit unsupported result.

Pointing at Ostare follows `0x0040ee70`: his projected actor bounds select the
person, his MCT label height places the half-transparent black nameplate, and
the renderer adds 300 to each visible part's color strength. RKC_UPDIB
strengths above 1000 move palette channels toward white, so this produces the
retail pale tint rather than merely making dark pixels brighter. His opening
message is measured as Shift-JIS-aware 6-by-12 text by the rules at
`0x00456550`; `0x00456bb0` builds the actor-relative frame and tail from
`Hukidasi.njp`. A retail-data render test covers the nameplate, tint, frame
pieces, dimensions, and anchor coordinates.

The player CAF path now follows `0x00434ef0` and the appearance refresh at
`0x00444ca0`: the MCT direction is preserved, and the per-part enable table
starts with only the base body and shadow. Equipment layers remain disabled
until the future inventory slice has an equipped item to select them. The
player shadow comes from
`Animation00.Sdw`; palette index zero remains the NJP cutout rather than being
drawn as a black pixel.

The initial scenario's MCT music field is `0`. The transition helper at
`0x004275e0` maps that to `System\Game\Music\BGM00.Voc`, and the update helper
at `0x004275a0` starts sample zero looping in voice slot 500. Gameplay now
starts the same track once the world is prepared, applies the configured BGM
volume through LAL, and releases it with the gameplay state.

The first interactive world loop follows the retail movement path through
`0x00441c00`, `0x00454210`, `0x00454930`, `0x004351f0`, and `0x00434ef0`.
LWL button state becomes a 640-by-480 ground command, the RKC_RPGSCRN inverse
projection turns it into a world destination, and the unequipped new player
gets speed tier five. `0x00450080` indexes the retail factor table, multiplies
its `1.0` result by the base speed `20.0`, and stores a 20-unit walk step plus
the doubled 40-unit run step. Retail's DBF thread advances movement roughly
every 33 ms. The portable shell now uses an elapsed-time accumulator to update
all game state at the same 30 Hz cadence while continuing to present at 60 Hz.
This keeps movement and CAF frame counters on one clock and preserves their
real-time speeds. A click remains a latched destination. Held input replaces
that destination without restarting the current action. The input record's
hold counter must pass nine updates before release cancels movement, matching
the test at `0x00441c00`; an ordinary multi-update click therefore still
auto-moves.
Speech-bubble clicks remain consumed until their button release, including
when an option closes the conversation on the press frame.

Walk is executable action 2 and CAF chart 1 at `0x004351f0`; run is action 3
and CAF chart 2 at `0x00435530`. The `R` binding now toggles the persistent
movement mode on its key-down edge. Switching while already moving resets the
animation counter and immediately changes both the chart and the movement
step, as the retail action transition does. Those same routines play
`Voice00.Voc` sample zero on action-counter zero and then every 12 walking
updates or eight running updates. The cadence is fixed rather than randomly
selecting several footstep samples.

GND loading now includes the second, 852-by-852 Remote Town judgement plane.
The portable RKC_RPGSCRN boundary checks its bit-zero blockers and the
status-one OBL judgement rectangles against the retail player box
`[-80, -80, 79, 79]`. The movement controller performs integer swept checks,
keeps the last walkable point, and preserves the retail movement/wall
direction state. The collision sweep follows the dominant axis and uses the
same integer interpolation as `0x00414990`; cardinal edge movement also keeps
the resolver's one-pixel side contact. On first contact, `0x00454930` selects
its movement/wall pair from the attempted quadrant, returned contact, and
one-pixel position probes. Its exit and corner changes use the original
destination-sign tables rather than a distance-improvement guess.

There is no A* fallback in this path. Player and PEOPLE actors share the same
controller and face their actual detour step. Live town actors contribute
their judgement rectangles, including the actor being approached. The
PEOPLE walk at `0x0045da25` supplies dynamic collision mask `0xffffffff`.
`0x004145b0` excludes only the mover's own character number, so wandering
town actors also collide with the current hero and every other live PEOPLE
rectangle. The portable scene keeps that shared set current after each actor
update and has direct tests for self-exclusion and actor detours. The
159-unit rectangle range ends an interaction approach before that target
becomes a collision. Fixtures cross the sacks beside Ostare, cover the exact
Ostare-to-Malse route with live actors present, and use successive ordinary
movement legs for longer trips to companion interactions. The renderer
reads chart zero for idle and chart one for walking directly from player state,
rebuilds the depth key from the moving position, and follows the player's
projected position with the retail camera offset.

The dynamic-entity loop at `0x00429ce0` calls every active-map entry without a
camera or clip test. Its insertion path at `0x004298c0` orders entries by
character number. The gameplay frame updates the player before that scenario
loop. Type-zero objects use character numbers `10000000 + local ID`; PEOPLE
use `12000000 + local ID`. The portable scene therefore updates the hero, all
seven Remote Town objects, and then all seven PEOPLE records, including actors
outside the current view. Each later actor sees positions produced earlier in
the same update.

The common MCT vector previously labelled as part overrides is the initial
three-channel entity state. The loader at `0x004300e0` maps runtime offsets
`+0x4c`, `+0x50`, and `+0x54` to script keys based at 100, 300, and 200
million respectively. The type-zero paths at `0x0045ddd0` and `0x0045e080`
use them as visibility, pointer-selection, and judgement/collision flags.
Remote Town's exact object triplets are `{0,1,0}`, `{1,1,1}`, `{1,0,0}`,
`{0,0,0}`, `{1,0,0}`, `{1,0,0}`, and `{1,1,1}`. Its first three PEOPLE
records start `{1,1,1}` and all four companion records start `{0,0,0}`.

Opcode 56 at `0x00433a78` is a separate effective-state override. After the
local-player ownership check it resolves the first operand as a scenario
character, sets runtime `+0xfc` to one, and copies operands one through three
to `+0x100`, `+0x104`, and `+0x108`. Type-zero update `0x0045ddd0` uses the
first value for drawing and `0x0045e080` uses the other two for pointer status
and judgement. The original 100-, 300-, and 200-million script-key values are
not changed.

Near Remote Town status kind five uses saved flag 71 to swap overlapping
objects 1030 and 1031 between `{0,0,0}` and `{1,0,0}`. Both branches now run
through the portable interpreter and the object state owner. The complete
shipped call-site scan found 66 calls across 13 scenarios and only type-zero
targets, but the override remains a property of common scenario-entity state.

The type-zero object constructor at `0x0045dca0` is now represented by a
portable actor. Resources 8 and 14 supply static `Pattern.Njp` and
`Pattern.Sdw`; resource 15 supplies the paired `Animation.Caf` and
`Animation.Njp` path. Static and animated objects use the MCT pattern/chart,
height, status, strength, and RGB fields in the shared shadow and visible
display lists. Judgement-enabled objects join the live movement blocker set.
Pointer-enabled objects now use opaque static NJP or current CAF cells, the
shared range square and priority path, a +300 pale tint, and their MCT
nameplate.

Object 200 status zero emits opcode 37 at `0x004334da` and opens the transport
owner rendered by `0x0040c950`. It reads the 51 destination name/scenario/entry
triples from Table 40, compacts enabled rows into ten entries per page, uses
Status patterns 13, 22 through 24, 11, and 12, and plays sample 58. New
characters enable row zero (`Remote Town`, scenario 0, entry 50). Its
same-scenario selection resolves entry key 200 and relocates to
`(94685,-2756)`, direction 7. Warehouse status zero emits opcode 41 argument
zero at `0x004335ac` and toggles the existing Special Item owner.

The `0x00426200` transition signature is now separated from the transport UI.
Its scenario argument selects a `%08d` decimal directory. A nonnegative entry
value resolves MCT key `local-player-number + entry-value * 4`; `-1` uses the
explicit coordinate pointer instead. The same-scenario branch skips resource
teardown, while a changed scenario releases the old dynamic/map owners,
switches music, reads the new map and Scenario NJP, and then applies the same
entry rule.

Both branches install that entry and relocate the local player before running
scenario status kind 7. The changed-map path reaches this order around
`0x0042642b`; the same-scenario path repeats it around `0x00427474` instead of
merely moving the actor. Opcode 50 at `0x004321cb` writes the installed entry
through the common operand destination. Scenario `00010000` proves why the
same-map pass matters: entry 1 selects `Dusty Ruins, B1F`, while entry 2
selects `Dusty Ruins, B2F`.

Opcode 49 at `0x0043389b` resolves its operand as a current-SCS message ID,
copies the message text to `0x0048d5f8`, and clears `0x0048d5f4` in the local
single-player path. The executable has direct writes but no discovered reads
of either global. OpenShadowFlare retains the raw ID and text in the script
owner and deliberately does not render a guessed area caption.

The player identity queries beside that path are reconstructed as general
host reads. Opcode 66 at `0x00433682` calls `0x00434cd0`, which returns the
player-list current slot from `+0x08`. Opcode 57 at `0x00433b1f` resolves that
player through `0x00434cb0` and reads runtime `+0x28`, corresponding to saved
gender at record `+0x18`; zero remains female and one remains male. Ten
shipped scenarios contain exactly one paired call of each, each with one
temporary-flag destination. Dusty Ruins entry zero now exercises the complete
status-kind-5 path with local slot two and both gender branches through the
normal opcode-27 label owner.

The script's inclusive random command is reconstructed at `0x00431c43`.
Opcode 39 evaluates lower and upper operands, calls the executable's Visual
C++ random routine at `0x00467c6e` once, computes the signed remainder over
`upper - lower + 1`, adds the lower bound, and writes through `0x00434920`.
The full SCS catalog contains 611 three-operand calls in 55 scenarios: 285
use 0..1, 41 use 20..40, and the rest include script-calculated bounds.
The portable library obtains that one draw from the world's shared retail
random owner through a narrow hook.

The neighboring writable arithmetic commands are reconstructed too. Opcode
13 keeps the low 32 bits of signed multiplication; opcodes 14 and 15 use the
x86 signed quotient and remainder, including truncation toward zero and the
dividend-signed remainder. A zero divisor succeeds without changing the
destination. The 67 multiplies, 126 divides, and 195 remainders in the shipped
scripts all retain their two-operand shape and temporary-flag destination.

Opcode 30 at `0x0043309b` now crosses the same boundary without inventing a
second effect type. Its fourteen evaluated operands build the retail
owner-zero request, projected origin, and selected 77-word combat-packet
fields, while one draw from the world's random stream chooses impact
presentation `21000..21003` or `21007..21009`. The request enters the existing
`0x0042fdc0` controller/one-pass-effect owner. All 411 shipped calls across 33
scenarios retain the same shape; Near Remote Town's first periodic spawn
sentence is covered directly.

Opcode 36 at `0x0043332d` is reconstructed as the packetless sibling of that
path. Seven evaluated operands create an owner-zero request with an explicit
position, display height, chart direction, and `{0,0,right,bottom}` judgement;
a negative direction becomes eight. The shared one-pass handler maps effects
20007, 20008, and 20009 to OPTION resources 11000005, 11000006, and 11000007,
then turns the lower-right values into point judgement
`{right+1,bottom+1,right+1,bottom+1}`. All 353 shipped calls across 26
scenarios retain the audited shape, and Near Remote Town's first six-effect
periodic group runs through the live depth-sorted actor owner.

The portable fresh-world path accepts scenario ID, entry value, and local
player explicitly. The first cross-map fixture is Table 40 row one: scenario
6, entry 4, key 16. It loads Wasteland of Pillars, `Map\f00_07.map`, position
`(35105,-6156)`, direction 7, BGM 1, 35 type-zero objects, and two PEOPLE
records. Its map-local MCT, SCS, collision, patterns, overview, exploration,
actors, and ground items now share one `ScenarioWorld` lifetime. Player data,
all four item owners, quests, missions, transports, and common resources live
outside that boundary. Script data is adopted only after the full scenario
owner succeeds, leaving the callback-bearing interpreter runtime in place.
Types 10 through 13 script values live outside the scenario transaction.

Live travel now uses the boundary: the same-scenario branch only relocates,
while a changed scenario prepares every new local owner before commit. A
failed load leaves the old map, script, player, items, missions, quests, and
transport flags usable. Success clears stale local interaction/audio state,
adopts the new SCS, relocates, and changes BGM. It then presents the new map
immediately because the synchronous load has already completed. The previous
120-frame Epilogue fade was removed after a closer trace showed that
`0x00417bd0` owns story/briefing visuals rather than ordinary map loading.
Retail's black crossed-swords screen exists only while loading work is
pending. The alternate presenter is now tied to script opcode 64 at
`0x00434001`: value zero selects the Epilogue page and values one through six
select the matching `VisualNN.njp`. Its 120-frame strength fade, 300-frame
advance gate, Return/Escape/primary-click inputs, multi-page reset, WaitIcon
offsets, input lock, and resource release are reconstructed. All 51
Table 40 rows are also checked against their shipped scenario directory and
single-player MCT entry.

Opcode 65 at `0x0043403e` owns the adjacent falling-streak emitter. It refreshes
one spawn flag plus evaluated RGB and count values. `0x0041fe20` consumes five
shared Visual C++ random draws per particle, starts it at Y `-30`, projects a
short DDA line from the exact `4.712388` and `3.141592` constants, applies a
random 300-through-1,000 opacity, and removes it at Y `479`. The shipped audit
holds seven opcode-64 calls across six scenarios and 22 opcode-65 calls across
21 scenarios, including both temporary distance-density operands.

The authored Remote Town exit is reconstructed too. `0x004305d0` runs status
kind five records, then scans kind three records and resolves each status
character to its live scenario entity. `0x00414350` compares that entity's
absolute rectangle with the local player's rectangle using inclusive edges;
the ordinary visible, pointer, and judgement state values do not gate this
contact check.

Remote Town object `10000000` is the invisible south-gate trigger at
`(90124,4275)`, bounds `[-106,66,964,604]`. Its status-three sentence 219
executes opcode 17 at `0x00432162` with `{1,0}`. The handler writes the
scenario and entry to runtime offsets `+0x440` and `+0x444`, sets the pending
flag at `+0x43c`, and writes `-1` at `+0x454`. Scenario 1 (`Near the Remote
Town`) then loads `f00_02`, BGM 1, entry key zero at `(90581,5288)` facing 7,
48 objects, and 127 enemies. Its own object-zero status-three sentence sends
`{0,0}`, returning to Remote Town entry zero and BGM 0. The portable runtime
publishes exactly one loading transition in each direction.

Periodic status kind five executes independent scenario callbacks. Remote
Town sentences 158, 173, 188, and 203 read player-record offset `0x140`
through opcode 44, compare it with each companion type and the play mode, then
use opcodes 22 or 23 to set all three entity channels. In single player this
hides the player's own companion and enables the other three town dogs.
Sentence 149 is a separate distance/effect callback and still reaches
unsupported native behavior; it must not stop the later independent status
records from running.

Owned inventory interaction now follows `0x00445bd0`, `0x00446320`,
`0x00447290`, `0x00447970`, and `0x004087b0`. Backpack and special-item clicks
address their visible grids, the four accessory cells join the equipment
owner, and category-three objects use the separate staggered belt. An owned
item is removed from its container and carried by the shared item pointer,
while its inventory artwork is centered under the cursor using the complete
multi-cell footprint. Placement rounds that centered top-left corner to a grid
cell, rejects out-of-bounds and multi-item overlaps without losing the held
item, and leaves a single displaced item on the pointer. Closing either panel
does not discard the held item.

Clicking the live world with an item follows the branch at `0x00441d96`.
`0x00413ec0` chooses one of eight directions from the hero to the pointer and
the drop is placed exactly 200 world units away on that direction's axes. It
then re-enters the same ground-item resource, CAF, color, bounce, depth, hover,
and pickup path as scenario-created items.

The player-side 14-word receiver profile is reconstructed from `0x00443cb0`,
`0x0044fba0`, `0x0044fca0`, and `0x0044fe30`. It carries family zero,
character number, and the three derived attack/defense values, followed by
eight elemental affinities. The base values use the retail Fire through Metal
anchors and truncating distance formula. Main hand, helmet, body, boots,
optional off hand, four accessories, and identified multi-cell category-two
backpack items contribute their exact definition and/or rolled instance
values before the final `-10..10` clamp.

The same trace names item instance words 39 through 46 as the rolled elemental
values and runtime `+0x1c` as the identified flag. Item name color instead
comes from the definition variant. `0x004672f0` also proves that subtype one,
subtype three, or weapon field `0xcc` suppresses the off hand; field `0xdc`
does not. The portable item owner, save round trip, tooltip, player appearance,
and combat profile now share those meanings.

This is still not complete gameplay. Enemy effect-actor impacts, remaining
script commands and operand domains, alternate conversation modes, darkness,
and saved-game scenario restoration are the next executable layers.

The first player attack action now follows the executable rather than a visual
approximation. `0x00450630` maps main-hand subtypes to actions 7 through 10
and the deferred ranged actions 19/20. `0x00439140` and `0x00435e60` provide
the distinct counter order, chart pairs, sound counters, movement lock, and
recovery completion. `0x00450c60` supplies Table 4's attack-speed tier and
overweight fallback. The CAF part-zero `0x40` marker is retained as a typed
impact event and revalidated against the live enemy before later damage code
may consume it. Character selection and the saved player record use the retail
encoding directly: zero is female and one is male. `0x00435e60` therefore
plays sample 96 for male and sample 99 for female without a second runtime
translation.

The first update of the death action uses that same field directly.
`0x00435b60` plays `14 - raw gender`, so male queues Voice00 sample 13 and
female queues sample 14 exactly once before chart four advances.

Enemy hover labels now follow the type-two branch in `0x0040ee70`. A
name-sized dark frame contains a translucent red life fill proportional to
current over maximum life, the remaining width stays black, and
`StatusIcon.njp` pattern `native element + 3` supplies the colored dot before
the name. This replaces the generic PEOPLE-style label previously used for
enemies.

Player impact delivery is now reconstructed through the live actor boundary.
`0x00413e00` uses derived hit rate at player `+0x1bc` against enemy pre-AI
word 10 and keeps the exact `20..98` clamp. Successful actions 7 through 10
build their 77-word family-zero packet with derived attack and physical
defense, level, affinities, the 17 runtime words from `+0x1dc`, main-hand
reaction parameters, reflection percentage, hit effect, and weapon identity.
The base effect draw, unconditional equipment-reflection draw, subtype-8/9
replacement draw, receiver draws, and post-receiver durability draw remain in
native order.

The existing enemy receiver now supplies the only damage calculation. Its
returned life, attribution, reaction, event, and defeat state are copied back
to the live `EnemyActor`; its samples and the player's post-hit sample 6 enter
the world audio queue. An occupied main hand rolls its 30-percent durability
loss after the receiver. A zero-condition weapon remains equipped and retains
its weight and instance effects, but `0x0044ea60` excludes its base derived
values and elemental strengths. Deterministic packet tests and a live
Wasteland enemy click cover both the passive boundary and actual world
attachment. Hit/death CAF presentation, reaction displacement, common
effect-list ownership, marker and death audio, fading, and removal from live
presentation now run at the live boundary while the inactive MCT slot remains
script-addressable. Packet effects 21000 through 21003 are ordinary
impact splatters and play for both surviving and lethal hits. Their one-pass
CAF owner is separate from enemy death effect 21010, which reaches its last
frame normally, holds it, and fades during updates 91 through 119.
Lethal hits also update persistent kill and
experience fields, apply novice level growth, create Table 30/31 item rolls
and Gold Find-scaled money through the full ground-item owner, and preserve
their constructor state through pickup and saving.

The enemy dispatcher at `0x00458f70` is live now. When presentation offset
`+0x254` is clear, it evaluates the current event at `+0x204`, writes the
selected AID action at `+0x200`, clears the native slot at `+0x1fc`, and then
atomically promotes and dispatches that record through `0x00459340`. Direct
and effect presentations hold the presentation lock; otherwise queued
presentation `+0x1f8` replaces current `+0x1f4`, and the counters advance
after dispatch. The portable actor keeps that order instead of polling an
independent behavior tree.

The living-target search at the top of `0x00458f70` uses the inclusive
`0..5000` judgement-distance window. Without such a target, the ordinary
single-player path resets the actor to idle instead of continuing AID patrol
work across the whole map. The portable dispatcher now has the same activation
gate. Movement intent and visible motion are kept separate as well: the shared
controller can retain its obstacle-edge state after a blocked probe, but chart
one is only submitted on an update where the enemy's world position actually
changed. A controller request that becomes inactive returns the actor to idle
so action ten can publish its completion event and the AID table can recheck
the direct-attack range.

Patrol, approach, retreat, wait, and walk-point actions now feed the existing
movement destination selector and shared collision controller. In particular,
`0x0045c3c0` uses AID parameter three scaled by the enemy movement factor,
spawn-relative MCT patrol bounds, parameter four as its movement duration, and
queues walk presentation eight. Parameter six is retained but is not a CAF
chart selector. `0x0045c780` chooses movement mode four for a player or mode
one for a scenario actor and uses parameters seven and eight for target
refresh and random turning.

Ordinary enemy actions one through three acquire their target on entry, face
it, scan the CAF marker, revalidate the impact target, and send the exact
packet through the live player receiver. The receiver commits life, mana,
equipment, backpack, Special Items, reaction, effects, reflection, and audio
before sample six. The base player magical-defense field is initial parameter
row eight and its matching equipped contribution is Item.Ibn derived
parameter six; magical attack is the separate row seven/parameter-four pair.
Player receiver actions four and five now own chart-three hit reaction and
chart-four death presentation, movement and attack interruption, action
locking, direction, and collision-aware displacement.

The live Wasteland regression begins with the authored event-zero patrol,
waits for the same enemy to approach and damage the player, requires the
receiver effect and sample, and verifies that inventory, belt, and equipment
ownership remain unchanged. It then writes and reloads a retail save and
checks the damaged life and those owned item containers again.

Type-1 and type-2 enemy effects now cross the live world boundary. The
portable owner preserves `FUN_00429ec0` actor updates before
`FUN_0042fd60` controller updates, assigns category-50000000 identities,
resolves the controller source from the current actor, and loads each emitted
OPTION CAF independently. Source and forward actors therefore keep separate
lifetimes; controller completion does not erase either actor. Actor height is
stored in retail tenths of a screen pixel, including the authored value 250.

The common collision pass queries the actor's current position before its
movement step, builds live player/object/PEOPLE/enemy target snapshots, and
delivers successful packets through the existing receiver owners. Type 2's
shipped enemy 316 in scenario `03000507` emits source resource 11000027,
forward resource 10000040, launch sample 94, and contact sample 20. The live
regression requires the CAF draw, player damage, actor cleanup, unchanged
inventory/belt/body equipment, and the same damaged life and item ownership
after a retail save round trip.

Miss dispatch follows `FUN_00417a60` and effect 20012 rather than disappearing.
`FUN_0042c750` loads static OPTION resource 11000011, starts at height 400,
adds vertical velocity in tenths, performs the 500/-100, 300/-100, and
200/-100 bounce phases, then writes opacity 1000 down through 100 before
expiring the actor. The portable resource owner deliberately accepts its
standalone `Pattern.Njp`; it is not forced through the CAF resource path. A
low-hit live projectile regression proves that the actual target miss request
creates and renders this actor.

The first outdoor encounter has had a second retail pass after several small
rules drifted apart. Enemy death action eleven clears judgement at its first
update, so a corpse may finish its animation and fade but must stop blocking
movement immediately. Ordinary weapon actions eight through ten play sample
96 for the female hero or 99 for the male hero at their CAF impact marker.
The live first-Goblin regression now also requires authored passive aggression,
continued retaliation after a player hit, the weapon voice, and non-blocking
death state.

`FUN_00417550` confirms that Table 31 is not laid out like the portable code
first assumed. Its five fields are category, upper loot level, lower loot
level, fixed definition ID, and episode mask. A fixed ID other than `-1`
bypasses the weighted selection. Weighted rows compare against a separate
Item.Ibn loot-level field rather than the player's required-level field. This
means loot row zero's shipped profiles yield their authored low-level fixed
consumables and mine instead of accidentally selecting high-level equipment.

The first quest-critical fixed row is covered end to end too. Black Hammer in
scenario `00000004` owns Table 30 row 6: zero attempts become the one active
single-player slot, its 100-percent check always succeeds, and all ten choices
point to Table 31 row 400. That row fixes category 4, definition `99000000`,
the stolen gem stored on automatic-item page zero. Remote Town sentence 37
opens message `1000028` and then immediately removes that item and completes
quest one before waiting for the bubble acknowledgement. Sample 66, the next
three messages, save/load of quest and script state, and the absent returned
item are held by one live regression.

The next Episode 1 mission now has a live regression instead of only catalog
coverage. Ostare's Remote Town script requires completed quest zero and hero
level 30 before messages `1000007` through `1000009` start quest three. Dusty
Ruins scenario `00010004` then waits for enemy registry entries
`14000000..14000007` to become inactive; zero life is deliberately not enough
while a Garam Goblin is still fading. The all-clear branch runs its authored
object toggles and sound before opcode 62 produces sample 66. Returning to
Ostare creates Table 30 row 4 exactly once, sets persistent flag two, and
continues to the Cold Svalt message. Quest state and the reward latch both
survive save/load.

Syria's linked side mission is covered through the real item owners too.
Mission three being active lets message `1000044` start mission two. Stone
Spike in scenario `00010005` owns fixed loot row 23, which resolves through
Table 31 row 401 to category 4 definition `99000001`. That stolen Spirit Stone
belongs to automatic page zero at `(1,0)`; it must not be confused with the
later definition `98000001` on page two. Syria's message `1000045` immediately
removes the item and completes the mission, then callback `1000046` creates
category 2 definition `1100001`. The completed save keeps the item absent and
returns later visits to the normal recovery branch without repeating sample
66 or the reward.

The two one-time Remote Town gifts after Dusty Ruins are covered through the
same script state. Malse's status chain requires completed mission three,
Ostare's saved reward flag, and clear flag eight, then runs messages
`1000025..1000027` before opcode 10 creates category two definition `1100000`.
Syria uses her separate flag seven and messages `1000042..1000043` before
creating definition `1100002`. Both items take the normal airborne landing
path and sample 93. Saving and loading the two latches keeps later Malse and
Syria visits on their ordinary branches without repeating either gift.

The authored Cold Svalt route is now held by a live map-edge regression too.
Scenario 1 object 6 leads to scenario 3 entry 1, scenario 3 object 0 leads to
scenario 5 entry 0, and scenario 5 object 1 leads to scenario 6 entry 1.
Wasteland of Pillars object 3 has a separate mission-three-complete branch;
before completion it is a no-op, and afterward opcode 17 enters enemy-occupied
Cold Svalt scenario `1000001` at entry zero. This keeps the route, gate, map
titles, and entry coordinates in MCT/SCS ownership rather than a portable
quest-name special case.

Cold Svalt's first mission is covered through the same owners. Occupied
scenario `1000001` has 108 enemies and its object-two overlap enters inhabited
scenario `1000000`, entry zero. Alex's messages `1000000..1000006` latch flag
11, while Rosanna uses flag 15 and two visits for messages `1000047..1000051`
before starting mission four. Wild Ice character `14000001` owns loot row 56,
which resolves to fixed category four definition `99000002` on automatic page
zero at `(2,0)`. Rosanna's return sentence removes it and completes the mission
before message acknowledgement, then callback `1000054` creates category two
definition `1100003`. Completion sample 66, reward landing sample 93, saved
quest and latch state, and the no-repeat `1000055` follow-up are all covered.

Alex's following Cold Ruins assignment now has the same end-to-end coverage.
Once mission four is complete, messages `1000009..1000012` start mission six
and its normal notice. Bottom-floor scenario `1020002` scans its seven enemy
slots until both Frost Golems, all four Knight Frost Goblins, and the King
Frost Goblin have finished fading. The clear branch hides object `10011000`,
shows `10011001` and `10011002`, and completes the mission with sample 66.
Returning to Alex creates category four definition zero with quantity 2,000,
which is the authored Gold reward and uses sample 85 when it lands. Callback
message `1000015` immediately starts mission seven with sample 65, while saved
active state returns through `1000016` without repeating either reward.

Mission seven is covered from its outdoor entrance through Alex's next
assignment. Vaporous Forest object two enters Purgatory scenario `1030000`,
and object one there enters the clear room in `1030002`. That room contains
exactly three Arc Shamans and four Arc Thunder Bats; its periodic opcode-31
scan waits for every death fade before toggling objects `10011000..10011002`
and completing the mission with sample 66. Alex then creates 4,000 Gold, whose
normal landing queues sample 85, and messages `1000018..1000020` start mission
eight with sample 65. A saved reload follows message `1000021` without
repeating the reward or handoff.

The Remains of Reincarnation path now follows it. Hanged Men's Forest object
one enters scenario `1040000`; the object-one edges through `1040001` reach
clear room `1040002`. Its two Earth Golems, two King Earth Goblins, and three
Arc Goblin Shamans stay active through their fades. The empty opcode-31 scan
opens both authored door pairs, plays samples 34 and 31, runs Table 30 row 63
for the room loot, and completes mission eight with sample 66. Alex then drops
6,000 Gold and messages `1000023..1000024` start mission nine with sample 65.
The item landing paths and saved `1000025` no-repeat branch are covered too.

Mission nine is now held by its actual discovery transition. Remains scenario
`1040002` object five enters Sea of Trees scenario `1000004`, entry one, and
object zero there enters Immortal Remains scenario `1050000`, entry zero. The
destination initialization completes mission nine with sample 66 immediately;
it does not wait for an enemy clear. Alex's `1000026..1000028` chain then
starts mission ten with sample 65 and intentionally creates no item reward.
Saving and reloading returns through active message `1000029` without
replaying the handoff.

The Immortal Remains battle now completes the same chain. Object one in
scenario `1050000` enters `1050001`, and object one there enters Gargoyle room
`1050002`. Characters `14000000..14000003` are ordinary Gargoyles using loot
row 55 and a 50-percent 200..300 Gold range; `14000004..14000006` are the
magic variants with guaranteed 600..800 Gold. The periodic opcode-31 scan
waits through all seven death fades before swapping objects
`10011000..10011002`, playing positioned sample 34, and completing mission ten
with sample 66.

Alex's message `1000030` creates 10,000 Gold and changes global flag 11 from
one to two. Message `1000031` follows on callback, then opcode 64 value zero
opens the Episode 1 Epilogue presenter. Once it closes, the flag-two branch
uses message `1000033` to send the player toward Mining Town and latches flag
71. The Gold landing sample 85, Epilogue launch, both saved flags, completed
quest, and no-repeat return branch are covered by the live shipped-data test.

The post-Epilogue route into Episode 2 is now traced and covered with shipped
data. Near Remote Town status kind five reads saved flag 71 and uses opcode 56
to swap characters `10001030` and `10001031`. Object four only follows its
sentence-eight branch to opcode 17 `{2999999, 0}` when that flag is one. With
flag zero, walking into the same authored trigger leaves the player in the
scenario.

Scenario `2999999` is `Caravan`; its object-one edge leads to `2000000`, then
object one leads to `2000001`, and the next object-one edge leads to `2100000`
entry zero. The two road scenarios are both titled `Forest` and use music one.
Caravan's visual-one branch belongs to object two when flag 71 is zero and
then writes value two, so the ordinary flag-one Episode route must not present
that visual.

Scenario `2100000` is `Kanfore, Mining Town`. Status kind seven issues opcode
6 for vendor indexes 0, 1, and 2 with Tables 6, 23, and 32. The decoded town
has 14 PEOPLE actors: IDs 0 through 12 plus Beboba at ID 100. Its object-zero
edge returns to `2000001`, entry one. The native route regression proves the
gate object swap, Caravan branch, road titles and music, town actors and
vendors, save/load persistence, and return edge without adding production
map-specific code.

Kyle's first Episode 2 mission is now traced and covered as well. PEOPLE zero
uses saved flag 23 for its first-visit latch. Messages `1000002..1000008`
finish by setting mission 11 active with opcode 62 and publishing it with
opcode 48. Table 41 row 11 is `Destroy thieves staying SE of Kanfore.` The
active return branch is message `1000009`.

Mining Town object one enters `2100001` (`Forest of Four Leaves`) at entry
zero, and its object one enters `2100002` (`Forest of Claws`) at entry zero.
The first periodic opcode-31 scan watches Oak Knights
`14010000..14010002`. After their death presentations expire, opcodes 23 hide
objects `10000700` and `10000701`, and positional opcode 16 requests sample
81. A second scan watches `14020000..14020002` and completes mission 11 with
sample 66. Both triples use loot row 85 and a ten-percent Gold chance.

Kyle's completed branch opens message `1000010`. Temporary flag `1000018` is
20,000, so opcode 10's equal minimum and maximum create exactly 20,000 Gold;
the common 10,000 cap splits it into two airborne stacks with two sample-85
landings. Messages `1000011..1000012` then start mission 12, `Head for the
Mining Tunnel of Yugunos.`, and the active branch becomes `1000013`. Native
coverage keeps both mission states, flag 23, map route, gate, quest cues, Gold,
landing sounds, and saved no-repeat behavior under shipped data.

The first mission-12 gate is now traced and covered. Mining Town object one
enters Forest of Four Leaves scenario `2100001`; object three there points to
Cross Agora scenario `2100004`. Garshwin is PEOPLE zero near the southern
edge. His status starts with default message `1000002`, but active mission 12
replaces it before display with `1000003`; its embedded continuation shows
`1000004`. The status writes saved flag 24 on this refusal branch.

Cross Agora object three checks mission 14 for state two before opcode 17 can
enter Fanann scenario `2200000`. At this point mission 14 is zero, so reaching
the live trigger rectangle leaves the player in Cross Agora. Kyle reads flag
24 on return and runs messages `1000020..1000029`, describing the dragon seal
and Kirushutat. The final callback starts mission 13 with opcode 62 and its
notice with opcode 48; Table 41 row 13 is `Meet with the Wizard Kirushutat.`
Mission 12 stays active. Native coverage preserves the refusal, locked edge,
flag, cue, sound, both mission states, and `1000013` saved return branch.

The mission-13 route continues from Cross Agora object one to scenario
`2100005`, whose authored title is `Forest of Sprits`, then from its object one
to `2110000`, `Tower of the Wizard`. The tower uses entries for its ten floors;
entry 18 places the player near Kirushutat, PEOPLE zero.

Kirushutat's active-mission-13 sentence shows messages `1000012..1000027`.
It first executes opcode 62 with mission 13, state two, and the last callback
executes opcode 62 with mission 14, state one, followed by opcode 48. Table 41
row 14 is `Take back the Seal Crystal.` The active-mission-14 return branch is
message `1000028`. A shipped-data regression covers both route edges, the
entry, full briefing, mission notice/sample 65, persistence, and the no-repeat
branch through the existing generic owners.

Mission 14's item chain is also recovered. Cross Agora object two enters
scenario `2100006`, `Forest of Knight's Misery`, and its object one enters
`2120000`, `Fort of Thieves`. Scenario enemy 65 is an Oak Warrior with loot
row 76. Table 30 row 76 guarantees Table 31 row 403, which constructs category
four definition `99000003`; `Item.Ibn` names it `Seal Crystal` and assigns
automatic page zero, cell `(3,0)`.

Kirushutat sentence 28 passes `(4,99000003)` to opcode 58. When present,
sentence 29 removes it with opcode 59, shows message `1000029`, sets temporary
conversation state 300, and completes mission 14 through opcode 62. The
callback continues through `1000030..1000031`; subsequent visits use
`1000032` until mission 17 is complete. With mission 14 in state two, Cross
Agora object three reaches `2200000`, `Fanann, Village of Elves`. Native
coverage preserves the route, fixed loot result, automatic owner, removal,
completion sample 66, save/no-repeat branch, and reopened gate.

Fanann's entry script fills vendor owners zero, one, and two with opcode 6 and
Table rows 7, 24, and 33. PEOPLE zero is Lytle. His flag-41-zero branch shows
message `1000002`; its MTP continuation chain supplies `1000003..1000004`,
then the saved flag becomes one. The next interaction takes message `1000006`
while mission 12 stays active and mission 14 stays complete.

Object one in Fanann executes opcode 17 for `2200001`, entry zero,
`Butterfly Hill`. Object one there executes the same general command for
`2200003`, entry zero, `Dragon Road`. Native coverage enters through the
newly opened Cross Agora gate, checks all three vendors and Lytle, round-trips
flag 41 through the retail save owner, proves the no-repeat text, and follows
both authored route edges. No Fanann-specific behavior was added to the world
owner.

Dragon Road object two enters `2210000`, `Mining Tunnel of Yugunos, B1F`, at
entry zero. B1F object one enters `2210001`, `Mining Tunnel of Yugunos, B2F`,
at entry zero. B2F object two is the protected-area trigger rather than a
floor exit. On first contact its kind-three sentence writes saved flag 38;
while saved flag 40 is zero, opcode 17 relocates the player within B2F to
entry two. Mission 12 stays active and mission 15 stays at zero.

B2F object zero returns to B1F entry one, and B1F object zero returns to
Dragon Road entry two. A shipped-data regression walks those four edges,
touches the live B2F protection rectangle, checks the same-scenario pushback,
and saves and reloads flag 38 with the exact Dragon Road entry. The deeper
stair and switch path is deliberately not bypassed by this checkpoint.

The deeper Yugunos route is now traced and covered as its own checkpoint.
B3F scenario `2210002` requires switches 40000 and 40002, uses object two for
its same-scenario entry-two stair, and reaches B5F scenario `2210003` through
object one. B5F again requires both switch pairs: 40002 opens the 11000 gate
group and 40000 opens the 11003 group. Object 800 then writes saved flag 39.
The regression walks the real collision maps and gate actors rather than
relocating between those interactions.

Fanann PEOPLE four, Kirarru, consumes that finding through shipped script
branches. Her first visit shows messages `1000048..1000050` and writes saved
flag 45. With flags 38, 39, and 45 set while mission 15 is zero, the next
interaction shows `1000052..1000055`, starts mission 15, and plays sample 65.
Mission 12 remains active. Saving and loading preserves all three flags and
both mission states; the active-mission return uses message `1000051` without
replaying sample 65.

Outdoor containers use ordinary scenario scripts rather than a separate
hard-coded chest owner. Their status hides the closed object, shows its open
partner, calls opcode 16 for positional sound, and calls opcode 24 with a
Table 30 row and world position. Opcode 16 follows `FUN_00417260`: the sound
is accepted unconditionally when its flag is set and otherwise only within
3000 world units. Opcode 24 feeds the same complete loot constructor used by
enemy death. A real `Scenario.Scs` regression checks the shipped closed/open
pair, sample 77, and authored row-four drop.

Level growth now publishes the native feedback as part of kill accounting.
`FUN_00412fb0` lists changed fields in HP, MP, Attack Speed, Walking Speed,
Strength, Attack, Defense, Hit Rate, Evasion Rate, Magical Attack, Magical
Defense, Magical Hit Rate, and Magical Evasion Rate order. It keeps the notice
for 900 updates and plays sample 63; reaching level five plays sample 64 just
before it. `FUN_00450fb0` creates an auto-sized text owner with four pixels of
padding, black opacity 250, and centered flag `0x80`, which centers it in the
640 by 416 play area. `FUN_00451a40` holds that position for 60 updates, slides
it to x `640 - width`, y 1 over ten updates, then leaves it there until update
900. It adds the separate thin white frame at opacity 500.
`FUN_00451cb0` only accepts clicks after the first 30 updates and dismisses a
click inside the current rectangle before it can reach world movement. The
portable notice now follows those same drawing, timing, and input rules while
later skill unlocks remain outside this slice.

The saved job's script boundary is reconstructed too. Opcode 70 at
`0x00434186` maps raw jobs Mercenary `16`, Warrior `6`, Hunter `5`, and
Wizard/Witch `9` to occupation-menu selections zero through three and writes
the result to its operand. Opcode 71 at `0x004341da` accepts evaluated choices
one through three and writes raw job `6`, `5`, or `9`; zero and out-of-range
values leave the player unchanged. The handler touches only runtime player
offset `+0x30`, corresponding to saved record offset `0x1c`, so existing job
history and level-derived stats remain intact. Scenario `03900003` sentences
366 and 381 are the shipped change/query path.

Gameplay panels are independent owners on opposite sides of the world view.
The Warehouse's opcode 41 owner stays on the left while backpack and equipment
stay on the right, and the camera remains centered when both are open. The
same camera policy applies to a left map with the right inventory. Item
transfers between Warehouse and backpack are exercised in both directions.

Player ranged action 20 now follows `FUN_00437fe0` through a shipped Wood
Bowgun encounter. Chart 10 has 17 frames, launches from its frame-three
`0x40` marker, plays sample 3 at counter six, and uses the ten ranged frame
factors from 0.3 through 2.0. Raw Item.Ibn weapon fields `0xb8`, `0xbc`,
`0xc0`, and `0xc8` select the generic effect family, one/two/three/five/seven
shot pattern, travel speed, and target piercing. The two-shot pattern places
parallel projectiles at explicit minus/plus-eight-degree origins; the other
straight and homing fans use their authored angular spreads.

Ranged physical attack keeps full strength in job five and otherwise uses the
job-five history count from `FUN_00450f80`, scaled from 40 through 90 percent.
The common family-zero packet now carries hit rate in word 36 for effect-actor
evasion. Reflection and hit-effect random draws stay in native order, the
whole fan costs one weapon durability, and explicit-origin projectiles keep
player kill attribution through their packet. Generic effect types 0, 1, 4,
and 5 enter the existing category-50000000 owner with 30-unit bounds, static
and target collision, sample 20, optional homing, and optional remembered
targets. Retail ships no subtype-four weapon record, so action 19 still has no
authored projectile fixture. Action 20's Increased-Power redirect is now
reconstructed. Direct local kills charge it at 50, or 30 with special item
`98000001`; P and the HUD cell activate its runtime-only 900-update state. It
owns the pulsing readiness cell, common Powerup aura, sample-76 cadence,
forced speed tier, two effective spell levels, 20-percent defense input,
three sustained-spell exclusions, death cleanup, and non-persistence.

An active job-five action 20 rolls the exact 33-percent redirect once,
captures up to 100 living enemy character numbers inside the player's
4000-by-4000 judgement query, and falls back without rerolling when that list
is empty. Action 21 shares chart ten, its marker and sample-three counter,
builds the presentation-20006 physical packet once, launches a randomly
delayed resource-9000 north-east strike at every captured target, and charges
one main-hand durability for the volley.

## Tower of Ordeal Blackjack

The Blackjack path is reconstructed as an executable-owned modal reached by
the scenario interpreter. Opcode 73 at `0x004343b0` initializes the modal
without operands. Opcode 74 at `0x00434412` writes the retained outcome using
the retail values draw 0, player win 1, and dealer win 2. Completion invokes
scenario status kind 8, which keeps the outcome dialogue in the shipped SCS
files `99000018` and `99000023`.

`0x00403560` owns the 15-update deal phases, unique 52-card draws, the hidden
opening card's 53rd joker possibility, Hit/Stand rectangles, dealer draw at
16 and stand at 17, bust reveal, and the 200-update result lifetime. The
helper at `0x004036d0` uses the standard rank values with flexible aces and
joker. Equal 21s use the two-card natural tie-break. Deals play sample 44;
player and dealer wins play 64 and 65, while a draw is silent.

`0x0040da90` draws the retail `Card.Njp` board, deck, hands, title, fixed hero
and owned-companion previews, controls, deal transition, natural/bust marks,
and outcome art over Status pattern 119. The portable state and renderer are
kept separate from the script library and are covered by deterministic rules,
timeline, artwork, and original-SCS tests.

## Magic window and selection

`FUN_00407a60` uses Status pattern 6 as the complete Magic frame and displays
six spells from the current zero-through-three page. Each row starts at y=59
and advances by 48. Status pattern 32 is the empty well; availability 3 draws
the learned `MagicIcon` pattern at `spell + 2`, while availability 1 draws it
dimly. Odd availability states show the spell name and values from Tables 16,
17, and 27. Hovering the name line builds its help text from Table
`600 + spell`.

`FUN_00447790` proves the pick cells are x=24..56 with y=56 plus 48 per row.
The previous and next page cells are x=16..48 and x=270..304 at y=335..351.
The eight assignment cells begin at x=29, y=356 and advance by 32.
Picking a learned spell plays sample 57. `FUN_00404e40` handles release:
it removes every other occurrence, assigns the destination slot, and plays
sample 58.

`FUN_00404ee0` draws the separate live gameplay bar at x=224, x=344 beside a
left panel, or x=124 beside a right panel. Slots are normally 16 pixels wide,
with an extra four pixels before slots zero and four. The selected spell uses
its large MagicIcon at y=382 and grows to 26 pixels, moving every later hit
rectangle; other spells and empty slots use MagicBarIcon at y=392.
`FUN_00447570` selects a learned entry or the final normal-target icon and
clears the other mode.

Portable `GameplayMagic` owns only panel interaction state and emits typed
intent. `PlayerMagic` remains the sole owner of availability, progression,
saved bar assignment, current selection, and the normal-target toggle. This
keeps the cast dispatcher and future status effects out of the UI files.

## Normal-target combo and Transport

The final HUD icon is now live gameplay rather than selection-only state.
`FUN_00441c00` turns equipped actions eight and nine into the targetless
right-click actions eleven and twelve. `FUN_004364e0`, `FUN_00436c20`, and
`FUN_004372b0` form the complete three-stage chain: charts 5/7/8 for a
one-handed weapon and 15/17/18 for a two-handed weapon. Every stage scans its
own CAF impact marker, performs its collision-aware forward step, plays one
weapon swing sound, hits all valid nearby enemies, and uses consecutive male
samples 96..98 or female samples 99..101. Native coverage starts this through
the actual normal-target secondary-click command as well as the isolated
retail CAF controller.

The opening speed is character-owned, not a starter-item exception. A new
Mercenary has raw attack speed 100, Table 4 maps it to tier five, and the combo
factor at that tier is 1.3. The Short Sword contributes zero to derived
attack-speed parameter eight; the Dagger contributes 50. Native coverage now
saves and reloads the Short-Sword-equipped hero and compares the full combo
update count before and after a same-entry revival transition; the cadence
stays identical.

Spell zero now reaches action 22. `FUN_0043a260` uses Table 20 row zero and
the clicked dominant axis, tries the exact 500-unit cardinal offsets and four
retail corridor rectangles, and falls back through the other directions when
blocked. `FUN_00420020` owns the paired field and Remote Town endpoints, the
four seven-update-staggered falling patterns, the delayed central animation,
and samples 79 and 51. `FUN_00420970` enforces exit before re-entry, sends the
field endpoint to player entry value 100 in the linked town, and sends the
town endpoint back to the exact field position before consuming it. The live
test covers right-click cast entry, placement, resource loading, presentation,
and endpoint contact state. Multiplayer replication and the owner-name hover
are still open.

## Character Status tab

`FUN_00405750` is the other half of the same live left-hand window. Status.njp
pattern 5 supplies its frame and labels; pattern 6 remains the Magic tab.
The executable places job and name at x 22 and 92, level and experience at the
right edge, followed by current/maximum HP, weight, physical attack and
defense, hit and evasion, walking and attack speed, current/maximum MP, and
the four magical values. `FUN_00405590` right-aligns digits in eight-pixel
steps and uses grey for base values, red for a reduction, and gold for an
increase.

`FUN_0044fba0` measures the saved elemental x/y point against the eight
20,000-unit anchors. `FUN_0044fca0` adds the equipment and carried-item
strengths from `FUN_0044fe30`, clamps every result to -10..10, and selects
Status patterns 36 through 56 at successive 16-pixel rows. Pattern 57 places
the diagram marker at `x * 48 / 20000 + 80`,
`330 - y * 48 / 20000`. The portable Status state owns only tab and dismissal
input; all displayed values remain in PlayerData, the runtime profile, and the
existing element-affinity calculation.

## Fire Ball cast

`FUN_00449a40` validates the pointed enemy and selected learned spell, derives
the effective level through `FUN_00451e60`, reads Table 16's MP cost, applies
equipped parameter 19 with a minimum of one, and consumes the target command
even when mana is insufficient. Fire Ball faces the target, deducts mana, and
enters action 23.

`FUN_00439730` runs CAF chart 13 followed by chart 14. Table 20 row one is
scaled by the ten attack-speed factors from 0.6 through 1.9. The first chart
13 status-`0x40` frame determines effect 10001's delay, while truncated
counter-times-speed frame selection and truncated `7 / speed` completion
allowance preserve the retail update cadence. The effect owner—not input or
rendering—creates resource 10000010, plays samples 19 and 20, performs
collision, and delivers the exact family-zero packet.

The packet carries player magical attack, defense and hit rate, eight element
affinities, seventeen state words, Table 19's type, the three-column banks
from Tables 70 through 78, and Fire Ball ID one in word 73.
`FUN_00459690` passes that spell ID to `FUN_0044f6f0` only when the packet
reaches an enemy. Practice therefore occurs on contact rather than cast.
`FUN_0044f6f0` adds one point, uses Table 27 for the threshold, raises at most
one level, caps at level 20, and keeps companion-only spells seven through
nine out of the ordinary path.

## Ice Bolt cast

`FUN_0043ae10` is spell two's action 24 dispatcher. It shares CAF charts 13
and 14 and the ten casting-speed factors with Fire Ball, but reads Table 20
row two. Its effect-10002 packet sets word 3 to subtype one, word 34 to
presentation 21013, and word 73 to spell two; every table-backed value uses
row two.

Effect controller 10002 creates source resource 11000027 immediately. At the
marker-derived delay it re-resolves the hero, projects 180 world units along
the target angle, and launches resource 10000040 with 50-unit bounds, sample
94, environment and first-target expiry, and the copied packet. Contact uses
sample 20 and enters the same receiver-time practice path as Fire Ball.
Portable player input, action timing, packet construction, effect dispatch,
audio, damage, and practice are covered together in a shipped-world
regression.

## Plasma cast

`FUN_0043a840` dispatches spell three as action 25 with CAF charts 11 and 12
and Table 20 row three. Its packet uses physical defense in word 5,
presentation 20005 in word 34, and spell three in word 73. Effect request
10003 has target mask four, the pointed enemy identity and angle, the hero
position as explicit origin, zero travel speed and display height, and the
effective spell level in constructor argument 17.

The existing type-three effect owner reads Table 205, attempts a wave every
four updates at radii `250 + wave * 200`, and permanently suppresses the
current and later waves after one obstructed placement. A clear wave consumes
one random chart and creates resources 10000030, 10000031, and 10000032;
only the first layer applies the copied packet to every overlapping enemy on
update zero. Sample 21 plays for each clear wave. Live coverage accepts the
retail obstruction path and separately proves a clear shipped-world wave,
damage, audio, and Plasma practice.

## Ground/self spell command and Hell Fire

The ordinary secondary-click branch in `FUN_00441c00` handles selected spells
which do not require a character target. It validates availability,
restrictions, effective level, and MP, stores the clicked world angle and
position, faces that point, enters `spell + 22` with target `-1`, and deducts
the cost. The portable action event preserves the ground aim separately from
the optional character identity so later spells do not have to reconstruct
UI cursor state.

`FUN_00439d10` dispatches Hell Fire as action 26 with CAF charts 13 and 14,
Table 20 row four, and the common marker-based casting cadence. Its
family-zero packet uses magical defense, presentation 20001, and spell four.
Effect request 10004 keeps target `-1`, target mask four, zero direction and
travel values, no explicit origin, and the source judgement rectangle.

The existing type-four effect owner shows resource 10000002 before the cast
marker. At the delay it creates two resource-10000000 layers, plays samples
29 and 23, expands the source area by 150 units, and applies the packet to all
valid targets. Contact plays sample 20, the burst shakes the camera for eight
updates, and successful receivers award Hell Fire practice. Live coverage
proves that pointing at an enemy still uses this ground/self command and that
insufficient MP consumes the click without starting an action or effect.

## Ice Blast cast

Ice Blast uses the same targetless secondary-click command as Hell Fire. The
click only turns the hero: `FUN_0043b3f0` passes target `-1`, direction zero,
no explicit origin, and the player judgement rectangle. Action 27 uses CAF
charts 11 and 12, Table 20 row five, and the shared marker-based casting
cadence.

Its family-zero packet has subtype one, magical defense in word five,
presentation 21013, spell five, and the row-five table banks. Effect request
10005 retains target mask four, the marker delay, packet kind eight, and Table
21 row five in its final constructor field.

The existing `FUN_0042cd70` owner captures the live hero position on update
three and creates resource 10000051. Its chart-zero frame count schedules
resource 10000050, the expanded one-update area packet and camera shake at
plus four, resource 10000052 at plus 15, six sample-22 pulses, and expiry at
plus 22. Live coverage proves the self-centered capture, all three layers,
pulse audio, damage, receiver-time practice, and the insufficient-MP path.

## Heal cast

Heal stays on `FUN_00441c00`'s targetless secondary-click path and enters
action 28. `FUN_0043ca60` uses CAF charts 11 and 12 and Table 20 row six, but
does not create an attack packet or delayed effect controller. It scans every
newly displayed chart-11 frame and resolves only when status `0x40` is
crossed.

The marker always creates effect 21020 with owner kind one, source judgement,
packet direction eight, and no packet. `FUN_0042b860` maps it to resource
11000060 at the hero for one CAF pass. If HP is below maximum, the action
restores Table 17 row six percent of maximum HP capped to the missing amount,
calls `FUN_0044f6f0` for spell six, and plays sample 17. Full HP still permits
the cast and visual after paying MP, but skips restoration, audio, and
practice. Live coverage proves both marker-time branches and insufficient MP.

## Moon and Berserker sustained spells

`FUN_0043d290` runs Moon action 29 and `FUN_0043ceb0` runs Berserker action
30. Both use CAF charts 11 and 12, the corresponding Table 20 speed row, and
toggle only when a newly crossed chart-11 frame carries status `0x40`. Moon
stores Table 200 row zero at runtime `+0x15e4`; Berserker stores Table 201 row
zero at `+0x15dc`. Their effective levels remain separate and neither live
state is persisted in the 0x160-byte save record.

`FUN_0044ea60` applies Berserker's Table 201 rows 1 through 12 after equipment
to attack speed, walking speed, both maximum pools, and all eight ordinary
physical and magical combat values. `FUN_0044f2f0` reads equipped rolled
parameters 17 and 18 as life and mana rates, adds five for special items
98000003 and 98000004, then adds Moon and Berserker to mana. Both resources
update every third tick with separate remainders at `+0x1638` and `+0x163c`.
Zero MP clears both spell rates and rebuilds the player and companion profiles.

`FUN_00444960` draws common player animation block 500, mapped by the retail
resource list to `Player/Common/Powerup.Caf` and `.Njp`, at the hero with
chart zero, direction eight, RGB 1000/200/200, and runtime frame `+0x15f4`.
Locally owned kills are recognized by source character number modulo ten;
while active they train Moon and Berserker for either hero or companion kills.

## Energy Shield toggle

`FUN_0043d670` runs Energy Shield action 31 on CAF charts 11 and 12 with Table
20 row nine. The ordinary targetless command pays its Table 16 cost first.
Each newly crossed chart-11 status-`0x40` marker toggles runtime flag `+0x15ec`,
but activation is refused if the up-front cost left current MP at zero.

There is no Table 202 or separate shield pool. `FUN_00443cb0` resolves spell
nine's current effective level on each locally owned hit. For ordinary packet
families, Table 17 row nine scales physical defense and the resulting damage
is routed wholly to MP while any remains. Excess damage does not spill into
HP; later ordinary damage reaches HP once MP is empty. Effect-family packets
bypass Energy Shield. `FUN_00443490` clears the flag at zero MP.

`FUN_00444be0` reuses player common animation block 500, mapped to
`Player/Common/Powerup.Caf` and `.Njp`, with chart zero, direction eight,
runtime frame `+0x15f8`, and RGB 1000/1000/300. It is drawn after Berserker's
red Powerup pass. The same modulo-ten local kill ownership test trains spell
nine for hero and companion kills while the shield is active.

## Magic Shield toggle and receiver path

`FUN_00440180` runs Magic Shield action 40 on CAF charts 11 and 12 with Table
20 row eighteen. Its targetless command pays the normal Table 16 cost. A newly
crossed chart-11 status-`0x40` marker toggles runtime flag `+0x1628`, resets
aura frame `+0x162c`, and clears Counter Burst flag `+0x1630` without changing
the inactive counterpart's frame.
The marker may briefly activate at zero MP after an exact-cost cast;
`FUN_00443490` clears it at the beginning of the next player update.

`FUN_00443cb0` applies the shield only to local family-three effect packets.
Table 17 spell eighteen parameter zero reduces resolved damage with a
minimum-one result. A post-reduction value of at least 20 trains the spell.
Each intercepted hit creates effect 21029/resource 11000241 and plays sample
60, then charges MP from parameter two of the currently selected magic row at
Magic Shield's effective level. Equipped instance parameter 19 reduces that
charge, whose minimum is one. Emptying MP clears the shield immediately.

`FUN_00444a20` loops resource 11000240 at the player with chart zero,
direction eight, frame `+0x162c`, and RGB 1000/1000/1000. The live flag and
frame survive normal scenario travel, are not part of the disk save, and are
cleared with the other player powerups on death.

## Counter Burst toggle and reflection

`FUN_00440530` runs Counter Burst action 41 on CAF charts 11 and 12 with Table
20 row nineteen. A targetless command pays the normal Table 16 cost. Its newly
crossed chart-11 status-`0x40` marker toggles flag `+0x1630`, resets Counter
Burst frame `+0x1634`, and clears Magic Shield flag `+0x1628` without resetting
the other frame. Exact-cost activation remains visible for that marker update
before `FUN_00443490` clears it at the next update start.

The `FUN_00443cb0` reflection branch requires packet word 38, a type-two
packet source, and a living source actor still present in the scenario. Table
17 row nineteen parameter zero is added to successful equipment reflection.
The returned packet scales the resolved incoming damage by that total, halves
source value 100, clamps to one, and keeps the retail player identity, defense,
level, randomized 20015..20017 presentation, and receiver flags.

Active Counter Burst creates effect 21030/resource 11000251, plays sample 60,
and trains spell nineteen for incoming damage at least 20. Its MP charge uses
parameter two from the currently selected magic row at Counter Burst's
effective level, minus equipped instance parameter 19 and with a minimum of
one. Empty MP disables the flag immediately. An invalid source skips the
whole Counter-specific path.

`FUN_00444b00` loops resource 11000250 at the player using chart zero,
direction eight, frame `+0x1634`, and RGB 1000/1000/1000. It draws after Magic
Shield and before Berserker and Energy Shield. The state survives ordinary
scenario travel, is excluded from disk saves, and clears on death.

## Explosion action and companion presentation

`FUN_0043fcc0` runs Explosion action 42 on player CAF charts 11 and 12 with
Table 20 row twenty. The targetless command stores the clicked world point and
pays Table 16 MP normally. At a newly crossed chart-11 status-`0x40` marker it
finds owned character `16000000 + local slot` and validates the companion's
full judgement bounds at that point. Missing, defeated, presentation 7/9/10,
and blocked companions all leave the paid player cast as a no-op. Owner mode
six waits behind ordinary attacks and hit presentations rather than cancelling
them.

The marker assigns companion owner mode six. `FUN_004627d0` requests
presentation action ten and clears that owner mode. `FUN_00461c40` plays
PARTNER chart six direction eight, relocates exactly when chart seven starts,
then plays chart seven direction eight. Its newly crossed status-`0x40` marker
creates effect 21031 through `FUN_0042f890`; completing chart seven unlocks the
companion and returns it to ordinary AI.

Effect 21031 creates two resource-10000000 visual actors on charts one and
zero with RGB strengths 500/500/1200, plays samples 29 and 23, and requests
camera shake 8/6 for an observer within 3001 units. The chart transition also
submits positional sample 45 twice, while the impact marker submits sample 46
immediately before the special effect. A 640-by-640 companion-centered box
selects living enemies, then companion hit rate rolls against physical
evasion. The shared family-zero subtype-three packet uses the companion as
source, the owner player's magical defense as word-four damage, and zero
defense in word five. It reads spell twenty's parameter rows but indexes them
with spell 21's effective level. Word 73 remains twenty, so successful
receiver contacts train Explosion. One ordinary 21000..21003 impact is
selected before the per-target rolls.

## Earth Spear cast

`FUN_0043e000` runs Earth Spear action 32 on CAF charts 11 and 12 with Table
20 row ten. It requires the selected living character, pays the ordinary
Table 16 cost, and sends effect 10010 the hero's cast-time origin and fixed
direction to that target. Constructor travel values six and seven remain
zero; the effect is a placed wave line rather than a moving projectile.

The family-zero subtype-three packet combines Table 17 row ten with magical
attack and magical hit rate, carries physical defense in word five, chooses
ordinary presentation 21000 through 21003 with one retail random draw, writes
one to word 72, and identifies spell ten in word 73. The previously
reconstructed `FUN_0042e7e0` controller uses Table 206, tries resource
10000060 every eight updates at `wave * 300 + 250`, applies its packet in a
150-unit area, plays sample 22, and requests nearby camera shake. A blocked
first placement suppresses the whole line. Successful receiver contacts
award Earth Spear practice.

## Flame Strike cast

`FUN_0043beb0` runs Flame Strike action 33 on CAF charts 13 and 14 with Table
20 row eleven. The pointed command supplies the selected living enemy. The
action passes effect 10011 its direction, target identity, player judgement,
Table 17 travel speed, height 200, marker delay, effective level, and
constructor field 22.

Its family-zero subtype-zero packet combines Table 17 with magical attack and
magical hit rate, carries magical defense in word five, presentation 20000 in
word 34, zero in word 72, and spell eleven in word 73. `FUN_0042d6e0` creates
resource 10000012 at the hero, then uses Table 204 to launch two through eight
resource-10000010 children around the full circle at the authored delay. They
start 180 units out, home with turn value 20, expire after 90 updates or on
scenery/target contact, play sample 19 on the final spawn and sample 20 on
contact, and train Flame Strike through the receiver.

## Dread Deathscythe cast

`FUN_0043c490` runs spell twelve as action 34 on CAF charts 13 and 14 with
Table 20 row twelve. It sends effect 10012 the pointed target, direction,
Table 17 travel speed, height 200, player judgement, marker delay, effective
level, and constructor field 22. Its family-zero subtype-one packet uses
magical attack, magical defense, magical hit rate, initial presentation 21013,
zero in word 72, and spell twelve in word 73.

`FUN_0042db10` creates source resource 11000027 and the Table-204-sized
resource-10000080 warning fan immediately. Column 29 controls its retail
2.5132736-radian spread. At the delay it launches resource 10000081 from 180
units around the hero. Those blades travel straight with 50-unit bounds and a
90-update lifetime, replace packet presentations with directional 21021 and
21022, expire on scenery or first contact, play sample 94 at the final spawn
and sample 20 on contact, and train spell twelve through the receiver.

## Lightning Storm cast

`FUN_0043b950` runs spell thirteen as action 35 on CAF charts 11 and 12 with
Table 20 row thirteen. Its command requires a pointed living enemy to set the
angle, while effect 10013 deliberately receives source `-1`, target mask four,
target `-1`, zero travel values, the fixed hero origin, no source judgement,
marker delay, effective level, and constructor field 22. The family-zero
subtype-zero packet keeps the actual player source, combines Table 17 with
magical attack and hit rate, uses physical defense, presentation 20005, and
spell thirteen.

`FUN_0042e240` uses Table 204 to attempt four radial shells at radii 350, 550,
750, and 950, four updates apart. Each ray has an independent permanent
placement cutoff. A clear ray consumes a random chart and creates resources
10000030, 10000031, and 10000032; only the first applies the packet in its
100-unit area. Sample 21 plays once per attempted shell, the controller ends
at delay plus 16, and successful receiver contacts train Lightning Storm.

## Medusa cast

`FUN_0043da20` runs spell fourteen as action 36 on CAF charts 13 and 14 with
Table 20 row fourteen. Its pointed effect-10014 request carries the player
source, target mask 0x14, selected enemy, Table 17 travel speed, height 200,
direction, player judgement, marker delay, and constructor field 22. The
family-zero subtype-two packet uses magical attack, magical defense, magical
hit rate, presentation 21019, and spell fourteen.

`FUN_0042e5c0` creates no source visual. At the delay it re-resolves the hero,
starts resource 10000070 180 units along the stored direction, and plays
sample 22. The straight projectile uses 80-unit bounds and expires on scenery
or first target. Contact plays sample 20 and trains Medusa through the normal
receiver.

## Sonic Blade cast

`FUN_00449a40` accepts spell fifteen only with an equipped main-hand subtype
zero, three, or one. The invalid-weapon path consumes the pointed command but
does not spend MP or enter action 37. `FUN_0043e5e0` uses the corresponding
CAF pairs 5/6, 15/16, and 19/20 with the attack-speed tier from
`FUN_00450c60`, rather than Table 20's spell timing.

Action entry creates effect 21025/resource 11000100 at the player. Every newly
crossed first-chart status-`0x40` marker re-resolves the selected target angle,
plays sample 154, and creates effect 10015 with Table 17 travel speed,
hard-coded delay one, and constructor field 22. Its physical type-zero packet
uses Table 17 parameter zero percent of physical attack, physical defense,
magical hit rate, presentation 21024, flag 72, and spell fifteen. Counter six
also plays the equipped weapon's selector-four sample.

`FUN_0042a300` maps effect 10015 to resource 10000090 and projects a live owner
200 units forward. The straight actor has `[-80,-80,79,79]` bounds, display
height 155, lifetime seven, scenery and first-target expiry, and sample 20 on
contact. Successful packet delivery trains Sonic Blade through the common
receiver.

## Mud Javelin cast

`FUN_0043ecf0` runs spell sixteen as action 38 on CAF charts 13 and 14 with
Table 20 row sixteen. Its action-entry effect-10016 request carries the player
source, target mask 0x14, selected living enemy, Table 17 travel speed, height
200, direction, player judgement, marker delay, and constructor field 22.

The family-zero magical subtype-three packet combines Table 17 with magical
attack and hit rate, uses magical defense, copies parameter five and the
normal element/state banks, selects presentation 21000 through 21003 with one
retail random draw, leaves word 72 zero, and records spell sixteen in word 73.

`FUN_0042ea50` launches resource 10000110 at the marker delay with 80-unit
bounds and sample 19. It follows that projectile until removal, then creates
resource 10000111 at the last position. The burst applies the packet to every
target in its 240-unit area on update five, plays sample 22, requests nearby
camera shake, and ends. Successful contacts train Mud Javelin through the
common receiver.

## Identify cast and item mode

`FUN_0043f8d0` runs spell seventeen as action 39 on CAF charts 11 and 12 with
Table 20 row seventeen. Entry creates effect 21028/resource 11000230 as a
one-pass player-owned visual. The first newly crossed status-`0x40` marker
sets the local Identify mode and requests Inventory on the right. Repeating
the spell while the mode is active is consumed before another MP charge.

The Identify branch in `FUN_00446320` only accepts an unidentified backpack
item while the pointer holds nothing. It changes the instance flag, mirrors
the retail save word, trains spell seventeen once through `FUN_0044f6f0`, and
leaves Inventory open. Known items and non-backpack storage do nothing;
secondary click or panel close cancels the mode. `FUN_004087b0` draws pattern
one from `System.njp` instead of the normal pattern zero while the mode is
active. Unidentified tooltips use the item description as their base name and
hide all instance values.

## Script target geometry

The opcode-33 handler at `0x0043288d` resolves a scenario character and calls
`FUN_00430b30` with mode one. That helper scans player slots zero through
three, accepts only active living players in the same scenario, applies
inclusive lower and upper judgement-distance bounds with `-1` as an open end,
and keeps the first slot at equal distance. A match writes slot, world X, and
world Y. No match writes only slot `-1`; an unresolved source writes nothing.

Opcode 35 at `0x00432831` passes its evaluated operands to
`FUN_00414080`, which calculates `atan2(-Y, X)`. It multiplies the result by
the double at `0x00475178` (`57.29579143313326`) and uses the truncating x87
conversion at `0x00467fe0`. Negative results are not normalized. The shipped
catalog has 126 target calls across 25 scenarios and 80 direction calls across
17 scenarios; all operand shapes are covered by the portable audit.

## Script actor-attached effects

The opcode-40 handler at `0x00433409` evaluates an effect number followed by
a source character. Source values zero through three resolve a live player
slot and use owner kind one with the judgement rectangle at player offset
`+0x2e8`. Every other value resolves through the scenario-character registry,
uses owner kind four, and copies the rectangle at the common actor offset
`+0x20`. A missing source returns successfully without creating anything.

The handler calls `FUN_0042fdc0` with target kind and identifier zero, no
explicit origin, the copied source rectangle, no combat packet, direction
eight, instance `-1`, common lifetime value 200, and zero in every remaining
constructor field. `FUN_0042b860` resolves the owner position when the request
is presented and turns the copied lower-right bound plus one into the effect's
point judgement. Effect 20010 selects OPTION resource 11000008 and effect
20018 selects resource 10000020. The shipped catalog contains 54 calls across
45 scenarios, all with two literal operands: eight use 20010 and 46 use
20018.

## Scripted unlock switches

`FUN_004346b0` operand type nine is the common switch-interaction gate. It
requires the local player to be in the running script's scenario with action
one, finds the requested character in the live object-display registry, and
uses `FUN_004143c0` to compare their judgement rectangles with player field
`+0x3f4`. The ordinary range is `0x9f`. A missing or undisplayed character,
different scenario, busy player, or distance above 159 returns zero.

The opcode-26 handler at `0x00432b02` resolves a player slot or scenario actor,
projects its position, and draws the evaluated fourth operand as `%d`. It uses
the supplied X/Y offsets and RGB strengths, six pixels per character for
centering, a fixed twelve-pixel upward baseline, and a black `(1,1)` shadow.
There is no backing rectangle. Missing actors and absent remote player slots
are successful no-ops.

Opcode 60 at `0x00433edf` writes the evaluated marker to local-player field
`+0x159c`. The player update clears it before status kind five. The player pass
at `0x00434ef0` maps animation 502 to `Player/Common/UnlockSW` and draws chart
zero, direction eight at the player with full RGB strengths while the script
keeps that field set.

Opcode 29 at `0x00433056` is only the network-client notification. It passes
scenario event kind six and the evaluated value to `FUN_00419050`, which emits
packet `0x22` only in client mode; it performs no local state mutation. The
shipped corpus contains 60 matching type-nine gates, opcode-26 labels, and
opcode-60 markers across 22 scenarios. Opcode 29 appears 61 times across 23
scenarios.
