# OpenShadowFlare roadmap

This is the working map for reconstructing `ShadowFlare.exe`. It is meant to
help us choose the next useful piece of work, not to lock the project into a
schedule. We will learn more from the retail game as we go, so later sections
will move around when the evidence tells us they should.

The rule is simple: get one small part behaving like the original, test it,
and only then build the next part on top of it.

## Where we are now

The compatibility-DLL milestone is complete. All fourteen DLLs build, reproduce
the original 763 exports, and run without forwarding into the retail DLLs.
Their Win32-compatible sources live in `src/reconstructed/`.

The detailed DLL scorecard remains in `fidelity/inventory.json`. It is kept
machine-readable so `tools/verify_fidelity.py` can catch export, ordinal, and
standalone-status regressions instead of making us maintain another table by
hand here.

The portable executable already has a solid front half:

- portable windowing, input, audio, and presentation through LWL, LAL, and LGL
- a backend-neutral graphics API with a 640×480 software renderer
- the title screen, its smoke animation, music, fades, and menu sounds
- new-character creation and the complete saved-game selection flow
- the original initial loading screen
- Remote Town's ground, static objects, shadows, player sprite, and music
- click-to-move movement, walk/run switching, matching animation, static
  collision, and camera following
- the in-game Settings, Help, Mission List, and Map screens

In other words, the game can reach the world and the player can now walk
around it. Remote Town now loads all seven PEOPLE records, their movement and
collision, the first human and companion conversations, and Ostare's four
opening item drops. The world renderer also uses the retail display lists and
full judgement rectangles, so large scenery such as walls and houses occludes
actors correctly.

Gameplay now receives a proper player-data handoff too. New characters are
initialized from the retail parameter tables, while selected saves contribute
their complete plain 0x160-byte character record. The first gameplay HUD layer
now reads that owner directly: the original bottom bar, level, life, mana, and
walk/run state are visible in the world. World pointing now uses opaque sprite
pixels, the retail click priorities and range square, and the opening quest's
items can be approached and picked up into a real inventory owner. Equipment
and combat can build on those owners rather than temporary values.

The current reverse-engineering notes live in:

- `reverse/shadowflare-exe/functions.csv`
- `reverse/shadowflare-exe/globals.csv`
- `reverse/shadowflare-exe/status.md`
- `documentation/exe-analysis.md`

Those files should be updated whenever a slice teaches us something new. Raw
decompiler output and tool projects stay under `reverse/`; only understood
behavior belongs in the portable source. The older work in
`reverse/references/` is handy for finding leads, but it must be checked against
the retail game before we treat it as faithful.

## A few ground rules

Faithfulness comes before improvements. Widescreen layouts, higher-resolution
assets, new renderers, modding features, and quality-of-life changes can all be
useful later, but they should not blur our picture of how ShadowFlare actually
worked.

`src/SF_EXE/` stays platform-neutral. It can use the C++ standard library, LWL,
LAL, LGL, and GAPI, but it should not grow Win32, Cocoa, X11, DirectSound, or
other platform code. Platform details belong in the small general-purpose
libraries that own them.

The old DLL boundaries remain visible in the portable executable. When we need
behavior that came from a DLL, it is ported into the matching static library
under `src/SF_EXE/libs/`. It must not be copied into the executable's state,
renderer, or world files just because that is convenient at the time.

The retail executable is our oracle. Function addresses, data layouts, update
order, timing, coordinates, sounds, and odd edge cases should come from
evidence. A reasonable-looking replacement is not automatically a faithful
one.

Finally, timing-sensitive game logic should preserve its original cadence.
Retail game state is driven by the DBF thread's roughly 30 Hz update, while
the portable shell presents at 60 Hz. The runtime uses separate fixed-step
clocks so rendering and window presentation do not decide how quickly the
simulation runs.

## Last completed milestone: make Remote Town feel like a game

This slice touched nearly every piece the rest of gameplay will need: input,
world coordinates, actor state, animation, collision, camera movement, and
depth-sorted rendering.

The first goal was simply to make a new character walk around Remote Town as
the original game does. That work has since grown into the common movement,
interaction, and display-order foundation used by the player, PEOPLE actors,
ground items, and scenery. The HUD, inventory, and combat are still waiting.

### What the retail path taught us

The first implementation pass traced a complete movement command through the
original executable:

- `0x00441c00` converts the primary-button screen point with
  `RKC_RPGSCRN::CalcWorldPos`;
- `0x00413ec0` divides the result into eight 45-degree directions;
- both new-character tables produce speed tier five, which the retail factor
  table turns into a 20-unit walk step and a 40-unit run step per 30 Hz
  gameplay update;
- chart zero is idle, chart one is walking, and chart two is running;
- the `R` binding switches the persistent walk/run movement mode;
- the starting player judgement rectangle is `[-80, -80, 79, 79]`;
- GND judgement bit zero and status-one OBL rectangles block the player;
- `0x00454210`/`0x00454930` form one shared movement controller for players,
  PEOPLE actors, and enemies. It uses direct collision sweeps plus stateful
  obstacle-edge steering rather than A*; enemy intent still comes from
  `RKC_RPG_AICONTROL`;
- the normal camera uses the projected player position minus the 320-by-240
  screen center, without another map-edge clamp. Live half-width panels move
  that anchor to 480-by-240 and restrict world input to the visible half.

The interaction path is traced too. `0x00449240` uses the judgement-rectangle
distance from `0x004143c0` and the player's 159-unit interaction range. A
distant click starts controller mode one, follows the actor, and opens status
zero only after the two bounds are close enough.

Those findings are recorded in the reverse maps rather than being left in a
scratchpad.

### The portable actor loop

The portable side needs a player actor with clear, ordinary state: current
position, destination, direction, motion state, animation chart, animation
frame, and the few counters the retail loop actually uses.

The gameplay update now:

1. accept a movement command from portable input;
2. turn the screen position into the same world position as the retail game;
3. advance the player at the retail rate;
4. resolve the movement against the same map and object bounds;
5. select the matching idle or walk animation;
6. update the camera;
7. pass the resulting state to the existing world renderer.

