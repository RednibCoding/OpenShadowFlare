# ShadowFlare executable reconstruction

This is the portable reconstruction of `ShadowFlare.exe`. It is intentionally
separate from the fourteen Win32 compatibility DLL binaries, while using their
tested reconstructions as the behavioral reference.

The executable uses the project-owned platform libraries:

- LWL for windows, input, and timing
- LGL for the small OpenGL/OpenGL ES function set used to present a finished
  frame
- LAL for WAV/PCM playback

Game drawing goes through `gapi`, the backend-neutral graphics interface. Its
first backend is a software renderer working on a fixed 640×480 RGBA surface,
like the original game's software DIB renderer. Final display is a separate
`SurfacePresenter` boundary. Its first implementation uploads the surface
through LGL and lets the GPU scale it to the window. Maximizing the window
therefore does not turn presentation into a large CPU scaling loop. Future
Vulkan, Metal, or console presenters do not need to alter the software renderer
or game runtime.

Code in this directory must use those portable APIs and the C++ standard
library. Platform lifecycle adapters are isolated under `runtime/platform/`,
and graphics-API implementations are isolated under `runtime/presentation/`.
Native window handles and operating-system messages still belong inside LWL
or LAL rather than game code.

## Building

From the repository root:

```bash
cmake -S . -B build/linux/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/linux/debug
```

Generated builds stay under `build/<target>/<configuration>`. The normal
development executable is written to
`build/linux/debug/src/SF_EXE/ShadowFlare_rebuilt`; release, web, and future
platform builds use neighboring folders rather than new directories at the
repository root.

The development executable can be launched directly from the build folder. It
looks for `tmp/ShadowFlare` relative to its own location, so starting it from a
file manager works even when that file manager chooses a different working
directory.

The executable can now read the original `SFlare.Cfg` and run the original
top-level title/character-selection/gameplay transitions. The title and
character-select enter/leave lifecycles are reconstructed too, including their
asset manifests, save-slot behavior, input tables, random smoke delays, and
shared menu music.
The original VOC containers are decoded portably and played through LAL, with
the configured effect and BGM volumes. Gameplay's Escape menu can change those
volumes, pointer range and priority, and the other reconstructed config fields
using the retail panel and coordinates. The portable executable always opens
in a window; the old fullscreen setting is intentionally not exposed. Its
Help row and the `H` shortcut open the original mouse/keyboard reference page,
with the authored preview, text layout, and animated close tab. The
broader reconstruction order and the current slice are tracked in the
repository's [`roadmap.md`](../../roadmap.md).

The title screen's per-frame rules are connected to LWL input: keyboard
navigation, mouse hover/click regions, unavailable-item skipping, fades, audio
cues, smoke timing, and delayed New Game, Continue, and Exit actions all
follow the retail function. The original `Title.njp` is now decoded by portable
code and the title background, availability-aware menu entries, fades, and
selection highlights are visible. The ten original CAF-driven steam layers
also play at their retail pipe positions. The title cue, delayed looping music,
navigation sound, and confirmation sound use their original samples and timing.
Version text and network messages still need their drawing paths.

Character selection has its outer fade and screen/transition dispatcher as
well. New Game now shows the original male/female artwork, accepts a portable
15-byte character name, and reproduces the retail 20-frame portrait slide,
opposite-character fade, reverse animation, name field, and block caret. It
continues through the original Online/Single and New/Join/host-address screens.
Load Game reads the real save summaries and BMP previews, keeps the retail
two-column navigation and double-click timing, and supports Continue, Delete,
Back, and Exit. Delete confirmation preserves the original Yes/No keyboard and
pointer behavior and removes both the save and its preview bitmap.

