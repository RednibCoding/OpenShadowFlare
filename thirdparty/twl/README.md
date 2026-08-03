<!--
Copyright (C) 2026 Michael Binder and contributors

This file is part of TWL.

TWL is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

TWL is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE. See the GNU General Public License for details.

You should have received a copy of the GNU General Public License along with
TWL. If not, see <https://www.gnu.org/licenses/>.
-->

# TWL

TWL is a small C11 display, input, timing, and controller library for programs
that need predictable memory use. It is independent of LWL and does not call a
memory allocator. The caller asks for the exact storage requirement, supplies
one suitably aligned block to `twl_init()`, and keeps ownership of that block.

The common code is freestanding-friendly. It does not depend on hosted libc
services such as files, standard I/O, string handling, or allocation. A
platform backend may call its native operating-system or browser API, and
those external APIs can manage opaque resources internally.

## Presentation

TWL accepts caller-owned RGB555, RGB565, and XRGB8888 surfaces. RGB555 uses the
PS1-friendly layout with red in bits 0-4, green in bits 5-9, blue in bits
10-14, and bit 15 left available to the renderer.

Presentation is deliberately not an application-side CPU format-conversion
pass. Linux and macOS upload the packed surface directly to an OpenGL texture,
the Web backend does the same with a WebGL 2 integer texture, and Windows hands
the packed DIB directly to GDI. A future console backend can consume its native
packed format directly.

TWL does not own the software framebuffer and never clears, fills, resizes, or
copies it. Rendering remains the application's responsibility. A constrained
renderer should draw directly into its target's packed format; asset or pixel
conversion belongs in offline tools or a one-time loading path, never in
`twl_present()`.

## Controllers

Controllers are part of the base API. Capacity is selected in `TwlConfig` and
their snapshots live in the caller's TWL memory block. Events and snapshots
use a platform-neutral layout:

- south, east, west, and north face buttons;
- shoulders, back, start, guide, and stick buttons;
- D-pad buttons;
- two signed stick pairs and two positive trigger axes.

Linux discovers `/dev/input/js*` devices without a helper library. Windows
uses XInput, macOS uses GameController, and the Web backend uses the browser
Gamepad API. Every backend generates the same connection, button, and axis
events.

Call `twl_pump_events()` once at the start of a frame, then drain the fixed
queue with `twl_poll_event()`. Polling the queue never calls into a platform
backend or rescans controllers.

## Backends

Implemented backends are:

- Linux: X11, OpenGL presentation, keyboard, pointer, and joystick input;
- Windows: Win32, direct packed DIB presentation, keyboard, pointer, and
  XInput controllers;
- macOS: Cocoa, OpenGL presentation, keyboard, pointer, and GameController;
- WebAssembly: HTML canvas, WebGL 2 presentation, browser input, and Gamepad;
- null: used on targets whose real backend has not been written yet.

Each backend lives in one source file and implements the private contract in
`twl_internal.h`. Adding another backend does not change `twl.c` or the public
API.

## Memory guarantees

- TWL never calls `malloc`, `calloc`, `realloc`, or `free`.
- Event and controller capacities are fixed during initialization.
- The caller owns every byte managed by TWL.
- Presentation never allocates or converts a framebuffer in C.
- Headless mode avoids opening a display and is suitable for tests and tools.

## License

TWL is licensed under the GNU General Public License, version 3 or later. The
full license text is available in
[`../../LICENSES/GPL-3.0-or-later.txt`](../../LICENSES/GPL-3.0-or-later.txt).
