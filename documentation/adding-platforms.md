# Adding a target platform

The portable executable is meant to grow beyond Windows, Linux, macOS, and the
browser. Android, Nintendo Switch, PlayStation, and anything else should fit
into the same boundaries instead of slowly filling the game code with platform
checks.

The short version is:

```text
game code
  |
  +-- FrameApplication -------- platform application loop
  +-- LWL --------------------- window, input, timing, GL context
  +-- LAL --------------------- audio device
  +-- SurfacePresenter -------- finished 640x480 RGBA frame to the display
```

Most ports need an application-loop adapter and LWL/LAL backends. A new
presenter is only needed when the platform cannot or should not use the current
OpenGL/OpenGL ES path.

## The rules

Platform SDK headers, compiler macros, native handles, and lifecycle calls must
stay in their adapter or library backend. They do not belong in `core/`,
`items/`, `render/`, `resources/`, `states/`, `world/`, or the portable DLL
libraries.

Inside `SF_EXE`, the two allowed integration areas are:

- `runtime/platform/<platform>/` for the application loop and lifecycle;
- `runtime/presentation/` for graphics-API-specific presentation.

Window, input, timing, and audio implementations belong in LWL and LAL. If one
of their public APIs is missing something a new platform genuinely needs,
extend it as a general-purpose library API. Do not add a ShadowFlare-specific
function or expose a platform SDK type through the public header.

Avoid `#ifdef` branches in shared runtime code. The build should select one
implementation of an interface instead.

When debug tools are enabled, a target may also provide a resident-memory probe
under `runtime/platform/<platform>/`. The portable contract and the meaning of
the displayed RAM/VRAM figures are covered in
[`profiling.md`](profiling.md). A missing probe must report `n/a`; do not move an
SDK memory API into shared game code just to make the overlay show a number.

## 1. Add the application host

`runtime/application_loop.hpp` is the boundary between the game and the
platform's run loop. The game supplies a `FrameApplication`; the host repeatedly
calls `frame()` until it returns `false`.

Add:

```text
src/SF_EXE/runtime/platform/<platform>/application_loop.cpp
```

Desktop uses a normal blocking loop. The browser registers a callback and owns
the application until that callback finishes. Mobile and console targets can
use whichever model their SDK expects, as long as ownership and shutdown stay
inside this file.

If a target needs suspend, resume, focus, or low-memory notifications that
cannot already travel through LWL events, add a platform-neutral lifecycle
operation to the runtime interface. The SDK callback still belongs in the
platform adapter.

Register the new host explicitly in:

```text
src/SF_EXE/cmake/configure_platform.cmake
src/SF_EXE/cmake/platforms/<Platform>.cmake
```

Do not make an unknown platform fall through to the desktop implementation.
Unsupported targets should fail during configuration with a useful message.

## 2. Add an LWL backend

Create a backend next to the existing ones:

```text
thirdparty/lwl/lwl_<platform>.c
```

It implements the public API in `lwl.h`: window creation, events, input,
timing, cursors, paths, and optional graphics contexts. Keep SDK objects private
to the backend file.

The default `LwlGlConfig` must say which API the context actually provides:

- `LWL_GL_API_DESKTOP` for desktop OpenGL;
- `LWL_GL_API_ES` for OpenGL ES.

Do not infer the graphics API from `_WIN32`, `__ANDROID__`, `__EMSCRIPTEN__`, or
another platform macro outside the backend.

Select the source and its system libraries in `thirdparty/lwl/CMakeLists.txt`.
If the platform has no OpenGL-style context, its LWL backend can leave that
optional path unsupported and use a different `SurfacePresenter`.

## 3. Add an LAL backend

Create:

```text
thirdparty/lal/lal_<platform>.c
```

The internal contract is deliberately small:

- `lal_platform_init()`
- `lal_platform_shutdown()`
- `lal_platform_lock()`
- `lal_platform_unlock()`
- `lal_mix_frames()` for filling the platform audio buffer

The mixer produces signed 16-bit stereo PCM at 44100 Hz. Conversion to a
platform device format belongs in the backend, not in `SF_EXE`.

Select the backend and its system libraries in
`thirdparty/lal/CMakeLists.txt`. Browser or SDK-specific exports must be owned
by LAL itself; the executable must not know private backend symbol names.

## 4. Choose a surface presenter

The software renderer finishes a tightly packed 640x480 RGBA surface. A
presenter transfers that surface to the real display and scales it to the
available viewport. It should not perform game rendering or an expensive
CPU-side resize.

Platforms with desktop OpenGL 3.3, OpenGL ES 3.0, or newer can reuse the LGL
presenter. LWL supplies the context and LGL loads only the functions the
presenter uses.

For another graphics API:

1. implement `SurfacePresenter` under `runtime/presentation/`;
2. keep all API and SDK headers in that implementation;
3. provide the `createSurfacePresenter()` factory;
4. add the backend to `cmake/configure_presentation.cmake`;
5. select it through `OPENSHADOWFLARE_PRESENTATION_BACKEND`.

`game_runtime.cpp`, GAPI, and the software renderer should not need changes.
A future hardware renderer is a separate GAPI concern; it should not be mixed
into the finished-surface presenter.

## 5. Handle startup and game data

The executable needs a readable data root containing `SFlare.Cfg` and the
original `System` directory. Desktop currently discovers it relative to the
working directory or executable. The web shell mounts selected files before
calling `main()`.

If a platform has an application package, asset manager, sandbox, or separate
writable-save directory, isolate that policy in its startup/platform layer.
Do not teach resource decoders about APKs, browser storage, console mount
names, or SDK file handles. Expose ordinary paths or add a general filesystem
boundary if ordinary paths are not possible.

Keep read-only game assets and writable saves/configuration distinct when the
platform requires it. That split will eventually need to become an explicit
portable runtime API rather than a collection of platform checks in
`main.cpp`.

## 6. Keep CMake local to the target

Target-specific compiler options, SDK libraries, packaging commands, exported
symbols, and deployment files belong in:

```text
src/SF_EXE/cmake/platforms/<Platform>.cmake
```

Library-specific options stay with their library. For example, an LAL backend
link requirement belongs to the LAL target, not the ShadowFlare executable.

The main `src/SF_EXE/CMakeLists.txt` should only assemble the portable runtime
and call the platform and presentation configuration functions.

Generated files belong under `build/<target>/<configuration>`, for example
`build/android/debug` and `build/android/release`. A port must not introduce a
new build directory at the repository root or place compiler output beside
source files.

## 7. Extend the guardrails

`tests/native/check_source_boundaries.cmake` rejects known platform headers and
macros outside the integration directories. Add the new SDK's identifying
headers, macros, and types to that check when bringing up a platform.

This is intentional duplication: CMake selects the right implementation, while
the boundary test prevents a later shortcut from leaking that implementation
back into shared code.

## Port checklist

Before calling a target supported:

- the target configures without pretending to be another platform;
- its application owns the runtime for the complete callback/loop lifetime;
- normal exit destroys audio, presentation, the window, and LWL cleanly;
- LWL input coordinates and resize events remain correct;
- LAL plays title music and sound effects without blocking the frame;
- presentation preserves aspect ratio and does not CPU-scale to the window;
- original game data can be found without hardcoded developer paths;
- saves and configuration go to writable storage;
- no platform SDK header or macro appears outside the allowed adapters;
- all portable tests still pass on a normal host;
- the target has at least a compile job and, where possible, a smoke test.

The first useful bring-up milestone is the three-frame `--smoke-test`. After
that, test title input and audio, character selection, loading Remote Town,
saving, and clean shutdown in that order.