Those character and mode menus are drawn by the software backend using
`Select.njp` and `Font00.njp`. Entering a single-player game now shows the
original Episode 1 loading artwork, swaps its loading label for the moving
confirmation arrow when setup is complete, and accepts Return or a click on
that arrow before handing off to the first world layer. The runtime reads
scenario `00000000`'s MCT header and entry table, uses its map path to decode
Remote Town's compressed `f00_01.Gnd` and 279-record `f00_01.Obl`, loads their
NJP/SDW pattern list, centers the camera on entry key zero, and draws the
chosen player animation among the original gates, walls, trees, and rocks.
The player uses the entry's facing direction, a separate SDW shadow, and the
same part-visibility table that keeps unequipped armor and weapons hidden.
Configured semi-transparent shadows apply to both scenery and the player.
Remote Town's MCT music index also starts the looping `BGM00.Voc` through LAL
at the configured BGM volume.

All seven dynamic `PEOPLE` records are read from the MCT rather than placed by
hand. They load their original `Character/PEOPLE` resources, positions,
directions, CAF layer masks, animations, judgement rectangles, and SDW
shadows. Their live bounds block player movement. Type-one MCT tails also drive
the original idle pause and short walks inside each actor's scenario-defined
rectangle.

Remote Town's `Scenario.Scs` is now decoded through the portable
`RKC_RPG_SCRIPT` boundary. Clicking Ostare derives his script character number
from the MCT people record, resolves the retail status trigger and sentence,
executes the initial comparisons, assignments, and actor commands, then shows
message `1000000` from the original script data. Closing each bubble fires
the actor's status-kind-one callback, taking the opening conversation through
all five retail messages. The third callback reads Ostare's current position
and creates the four original ground-item records; closing the last one
releases him and restores world control. Clicking Ostare again keeps that
script state, reads the level-one player through retail opcode 61, and follows
both messages in the original no-new-information response. The item records
are resolved through the retail `Item.Ibn` database and drawn from the
separate `Character/ITEM` CAF, NJP, and SDW ground resources—not their larger
inventory icons. They also follow the original two-bounce drop arc before
settling into the world depth pass. CAF chart palettes and the default RGB
strengths stored in `Item.Ibn` supply their original ground colors. The
format and interpreter architecture are documented in
[`documentation/script-engine.md`](../../documentation/script-engine.md).

All seven people records in Remote Town are loaded from `Scenario.Mct` now.
Malse and Syria can be selected just like Ostare and run their actual
new-game dialogue branches from `Scenario.Scs`. Syria's callback also reaches
the first quest-state commands: it starts quest zero and selects the matching
retail quest notice without putting quest IDs or dialogue into `WorldScene`.
The Mission List exposes that state through the original `Q` shortcut and
Settings-menu row. Its 48 titles and per-mission description lines come from
`Table.Tbd`; the portable screen keeps the retail two-page layout, closed and
open lock icons, selection hit boxes, colors, and detail panel.

The first world interaction is in place too. Clicking the ground moves the
player at the original gameplay cadence, follows the cursor with all eight
directions, and moves the camera with the player. `R` switches between the
retail walking and running speeds, using CAF charts one and two respectively.
Remote Town's GND judgement layer and OBL rectangles stop the player at walls
and scenery. The renderer uses the retail status classes and full judgement
rectangles—not a single Y anchor—to sort nearby objects and people in front of
or behind the moving sprite, including the large town houses and walls.
NPC-specific behavior, most script commands, the HUD, darkness, and the rest
of gameplay simulation are still in progress.

The inventory is a live panel rather than a mock-up. Items keep their original
multi-cell sizes, can be picked up, moved, swapped, dropped, and equipped in
the five ordinary slots. Hovering a backpack or equipped item also opens the
small retail information overlay after its original short delay. The text is
built from `Item.Ibn`, including combat values, durability, weight, level,
condition-adjusted sale price, and the eight elemental values.

