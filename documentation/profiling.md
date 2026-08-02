# Runtime profiling

The F12 menu has a small profiling overlay for the numbers that matter most to
the software renderer. Turn on **Profiling** and the measurements appear in the
top-right corner, just below the FPS counter when that is enabled too.

The overlay shows:

- **RAM:** the process's resident memory on desktop, or the committed WebAssembly
  memory in the browser;
- **VRAM:** memory allocated by the active surface presenter that the project can
  account for directly;
- **FILL:** the rolling average time spent clearing and drawing the 640x480
  software framebuffer;
- **PRESENT:** the rolling average time spent uploading or copying that finished
  framebuffer, scaling it, and submitting it for display. Buffer swapping and
  display synchronization are deliberately excluded.

The timing averages cover the latest 120 frames. Memory is sampled twice per
second because asking the operating system every frame would make the profiler
part of the problem it is trying to measure. The profiler itself uses fixed
storage and does not allocate while recording frames.

The presenter has separate frame-preparation and display steps so the common
runtime can measure useful presentation work without including refresh-rate
waiting. Platform presenters contain no profiling code. FPS remains the useful
measure of whether the complete pipeline, including display synchronization,
keeps up with the target refresh rate.

VRAM deserves one caveat: portable OpenGL cannot reliably report everything a
driver has allocated. The value therefore counts allocations owned by our
presenter, not the whole graphics driver. The LGL presenter currently owns one
640x480 RGBA texture. A native presenter should report its textures,
framebuffers, and other known graphics-memory allocations through the same
boundary. If a platform cannot provide a useful number yet, the overlay says
`n/a` instead of guessing.

## Compiling the tools out

Debug tools are enabled by default while the reconstruction is in development.
For a clean production or constrained-platform build, configure with:

```sh
cmake -S . -B build/linux/release \
  -DCMAKE_BUILD_TYPE=Release \
  -DOPENSHADOWFLARE_ENABLE_DEBUG_TOOLS=OFF
```

That option removes the F12 menu and its render code, the profiler, the memory
probe, the FPS measurements, and the profiling tests from the target. It is a
compile-time feature switch; there is no dormant runtime profiler left polling
in the finished executable.

## Adding a memory probe

The portable declaration is
`runtime/platform/memory_usage.hpp`. Each target supplies exactly one
implementation from its own `runtime/platform/<platform>/` directory and adds
it in the target's CMake adapter only when debug tools are enabled. The probe
returns bytes when the platform can provide a useful process-memory figure, or
`std::nullopt` when it cannot.

Presentation memory belongs to `SurfacePresenter`. Report only allocations the
backend actually owns and can size. This keeps OS and graphics-API details out
of the game, state, world, and software-renderer code.
