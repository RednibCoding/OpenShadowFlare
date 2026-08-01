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
- the shared Status/Magic window, derived character values, elemental display,
  four spell pages, drag-and-drop bar, and live spell selection
- the inventory, equipment, belt, Special Item, tooltip, and retail save owners
- the authored Remote Town exit and return loading transitions
- ordinary melee and basic ranged combat through death, rewards, and pickup
- the player's table-backed owned companion, including its PARTNER visual,
  depth sorting, collision, scenario travel, retail follow distances, enemy
  acquisition, ordinary melee attack, damage reactions, death, timed revival,
  and capped table-backed progression

In other words, the game can reach the world and the player can now walk
around it, leave through the south gate, and fight the first Goblin outside.
Remote Town loads all seven PEOPLE records, their movement and collision, the
first human and companion conversations, and Ostare's four opening item drops.
The world renderer also uses the retail display lists and full judgement
rectangles, so large scenery such as walls and houses occludes actors
correctly.

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

## Current milestone: skills, magic, and status effects

The ordinary melee, basic ranged, and owned-companion encounter paths are now
proven in the live outdoor world. The companion can acquire and attack enemies,
be selected by enemy direct attacks and runtime effects, play its retail hit
and death charts, fade out, wait 900 updates (or 600 while the authored
backpack item is present), and revive beside its owner through chart seven.
Kills credited to the owner or companion add one companion experience point,
use table row 18 for level thresholds, obey the player-level cap, rebuild the
table-backed profile, and fully heal on a level gain. The defeated countdown,
level, and experience stay in the retail player record and survive saving.

Player death and recovery are now reconstructed. Retail locks ordinary input,
plays chart four facing direction eight, holds its final frame for 120 game
updates, then returns the hero to the current scenario entry through a revive
transition. That transition restores both life and mana to their maximums.
The portable menus no longer interrupt that locked action, so Save & Exit
cannot preserve a dead actor. Saves made by older builds which already contain
zero life are repaired through the same revive reset when they enter the
world.

The save-owned foundation for that path is complete. `PlayerMagic` keeps the
22 availability values, levels, and experience counters plus all eight
magic-bar slots behind one boundary. New characters receive the retail
`0/1/0` array defaults and empty `-1` slots. Existing retail saves restore the
block after the three progress arrays, and new saves rewrite it without
disturbing later unknown state.

The selection side is complete too. `S` and `M` open the two tabs of the
authored left-hand Status/Magic panel without pausing the world, shift the
camera into the visible half, and can stay open beside the right-hand
inventory. Status uses pattern 5 for the saved identity, current and derived
stats, elemental affinities and marker. Magic's four six-spell pages use
the retail Status, MagicIcon, and MagicBarIcon artwork, availability states,
level/experience/MP/effect rows, description tables, arrows, hit rectangles,
and samples 57 and 58. A learned spell can be dragged into one of eight saved
bar slots; assigning it elsewhere removes the old copy. The bottom gameplay
bar follows the retail left/right-panel offsets and selects either a learned
spell or normal attack targeting. The UI state only emits intent, so saved
spell ownership and future cast logic remain in the world boundary.

The first complete targeted cast is now reconstructed. Right-clicking a
pointed enemy with Fire Ball selected validates its learned state and MP,
deducts the table-backed cost, locks the player in action 23, and creates the
retail family-zero effect packet. The existing effect-10001 owner keeps
responsibility for the delayed launch, samples 19 and 20, projectile travel,
collision, and impact. A successful packet contact awards one practice point;
misses and cancelled casts do not train the spell. The saved magic block
already preserves the resulting level and experience.

The targeted cast path now supports Ice Bolt too. It reuses the command and
action boundaries without pretending the spells are identical: action 24
uses Table 20 row two, effect 10002, packet subtype one, impact presentation
21013, resource 10000040, and launch sample 94. Both spells share only the
parts retail actually shares, and both have live shipped-world coverage from
selection through contact-time practice.

Plasma is complete as the first multi-wave cast. Action 25 uses player CAF
charts 11 and 12, Table 20 row three, and a family-zero packet whose defense
field and presentation differ from the two projectiles. Effect 10003 receives
the hero origin and target angle, then owns the Table 205 wave count, four
update spacing, 250-plus-200-unit placement, permanent obstruction cutoff,
randomized primary charts, three visual layers, sample 21, area contacts, and
receiver-time practice.

The ground/self casting command and Hell Fire are complete. Retail does not
route every spell through pointed-enemy selection: the ordinary world-click
path validates the selected spell, saves the cursor angle and ground
coordinates, and enters `spell + 22` with no character target. Hell Fire now
uses that path as action 26, including its warning, delayed two-layer burst,
area contacts, samples, camera shake, and receiver-time practice.

