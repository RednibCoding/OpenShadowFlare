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

LGL is a small C99 OpenGL 3.3 core loader. It exposes only functions that have
real callers in the project. The initial surface contains context-version
queries, viewport setup, and framebuffer clearing; rendering functions will be
added as the renderer begins using them.

The library does not create windows or contexts and has no platform-specific
dependencies. Call `lgl_load()` with a function-address callback after making
an OpenGL context current. This works with LWL through
`lwl_gl_get_proc_address()`, but LGL does not depend on LWL.

`lgl_load()` verifies that the active context provides OpenGL 3.3 or newer.
Call `lgl_reset()` before destroying the final context.

## License

LGL is licensed under the GNU General Public License, version 3 or later. The
full license text is available in
[`../../LICENSES/GPL-3.0-or-later.txt`](../../LICENSES/GPL-3.0-or-later.txt).