The game state remains at 30 Hz, but the 60 Hz presentation now interpolates
the previous and current actor snapshots. This removes the regular camera
judder without changing movement distance, collision, CAF timing, or scripts.

Input handling belongs in the runtime adapter, game decisions belong in the
gameplay/world code, and any reused DLL behavior belongs in its matching
library. The renderer should only draw the state it receives.

### What is covered now

The slice is finished when all of these are true:

- screen/world conversion and all eight directions have deterministic tests;
- five retail-cadence movement steps move a walking player exactly 100 world
  units, while running covers 200;
- idle, walking, arrival, and return-to-idle chart timing is covered;
- the complete retail Remote Town GND judgement plane is decoded and tested;
- static ground and object collision uses the retail dominant-axis integer
  sweep and keeps the last walkable contact point;
- the controller uses the original quadrant table, one-pixel probes,
  movement/wall direction pairs, and corner transitions to follow an obstacle
  edge. There is no added A* fallback;
- live town actors use their decoded judgement rectangles as movement
  blockers. The actor being approached stays solid too; interaction finishes
  at the retail 159-unit rectangle distance before the two actors collide;
- camera placement is rebuilt from the live player position. Static scenery,
  people, ground items, and the player share the retail combined display
  lists; the final full-judgement-rectangle sort correctly puts actors behind
  Remote Town's houses and long wall segments;
- held input replaces the destination, releasing after the retail ten-update
  hold threshold stops the hero immediately, and an ordinary click remains
  latched;
- a native live run reaches Remote Town and moves the camera and player from a
  ground click;
- the retail Remote Town fixture crosses the full sacks footprint, covers the
  exact Ostare-to-Malse approach with live actor blockers, walks longer trips
  as successive click-sized legs, and completes rendered companion choices
  plus Harley's two-message explanation branch.

This gives us the first genuinely interactive gameplay milestone: we can walk
around Remote Town. Any collision corner that looks different in a
side-by-side retail check should be kept as a small movement follow-up rather
than worked around in later actor code.

## Completed foundation: load and run Remote Town from its data

The order below follows dependencies. Each heading is still meant to become
several small commits rather than one giant implementation.

Remote Town began as a carefully reconstructed first case. The loader now
accepts an explicit decimal scenario ID, entry value, and local-player number
instead of embedding scenario zero or entry zero in the code. It resolves the
retail `%08d` scenario directory, reads that scenario's MCT, SCS, and overview
NJP, and selects the MCT entry key as `local player + entry * 4`, matching
`0x00426200`.

Remote Town remains the default start. The first nonzero fixture is Table 40's
`Wasteland of Pillars`: scenario 6, entry value 4, MCT key 16. A fresh world
now loads its `f00_07` ground, object, and pattern resources, its 35 type-zero
objects and two PEOPLE records, entry position `(35105,-6156)`, direction 7,
and music index 1. Missing directories or entries clear the partial world and
report an error. A catalog regression also checks every one of Table 40's 51
rows against its shipped decimal scenario directory and single-player MCT
entry key.

The scenario-local half of the world now has one explicit owner. `ScenarioWorld`
loads and releases the MCT, SCS data, map collision, map patterns, overview,
exploration mask, dynamic objects, PEOPLE actors, and ground items together.
The player record, equipment, backpack, belt, Special Items, item definitions,
quests, missions, and transport flags stay outside that owner. Script data is
handed into the existing interpreter runtime after a complete scenario has
loaded, so its callbacks never point into a temporary or moved runtime. This
is the transaction boundary the live map-change path will use.

Live changes now use that boundary too. A same-map destination keeps the
retail fast path and only relocates the player. A different scenario is fully
prepared before it replaces the old map, then its script data is adopted by
the stable interpreter runtime. Failed preparation leaves the current map,
script, player, inventory, equipment, belt, Special Items, missions, quests,
and transport flags untouched. Successful changes restart the scenario's BGM
and run the later `Waiting.njp`/`WaitIcon.njp` presentation for 120 rendered
frames while gameplay input and simulation are held.

The next slices identified the three resource preload lists, the variable
common entity record, and all four entity-group shapes. Objects and `PEOPLE`
decode with their retail resource IDs, names, positions, judgement boxes,
directions, custom CAF part masks, and type-specific tails. Enemy records
retain the 15 values before their fixed 32-byte AI-controller name and the 56
values after it. Placed items expose their category, definition ID, and
minimum/maximum quantity. The loader follows the object, PEOPLE, enemy, item,
entry, and footer sequence directly; every one of the 209 shipped MCT files
passes that path, covering 5,203 objects, 163 PEOPLE actors, 18,788 enemies,
and 84 placed items.

Placed items now have their runtime side too. `ScenarioWorld` creates them as
one actor per MCT record, assigns the retail `18000000 + local ID` script
identity, resolves their `Item.Ibn` visuals before committing a map change,
and keeps their three script-controlled state channels live. They use retail
mode 1: already settled, fixed runtime bounds, no bounce, and no landing
sound. Both animated `Animation.*` item resources and the static
`Pattern.njp`/`Pattern.sdw` layout are supported for drawing and opaque-pixel
selection. Leaving a map releases these actors with the rest of its
`ScenarioWorld`; a failed load keeps the current set intact.

Enemies now cross the same scenario boundary for the first time. The
Wasteland of Pillars fixture creates all 66 records, loads their shared
`Character/ENEMY` CAF/NJP/SDW resources through the general character cache,
and keeps their common MCT state, identity, bounds, direction, name, part
colors, and AI-control name. Their retail default action renders chart-zero
idle frames at active-map cadence, and enabled judgement rectangles are live
player blockers. The 34 resource-less `Enemy Hole` records found across the
catalog are preserved as invisible, non-colliding actors instead of being
rejected or assigned made-up graphics. This is intentionally not called enemy
AI yet: target selection, movement, attacks, health, drops, and the AI event
interpreter remain separate slices.

The data side of that interpreter is now in place. The portable
`RKC_RPG_AICONTROL` boundary decodes all 64 lists, their fixed 18 event
buckets, and 1,338 action candidates from `Control.aid`. Every shipped MCT
enemy name resolves through exact byte-name lookup, and each runtime enemy
keeps the resulting stable list index. The nine parameter and six condition
values stay raw until their executable consumers prove names and units.