Ice Blast is complete too. It continues the targetless ground command as
action 27 but uses CAF charts 11 and 12, Table 20 row five, effect 10005,
packet subtype one, and presentation 21013. Retail analysis showed that the
click controls facing only: effect 10005 captures the hero on update three,
then runs its three authored layers, area contact, camera shake, and six pulse
sounds around that position.

Heal is complete as the first restorative spell. Action 28 stays on the
targetless command but resolves at the chart-11 `0x40` marker instead of
queuing an attack controller immediately. It always shows effect 21020 and
resource 11000060; missing HP restores the Table 17 percentage, plays sample
17, and trains the spell, while full HP still spends MP and shows the visual
without restoration, audio, or practice.

Moon is complete as the first sustained companion spell. Action 29 toggles at
the chart-11 marker and keeps its state in the live player rather than the save
record. While active, Table 200 supplies its maximum-MP drain and thirteen
companion stat percentages. Resource 11000040 follows a living companion and
pauses during its defeated and revival presentations. Kills owned by the
local hero slot, whether dealt by the hero or companion, train Moon while it
is active.

Berserker is complete as the first sustained player spell. Action 30 toggles
at the same authored chart marker but keeps its own Table 201 level and MP
rate. Its twelve percentage rows are applied after equipment to the one
derived player profile used by movement, action timing, physical and magical
combat, hit checks, and defense. The shipped table boosts speed and offense,
reduces physical defense and evasion, and leaves maximum life and mana alone.
The red `Player/Common/Powerup` animation follows the hero while active.

Moon and Berserker feed one retail-style mana-rate controller. Their rates
are added before maximum MP is scaled every third update, they share the one
fractional remainder, and zero MP switches both off before rebuilding player
and companion profiles. The same resource boundary now includes equipped
life/MP rate parameters and the two authored five-point recovery special
items, with the separate retail life remainder and living-player clamp. This
also fixes Moon practice to use retail's local kill-owner test instead of
requiring the companion to land the final hit.

Energy Shield is complete. Action 31 toggles its runtime flag at the chart-11
marker after paying the ordinary cast cost, but cannot activate when that cost
used the last MP. It has no Table 202 or separate pool: Table 17 scales the
physical-defense input, ordinary damage is routed to MP without spillover,
and effect-family damage still reaches HP. Zero MP shuts it off, owned kills
train it, and the yellow `Player/Common/Powerup` pass follows the hero after
Berserker's red pass.

Earth Spear is complete. Action 32 returns to the pointed-enemy path and sends
its exact subtype-three packet, fixed cast origin, and target angle to effect
10010. Table 206 drives the eight-update stone-ridge line, including authored
spacing, placement collision, first-wave cutoff, area contacts, sample 22,
camera shake, random ordinary impact presentation, and receiver-time practice.

Flame Strike is complete. Action 33 uses the pointed-enemy command, charts 13
and 14, and its exact magical subtype-zero packet. Effect 10011 now receives
the cast through the player boundary and uses Table 204 for its full-circle
homing fan, including the source visual, authored delay, travel, turning,
collision expiry, samples 19 and 20, and receiver-time practice.

Dread Deathscythe is complete. Action 34 keeps the pointed command but sends
the exact subtype-one packet to effect 10012. Table 204 owns its warning fan
and straight projectile fan, including retail spread math, even-count offset,
directional dual presentations, scenery and target expiry, samples 94 and 20,
and receiver-time practice.

Lightning Storm is complete. Action 35 uses a pointed enemy only for its angle
and sends effect 10013 a fixed hero origin with anonymous effect identities,
while its packet keeps the real player source. Table 204 drives four radial
shells with independent obstruction cutoffs, all three visual layers, random
primary charts, area contacts, sample 21, and receiver-time practice.

Medusa is complete. Action 36 uses the pointed-enemy path and exact
subtype-two packet. Effect 10014 owns its delayed straight
resource-10000070 projectile, including the 180-unit launch offset, scenery
and target expiry, samples 22 and 20, and receiver-time practice.

Sonic Blade is complete. Action 37 accepts only equipped weapon subtypes zero,
three, and one, then uses their ordinary 5/6, 15/16, and 19/20 attack chart
pairs at the retail attack-speed rate. Effect 21025 supplies the immediate
charge pass. A first-chart `0x40` marker launches resource 10000090 through
effect 10015 with its seven-update lifetime, samples 154 and 20, and exact
physical packet; the normal weapon sample still occurs at action counter six.

Mud Javelin is complete. Action 38 returns to the normal Table 20 casting
timeline on charts 13 and 14. Its effect-10016 request carries the pointed
enemy, the exact magical subtype-three packet, and the retail randomized hit
presentation. Resource 10000110 owns the tracked projectile and sample 19;
resource 10000111 owns the finishing area burst, sample 22, and camera shake.

