/*
 * Copyright (C) 2026 Michael Binder
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
typedef unsigned int LGLuint;
typedef int LGLint;
typedef int LGLsizei;
typedef float LGLfloat;
typedef char LGLchar;

typedef void* (*LglLoadProc)(const char *name, void *user_data);

typedef enum {
  LGL_API_DESKTOP_OPENGL,
  LGL_API_OPENGL_ES,
} LglApi;

enum {
  LGL_COLOR_BUFFER_BIT = 0x00004000,
  LGL_MAJOR_VERSION = 0x821B,
  LGL_MINOR_VERSION = 0x821C,
  LGL_TEXTURE_2D = 0x0DE1,
  LGL_TEXTURE_MAG_FILTER = 0x2800,
  LGL_TEXTURE_MIN_FILTER = 0x2801,
  LGL_NEAREST = 0x2600,
  LGL_RGBA = 0x1908,
  LGL_RGBA8 = 0x8058,
  LGL_UNSIGNED_BYTE = 0x1401,
  LGL_UNPACK_ALIGNMENT = 0x0CF5,
  LGL_VERTEX_SHADER = 0x8B31,
  LGL_FRAGMENT_SHADER = 0x8B30,
  LGL_COMPILE_STATUS = 0x8B81,
  LGL_LINK_STATUS = 0x8B82,
  LGL_INFO_LOG_LENGTH = 0x8B84,
  LGL_TRIANGLES = 0x0004
};

typedef void (LGL_APIENTRY *LglGetIntegervProc)(
  LGLenum name, LGLint *value);
typedef void (LGL_APIENTRY *LglViewportProc)(
  LGLint x, LGLint y, LGLsizei width, LGLsizei height);
typedef void (LGL_APIENTRY *LglClearColorProc)(
  LGLfloat red, LGLfloat green, LGLfloat blue, LGLfloat alpha);
typedef void (LGL_APIENTRY *LglClearProc)(LGLbitfield mask);
typedef void (LGL_APIENTRY *LglGenTexturesProc)(
  LGLsizei count, LGLuint *textures);
typedef void (LGL_APIENTRY *LglDeleteTexturesProc)(
  LGLsizei count, const LGLuint *textures);
typedef void (LGL_APIENTRY *LglBindTextureProc)(
  LGLenum target, LGLuint texture);
typedef void (LGL_APIENTRY *LglTexParameteriProc)(
  LGLenum target, LGLenum name, LGLint value);
typedef void (LGL_APIENTRY *LglPixelStoreiProc)(
  LGLenum name, LGLint value);
typedef void (LGL_APIENTRY *LglTexImage2DProc)(
  LGLenum target, LGLint level, LGLint internal_format,
  LGLsizei width, LGLsizei height, LGLint border,
  LGLenum format, LGLenum type, const void *pixels);
typedef void (LGL_APIENTRY *LglTexSubImage2DProc)(
  LGLenum target, LGLint level, LGLint x, LGLint y,
  LGLsizei width, LGLsizei height,
  LGLenum format, LGLenum type, const void *pixels);
typedef LGLuint (LGL_APIENTRY *LglCreateShaderProc)(LGLenum type);
typedef void (LGL_APIENTRY *LglShaderSourceProc)(
  LGLuint shader, LGLsizei count,
  const LGLchar *const* strings, const LGLint *lengths);
typedef void (LGL_APIENTRY *LglCompileShaderProc)(LGLuint shader);
typedef void (LGL_APIENTRY *LglGetShaderivProc)(
  LGLuint shader, LGLenum name, LGLint *value);
typedef void (LGL_APIENTRY *LglGetShaderInfoLogProc)(
  LGLuint shader, LGLsizei capacity,
  LGLsizei *length, LGLchar *log);
typedef void (LGL_APIENTRY *LglDeleteShaderProc)(LGLuint shader);
typedef LGLuint (LGL_APIENTRY *LglCreateProgramProc)(void);
typedef void (LGL_APIENTRY *LglAttachShaderProc)(
  LGLuint program, LGLuint shader);
typedef void (LGL_APIENTRY *LglLinkProgramProc)(LGLuint program);
typedef void (LGL_APIENTRY *LglGetProgramivProc)(
  LGLuint program, LGLenum name, LGLint *value);
typedef void (LGL_APIENTRY *LglGetProgramInfoLogProc)(
  LGLuint program, LGLsizei capacity,
  LGLsizei *length, LGLchar *log);
typedef void (LGL_APIENTRY *LglDeleteProgramProc)(LGLuint program);
typedef void (LGL_APIENTRY *LglUseProgramProc)(LGLuint program);
typedef void (LGL_APIENTRY *LglGenVertexArraysProc)(
  LGLsizei count, LGLuint *arrays);
typedef void (LGL_APIENTRY *LglDeleteVertexArraysProc)(
  LGLsizei count, const LGLuint *arrays);
typedef void (LGL_APIENTRY *LglBindVertexArrayProc)(LGLuint array);
typedef void (LGL_APIENTRY *LglDrawArraysProc)(
  LGLenum mode, LGLint first, LGLsizei count);

extern LglGetIntegervProc lglGetIntegerv;
extern LglViewportProc lglViewport;
extern LglClearColorProc lglClearColor;
extern LglClearProc lglClear;
extern LglGenTexturesProc lglGenTextures;
extern LglDeleteTexturesProc lglDeleteTextures;
extern LglBindTextureProc lglBindTexture;
extern LglTexParameteriProc lglTexParameteri;
extern LglPixelStoreiProc lglPixelStorei;
extern LglTexImage2DProc lglTexImage2D;
extern LglTexSubImage2DProc lglTexSubImage2D;
extern LglCreateShaderProc lglCreateShader;
extern LglShaderSourceProc lglShaderSource;
extern LglCompileShaderProc lglCompileShader;
extern LglGetShaderivProc lglGetShaderiv;
extern LglGetShaderInfoLogProc lglGetShaderInfoLog;
extern LglDeleteShaderProc lglDeleteShader;
extern LglCreateProgramProc lglCreateProgram;
extern LglAttachShaderProc lglAttachShader;
extern LglLinkProgramProc lglLinkProgram;
extern LglGetProgramivProc lglGetProgramiv;
extern LglGetProgramInfoLogProc lglGetProgramInfoLog;
extern LglDeleteProgramProc lglDeleteProgram;
extern LglUseProgramProc lglUseProgram;
extern LglGenVertexArraysProc lglGenVertexArrays;
extern LglDeleteVertexArraysProc lglDeleteVertexArrays;
extern LglBindVertexArrayProc lglBindVertexArray;
extern LglDrawArraysProc lglDrawArrays;

bool lgl_load(LglLoadProc load, void *user_data);
bool lgl_load_for_api(
  LglLoadProc load, void *user_data, LglApi api);
void lgl_reset(void);
bool lgl_is_loaded(void);
LglApi lgl_api(void);
int lgl_version_major(void);
int lgl_version_minor(void);
const char* lgl_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
