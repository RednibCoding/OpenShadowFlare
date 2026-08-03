# Runtime profiling

The F12 menu has a small profiling overlay for the numbers that matter most to
the software renderer. Turn on **Profiling** and the measurements appear in the
top-right corner, just below the FPS counter when that is enabled too.

The overlay shows:

- **GAME:** active memory owned by portable game resources, decoded map and
  actor graphics, and the software framebuffer, excluding audio;
- **AUDIO:** active decoded LAL sound data;
- **TOTAL RAM:** GAME plus AUDIO, which is the complete portable memory total
  tracked by the profiler;
- **VRAM:** memory allocated by the active surface presenter that the project can
  account for directly;
- **FILL:** the rolling average time spent clearing and drawing the 640x480
  software framebuffer;
- **PRESENT:** the rolling average time spent uploading or copying that finished
  framebuffer, scaling it, and submitting it for display. Buffer swapping and
  display synchronization are deliberately excluded.

TOTAL RAM deliberately does not use process RSS. RSS includes the operating
system, shared libraries, window system, graphics driver, allocator high-water
marks, and other costs that change completely between Linux, a browser, and a
console. The portable total follows allocations we own instead, which makes it
useful when working toward a fixed-memory target such as the PlayStation 2.
Small STL nodes and platform-library internals are not guessed, so the number
is a stable managed working set rather than a claim about every byte in the
process.

The timing averages cover the latest 120 frames. Managed memory is sampled
twice per second; walking every loaded resource each frame would make the
profiler part of the problem it is trying to measure. The profiler itself uses
fixed storage and does not allocate while recording frames.

The presenter has separate frame-preparation and display steps so the common
runtime can measure useful presentation work without including refresh-rate
waiting. While profiling is enabled, the common runtime requests unsynchronized
display through the generic presenter interface and keeps its normal 60 Hz
frame limiter active. Turning profiling off restores display synchronization.
This may allow tearing during a profiling run, but prevents a driver from
charging a deferred vblank wait to the next texture upload. Platform presenters
contain no profiling code. FPS remains the useful measure of whether the
complete synchronized pipeline keeps up with the target refresh rate.

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
accounting pass, the FPS measurements, and the profiling tests from the target.
It is a compile-time feature switch; there is no dormant runtime profiler left
polling in the finished executable.

## Extending memory accounting

Decoded asset types are counted by `resources/resource_memory.*`. Resource
owners expose their active payload total, and the runtime combines the frontend
manager, current world, LAL sounds, and software framebuffer. When a new cache
or large portable allocation is added, account for it at its owner and include
that owner in the nearest aggregate. Platform adapters must not contain RAM
profiling code.

Presentation memory belongs to `SurfacePresenter`. Report only allocations the
backend actually owns and can size. This keeps OS and graphics-API details out
of the game, state, world, and software-renderer code.