The executable-owned event selector is reconstructed separately from the data
library. It applies the proven life-percentage and target-range conditions,
preserves retail's reverse temporary-list order and later-lower-priority quirk,
draws with parameter two as the weight, and falls back to event zero for the
same event set as retail. It is not attached to live enemies yet: doing that
before selected-action storage and complete movement/presentation consumers
are present would create a convincing but incomplete behavior path. Enemy
life, target queries, and the native dispatcher are now ready behind that
boundary.

The live actor now has the initializer fields needed by those next steps.
Pre-controller MCT values 1 through 4 are the spawn-relative patrol rectangle,
value 8 initializes both current and maximum life, and post-controller value
54 scales AID movement speed in thousandths. The post-controller triples used
by presentation actions one through six are named too: target distance,
chart/speed selection, and the four effect-construction values. All other
initializer words stay indexed until another executable consumer proves them.

Native actions zero through eleven now have an executable-owned controller
behind the live-actor boundary. It reproduces timed waiting, bounded patrol
requests, movement and idle phases, speed scaling, and the exact event 11/12
holding states. Its tests also cover the six shipped zero-duration patrol
records, which must not consume random state. The two three-variant animated
action families preserve their presentation mappings and entry-only counter
reset, while action eight keeps the presentation already in progress. Actions
nine and ten emit typed retreat/approach requests with the retail
player-versus-scenario-actor modes, target lookup rules, stop distances,
refresh/random-turn parameters, and event 14/15 timing. Action eleven emits
the 90-update fixed-point return using the AI list's walk speed. This includes
the unused action-four, action-eight, and action-eleven paths.

The generic destination selector used below that dispatcher is reconstructed
for modes zero through six. Fixed points, inclusive bounded patrols, player
and scenario-actor approach/retreat, target refreshes, random turns, and the
rectangle-edge projection all preserve retail integer and random-state
behavior. It emits a destination only; collision and stepping continue to
belong to the shared movement controller. Presentation-side targeting,
damage, animation completion, and live enemy attachment still have to land
together before this code drives a live enemy.

The two retail target searches are reconstructed as one shared service with
separate ranged and default entry points. Both search player slots before
companions regardless of distance and preserve strict-nearer tie ordering.
The ranged path uses active-state-one same-scenario players followed by the
four exact companion character numbers, inclusive judgement-bound limits,
optional living checks, script activity, and owner mode. The default path
keeps its looser nonzero player state and does not apply the companion
script-active gate. The evaluator and dispatcher now consume the same typed
target result rather than maintaining look-alike boolean queries.

Presentation actions one through six now have their own passive controller.
It reproduces entry-only target acquisition and facing, direct versus effect
chart selection, the ten exact animation-speed multipliers, truncating frame
timing, and the part-zero scan across every newly crossed frame. Impact bit
`0x40` and the three CAF sound markers are returned as typed events. The last
frame is drawn once before idle action seven and completion events two through
seven are restored only over an event of minus one, including the native
fast-frame skip and resource-less completion paths. Every visual enemy across
the retail MCT catalog has a valid speed index and referenced chart.
Effect construction itself is now decoded. Actions four through six expand
their MCT type, subtype, parameter, and additive through `Table.Tbd` tables
18, 19, 21, 35, and 70 through 78, build the exact 77-word packet fields the
executable writes, preserve the twelve shipped type-specific constructor
variants and random draw, and emit a typed spawn request. Type 12 repeats the
default target lookup at impact and keeps the player-slot versus
companion-character distinction. Unwritten stack words are marked instead of
being given invented meanings, and the catalog's disabled type `-1` stays a
no-op.

Direct impacts are decoded through the damage-owner boundary too. Their
impact-time search is deliberately separate from animation-entry targeting:
it accepts nonzero-state players across the facing sector and its two
neighbors, then falls back to active type-five actors in the exact facing
sector. Players still win even when a companion is closer. The resolver
builds the exact written words of the shared 77-word packet, consumes the
visual draw before the clamped 20-to-98-percent hit roll, and emits distinct
damage, miss, sample-six, and event-17 requests without applying them early.
Successful damage requests retain the attacker's current position passed to
the retail target callback.
It also preserves the MCT-controlled special branch, its two random draws,
all shipped effect-number switches, and its bypass of normal impact targeting.
The shared packet-to-damage calculation underneath those callbacks is
reconstructed too. It keeps the immediate damage override, effect family,
all four ordinary packet/receiver combinations, tables 7 and 11, opposing-
and same-element modifiers, separate physical and magical defense words,
minimum-one clamp, source lookup request, and every conditional random draw in
retail order.

The enemy side of that receiver boundary is reconstructed as a passive result.
It decodes the enemy's native element, physical and magical defense, two
  reaction defenses, and always-suppress-displacement flag from the MCT
  initializer. Local
source ownership, client minimum-one prediction, authoritative damage
attribution, tables 24 and 25, effect element banks, reaction stages,
reflection, the two configured effects, the 20-percent random hit effect,
samples 61 and 119, source status 73, on-kill statuses 7 through 9, death
metadata, and presentation actions 10 and 11 all keep retail ordering and
random draws. Effect requests also distinguish a real combat packet pointer
from the null pointers used by receiver-side visuals.

The player profile feeding the shared damage function is reconstructed too.
It carries the character number and three already-derived combat values, then
builds all eight elemental affinities from the retail two-dimensional anchor
formula. Equipment combines definition and rolled instance values, a
two-handed main weapon suppresses the off hand, accessories use their rolled
values, and identified multi-cell category-two backpack items provide their
passive values. Every channel clamps to `-10..10`. The trace also corrected
two adjacent mistakes: saved item metadata is an identified flag rather than
the tooltip color tier, and the two-handed weapon classifier uses raw weapon
field `0xcc`, not the unrelated field at `0xdc`.

The player receiver is reconstructed as a separate passive boundary. It keeps
local ownership, Increased Power, Energy Shield, Magic Shield, life and mana
routing, the category-four revival item, exact helmet/body/off-hand/boots
durability checks, equipment and Counter Burst reflection, tables 25 and 26
reaction selection, packet effects, training, audio, death action, and random
draw ordering.

