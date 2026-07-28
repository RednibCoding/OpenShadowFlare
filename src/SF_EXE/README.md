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

The current executable is only a platform and render-loop foundation. It opens
a 640x480 OpenGL window and initializes audio; it does not load game data yet.
Run it with `--smoke-test` to close automatically after three frames.

## Reverse-engineering records

`reverse/functions.csv` and `reverse/globals.csv` connect readable
reconstructed code to retail addresses without forcing the new executable to
copy the original process layout. `reverse/status.md` describes the confidence
labels used by those maps.

Raw decompiler output stays in `/ghidra`. Only understood, readable behavior
belongs in the portable implementation.
