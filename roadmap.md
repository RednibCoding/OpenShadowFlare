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

Remote Town began as a carefully reconstructed first case. The first loader
slice now reads `Scenario/00000000/Scenario.Mct` itself instead of embedding
its map name, entry position, facing direction, title, and music index in the
code. The loader understands the fixed MCED header and the trailing entry-point
table, and tests those fields against the retail file.

The next slices identified the three resource preload lists, the variable
common entity record, and the complete object and `PEOPLE` group shapes.
Remote Town's seven objects and seven people now decode with their retail
resource IDs, names, positions, judgement boxes, directions, custom CAF part
masks, and type-specific tails. The loader follows the object, PEOPLE, enemy,
item, entry, and footer sequence directly; every one of the 209 shipped MCT
files passes that path.

Ostare's first type-one behavior is covered too. The people tail gives him a
30-update idle pause, a 30-update walking limit, speed 10, and a small
spawn-relative movement rectangle. He now alternates chart-zero idling with a
short chart-one walk to a random point inside those bounds, as the original
update path does.

This is still not a complete MCT decoder. Enemy and item tails remain to be
named and exposed, as does the last people-specific field and the more
involved runtime behavior. Partners and other runtime categories are created
outside this four-group MCT sequence and should be traced at their actual
owners.

We need to finish the general MCT path around `0x00427b50` and the scenario
transition path around `0x00426200`:

- name and expose the enemy and item tails which the exact decoder currently
  validates and skips;
- identify the final unnamed PEOPLE-tail value and connect it if it affects
  portable state;
- select arbitrary scenario IDs and entry keys during transitions;
- load GND, OBL, LST, NJP, SDW, and CAF resources through reusable code;
- preserve the original pattern-number relationships across those files;
- represent dynamic entities separately from static OBL scenery;
- release the old scenario in the same order the original does;
- reconstruct the later `VisualNN.njp`/`WaitIcon.njp` loading path at
  `0x00417bd0`;
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
`300`. Their records and 13-value tails are preserved now, including the
confirmed static-pattern/CAF choice, height, draw flags, draw strength, and
RGB strengths. The next town slice should load their `Character/OBJECT`
resources and reconstruct their type-zero update, display, collision, and
script behavior before extending live collision to enemies.

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
backpack, and the belt, including exact grid placement, Gold quantities,
durability, quality, and preserved instance bytes. Unknown equipment records,
special items, and the rest of an original save remain untouched until their
owners are reconstructed.

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
text grid, stat order, quality colors, pointer-relative position, and screen
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
