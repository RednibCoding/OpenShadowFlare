# ShadowFlare executable reconstruction

This is the portable reconstruction of `ShadowFlare.exe`. It is intentionally
separate from the fourteen compatibility DLLs.

The executable uses the project-owned platform libraries:

- LWL for windows, input, timing, and an optional RGBA software framebuffer
- LGL for the small OpenGL 3.3 function set used by the renderer
- LAL for WAV/PCM playback

Code in this directory must use those portable APIs and the C++ standard
library. Native window handles, operating-system messages, platform headers,
and conditional platform implementations belong inside LWL or LAL, not here.

## Building

From the repository root:

```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug
```

The executable can now read the original `SFlare.Cfg`, handle the retail `/w`
and `/f` switches, and run the original top-level
title/character-selection/gameplay transitions. The title and character-select
enter/leave lifecycles are reconstructed too, including their asset manifests,
save-slot behavior, input tables, random smoke delays, and shared menu music.

The title screen's per-frame rules are connected to LWL input: keyboard
navigation, mouse hover/click regions, unavailable-item skipping, fades, audio
cues, smoke timing, and delayed New Game, Continue, and Exit actions all
follow the retail function. Character selection has its outer fade and
screen/transition dispatcher as well. Its individual character and save-list
screens, plus all menu drawing, are the next pieces to recover, so the current
window still shows the render-loop foundation rather than the original menu.

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
- `states/` contains the top-level dispatcher and reconstructed game states
- `runtime/` contains the small executable shell using LWL, LGL, and LAL
