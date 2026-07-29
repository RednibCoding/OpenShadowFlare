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

In other words, the game can reach the world and the player can now walk
around it. Ostare is the first NPC reconstructed from scenario data, including
his idle wandering, hover state, speech bubbles, and complete opening
conversation. The script engine is still small, and the HUD and most gameplay
systems are still missing.

The current reverse-engineering notes live in:

- `src/SF_EXE/reverse/functions.csv`
- `src/SF_EXE/reverse/globals.csv`
- `src/SF_EXE/reverse/status.md`
- `documentation/exe-analysis.md`

Those files should be updated whenever a slice teaches us something new. Raw
decompiler output can stay in `ghidra/`; only understood behavior belongs in
the portable source.

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

## Last completed milestone: make the player walk

This slice touched nearly every piece the rest of gameplay will need: input,
world coordinates, actor state, animation, collision, camera movement, and
depth-sorted rendering.

The deliberately narrow goal was to walk the new character around Remote Town
as the original game does. No combat, NPCs, HUD, inventory, or scenario
changes are part of this milestone.

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
- `0x00454930` keeps contact position and tries axis slides;
- the camera uses the projected player position minus the 320-by-240 screen
  center, without another map-edge clamp.

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
- static ground and object collision stop at the last walkable integer point;
- camera and depth keys are rebuilt from the live player position;
- held input replaces the destination and out-of-window input is rejected;
- a native live run reaches Remote Town and moves the camera and player from a
  ground click.

This gives us the first genuinely interactive gameplay milestone: we can walk
around Remote Town. Any collision corner that looks different in a
side-by-side retail check should be kept as a small movement follow-up rather
than worked around in later actor code.

## Current milestone: turn the first map into a real scenario loader

The order below follows dependencies. Each heading is still meant to become
several small commits rather than one giant implementation.

Remote Town began as a carefully reconstructed first case. The first loader
slice now reads `Scenario/00000000/Scenario.Mct` itself instead of embedding
its map name, entry position, facing direction, title, and music index in the
code. The loader understands the fixed MCED header and the trailing entry-point
table, and tests those fields against the retail file.

The next slice identified the leading ID lists, the variable common entity
record, and the complete `PEOPLE` group shape. Remote Town's seven people
records now decode, and Ostare is loaded as the first portable NPC with his
retail resource ID, name, position, judgement box, direction, custom CAF part
mask, shadow, and idle animation. We are intentionally loading one person
until that path is solid instead of switching on all seven at once.

Ostare's first type-one behavior is covered too. The people tail gives him a
30-update idle pause, a 30-update walking limit, speed 10, and a small
spawn-relative movement rectangle. He now alternates chart-zero idling with a
short chart-one walk to a random point inside those bounds, as the original
update path does.

This is still not a complete MCT decoder. Later object, enemy, item, partner,
and option groups remain to be named and mapped, as do the last two
people-specific fields and the more involved AI paths.

We need to finish the general MCT path around `0x00427b50` and the scenario
transition path around `0x00426200`:

- identify and decode the remaining variable entity groups in the MCT;
- connect the remaining people fields to portable AI and interaction state;
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

## What follows the scenario loader

### 1. Reconstruct the player data used during gameplay

The menu currently carries only enough character information to enter the
world. Gameplay needs the real character record:

- class, gender, level, experience, life, and mana;
- base and derived attributes;
- current equipment and appearance selections;
- action, target, and status flags;
- the initial values used for a newly created character;
- the matching fields in an existing `.Ssv` save.

This is where we should start mapping the large save/load routines at
`0x0044b580` and `0x0044cac0`, even though full save writing can wait. Knowing
the real stored model early will keep us from inventing a temporary player
structure that has to be thrown away later.

### 2. Draw the gameplay HUD and cursor

Once the player has real values, the main interface can display something
meaningful. Reconstruct it in layers:

- fixed HUD artwork and screen-space layout;
- life, mana, experience, level, and quick-slot values;
- the gameplay cursor and its different interaction states;
- clicked-ground and selected-target feedback;
- message and help text;
- darkness and other final world overlays;
- resizing behavior through the fixed 640×480 GAPI surface.

HUD coordinates and visibility rules should come from the retail draw packets.
The HUD must remain separate from the world camera and must not be baked into
the world renderer.

### 3. Populate the town

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
- add actor-to-world and actor-to-actor collision;
- reproduce the original update order and off-screen behavior;
- extend the reconstructed pointer hover, pale actor tint, nameplates, and
  selection path from Ostare to every dynamic actor and the retail interaction
  range;
- verify the town population and positions against the retail game.

That path now builds all seven Remote Town people records and resolves their
shared or individual `Character/PEOPLE` resources from the MCT table. Ostare's
part mask, idle pause, bounded walk, shadow, depth pass, hover tint, nameplate,
and actor-anchored speech bubble remain the detailed reference case. Malse and
Syria are selectable and run their real first conversations. The next town
work should map the behavior that differs between the three human NPCs and the
four animals, then add dynamic actor collision and the remaining pointer
selection rules.

### 4. Bring up scripts, conversations, and town interaction

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
which do not run an explicit facing command. The notice consumer and cue audio
are still pending; neither has been guessed.

### 5. Items, inventory, and equipment

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
`Item.Ibn`. The next checkpoint is retail pointer selection and pickup for one
of them. Equipping that item and seeing both the correct stat change and
player artwork comes after that.

### 6. Combat and death

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

### 7. Skills, magic, status, and the remaining game screens

Once the ordinary combat loop is reliable, add the systems that modify it:

- skill and spell databases;
- mana use, cooldowns, targeting, projectiles, and area effects;
- buffs, debuffs, resistances, reflection, and absorption;
- character status and detailed stat panels;
- skill assignment and quick slots;
- journal, map, options, and the remaining modal screens;
- the in-game sound, display, input, and gameplay settings.

The large UI functions should be split by screen and concern in the portable
code even when the original compiler emitted one enormous function.

### 8. Finish save and load

Save support should preserve the retail format so original characters remain
usable and saves written by OpenShadowFlare can be opened by the original
game.

That means:

- map the full `.Ssv` structure and its version checks;
- reproduce the XOR encryption and the retail validation behavior;
- restore scenario, position, player stats, inventory, equipment, skills,
  quest flags, and other persistent state;
- write the paired preview bitmap at the correct moment;
- preserve unknown bytes when the original does;
- handle truncated or corrupt saves without losing another slot;
- compare round trips byte for byte where the original format permits it;
- test original → OpenShadowFlare and OpenShadowFlare → original.

Save parsing can grow alongside earlier slices, but writing should only be
declared complete once all persistent gameplay systems have a real owner.

### 9. Play through Episode 1

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

### 10. Cover the remaining episodes

Once Episode 1 is solid, run the same process through Episodes 2–4. Most of the
engine should already exist by then, but later content will expose less common
script commands, AI actions, effects, items, and map combinations.

Keep fixes general. If a later map needs a special case, first prove that the
original really has one.

### 11. Multiplayer

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
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
./cmake-build-debug/src/SF_EXE/ShadowFlare_rebuilt --smoke-test
./tests/run.sh
```

Use `./tests/run.sh --wine` when a change touches the compatibility DLLs or
when a differential probe is part of the slice.

Tests are not the whole fidelity argument. A gameplay slice should also have a
brief note explaining what was compared with the retail game and what is still
unknown. If we cannot say what “matching” means for a change, the slice is
probably still too large or not understood well enough.
