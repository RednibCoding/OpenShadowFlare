/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of LGL.
 *
 * LGL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * LGL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along with
 * LGL. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LGL_H
#define LGL_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define LGL_APIENTRY __stdcall
#else
#define LGL_APIENTRY
#endif

typedef unsigned int LGLenum;
typedef unsigned int LGLbitfield;
typedef int LGLint;
typedef int LGLsizei;
typedef float LGLfloat;

typedef void* (*LglLoadProc)(const char *name, void *user_data);

enum {
  LGL_COLOR_BUFFER_BIT = 0x00004000,
  LGL_MAJOR_VERSION = 0x821B,
  LGL_MINOR_VERSION = 0x821C
};

typedef void (LGL_APIENTRY *LglGetIntegervProc)(
  LGLenum name, LGLint *value);
typedef void (LGL_APIENTRY *LglViewportProc)(
  LGLint x, LGLint y, LGLsizei width, LGLsizei height);
typedef void (LGL_APIENTRY *LglClearColorProc)(
  LGLfloat red, LGLfloat green, LGLfloat blue, LGLfloat alpha);
typedef void (LGL_APIENTRY *LglClearProc)(LGLbitfield mask);

extern LglGetIntegervProc lglGetIntegerv;
extern LglViewportProc lglViewport;
extern LglClearColorProc lglClearColor;
extern LglClearProc lglClear;

bool lgl_load(LglLoadProc load, void *user_data);
void lgl_reset(void);
bool lgl_is_loaded(void);
int lgl_version_major(void);
int lgl_version_minor(void);
const char* lgl_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