Identify is complete. Action 39 uses charts 11 and 12 and shows one-pass
effect 21028/resource 11000230 at entry. Its `0x40` marker opens Inventory on
the independent right side and changes the common cursor into the retail
Identify pointer. Only an unidentified backpack item completes the command;
the item flag and its retail save mirror change together, one practice point
is awarded, and right-click or closing Inventory cancels the mode. Recasting
while it is already active does not spend MP again.

Magic Shield is complete. Action 40 toggles its runtime flag at the chart-11
marker and drives the authored resource-11000240 player aura. Effect-family
hits use the retail reduction, practice threshold, effect 21029, sample 60,
selected-magic MP-cost quirk, equipment discount, and immediate empty-MP
shutdown. Ordinary scenario travel preserves the live shield while death and
a fresh game clear it.

Counter Burst is complete. Action 41 toggles at the chart-11 marker, resets
its authored resource-11000250 aura, and excludes Magic Shield without
rewinding the inactive counterpart's old frame. A valid living-enemy source
runs the retail combined-reflection packet, effect 21030/resource 11000251,
sample 60, training threshold, selected-magic MP-cost quirk, equipment
discount, and empty-MP shutdown. Ordinary travel preserves the runtime state;
death and a fresh game clear it.

Explosion is complete. Action 42 pays the ordinary targetless command cost
and waits for the chart-11 marker before handing the clicked walkable point to
the owned companion. The companion plays its PARTNER chart-six departure,
relocates at the chart-seven boundary, then runs the authored two-layer burst,
samples, camera shake, 640-by-640 enemy check, retail packet, and spell
practice before returning to ordinary AI. The original spell-21 scaling-level
and owner-magical-defense damage quirks are kept deliberately.

Elemental Strike is complete. Action 43 returns to the pointed-enemy command
and charts 13 and 14, then queues effect 10021 immediately with the first
chart-13 `0x40` frame converted into its launch delay. Its family-zero packet
uses the player's magical attack, magical defense, hit rate, affinities, and
spell-21 tables, while the effect request keeps retail's zero display-height
offset, null source judgement, random ordinary impact, and effective level.
The shared controller uses Table 207 to launch one, three, or five homing rays
and tracks each ray through its four timed elemental stages, collision,
presentations, samples, final camera shake, and receiver-time practice. The
same controller remains shared with the already proven enemy-owned cast.

The portable testing path now has its own F12 debug menu instead of changing
the retail Escape menu. The FPS counter, All Spells, Infinite HP, and Infinite
MP overrides are separate runtime-only switches. Debug availability, bar
assignments, and effective resource pools never enter the retail save record,
so testing unfinished combat cannot silently change a character.

That completes the retail player's 22 ordinary spell actions.

Increased Power and its Hunter ranged redirect are complete too. Direct local
kills charge the runtime-only state to 50, or to 30 while special item
`98000001` is owned. P and the authored HUD cell both activate 900 updates,
reset the charge without consuming that item, force movement speed tier nine,
add two effective spell levels, raise the receiver's defense input by 20
percent, block Moon, Berserker, and Energy Shield, loop the common Powerup
aura, and play sample 76 every 15 updates. The readiness cell uses retail's
two-frame 1000/800 pulse. Ordinary travel preserves the state; death clears
the active timer, and neither charge nor time is written to the character
save.

While active, Hunter action 20 consumes one 33-percent random roll at action
entry. A successful roll snapshots at most 100 living enemies whose judgement
intersects the player's 4000-by-4000 square and enters action 21 on the same
chart-ten timing. With no captured target it falls back to ordinary action 20
without rerolling. Every crossed marker builds the one physical packet with
presentation 20006 and launches an independently delayed effect-9000 diagonal
strike at each captured character number. The complete volley costs one
main-hand durability and keeps sample 3 at counter six.

Land Mines are complete as the remaining player combat shortcut. `B` and the
authored HUD cell spend the separate saved counter and place static OPTION
resource 1000 at the hero after the ten-update lockout check. Counter 40 arms
enemy-and-scenario-object collision and starts sample-54 warning beeps; contact
or the 300-update lifetime advances into the Table-23 area packet, resource
1001, sample 29, expanding 1002..1004 rings, and paired 1005..1008 bouncing
debris. Mine pickups, the equipment-derived capacity and damage bonuses, map
transition cleanup, and the exact post-magic retail save field share the
existing item, receiver, renderer, and save owners rather than bypassing them.

## Completed foundation: make Remote Town feel like a game

This slice touched nearly every piece the rest of gameplay will need: input,
world coordinates, actor state, animation, collision, camera movement, and
depth-sorted rendering.