The owned companion uses a third receiver at retail address `0x0045f9f0`,
not either of those paths. Its family-one profile, owner-slot life mutation,
actions 7/8/10 rejection, tables 24 and 25 reaction, action-five hit stages,
action-six death, distinct effect owner kinds, sample 119, event four, and
random draws are reconstructed separately. The enemy result is now attached
to the live actor, including reaction displacement, hit/death presentation,
common world effects, and audio. Player and companion results remain passive
until their own live state, effect, equipment, and networking owners can be
attached without skipping side effects. Kill accounting, experience, and
drops remain a separate enemy-death slice.

The marker-to-sample lookup checks the exact
25-by-3-by-10 resource override table first, then the three ten-chart fallback
rows; all 59 populated overrides are preserved and chart three's sample 86 is
the only fallback. Live hit and death actions return those samples to the
world audio owner, and death also uses the resource-specific 25-entry voice
table.

Ostare's first type-one behavior is covered too. The people tail gives him a
30-update idle pause, a 30-update walking limit, speed 10, and a small
spawn-relative movement rectangle. He now alternates chart-zero idling with a
short chart-one walk to a random point inside those bounds, as the original
update path does.

The MCT is now structurally decoded from beginning to end. The last PEOPLE
value is retained as reserved state: retail copies it into the actor
initializer, but its PEOPLE update and render paths do not read it. The enemy
parameters without proven consumers deliberately remain indexed until their
actual AI and combat paths establish names. Partners and other runtime
categories are created outside this four-group MCT sequence and still need to
be traced at their actual owners.

We need to finish the general MCT path around `0x00427b50` and the scenario
transition path around `0x00426200`:

- load GND, OBL, LST, NJP, SDW, and CAF resources through reusable code;
- preserve the original pattern-number relationships across those files;
- connect the reconstructed enemy AI event evaluator to proven enemy life,
  target, and selected-action state, then implement its native movement
  actions on top of the shared movement controller;
- release the old scenario in the same order the original does;
- identify the condition and sequence for the alternate `VisualNN.njp`
  artwork in `0x00417bd0`; its standard `Waiting.njp`/`WaitIcon.njp` path is
  reconstructed;
- support returning to the title cleanly when loading fails.

Remote Town should then be one input to the loader, not a special hard-coded
world.

## Completed foundation: give gameplay a real player record

The menu used to carry only enough character information to enter the world.
The first player-data slice established the real handoff without pretending
that the complete save format is already understood. It now:

- decodes `Table.Tbd` through the first portable `RKC_RPG_TABLE` library;
- preserves the retail 0x160-byte character record, including fields which
  are not named yet;
- exposes the confirmed name, gender, job, level, life, mana, and initial
  parameter fields through one `PlayerData` owner;
- creates new characters from tables 900 and 901 as `0x00440f70` does;
- reads the same 0x160-byte record from a selected `.Ssv`;
- passes the selected name, gender, and save path from the front end into the
  world instead of keeping a separate runtime gender variable;
- sends script level queries to `PlayerData`;
- keeps `PlayerActor` responsible for movement and animation only.

The encrypted payload after the character record holds inventory, scenario
state, equipment objects, flags, and other dynamic data. It can be mapped a
piece at a time as those systems gain real owners. This milestone does not
claim full save loading or writing.

## Current milestone: draw the gameplay HUD and world-pointer feedback

The first layer is live. `0x004039f0` supplies the exact `Bar.njp` patterns,
screen coordinates, digit placement, and 206-pixel life and mana calculations.
The renderer draws those packets after the camera-driven world and before
actor speech. The HUD owns y=400 through y=479, so clicks there no longer pass
through as movement commands.

The retail window class loads the ordinary system arrow and never replaces it
with another cursor. LWL already supplies that native arrow on every desktop
platform. What changes during play is the feedback drawn into the world, not
the pointer shape.

That feedback is live now too. The configured range square is tested against
opaque NJP pixels in the current CAF frame. An exact cursor-tip hit wins
inside a priority group, then the nearest candidate, while the five
configurable retail priorities choose between target types. Disabling the
range restores exact-tip picking. People keep their pale tint and nameplate.
Ground items now receive the same tint, their `Item.Ibn` name (or quantity
plus `Gold`), and the original yellow target square. Empty ground and people
use the original white square at their different strengths. Conversations
suppress it.

Escape now opens the original in-game settings panel as well. It uses the
authored `Status.njp` frame, retail text and hit coordinates, priority
reordering, and the original effect/BGM slider scale. Pointer, shadow,
occluding-object, and audio changes apply while the panel is open, and changed
settings are saved back to `SFlare.Cfg` on exit. The old fullscreen row is
intentionally blank, but its space remains so none of the following rows move.
Save and Return and Save and Exit now open their original confirmation states,
write the retail save envelope, and only leave gameplay after a successful
write. With `Save Image at Game End` enabled, they also write the paired
391-by-114 BMP from the player-centered world view before any HUD or menu is
drawn. Help now opens its original full-width reference screen from either the
menu row or `H`, including the animated player preview and the menu-owned
`CLOSE` tab. The Mission List is live from both its menu row and `Q`. It reads
all 48 titles and their description tables from `Table.Tbd`, shows only
script-started missions, keeps the original two-page layout and lock states,
and opens the retail detail panel when a title is clicked. The Settings Map
row and `N` now open the original half-width map too. It uses each scenario's
authored overview, keeps unexplored ground black, reveals the same 68-by-46
area while walking, and retains the original marker blink, scrolling,
recentering, frame, and dismissal behavior.

The remaining layers are:

- identify the experience field and table calculation, then draw its clipped
  fill;
- reconstruct quick-slot ownership and values instead of painting placeholders;
- finish the remaining message-window variants;
- darkness and other final world overlays;
- resizing behavior through the fixed 640×480 GAPI surface.

