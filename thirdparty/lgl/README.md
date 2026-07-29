<!--
Copyright (C) 2026 Michael Binder and contributors

This file is part of LGL.

LGL is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

LGL is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE. See the GNU General Public License for details.

You should have received a copy of the GNU General Public License along with
LGL. If not, see <https://www.gnu.org/licenses/>.
-->

# LGL

LGL is a small C99 OpenGL loader for desktop OpenGL 3.3 and OpenGL ES 3.0. It
exposes only functions that have real callers in the project. Alongside
context-version queries, viewport setup, and framebuffer clearing, it includes
the small shader, texture, and vertex-array subset used to present GAPI's
fixed-resolution software surface. It does not contain game rendering code.

The library does not create windows or contexts and has no platform-specific
dependencies. Call `lgl_load()` with a function-address callback after making
an OpenGL context current. This works with LWL through
`lwl_gl_get_proc_address()`, but LGL does not depend on LWL. Use
`lgl_load_for_api()` when loading an OpenGL ES context; `lgl_load()` remains
the desktop OpenGL convenience entry point.

The loader verifies OpenGL 3.3 or OpenGL ES 3.0 according to the requested
API. Call `lgl_reset()` before destroying the final context.

## License

LGL is licensed under the GNU General Public License, version 3 or later. The
full license text is available in
[`../../LICENSES/GPL-3.0-or-later.txt`](../../LICENSES/GPL-3.0-or-later.txt).