The first goal was simply to make a new character walk around Remote Town as
the original game does. That work grew into the common movement, interaction,
and display-order foundation used by the player, PEOPLE actors, ground items,
and scenery. The HUD, inventory, and ordinary combat milestones described
later now build on that foundation.

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
and return directly to the new world once its synchronous load is complete.
An earlier pass had mistaken the Epilogue/`VisualNN` presenter at `0x00417bd0`
for the ordinary map loader and held it on screen for 120 frames. Retail only
shows its black crossed-swords loading image while work is actually pending;
the current synchronous portable load is normally too quick to expose an
intermediate frame.

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
draw ordering. The attached death presentation plays its retail gender voice
once on the first action update: sample 13 for male and sample 14 for female.

The owned companion uses a third receiver at retail address `0x0045f9f0`,
not either of those paths. Its family-one profile, owner-slot life mutation,
actions 7/8/10 rejection, tables 24 and 25 reaction, action-five hit stages,
action-six death, distinct effect owner kinds, sample 119, event four, and
random draws are reconstructed separately. All three passive calculations are
attached to their own live actors. Companion action five uses PARTNER chart
three and its collision-aware impulse; action six uses chart four direction
eight, creates effect 21010, holds its final frame, and fades over 60 updates.
The persistent countdown then starts action eight at the player's position,
restores maximum life, and plays chart seven direction eight before ordinary
AI resumes.

Enemy kill accounting also preserves the companion side of `0x004134a0`.
Eligible owner or companion kills add one point, table `800 + type` row 18
provides each threshold, and the cap is `player level / 3 + 2` up to 35. The
kill point is awarded before player leveling while companion thresholds are
applied afterward, so a shared leveling kill uses retail's new player-level
cap.

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
- identify the story/briefing owner that requests the Epilogue and alternate
  `VisualNN.njp` artwork rendered by `0x00417bd0`;
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

## Completed foundation: draw the gameplay HUD and world-pointer feedback

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
When a left-hand panel, right-hand panel, or both are visible, Escape closes
that complete gameplay-panel set and restores the centered camera instead.
Only another press with no gameplay panel left open reaches Settings.
Save and Return and Save and Exit now open their original confirmation states,
write the retail save envelope, and only leave gameplay after a successful
write. Their deferred transition runs before the next UI update can consume
more input. With `Save Image at Game End` enabled, they also write the paired
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

The matching discovery path is live too. Periodic status-kind-5 sentences use
opcode 34 to measure the hero against each teleporter's hidden activation
object. Overlap enables that scenario's Table 40 row, immediately adds it to
the compact transport list, and survives the normal save/load round trip.

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
quest states directly and gets its text from the retail parameter tables. The
short-lived notice now uses the recovered bottom-right coordinates, brackets,
shadow, exact clickable title rectangle, and the script's samples 65 and 66;
clicking it opens the Mission List. The persistent StatusIcon lock shortcut is
drawn and clickable while any quest remains active. Syria's subsequent
interactions now read the real type-12 quest owner and follow her normal
healing/blessing branch. Near Remote Town's authored status-kind-four callback
also completes quest zero after Red Goblin `14010000` finishes its death
presentation.

Malse's next authored branch is live too. Completing the Red Goblin quest is
what advances his script into the merchant introduction and later service
menu; the engine does not special-case his name or quest ID. Scenario status
kind 7 and opcode 6 now build numbered vendor stock from Tables 32 and 33,
including fixed entries, weighted random definitions, normal rolled item
instances, and the original 9-by-10 placement starts. Choosing `Trade` reaches
opcode 5, opens that stock on the left with the normal inventory on the right,
and supports buying, selling, gold changes, delayed price overlays, item audio,
and returning an unfinished purchase when the panels close. Identify and
Repair remain separate service slices behind Malse's other authored choices.

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
- shops, prices, and money when the script layer requests them;

The opening quest's four real ground items are now loaded and drawn from
`Item.Ibn`. Pointer selection and the first complete pickup path are live:
distant clicks approach through the shared movement controller, then ownership
moves into `PlayerInventory` before the world entity is erased. Gold fills
existing stacks up to the retail 10,000 limit. Scripted and player-created
drops also play their category-specific retail sound on the first ground
impact. If a clicked item cannot fit in the backpack, it stays on the ground
and visibly repeats that two-bounce drop and first-impact sound, matching the
retail failed-pickup feedback.

The real 9-by-4 backpack grid is now in place. Width and height come from
`Item.Ibn`, placement respects multi-cell footprints, and a full inventory
rejects a pickup without losing or partly inserting it; the retained world
item restarts its drop presentation instead of appearing unresponsive. The
authored right
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
only when it actually changes the target, matching the executable. A
right-click uses those medicines directly from either the backpack or belt;
full life or mana leaves the corresponding item untouched.

