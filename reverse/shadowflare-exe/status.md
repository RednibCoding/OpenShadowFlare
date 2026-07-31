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
  death audio, fade timing, and final actor removal
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
the Table 13-driven 109-pixel experience fill and frame. GAPI has a general
destination clip for the live fills, so the original artwork is revealed
rather than stretched. The HUD
is a screen-space renderer outside the world camera and owns the lower input
band. Retail registers the standard Windows arrow once and never calls
`SetCursor`; LWL's native platform arrow is therefore the portable equivalent.

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
companion and status-effect branches remain pending.

New-character equipment and owned items now follow `0x00440f70` as well.
Category one definition zero is equipped in the body slot. Category-three
definition zero fills backpack column zero and belt row zero four times;
definition `10000000` does the same for backpack column one and belt row one.
The separate mine counter starts at five. This initialization runs only for a
new character, not as a fallback for the still-undecoded save payload.

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
The owner now restores and rewrites its exact save-payload container. The
inventory-panel transfer button at classifier case 10 remains pending.

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
mines, and the remaining dynamic payload still need owners. Writes go
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
the adjacent counter. The counter's consumer and the two cue sounds remain to
be traced.

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
pending. The alternate `VisualNN` selector remains pending. All 51
Table 40 rows are also checked against their shipped scenario directory and
single-player MCT entry.

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
effect-list ownership, marker and death audio, fading, and actor removal now
run at the live boundary. Packet effects 21000 through 21003 are ordinary
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
later job selection and skill unlocks remain outside this slice.

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
targets. Retail ships no subtype-four weapon record, and action 20's
increased-power redirect to action 21 remains with the later status/skill
work.

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