The bar currently shows full new-character life and mana from `PlayerData`,
the centered one-to-three-digit level display, and the persistent walk/run
indicator. Damage/healing lag colors, bar particles, condition icons, a
companion bar, and the level-up pulse can be added when the corresponding
gameplay state exists. HUD coordinates and visibility rules must continue to
come from retail draw packets; the interface stays separate from the world
camera.

## What follows the HUD slice

### 1. Finish the remaining town actor behavior

The next visible milestone is a Remote Town with its original NPCs and other
dynamic objects.

This will require the first portable slices of `RKC_RPG_AICONTROL`,
`RKC_RPG_TABLE`, and more of `RKC_RPGSCRN`:

- create actors from the scenario entity records;
- load their animation, palette, and judgement data;
- reproduce idle animation and facing;
- reproduce each actor's bounded idle/walk behavior where its MCT tail enables
  it;
- place actors in the same shadow and visible-object passes as the player;
- finish actor-to-actor collision for enemy movement. PEOPLE movement now
  uses the same live hero-and-actor blocker set as retail, excluding only the
  actor currently moving;
- preserve the reconstructed active-map update rules when later dynamic actor
  classes are added. The player now updates first, followed by ascending
  PEOPLE IDs, and PEOPLE continue updating while offscreen;
- extend pointer hover, pale tint, nameplates, and selection from people and
  ground items to the remaining dynamic actor classes;
- verify the town population and positions against the retail game.

That path now builds all seven Remote Town people records and resolves their
shared or individual `Character/PEOPLE` resources from the MCT table. Ostare's
part mask, idle pause, bounded walk, shadow, depth pass, hover tint, nameplate,
and actor-anchored speech bubble remain the detailed reference case. Malse and
Syria are selectable and run their real first conversations. All seven use the
same type-one updater: only Ostare wanders, Malse ignores scripted turn
requests, and the others can turn without autonomous movement. They update
offscreen in ascending character-number order after the player, matching the
active-map entity loop.

The same MCT also contains seven type-zero dynamic objects, separate from
`f00_01.Obl`: local IDs `0`, `200` through `204`, and the named Warehouse at
`300`. They now have their own portable actor and `Character/OBJECT` resource
owner. Mixed-case retail filenames are resolved without platform assumptions,
static NJP/SDW pairs and CAF animations stay distinct, and the MCT height,
draw flags, opacity, and color strengths feed the shared depth-sorted shadow
and visible passes. Active object judgement joins the same live blocker set as
PEOPLE actors.

The three common values before the CAF part tables are understood now too.
They are initial visibility, pointer, and judgement state, not part overrides.
The scenario script addresses those channels through its 100-, 300-, and
200-million key ranges. Remote Town's periodic companion sentences use the
same state path: the player's own companion stays absent while the other three
town companions become visible, selectable, and solid.

Type-zero pointing and the first two object services are now live as well.
Static objects use their opaque NJP pixels, animated objects use their current
CAF cells, and both share the retail range square, display ordering, pale hover
tint, and MCT-owned nameplate data. Clicking object 200 runs its real
status-zero sentence and opcode 37. That opens the left-hand transport panel,
whose destination names and scenario/entry values come from all 51 rows of
Table 40. The panel compacts enabled destinations into ten rows per page, uses
the retail frame, row and arrow patterns, plays sample 58 for navigation, and
moves the player through the scenario entry table. Its shifted right-hand
world view keeps updating and accepts world input while the left panel owns
its clicks. Remote Town starts with only its own row enabled, exactly as a new
retail character does.

The named Warehouse object follows the same pointer and range path. Its
status-zero sentence reaches opcode 41 with argument zero and toggles the
existing 9-by-10 Special Item owner instead of creating a second warehouse
inventory. UI ownership stays in the runtime controller; scripts only request
the service and the world only owns scenario data and relocation.

### 2. Grow scripts, conversations, and town interaction

NPCs become useful when the scenario script can drive them. The reconstructed
`RKC_RPG_SCRIPT` DLL is the reference here; the portable version belongs in
`src/SF_EXE/libs/RKC_RPG_SCRIPT/`.

The script work should grow from real Remote Town interactions:

- load the compiled `.Scs` data;
- recreate script variables, temporary flags, and persistent flags;
- implement the interpreter loop at `0x00430f80` one exercised opcode at a
  time;
- split the native game actions reached by the opcode switch into small,
  named engine hooks;
- support conversations, choices, messages, gates, warps, and quest flags;
- add shops and services when their scripts first require them;
- preserve wait states and update ordering instead of running a whole script
  in one frame;
- save unknown opcodes and data instead of silently discarding them.

The first checkpoint is now live. Remote Town's SCS decoder reads all 66
temporary flags, 61 messages, 23 status triggers, 220 sentences, and 608
commands. Clicking Ostare derives his script character number from the MCT
record, resolves status kind zero to sentence four, and runs the retail script
until message `1000000` waits for Return or another click. The initial
interpreter covers comparisons, assignments, messages, nested sentence calls,
and the two native actor commands reached by that path.

The message-close path is covered now too. Retail message commands finish
their immediate sentence work before waiting for input; closing the bubble
then invokes status kind one for the same actor. Following those callbacks
runs all five opening messages, advances the scenario flags, and reaches the
four original item-placement commands. Those commands now create ground-item
records with the retail categories, definition IDs, quantities, and
actor-relative positions. The repeat interaction also follows its second
callback message and releases Ostare through the script instead of a world
shortcut.

That visible checkpoint is done too. The executable-owned `Item.Ibn` loader
now applies the retail substitution table and RCLIB-L path and keeps the
unknown record fields intact. Its inventory icon fields remain assigned to
`Item0000.njp` through `Item0013.njp`, while separate fields select the
`Character/ITEM` resource and CAF chart used on the map. Ostare's Short Sword,
Round Shield, Dagger, and Gold now use those smaller ground animations,
matching chart palettes, default `Item.Ibn` color strengths, shadows, and the
original two-bounce drop arc in the same depth-sorted pass as actors and map
objects. The recovered format,
architecture, and extension rules are kept in
[the script-engine notes](documentation/script-engine.md).