A new character now receives the loadout built by `0x00440f70`: Leather Cloth
in the body slot, four Tablets and four Capsules in the first two backpack
columns, the same four-plus-four medicine layout in the belt, and five mines
in the player's separate mine counter. Moving, swapping, equipping, and
dropping owned items also use the retail category/weight sound selection;
successful medicine use plays its own sound.

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
the wide Price row rather than collapsing to a name-only tooltip. The other
small-item branches now follow `0x00409a60` as well: category-two items show
weight, required level, and sale price, while category-three consumables such
as Tablet show sale price. Their one-cell icons therefore keep the original
wide information panel.

The retail condition warning is now shared by backpack, equipment, and held
items. Weapons and armor below ten percent durability blink `Status.njp`
pattern 16 for eight updates on and eight off; broken gear keeps it visible.
The player-life and player-mana fields of category-three records are decoded.
Land Mines now use their retail path rather than pretending to be backpack
items or magic. `B` and the exact HUD cell spend the separate counter, obey the
ten-update placement lockout, and create static resource 1000 at the hero.
The mine arms at update 40, beeps every 20 updates, reacts to enemy and active
scenario-object contact, and expires at update 300. Its Table-23 area packet,
sample 29, resource-1001 explosion, expanding 1002..1004 rings, and paired
1005..1008 bouncing debris run through their own small controller and the
shared receiver/depth-sort boundaries. Mine pickups fill the counter up to the
equipment-derived maximum before falling back to ordinary backpack pickup.
The base count, instance-word 84 capacity bonus, instance-word 81 damage
bonus, and post-magic retail save field are all owned and tested, including
older sparse portable saves.

Companion restoration, timed/status effects, and the script-facing
special-item commands remain the next item checkpoint. Those paths should
keep using these owners rather than adding parallel inventory models.

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
The dispatcher's opening target search is part of that loop too: living
enemies only run autonomous AID work while a living target is within 5000
judgement units. Enemies outside that range return to idle instead of marching
through the same patrol cycle off-screen. A blocked movement request may keep
its obstacle-following state, but the renderer only shows the walk chart on an
update that actually changed the actor's position.

Direct enemy impacts now pass through the reconstructed player receiver. The
live receiver snapshot uses the named player base rows and matching equipment
contributions, including row eight and item parameter six for magical defense.
Returned life, mana, backpack, equipment, Special Items, reaction state,
durability changes, effects, reflection, and audio are committed in retail
order; the separate belt remains untouched. Player actions four and five show
the hit and death CAF presentations and interrupt movement or attacks. A live
regression waits for a Wasteland enemy to damage the player, requires its hit
sound and ordinary impact splatter, then saves and reloads the damaged
character to make sure no owned items are lost as an accidental side effect.

The next combat slices should finish enemy effect attacks. Actions four through
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

The common target half is reconstructed now too. The actor queries its current
position before movement, preserves display-query order, handles all five mask
families, chooses either every eligible target or the first nearest target,
and keeps the retail 500-identity repeat-hit list. Physical and magical
evasion consume the portable Visual C++ random stream in retail order, misses
remain remembered, and typed receiver and positional-audio requests retain
the original once-per-update sound guard and NPC mode.

The live owner for types 1 and 2 is complete now. It builds current player,
object, PEOPLE, and enemy snapshots, resolves the controller's source again on
each update, and keeps controller and category-50000000 actor lifetimes
separate. Existing actors move and collide before controllers run, so a newly
created source or projectile waits for the following actor update just as it
does in retail. The renderer uses the actor's CAF, direction, frame, depth,
and tenths-of-a-pixel height rather than treating the request as a flat hit
animation.

Successful contacts pass the original packet into the live player or enemy
receiver and queue the configured positional sound. Misses create effect
20012 instead: its static OPTION 11000011 pattern follows the original
three-bounce height controller and ten-step opacity fade. A shipped
`03000507` enemy regression proves type 2's source and forward resources,
samples 94 and 20, player damage, rendering, independent cleanup, unchanged
starter items, and save/reload ownership. A separate low-hit live case proves
the MISS path. Types 1 and 2 can therefore be treated as complete.

Type 3 is complete now as well. Plasma Bat subtype 20 reads five waves from
Table 205, expands them every four updates from the stored impact origin,
checks each 100-unit placement against the map and live scenario objects, and
permanently stops after the first blocked position. Every clear wave creates
the retail `10000030/31/32` layers and sample 21; only the random-chart first
layer applies the packet on update zero. Scenario `00010001` provides the
shipped live regression for rendering, player damage, controller cleanup, and
unchanged item ownership.

