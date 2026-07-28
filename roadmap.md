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

In other words, the game can reach the world, but the world is not alive yet.
The player cannot walk, there are no NPCs or scripts, and the HUD and gameplay
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

Finally, timing-sensitive game logic should stay based on the original 60 Hz
update. Rendering and window presentation must not decide how quickly the
simulation runs.

## Up next: make the player walk

This is the best next slice. It is small enough to finish and compare, but it
touches nearly every piece the rest of gameplay will need: input, world
coordinates, actor state, animation, collision, camera movement, and
depth-sorted rendering.

The first goal is deliberately narrow: walk the new character around Remote
Town as the original game does. No combat, NPCs, HUD, inventory, or scenario
changes yet.

### First, study the retail path

Before writing the movement code, trace one complete movement command through
the original executable:

- how a screen click becomes a world-space destination
- which mouse buttons and modifier keys create or cancel movement
- how the click-range settings affect the command
- how the player chooses one of the movement directions
- the exact movement speed and whether it changes by direction
- when the idle CAF chart switches to walking and back
- how frame advancement behaves at 60 Hz
- how ground and object judgement data stop movement
- whether blocked movement slides along an obstacle or stops outright
- how the camera follows the player and clamps at map edges
- when the player's depth key is rebuilt during movement

This work should add names and notes to the reverse maps instead of living only
in somebody's scratchpad.

### Then build the smallest useful actor loop

The portable side needs a player actor with clear, ordinary state: current
position, destination, direction, motion state, animation chart, animation
frame, and the few counters the retail loop actually uses.

The gameplay update should:

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

### How we will know it is done

The slice is finished when all of these are true:

- clicking several places in Remote Town produces the same destination and
  direction as the retail game;
- movement distance over a fixed number of 60 Hz updates matches;
- idle and walking frame sequences match in every direction;
- walls, gates, scenery, and map edges block the player correctly;
- the camera follows and clamps without visible jumps;
- scenery continues to sort in front of and behind the moving player;
- releasing, replacing, and issuing an unreachable movement command behaves
  like the original;
- the behavior has deterministic native tests and a short side-by-side Wine
  comparison;
- the executable smoke test still reaches the world.

Completing this gives us the first genuinely interactive gameplay milestone:
we can walk around Remote Town.

## What comes after movement

The order below follows dependencies. Each heading is still meant to become
several small commits rather than one giant implementation.

### 1. Turn the initial map loader into a real scenario loader

Remote Town currently works as a carefully reconstructed first case. The next
step is to remove the assumptions that only scenario `00000000` and map
`f00_01` exist.

We need to finish the general MCT path around `0x00427b50` and the scenario
transition path around `0x00426200`:

- decode the complete MCED/MCT header and entry tables;
- select maps, entry points, direction, music, and resource lists from data;
- load GND, OBL, LST, NJP, SDW, and CAF resources through reusable code;
- preserve the original pattern-number relationships across those files;
- represent dynamic entities separately from static OBL scenery;
- release the old scenario in the same order the original does;
- reconstruct the later `VisualNN.njp`/`WaitIcon.njp` loading path at
  `0x00417bd0`;
- support returning to the title cleanly when loading fails.

Remote Town should then be one input to the loader, not a special hard-coded
world.

### 2. Reconstruct the player data used during gameplay

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

### 3. Draw the gameplay HUD and cursor

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

### 4. Populate the town

The next visible milestone is a Remote Town with its original NPCs and other
dynamic objects.

This will require the first portable slices of `RKC_RPG_AICONTROL`,
`RKC_RPG_TABLE`, and more of `RKC_RPGSCRN`:

- create actors from the scenario entity records;
- load their animation, palette, and judgement data;
- reproduce idle animation and facing;
- place actors in the same shadow and visible-object passes as the player;
- add actor-to-world and actor-to-actor collision;
- reproduce the original update order and off-screen behavior;
- add pointer hover, selection, names, and interaction range;
- verify the town population and positions against the retail game.

Start with one well-understood NPC, then generalize. Loading every record at
once before one actor behaves correctly will only make mistakes harder to see.

### 5. Bring up scripts, conversations, and town interaction

NPCs become useful when the scenario script can drive them. The reconstructed
`RKC_RPG_SCRIPT` DLL is the reference here; the portable version belongs in
`src/SF_EXE/libs/RKC_RPG_SCRIPT/`.

The script work should grow from real Remote Town interactions:

- load the compiled `.Scs` data;
- recreate script variables, temporary flags, and persistent flags;
- implement the interpreter loop at `0x00430f80` one exercised opcode at a
  time;
- map the large command dispatcher at `0x00429ec0` into smaller named actions;
- support conversations, choices, messages, gates, warps, and quest flags;
- add shops and services when their scripts first require them;
- preserve wait states and update ordering instead of running a whole script
  in one frame;
- save unknown opcodes and data instead of silently discarding them.

The first target should be one complete conversation or town service that can
be followed from click to visible result.

### 6. Items, inventory, and equipment

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

A good first checkpoint is equipping one real item and seeing both the correct
stat change and the correct player artwork.

### 7. Combat and death

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

### 8. Skills, magic, status, and the remaining game screens

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

### 9. Finish save and load

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

### 10. Play through Episode 1

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

### 11. Cover the remaining episodes

Once Episode 1 is solid, run the same process through Episodes 2–4. Most of the
engine should already exist by then, but later content will expose less common
script commands, AI actions, effects, items, and map combinations.

Keep fixes general. If a later map needs a special case, first prove that the
original really has one.

### 12. Multiplayer

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
- keep the 60 Hz simulation independent from presentation speed;
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