The next interpreter checkpoint is live as well. Malse follows the short
two-message branch used by a new game; his later quest dialogue remains gated
by the retail Red Goblin progression value. Syria follows her two-message
new-game branch and reaches opcodes 62 and 48, which now update world-owned
quest state and select the retail 600-count quest notice. Message events retain
their script character number, so actor bubbles stay anchored even on branches
which do not run an explicit facing command. The Mission List consumes those
quest states directly and gets its text from the retail parameter tables.
The short-lived 600-count notice and cue audio are still pending; neither has
been guessed.

### 3. Items, inventory, and equipment

Item support should use the real table data rather than a hand-written list.
The work around `0x00462f80` and the proven `RKC_RPG_TABLE` reconstruction
should guide it.

This slice includes:

- item database loading and stable item identifiers;
- inventory ownership, stacking, moving, dropping, and picking up;
- inventory and equipment panels;
- requirements and the original stat calculations;
- equipping and unequipping armor, shields, and weapons;
- enabling only the corresponding CAF appearance layers;
- item names, descriptions, rarity colors, and comparison text;
- shops, prices, and money once the script layer requests them.

The opening quest's four real ground items are now loaded and drawn from
`Item.Ibn`. Pointer selection and the first complete pickup path are live:
distant clicks approach through the shared movement controller, then ownership
moves into `PlayerInventory` before the world entity is erased. Gold fills
existing stacks up to the retail 10,000 limit. Scripted and player-created
drops also play their category-specific retail sound on the first ground
impact.

The real 9-by-4 backpack grid is now in place. Width and height come from
`Item.Ibn`, placement respects multi-cell footprints, and a full inventory
rejects a pickup without losing or partly inserting it. The authored right
panel stays open over a live left-hand world view, with its original camera
anchor, input boundary, gender silhouette, gold and equipped-weight values,
Close tab, and `Item0000.njp` through `Item0013.njp` artwork. `I` and the HUD
ITEM button both control it.

Owned-item interaction is live now. A click removes the item from its backpack
container and carries the full icon under the pointer. Another click places its
centered footprint into free cells; invalid placements leave it on the pointer,
and placing it over one other item swaps which item is being carried. The held
item survives closing the panel and can be dropped into the live world in the
same eight directions and at the same 200-unit distance used by retail.

All nine equipment boxes now share one complete ownership path.
Helmet, body, boots, main hand, and off hand use their original hit regions
and category/subtype rules, enforce the item's level requirement, and swap
cleanly with the pointer. The four exact 1-by-1 accessory cells accept
category-two records and enforce the requirement stored in their own table
layout. Equipped weight and all ten decoded base contributions are summed
across the owner. The Short Sword enables CAF part 12 and the Round Shield
enables part 9 with its original color strengths; body armor and secondary
weapon parts use the same table-backed appearance path. Weapons marked by the
original classifier suppress the off-hand layer. As in the retail refresh,
helmets and boots affect equipment values but do not independently enable a
player CAF layer.

The lower HUD's belt is a separate 4-by-2 owner rather than part of the
backpack. It accepts only category-three items, retains full multi-cell
footprints, uses the retail staggered screen origins, and supports pointer
pickup and swapping even while the main inventory is closed. The `1` through
`8` shortcuts address its four upper pockets followed by its four lower
pockets. Tablets restore life, Capsules restore mana, and an item is removed
only when it actually changes the target, matching the executable.

A new character now receives the loadout built by `0x00440f70`: Leather Cloth
in the body slot, four Tablets and four Capsules in the first two backpack
columns, the same four-plus-four medicine layout in the belt, and five mines
in the player's separate mine counter. Moving, swapping, equipping, and
dropping owned items also use the retail category/weight sound selection;
successful belt use plays its own medicine sound.

Those owned items now survive the real `.Ssv` path. The obfuscated payload's
retail item prefix restores and rewrites all nine player equipment slots, the
backpack, belt, and Special Item owner, including exact grid placement, Gold
quantities, durability, identified state, and preserved instance bytes. Unknown
equipment records and the rest of an original save remain untouched until
their owners are reconstructed. The counted transport flags following the
owned-item prefix are also restored against Table 40, while new saves without
that later retail section keep the new-character default.

`X` now opens the separate special-item owner on the left. Its 9-by-10 grid,
Status patterns 14 and 15, item origins, centered placement, swapping, Gold
stacking, hover information, camera anchor, and world-input boundary follow
the corresponding retail paths. Inventory and Special Item close each other
instead of pretending to be two views of one container.

The ordinary item information display is live now. Resting the pointer over a
backpack, equipment, or special item for the same short delay as retail draws
its name,
non-zero combat values, durability, weight, required level, condition-adjusted
sale price, and all eight elemental values. It uses the original six-pixel
text grid, stat order, definition-variant colors, pointer-relative position, and screen
edge clamping, including the padded translucent black backing and faint white
frame. The values come from `Item.Ibn`; the Dagger, for example, shows the same
10 attack, 120 hit rate, 50 attack speed, 300 durability, and 100 sale price as
retail. Gold follows its separate retail branch and shows the stack amount in
the wide Price row rather than collapsing to a name-only tooltip.

The retail condition warning is now shared by backpack, equipment, and held
items. Weapons and armor below ten percent durability blink `Status.njp`
pattern 16 for eight updates on and eight off; broken gear keeps it visible.
The player-life and player-mana fields of category-three records are decoded.
Companion restoration, timed/status effects, mine placement, and the
script-facing special-item commands remain the next item checkpoint. Those
paths should keep using these owners rather than adding parallel inventory
models.

### 4. Combat and death

Combat should be built on the same command, actor, animation, and collision
systems used for movement. Avoid creating a separate shortcut just to make an
enemy lose health.

Work through it in this order:

- target selection and attack range;
- attack start, facing, movement cancellation, and CAF timing;
- the exact frame or event that applies a hit;
- hit chance, damage, defense, elemental modifiers, and critical behavior;
- enemy reaction, knockback or movement rules where present;
- life changes, death animation, removal, experience, and drops;
- player death and the original recovery or menu path;
- ranged attacks and projectiles;
- companion attacks after the basic player/enemy loop is proven.

One player class fighting one known enemy is enough for the first combat
slice. Other classes and special attacks come after the basic loop matches.

