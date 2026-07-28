# ShadowFlare executable reconstruction

This is the portable reconstruction of `ShadowFlare.exe`. It is intentionally
separate from the fourteen Win32 compatibility DLL binaries, while using their
tested reconstructions as the behavioral reference.

The executable uses the project-owned platform libraries:

- LWL for windows, input, and timing
- LGL for the small OpenGL 3.3 function set used to present a finished frame
- LAL for WAV/PCM playback

Game drawing goes through `gapi`, the backend-neutral graphics interface. Its
first backend is a software renderer working on a fixed 640×480 RGBA surface,
like the original game's software DIB renderer. A tiny LGL presenter uploads
that surface once per frame and lets the GPU scale it to the window. Maximizing
the window therefore does not turn presentation into a large CPU scaling loop.
Other render backends can implement the same `gapi::Backend` interface later.

Code in this directory must use those portable APIs and the C++ standard
library. Native window handles, operating-system messages, platform headers,
and conditional platform implementations belong inside LWL or LAL, not here.

## Building

From the repository root:

```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug
```

The development executable can be launched directly from the build folder. It
looks for `tmp/ShadowFlare` relative to its own location, so starting it from a
file manager works even when that file manager chooses a different working
directory.

The executable can now read the original `SFlare.Cfg`, handle the retail `/w`
and `/f` switches, and run the original top-level
title/character-selection/gameplay transitions. The title and character-select
enter/leave lifecycles are reconstructed too, including their asset manifests,
save-slot behavior, input tables, random smoke delays, and shared menu music.
The original VOC containers are decoded portably and played through LAL, with
the configured effect and BGM volumes. The broader reconstruction order and
the current slice are tracked in the repository's
[`roadmap.md`](../../roadmap.md).

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

The first dynamic `PEOPLE` record is live as well. Ostare is read from the MCT
rather than placed by hand, loads `Character/PEOPLE/00000013`, and uses his
original position, direction, custom CAF layer mask, idle animation, and SDW
shadow. His MCT tail also drives the original one-second idle pause followed
by a short chart-one walk inside his scenario-defined rectangle. For now this
slice deliberately stops at one NPC; the other six Remote Town people, names,
interaction, and more involved behavior still need to be connected.

The first world interaction is in place too. Clicking the ground moves the
player at the original gameplay cadence, follows the cursor with all eight
directions, and moves the camera with the player. `R` switches between the
retail walking and running speeds, using CAF charts one and two respectively.
Remote Town's GND judgement layer and OBL rectangles stop the player at walls
and scenery while the renderer keeps sorting nearby objects and Ostare in
front of or behind the moving sprite. The remaining NPCs, HUD, scripts,
darkness, and the rest of gameplay simulation are still in progress.

Run it with `--smoke-test` to close automatically after three frames. You can
also pass `/w` to keep a smoke-test window out of fullscreen mode.

## Reverse-engineering records

`reverse/functions.csv` and `reverse/globals.csv` connect readable
reconstructed code to retail addresses without forcing the new executable to
copy the original process layout. `reverse/status.md` describes the confidence
labels used by those maps.

Raw decompiler output stays in `/ghidra`. Only understood, readable behavior
belongs in the portable implementation.

The portable game code lives in the `OpenShadowFlare::GameCore` CMake target.
DLL-derived behavior lives under `libs/`, with one directory per original DLL.
Each implemented counterpart is a statically linked, cross-platform library
with one public API header. The working Win32 reconstruction under
`src/reconstructed/<DLL name>` remains the strong behavioral reference; the
portable version keeps the behavior but does not preserve its ABI, object
layout, or platform-specific plumbing.

The first six static counterparts are:

- `OpenShadowFlare::RK_FUNCTION` for RCLIB-L decompression
- `OpenShadowFlare::RKC_DBFCONTROL` for the software framebuffer backend
- `OpenShadowFlare::RKC_DIB` for portable BMP images
- `OpenShadowFlare::RKC_DSOUND` for VOC decoding and LAL playback
- `OpenShadowFlare::RKC_UPDIB` for NJP/SDW patterns
- `OpenShadowFlare::RKC_RPGSCRN` for CAF, GND, and OBL data

Windowing and final presentation stay in the thin executable runtime and the
LWL and LGL libraries. This keeps the reconstructed rules independently
testable without starting a window.

Source files are grouped by responsibility while keeping headers beside their
implementations:

- `core/` contains executable config, command-line, and retail utility code
- `gapi/` contains the backend-neutral graphics interface
- `libs/` contains the fourteen portable DLL boundaries
- `render/` translates reconstructed draw rules into backend-neutral GAPI work
- `states/` contains the top-level dispatcher and reconstructed game states
- `world/` contains executable-owned scenario orchestration
- `runtime/` contains the executable shell and fixed-surface LGL presenter