Type 4 is complete too. Its moving-source warning appears on update three as
resource `10000002` in retail display class two. At the authored update-ten
burst, the source is resolved again and resource `10000000` is created twice:
chart one at the upper judgement corner and chart zero at the lower corner,
with both lifetimes taken from chart one. Samples 29 and 23 play together, an
invisible one-update actor applies the copied packet across the source bounds
expanded by 150 and plays contact sample 20. A player within 3001 world units
receives the exact eight-update, six-pixel alternating vertical camera jolt.
Scenario `01000004` provides the shipped live render, damage, launch and
contact audio, camera, and ownership case.
Type 5 is complete now. Update three captures the moving source once and
creates resource `10000051`; the rest of the sequence stays at that position.
The first resource's real chart-zero frame count schedules resource
`10000050`, the area packet four updates later, resource `10000052` another
eleven updates after that, and controller expiry at frame-count plus 22. The
area packet shares type 4's 150-unit expansion, contact sample 20, and nearby
camera jolt. Six sample-22 pulses land at offsets 6, 9, 12, 15, 18, and 21.
The final visual uses retail display class two. Enemy 48 in scenario
`04060004` supplies the shipped live case and proves all three visuals,
damage, launch and contact audio, cleanup, and unchanged item ownership.

Type 10 is complete too. Table 206 supplies five waves for shipped subtype
20. Starting at its authored delay, the controller projects one attempt every
eight updates from the stored impact point, beginning 250 units out and adding
300 units each time. Each clear 150-unit placement creates resource
`10000060`, applies the copied packet to every overlapping target on update
zero, plays sample 22, and requests the familiar eight-by-six camera shake
when the player is within the strict 3001-unit range. The first blocked
placement permanently suppresses that and every later wave, while the
controller still runs through its complete Table 206 timeline. Enemy 26 in
scenario `04060004` provides the shipped live render, damage, audio, camera,
blocked-tail, cleanup, and item-identity regression.

Type 11 is complete now. It creates resource `10000012` at the source on
update zero, then reads Table 204 at the authored delay and distributes two
through eight resource-`10000010` actors around retail's slightly truncated
full circle. Live owners place each actor 180 units out; fixed-origin owners
start every actor at the supplied point. The children keep the unusual
`[-80,-80,79,79]` bounds, 90-update lifetime, contact sample 20, static
collision expiry, copied packet, and homing movement mode with its
20-degree-per-update turn limit. Sample 19 plays once at the last child's
spawn position before the controller expires.

The common runtime actor now owns that homing behavior. It looks up the live
player or scenario target, turns by the shortest retail angle, moves from its
current position instead of replaying a line from its start, updates its CAF
direction, stops steering after it passes the target, and permanently falls
back to straight travel when the target disappears. A shipped subtype-ten
enemy from Tower of Ordeal scenario 15 supplies the live source, radial
render, impact, audio, lifetime, and item-identity regression.

Type 12 is complete now. It creates resource `11000027` at the source and a
Table 204 warning fan on update zero. The warning actors use resource
`10000080`, sit 150 units from a live owner, and retain the subtype as their
lifetime. At delay ten the controller re-resolves the owner and emits the
same fan at radius 180 using moving resource `10000081`.
Those projectiles live for 90 updates, expire on static or target contact,
carry sample 20, and rewrite packet pairs 34/35 and 74/75 to the retail
directional effects `21021` and `21022`. Sample 94 plays once at the final
projectile. A shipped subtype-ten Dread Wisp from `North of The Remains of
The Dead` (`03010003`) supplies the live warning, launch, render, damage,
audio, cleanup, and item-identity regression.

Type 13 is complete now. Table 204 supplies the number of points in each
shell. Beginning at the authored delay, the controller emits four shells,
four updates apart, at radii 350, 550, 750, and 950 around the fixed impact
origin. Every clear point creates the same three resources as type 3:
`10000030`, `10000031`, and `10000032`. Only the first layer uses a random
chart and applies the copied packet; sample 21 plays once per shell at its
last radial point.

Unlike the earlier one-direction wave controllers, every radial direction
has its own obstruction latch. A blocked ray remains suppressed in later
shells without stopping the others. Lightning Gargoyle 11 in Ancient Ruins
B1F (`03140000`) supplies the shipped subtype-20 live render, damage, audio,
cleanup, and item-identity regression.

Type 14 is complete now. It has no source or warning visual. At the authored
delay it resolves a live owner and launches resource `10000070` from 180
units along the stored angle. The projectile keeps the familiar 50-unit
bounds, authored speed and display height, copied packet, static and first
target expiry, and contact sample 20. Sample 22 plays at launch and the
controller ends immediately.