The first item is now reconstructed. Type-two enemies participate in the same
opaque-pixel click square, depth order, configurable priority, nameplate, and
pale hover tint as the other world targets. A valid hostile click follows the
moving enemy until the exact judgement-rectangle gap reaches the inclusive
retail range of `0x9f`, then stops, faces it, and promotes the target from
approach to attack-ready state. Ground commands, other interactions, target
loss, death, visibility changes, and pointer-status changes clear that combat
state. The next slice starts the ordinary player attack CAF from this ready
target and preserves its movement-cancellation and frame timing.

That attack-start slice is complete too. `0x00450630` selects action 7 for an
empty hand or an unclassified weapon, actions 8, 9, and 10 for the three
proven ordinary weapon subtype branches, and leaves the separate actions 19
and 20 to the ranged slice. The ordinary actions lock movement, keep the
facing chosen at the end of the approach, and play the authored first/recovery
chart pairs 5/6, 15/16, or 19/20. Their frame counter uses the ten retail
attack-speed factors from 0.6 through 1.5 and the tier produced by Table 4;
carrying more equipped weight than the player's capacity forces tier zero.
That tier is refreshed on every action update rather than frozen at startup.

The hit point in the animation is data rather than a hard-coded delay.
Actions scan every newly crossed cell in part zero of the first CAF chart for
status bit `0x40`, including frames crossed by a fast attack. Both shipped
male and female chart 5 fixtures have ten attack frames, the marker on frame
7, and nine recovery frames. The empty-hand action emits its swing sound at
counter five, the weapon actions at counter six, and input stays locked until
the last recovery frame has been presented. The world rechecks the retained
enemy's life, visibility, pointer state, and inclusive range before exposing
the impact event.

That impact now reaches the retail combat path. The player's derived hit rate
is checked against the enemy's MCT evasion value with the shared `20..98`
clamp before the 77-word packet is built. Attack, physical defense, level,
element affinities, persistent packet state, weapon reaction values,
reflection, effect number, and weapon identity occupy the same words as
`0x00439140` and `0x00435e60`. The ordinary and subtype-8/9 effect draws,
reflection draw, enemy receiver draws, and final durability draw keep their
retail order.

A hit now goes through the already reconstructed enemy receiver instead of a
second damage formula. Its returned life, attribution, reaction, event, and
death state are committed to the live enemy; sample 6 and receiver-owned
samples reach the world audio queue. An equipped weapon makes the retail
30-percent durability roll only after the receiver returns. Broken weapons
still weigh the same and remain equipped, but no longer contribute their base
stats or elemental strengths. A live retail-world regression clicks a real
enemy and requires the CAF marker to lower its life and queue sample 6.

The receiver's presentation state is live now. Action 10 selects CAF chart
two and stretches its frames over the exact reaction duration. Its sound
markers still use the retail counter-based cell lookup, and the last update
shows the last frame before returning to idle and publishing event 16. Packet
word 40 and the enemy's MCT flag are now named for what their consumers
actually do: they suppress displacement. When displacement is allowed, the
enemy receives the decreasing 120-unit reaction impulse and uses the shared
map, object, and live-actor collision sweep. Render interpolation keeps that
movement smooth without changing the 30 Hz simulation.

Action 11 selects CAF chart three, uses direction eight only when that chart
really supplies it, scans the same three sound markers, and plays the separate
resource-specific death sample on update one. It holds the final frame through
the original 120-update fade and then removes the enemy from the scenario.
Its first update creates the authored item drops and Gold first, then effect
21010 with the next random direction, preserving the executable's order.

The world now owns the ordinary receiver visuals too. Effects 21000 through
21014 resolve through the executable's dispatch table to their exact
`Character/OPTION` resources, snapshot the source actor after its update,
join the normal depth-sorted world pass, and expire after one CAF pass.
Effects 21010 through 21012 keep their separate 120-update lifetime,
500-strength start, and 30-update fade. Specialized reflection, staged
reaction, projectile, and spell effect handlers remain typed requests until
their own dispatch branches are reconstructed.

The lethal reward path is complete now. MCT pre-AI values 13 and 14 supply
experience and the Table 30 loot row; post-AI values 26 through 28 supply the
Gold chance and inclusive amount range. Table 31 profiles select item
categories, level ranges, episode masks, and quality variants. Their weighted
choice keeps the original nine-digit random helper and each selected
definition rolls all 39 instance parameters plus eight elemental values in
the same constructor order.

Damage attribution awards the local player Table 14's share of the enemy's
experience, updates the retail weapon/effect kill counters, checks Table 13,
and performs novice stat growth from the gender-specific 900-series table.
The HUD now reveals pattern 14 across the original 109-pixel experience bar.
Gold Find reads rolled equipment parameter 26. Item and Gold drops keep their
full instance object while bouncing, so landing audio, pickup, inventory
placement, equipment, and save/load do not silently discard rolled values.

The live enemy dispatcher is connected now. Each enemy evaluates its current
AID event, promotes the chosen native action, and then updates either that
action or its locked presentation in the same order as `0x00458f70`. Patrol,
approach, retreat, wait, and walk-point requests all use the existing
destination selector and movement controller, including live actor blockers.
The Wasteland fixture proves that an authored event-zero patrol turns into an
approach and ordinary attack rather than relying on a test-only enemy.

Direct enemy impacts now pass through the reconstructed player receiver. The
live receiver snapshot uses the named player base rows and matching equipment
contributions, including row eight and item parameter six for magical defense.
Returned life, mana, backpack, equipment, Special Items, reaction state,
durability changes, effects, reflection, and audio are committed in retail
order; the separate belt remains untouched. Player actions four and five show
the hit and death CAF presentations and interrupt movement or attacks. A live
regression waits
for a Wasteland enemy to damage the player, requires its hit effect and sound,
then saves and reloads the damaged character to make sure no owned items are
lost as an accidental side effect.

The next combat slice should finish enemy effect attacks. Actions four through
six already evaluate their authored data and create typed effect requests.
The executable trace now also proves that these requests enter a controller
list first and that a controller may create several category-50000000 actors
over time. Types 1 and 2, for example, create one source animation immediately
and a second actor 180 units forward only after the authored delay. The
controller and renderable-actor lifetimes must stay separate.

