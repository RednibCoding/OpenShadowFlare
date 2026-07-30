# macOS port

## observation

- The event loop translates and returns events for the game window without also
  calling `sendEvent:`, so the title-bar drag and traffic-light buttons (close /
  minimize) may not respond. the game quits through its own EXIT
  menu, and closing the window still posts an `LWL_EVENT_QUIT`

## Building

Needs Xcode command-line tools (`xcode-select --install`) and CMake.

```bash
cmake -S . -B build/macos -DCMAKE_BUILD_TYPE=Release
cmake --build build/macos
```

Produces a plain executable at `build/macos/src/SF_EXE/ShadowFlare_rebuilt`. Run it
with the game data reachable from the working directory or the executable (same
discovery as Windows/Linux — it looks for `SFlare.Cfg` + `System/`).

- For a universal binary add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` (the stret fix makes the
  x86_64 slice correct).
- **`.app` bundle (optional):** `-DOPENSHADOWFLARE_MACOS_BUNDLE=ON` produces
  `ShadowFlare_rebuilt.app`. Place the game data next to the `.app` — `findDataRoot`
  walks up out of `Contents/MacOS` to find it