Owner kind zero is the deliberate exception: it starts directly at the
supplied origin without the 180-unit projection. Rechecking that branch also
fixed the same old edge case in types 1 and 2. Stone Wisp 2 in Ancient Ruins
B1F (`03140000`) supplies the shipped subtype-one live render, packet, launch
and impact audio, damage, cleanup, and item-identity regression.

Type 16 is complete now. At the authored delay it launches resource
`10000110`, stores that actor's real runtime identity, and follows its current
position while it remains in the world. The projectile uses 80-unit bounds,
the authored speed and display height, static and first-target expiry, contact
sample 20, and launch sample 19.

Once the projectile is removed, the controller creates resource `10000111`
at its last recorded position and ends. This second actor uses 240-unit
bounds, its full chart-zero lifetime, and applies the copied packet to every
overlapping target only on update five. Sample 22 plays when it appears, and
a player within 3000 units receives the usual eight-update, six-pixel camera
jolt. Goliate's second effect variant in Goliate's Mansion B3F (`04050002`)
supplies the shipped live projectile, follow-up position, render, damage,
audio, camera, cleanup, and item-identity regression.

Type 21 completes the specialized enemy-effect controllers. Update zero
creates source resource `11000210`. At the authored delay, Table 207 chooses
one, three, or five evenly spaced resource-`10000100` rays. A live owner
launches each ray 180 units out; owner kind zero keeps the explicit origin.
The rays use 80-unit bounds, twenty-degree homing turns, speed-scaled
animation, their full chart-zero lifetime, static and first-target expiry,
contact sample 20, and one sample 19 at the final launch point.

The controller tracks every ray separately. Once a ray disappears, its saved
position advances through four stages at four-update intervals. Resources
`12000000`, `11000033`, `10000030`, and `10000060` rewrite packet words 32
and 34 to `{0,20000}`, `{1,21013}`, `{2,20005}`, and `{3,21000}`. The odd
stages are visual; the even stages apply their packet to every overlapping
target on update zero. Stage three also creates the matching `10000031` and
`10000032` visual layers. Every stage plays sample 19. The last also plays
sample 22 and requests the nearby eight-update, six-pixel camera jolt.

Arc Angel's third attack in scenario `99000036` supplies the shipped
subtype-30 five-ray regression. It covers the source, launch, independent
tracking, all four timed stages, three-layer render, both damage windows,
audio, camera motion, controller cleanup, and adjacent item ownership.

The first half of the next player-visible checkpoint is complete. Remote
Town's invisible south-gate object uses status kind three and the retail
inclusive rectangle check; its sentence calls opcode 17 with scenario 1 and
entry zero. Walking through it now shows the existing loading presentation
and enters `Near the Remote Town` at `(90581,5288)`, facing 7, with music
track 1, its map collision, 48 objects, and all 127 enemies. The matching
outdoor trigger returns through `{0,0}` to Remote Town's original entry,
camera, and music. A live regression walks both directions and checks that
each crossing publishes one loading transition.

That player-visible pass is complete too. The first authored target outside
the gate is local enemy 101, the level-one Goblin with 40 life, one experience
point, loot row zero, and its 10-percent roll for 10 through 20 Gold. A live
regression equips Ostare's actual Short Sword, approaches that exact actor
through the outdoor map, and keeps every click in the normal pointer, movement,
CAF attack, receiver, reaction, and death paths. It requires the hit and death
effects, swing and impact sounds, experience and kill credit, the authored
drop, its bounce and landing sound, and normal approach-and-pickup behavior.
Ground pickup now also queues selector zero's retail item-move sound instead
of silently moving the item into the backpack. The resulting inventory,
experience, kill count, and equipped sword are written through the `.Ssv`
owner and checked again after a real reload. Gold's separate chance, amount,
Gold Find, stacking, and save path remain covered by their deterministic
reward and owner tests because a faithful 10-percent roll must not be forced
to succeed in this live encounter.

The standard ranged-player path is complete now. A shipped Wood Bowgun starts
action 20 at any pointer-visible enemy distance, holds the player in CAF chart
10, launches on its authored frame-three marker, and plays sample 3 at counter
six. Ranged animation uses its own ten speed factors from 0.3 through 2.0 and
ends on chart 10's final frame without a melee recovery chart.

Item.Ibn drives the projectile rather than a weapon-name switch. Its effect
selector chooses generic actor type 1, 0, 4, or 5; its pattern selects the
straight or homing one-, two-, three-, five-, or seven-shot layout; and its
remaining fields provide travel speed and piercing. Double Bowgun creates two
explicit launch points at minus and plus eight degrees while keeping the shots
parallel. The wider fans use the original 8-, 10-, or 15-degree spacing.