The portable work should follow that split: add the controller owner, port one
specialized dispatch family at a time, then attach the common runtime actor's
movement, inclusive collision window, target filtering, hit bookkeeping,
evasion check, receiver callback, and audio. Each family needs a passive
timing test and at least one shipped live enemy case before it is marked done.
Do not map `type + 10000` straight to one OPTION animation; that would skip
retail state and repeat the kind of adjacent-behavior loss this roadmap is
meant to prevent.

That split has started with types 1 and 2. Their portable controller now emits
the immediate source animation and delayed forward actor as two independent
requests, including source re-resolution, exact bounds, packet data, and
positional samples 19 and 94. The shared category-50000000 actor now also has
its first passive slice: chart-zero source lifetime, forward movement from the
spawn point, the zero-distance first update, static collision, contact expiry,
special-ground filtering, and inclusive target-window timing.

The next slice is target filtering, hit bookkeeping, evasion, receiver
dispatch, and configured positional audio. Types 1 and 2 should only be
connected to live enemy attacks once that common path can carry their actual
hit and sound behavior.

### 5. Skills, magic, status, and the remaining game screens

Once the ordinary combat loop is reliable, add the systems that modify it:

- skill and spell databases;
- mana use, cooldowns, targeting, projectiles, and area effects;
- buffs, debuffs, resistances, reflection, and absorption;
- character status and detailed stat panels;
- skill assignment and quick slots;
- map and the remaining modal screens;
- the in-game sound, display, input, and gameplay settings.

The large UI functions should be split by screen and concern in the portable
code even when the original compiler emitted one enormous function.

### 6. Finish save and load

Save support should preserve the retail format so original characters remain
usable and saves written by OpenShadowFlare can be opened by the original
game.

That means:

- map the full `.Ssv` structure and its version checks;
- reproduce the XOR encryption and the retail validation behavior;
- restore scenario, position, player stats, inventory, equipment, skills,
  quest flags, and other persistent state;
- preserve unknown bytes when the original does;
- handle truncated or corrupt saves without losing another slot;
- compare round trips byte for byte where the original format permits it;
- test original → OpenShadowFlare and OpenShadowFlare → original.

The envelope writer is now in place, including the random XOR byte,
signed-byte checksum, substitution pass, and safe preservation of an existing
unknown payload. New saves carry the player record we currently own. Writing
also captures the retail-sized paired preview from the world-only render when
the option is enabled. Saving is not complete yet: each persistent gameplay
owner still has to contribute its real payload fields, and loading must restore
those fields before OpenShadowFlare can claim full round-trip compatibility.

### 7. Play through Episode 1

At this point the work changes from building the engine's backbone to finding
all the assumptions that only worked in Remote Town.

Play Episode 1 from a new character and fix each missing case in order:

- every scenario transition and loading screen;
- all NPC, enemy, object, script, and quest types;
- bosses and special encounters;
- indoor and outdoor rendering differences;
- music and sound transitions;
- episode completion and return paths;
- saving and resuming at several points.

The goal is not merely reaching the final map. A normal playthrough should be
possible without developer shortcuts, hard-coded quest flags, or falling back
to the retail executable.

### 8. Cover the remaining episodes

Once Episode 1 is solid, run the same process through Episodes 2–4. Most of the
engine should already exist by then, but later content will expose less common
script commands, AI actions, effects, items, and map combinations.

Keep fixes general. If a later map needs a special case, first prove that the
original really has one.

### 9. Multiplayer

Networking comes after single-player simulation is deterministic and complete
enough to synchronize. The reconstructed `RKC_NETWORK` DLL gives us a strong
transport and packet reference, but the executable still owns the meaning of
many packets.

The multiplayer work includes:

- host, join, disconnect, and failure flows;
- player identity and character exchange;
- scenario and actor synchronization;
- commands, combat, drops, flags, chat, and transitions;
- ordering, retries, timeouts, and malformed packet handling;
- compatibility experiments with the retail client where practical;
- a portable `RKC_NETWORK` static library, with any operating-system socket
  adapter kept outside `SF_EXE`.

This should not be allowed to distort the single-player game loop. Both modes
need to use the same simulation rules.

## Work that continues alongside the slices

The reconstructed DLLs are complete enough to be our reference, but not every
obscure path has equal test coverage. When an executable slice depends on one
of those paths, add a focused differential probe before porting it. There is no
need to pause all executable work to chase an unused API.

We should also keep doing the quiet maintenance that prevents fidelity from
drifting:

- record retail addresses and confidence in the reverse maps;
- add deterministic native tests for every game rule we understand;
- use original assets in focused parser and rendering tests;
- make short side-by-side recordings or screenshots for visual work;
- keep malformed-data behavior deliberate and tested;
- keep each reconstructed simulation cadence independent from presentation
  speed;
- run the boundary test so DLL-derived code stays in `SF_EXE/libs/`;
- keep Linux and Windows builds green and regularly test real macOS hardware.

## What can wait

These are good ideas, just not reconstruction blockers:

- widescreen or redesigned interfaces;
- high-resolution assets and texture filtering options;
- Vulkan, Metal, or Direct3D GAPI backends;
- a public modding or plug-in API;
- balance changes and new gameplay;
- asset conversion tools that the reconstruction itself does not need;
- major optimization before a representative gameplay scene can be profiled.

The software renderer and LGL presenter are intentionally enough for now. A
new backend becomes worthwhile when the complete 640×480 game gives us
something meaningful to measure.

## Checks before a slice is ready to commit

At minimum:

```bash
cmake -S . -B build/linux/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/linux/debug
ctest --test-dir build/linux/debug --output-on-failure
./build/linux/debug/src/SF_EXE/ShadowFlare_rebuilt --smoke-test
./tests/run.sh
```

Use `./tests/run.sh --wine` when a change touches the compatibility DLLs or
when a differential probe is part of the slice.

Tests are not the whole fidelity argument. A gameplay slice should also have a
brief note explaining what was compared with the retail game and what is still
unknown. If we cannot say what “matching” means for a change, the slice is
probably still too large or not understood well enough.