Gameplay now owns a real `PlayerData` record rather than storing level on the
movement actor. The portable `RKC_RPG_TABLE` library decodes `Table.Tbd`, and
new male and female characters receive the thirteen values from retail tables
901 and 900. A selected save contributes its complete plain 0x160-byte player
record, including unknown bytes that later inventory, equipment, and
progression slices will need. The in-game save actions now write the retail
envelope and preserve the unknown dynamic payload of an existing save while
updating its repeated player record. When the matching option is enabled, the
same action captures the world without the HUD or menu and writes the retail
391×114 preview bitmap used by Load Game. Full dynamic-state decoding and
serialization are still pending.

Run it with `--smoke-test` to close automatically after three frames.

## Reverse-engineering records

[`functions.csv`](../../reverse/shadowflare-exe/functions.csv) and
[`globals.csv`](../../reverse/shadowflare-exe/globals.csv) connect readable
reconstructed code to retail addresses without forcing the new executable to
copy the original process layout.
[`status.md`](../../reverse/shadowflare-exe/status.md) describes the
confidence labels used by those maps.

Raw decompiler output and analysis projects stay under
[`reverse/`](../../reverse/README.md). Only understood, readable behavior
belongs in the portable implementation.

`OpenShadowFlare::GameCore` is the convenient build target for the whole
portable game. Underneath it, core utilities, items, resources, states, world
simulation, rendering, and GAPI are separate static libraries. The dependency
direction is checked by the `source_boundaries` test, so a lower layer cannot
quietly start including world, rendering, or runtime code just because another
target happened to make it link.

DLL-derived behavior lives under `libs/`, with one directory per original
DLL. Each implemented counterpart is a statically linked, cross-platform
library with one public API header. The working Win32 reconstruction under
`src/reconstructed/<DLL name>` remains the strong behavioral reference; the
portable version keeps the behavior but does not preserve its ABI, object
layout, or platform-specific plumbing.

The first eight static counterparts are:

- `OpenShadowFlare::RK_FUNCTION` for RCLIB-L decompression
- `OpenShadowFlare::RKC_DBFCONTROL` for the software framebuffer backend
- `OpenShadowFlare::RKC_DIB` for portable BMP images
- `OpenShadowFlare::RKC_DSOUND` for VOC decoding and LAL playback
- `OpenShadowFlare::RKC_UPDIB` for NJP/SDW patterns
- `OpenShadowFlare::RKC_RPGSCRN` for CAF, GND, and OBL data
- `OpenShadowFlare::RKC_RPG_SCRIPT` for compiled scenario data and execution
- `OpenShadowFlare::RKC_RPG_TABLE` for general parameter-table databases

Windowing and final presentation stay in the thin executable runtime and the
LWL and LGL libraries. This keeps the reconstructed rules independently
testable without starting a window.

Source files are grouped by responsibility while keeping headers beside their
implementations:

- `core/` contains executable config, command-line, and retail utility code
- `gapi/` contains the backend-neutral graphics interface
- `items/` contains the executable-owned item database and item rules
- `libs/` contains the fourteen portable DLL boundaries
- `render/` translates reconstructed draw rules into backend-neutral GAPI work
- `resources/` owns shared decoded assets and retail filesystem lookup
- `states/` contains the top-level dispatcher and reconstructed game states
- `ui/` contains layout shared by input handling and drawing
- `world/` contains actors, scenario orchestration, and script-to-world glue
- `runtime/` contains startup, input/audio adapters, and frontend assets
- `runtime/platform/` owns application-loop and lifecycle adapters
- `runtime/presentation/` owns the final-surface presentation interface and
  concrete graphics backends

`WorldScene` is the public facade used by gameplay and rendering, but the work
behind it is kept in focused pieces: loading, interaction, item preparation,
script bridging, player appearance, actors, pointer selection, quests, and
movement all have their own implementation units or objects. The executable
runtime follows the same rule. Frame composition lives in `RuntimeRenderer`,
gameplay panels in `GameplayUiController`, and state hook wiring in
`state_bindings`; the main runtime is only responsible for lifecycle and the
fixed-step loop.

The steps and boundary rules for bringing up another operating system or
console are in
[`documentation/adding-platforms.md`](../../documentation/adding-platforms.md).