These projectiles enter the same category-50000000 actor system as enemy
effects, with retail bounds, projected or explicit origins, target mask,
environment collision, evasion, sample 20, receiver packet, homing, piercing
memory, and first-target expiry. The family-zero packet preserves player
attribution even for explicit-origin shots. Job-five history supplies the
off-job physical-attack scale, and a complete fan costs one point of main-hand
durability. Passive tests cover every pattern, effect selector, contact, and
receiver handoff; a shipped live encounter covers the CAF, projectile render
owner, launch audio, no-approach targeting, and durability without requiring
a straight projectile to hit an enemy moving around the companion.

Retail contains no subtype-four weapon record to exercise action 19. That
missing authored fixture remains recorded rather than filling action 19 with
made-up projectile behavior; action 20 and its Increased-Power action-21
redirect are both live.

The next combat work belongs to skills, magic, and status effects, keeping one
shipped live case beside each passive reconstruction and sharing the existing
receiver, effect, reward, audio, and persistence owners.

The owned-companion foundation is complete. `0x004501c0` creates character
`16000000 + player slot` at the player's scenario entry and takes its name,
PARTNER resource, and RGB strengths from Table 60. Tables 800 through 805 are
summed through the saved companion level for movement and combat values.
Kerberos therefore starts with the retail level-one profile rather than a
copy of a town NPC.

The default follow half of `0x004622b0` is live too. A companion idles inside
160 judgement units, waits five updates before leaving that close band, walks
below 600, runs at 600 or farther, and snaps to the player plus `(200,200)`
only at 4000 or farther. It uses PARTNER charts zero, one, and two, routes
through the common movement owner, participates in normal depth sorting and
actor collision, and is relocated with the player on scenario changes. Its
1200-unit living-enemy search, attack-mode approach, chart-five marker timing,
enemy receiver handoff, damage lifecycle, and progression are now live as
separate companion concerns rather than shortcuts in the follower.

A fidelity cleanup now protects that checkpoint too. The first Goblin must
acquire and attack a passive player, continue retaliating after being struck,
and play the gender-specific hero voice on ordinary weapon impacts. Death
disables the enemy judgement rectangle immediately, before the corpse has
finished fading. Table 31 now uses its real category, loot-level bounds, fixed
definition, and episode fields, so a level-one encounter cannot manufacture
late-game equipment through a column mix-up.

The nearby outdoor containers run their shipped scripts all the way through
the positional opening sound and Table 30 loot command. Level gains publish
the original changed-stat text for 900 updates and play their retail samples.
The notice starts centered with its black translucent fill and white frame,
slides to the upper-right after 840 updates, and consumes a dismissing click
only after its first 30 updates. Ordinary `21000..21003` impact splatters play
on every hit, while the separate bloody `21010` presentation remains tied to
death and keeps its own 120-update hold-and-fade lifetime.
The Warehouse and map remain left-side owners while Inventory remains an
independent right-side owner; opening both keeps the world camera centered and
allows items to move between the Warehouse and backpack.

### 5. Skills, magic, status, and the remaining game screens

Once the ordinary combat loop is reliable, add the systems that modify it:

- spell save ownership and the four-page Magic window are complete;
- the eight-slot drag bar, persistent HUD selection, and normal-attack toggle
  are complete;
- one faithful Fire Ball cast from selection through impact and spell
  training is complete;
- normal-target right-click now runs the retail three-stage one-handed and
  two-handed melee combos, including their separate voices and forward steps;
- Transport creates its collision-checked paired field/Remote Town endpoints,
  staggered portal presentation, sounds, and two-way scenario travel;
- Fire Ball and Ice Bolt prove the reusable single-target projectile spell
  dispatch;
- Plasma's action-25 multi-wave area-effect path is complete;
- the ground/self casting command and Hell Fire action 26 are complete;
- Ice Blast action 27 and effect 10005 are complete;
- Heal action 28 and its marker-time restorative path are complete;
- Moon action 29, its companion modifiers, aura, and MP lifetime are complete;
- Berserker action 30 through Elemental Strike action 43 are complete, which
  closes the 22-spell player list;
- skill and spell databases beyond the proven table-backed spell values;
- mana use, cooldowns, targeting, projectiles, and area effects;
- buffs, debuffs, resistances, reflection, and absorption;
- the character Status tab and its detailed derived values are complete;
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
unknown payload. New saves carry the player record and owned items we currently
own. The three counted retail arrays after the item stream now preserve
type-12 quest state, type-10 transport unlocks, and type-11 general script
state; this is
covered by saving after Ostare's opening and proving his starter reward does
not repeat after loading. Walk/run also survives a portable round trip through
a small versioned tail until its exact retail save owner is identified.
Writing captures the retail-sized paired preview from the world-only render
when the option is enabled. Saving is not complete yet: the remaining
persistent gameplay owners still have to contribute their real payload fields
before OpenShadowFlare can claim full round-trip compatibility.

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
