# ShadowFlare executable reconstruction

This is the portable reconstruction of `ShadowFlare.exe`. It is intentionally
separate from the fourteen compatibility DLLs.

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
the configured effect and BGM volumes.

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
`Select.njp` and `Font00.njp`. The transition into gameplay is connected;
gameplay rendering itself is the next visible executable slice.

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
It only uses the C++ standard library. Windowing, rendering, and audio stay in
the thin executable runtime and the LWL, LGL, and LAL libraries, which keeps
the reconstructed rules testable without starting a window.

Source files are grouped by responsibility while keeping headers beside their
implementations:

- `core/` contains config, command-line, and retail utility code
- `gapi/` contains the graphics interface, NJP decoder, and software backend
- `render/` translates reconstructed draw rules into backend-neutral GAPI work
- `states/` contains the top-level dispatcher and reconstructed game states
- `runtime/` contains the executable shell and fixed-surface LGL presenter
