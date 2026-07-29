<!--
Copyright (C) 2026 Michael Binder and contributors

This file is part of LWL.

LWL is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

LWL is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE. See the GNU General Public License for details.

You should have received a copy of the GNU General Public License along with
LWL. If not, see <https://www.gnu.org/licenses/>.
-->

# LWL

LWL is the project's small C platform layer for native window creation, input,
timing, and presentation of a 32-bit RGBA software framebuffer.

Bundled backends currently cover Win32, macOS, Linux/X11, and Emscripten. On
Linux, the build enables XShm and Xinerama. The application must reacquire the
framebuffer pointer after a resize because the backend may reallocate it.

LWL can also create an optional OpenGL context for a window. Contexts are
configured with `LwlGlConfig`, including an explicit desktop OpenGL or OpenGL
ES API choice. Desktop defaults request at least an OpenGL 3.3 core context
(OpenGL 4.1 on macOS), while the Emscripten backend requests OpenGL ES 3.0
through WebGL 2. Function loading deliberately stays outside LWL:
`lwl_gl_get_proc_address()` supplies addresses to any loader the application
chooses. Software-framebuffer applications can ignore the OpenGL API. Destroy
an OpenGL context before destroying the window that owns it.

The public framebuffer is always byte-ordered RGBA. Backends convert it to the
native display format when rectangles are presented, so application code never
needs Windows- or X11-specific color packing.

## License

LWL is licensed under the GNU General Public License, version 3 or later. The
full license text is available in
[`../../LICENSES/GPL-3.0-or-later.txt`](../../LICENSES/GPL-3.0-or-later.txt).
