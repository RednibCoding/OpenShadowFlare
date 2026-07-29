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

#include "lgl.h"

#include <stdio.h>
#include <string.h>

LglGetIntegervProc lglGetIntegerv;
LglViewportProc lglViewport;
LglClearColorProc lglClearColor;
LglClearProc lglClear;
LglGenTexturesProc lglGenTextures;
LglDeleteTexturesProc lglDeleteTextures;
LglBindTextureProc lglBindTexture;
LglTexParameteriProc lglTexParameteri;
LglPixelStoreiProc lglPixelStorei;
LglTexImage2DProc lglTexImage2D;
LglTexSubImage2DProc lglTexSubImage2D;
LglCreateShaderProc lglCreateShader;
LglShaderSourceProc lglShaderSource;
LglCompileShaderProc lglCompileShader;
LglGetShaderivProc lglGetShaderiv;
LglGetShaderInfoLogProc lglGetShaderInfoLog;
LglDeleteShaderProc lglDeleteShader;
LglCreateProgramProc lglCreateProgram;
LglAttachShaderProc lglAttachShader;
LglLinkProgramProc lglLinkProgram;
LglGetProgramivProc lglGetProgramiv;
LglGetProgramInfoLogProc lglGetProgramInfoLog;
LglDeleteProgramProc lglDeleteProgram;
LglUseProgramProc lglUseProgram;
LglGenVertexArraysProc lglGenVertexArrays;
LglDeleteVertexArraysProc lglDeleteVertexArrays;
LglBindVertexArrayProc lglBindVertexArray;
LglDrawArraysProc lglDrawArrays;

static bool g_loaded;
static LglApi g_api;
static int g_major_version;
static int g_minor_version;
static char g_error[128];

void lgl_reset(void) {
  lglGetIntegerv = NULL;
  lglViewport = NULL;
  lglClearColor = NULL;
  lglClear = NULL;
  lglGenTextures = NULL;
  lglDeleteTextures = NULL;
  lglBindTexture = NULL;
  lglTexParameteri = NULL;
  lglPixelStorei = NULL;
  lglTexImage2D = NULL;
  lglTexSubImage2D = NULL;
  lglCreateShader = NULL;
  lglShaderSource = NULL;
  lglCompileShader = NULL;
  lglGetShaderiv = NULL;
  lglGetShaderInfoLog = NULL;
  lglDeleteShader = NULL;
  lglCreateProgram = NULL;
  lglAttachShader = NULL;
  lglLinkProgram = NULL;
  lglGetProgramiv = NULL;
  lglGetProgramInfoLog = NULL;
  lglDeleteProgram = NULL;
  lglUseProgram = NULL;
  lglGenVertexArrays = NULL;
  lglDeleteVertexArrays = NULL;
  lglBindVertexArray = NULL;
  lglDrawArrays = NULL;
  g_loaded = false;
  g_api = LGL_API_DESKTOP_OPENGL;
  g_major_version = 0;
  g_minor_version = 0;
  g_error[0] = '\0';
}

static bool load_function(
    void *destination, size_t destination_size,
    LglLoadProc load, void *user_data, const char *name) {
  void *address;

  address = load(name, user_data);
  if (address == NULL) {
    snprintf(
      g_error, sizeof(g_error),
      "Required OpenGL function %s is unavailable.", name);
    return false;
  }
  if (destination_size != sizeof(address)) {
    snprintf(
      g_error, sizeof(g_error),
      "OpenGL function pointers use an unsupported representation.");
    return false;
  }
  memcpy(destination, &address, destination_size);
  return true;
}

bool lgl_load(LglLoadProc load, void *user_data) {
  return lgl_load_for_api(
    load, user_data, LGL_API_DESKTOP_OPENGL);
}

bool lgl_load_for_api(
    LglLoadProc load, void *user_data, LglApi api) {
  char error[sizeof(g_error)];

  lgl_reset();
  if (load == NULL) {
    snprintf(g_error, sizeof(g_error), "No OpenGL load callback was supplied.");
    return false;
  }
  if (api != LGL_API_DESKTOP_OPENGL &&
      api != LGL_API_OPENGL_ES) {
    snprintf(g_error, sizeof(g_error), "Unknown OpenGL API.");
    return false;
  }

  if (!load_function(
        &lglGetIntegerv, sizeof(lglGetIntegerv),
        load, user_data, "glGetIntegerv") ||
      !load_function(
        &lglViewport, sizeof(lglViewport),
        load, user_data, "glViewport") ||
      !load_function(
        &lglClearColor, sizeof(lglClearColor),
        load, user_data, "glClearColor") ||
      !load_function(
        &lglClear, sizeof(lglClear),
        load, user_data, "glClear") ||
      !load_function(
        &lglGenTextures, sizeof(lglGenTextures),
        load, user_data, "glGenTextures") ||
      !load_function(
        &lglDeleteTextures, sizeof(lglDeleteTextures),
        load, user_data, "glDeleteTextures") ||
      !load_function(
        &lglBindTexture, sizeof(lglBindTexture),
        load, user_data, "glBindTexture") ||
      !load_function(
        &lglTexParameteri, sizeof(lglTexParameteri),
        load, user_data, "glTexParameteri") ||
      !load_function(
        &lglPixelStorei, sizeof(lglPixelStorei),
        load, user_data, "glPixelStorei") ||
      !load_function(
        &lglTexImage2D, sizeof(lglTexImage2D),
        load, user_data, "glTexImage2D") ||
      !load_function(
        &lglTexSubImage2D, sizeof(lglTexSubImage2D),
        load, user_data, "glTexSubImage2D") ||
      !load_function(
        &lglCreateShader, sizeof(lglCreateShader),
        load, user_data, "glCreateShader") ||
      !load_function(
        &lglShaderSource, sizeof(lglShaderSource),
        load, user_data, "glShaderSource") ||
      !load_function(
        &lglCompileShader, sizeof(lglCompileShader),
        load, user_data, "glCompileShader") ||
      !load_function(
        &lglGetShaderiv, sizeof(lglGetShaderiv),
        load, user_data, "glGetShaderiv") ||
      !load_function(
        &lglGetShaderInfoLog, sizeof(lglGetShaderInfoLog),
        load, user_data, "glGetShaderInfoLog") ||
      !load_function(
        &lglDeleteShader, sizeof(lglDeleteShader),
        load, user_data, "glDeleteShader") ||
      !load_function(
        &lglCreateProgram, sizeof(lglCreateProgram),
        load, user_data, "glCreateProgram") ||
      !load_function(
        &lglAttachShader, sizeof(lglAttachShader),
        load, user_data, "glAttachShader") ||
      !load_function(
        &lglLinkProgram, sizeof(lglLinkProgram),
        load, user_data, "glLinkProgram") ||
      !load_function(
        &lglGetProgramiv, sizeof(lglGetProgramiv),
        load, user_data, "glGetProgramiv") ||
      !load_function(
        &lglGetProgramInfoLog, sizeof(lglGetProgramInfoLog),
        load, user_data, "glGetProgramInfoLog") ||
      !load_function(
        &lglDeleteProgram, sizeof(lglDeleteProgram),
        load, user_data, "glDeleteProgram") ||
      !load_function(
        &lglUseProgram, sizeof(lglUseProgram),
        load, user_data, "glUseProgram") ||
      !load_function(
        &lglGenVertexArrays, sizeof(lglGenVertexArrays),
        load, user_data, "glGenVertexArrays") ||
      !load_function(
        &lglDeleteVertexArrays, sizeof(lglDeleteVertexArrays),
        load, user_data, "glDeleteVertexArrays") ||
      !load_function(
        &lglBindVertexArray, sizeof(lglBindVertexArray),
        load, user_data, "glBindVertexArray") ||
      !load_function(
        &lglDrawArrays, sizeof(lglDrawArrays),
        load, user_data, "glDrawArrays")) {
    snprintf(error, sizeof(error), "%s", g_error);
    lgl_reset();
    snprintf(g_error, sizeof(g_error), "%s", error);
    return false;
  }

  lglGetIntegerv(LGL_MAJOR_VERSION, &g_major_version);
  lglGetIntegerv(LGL_MINOR_VERSION, &g_minor_version);
  if ((api == LGL_API_OPENGL_ES && g_major_version < 3) ||
      (api == LGL_API_DESKTOP_OPENGL &&
       (g_major_version < 3 ||
        (g_major_version == 3 && g_minor_version < 3)))) {
    int major_version = g_major_version;
    int minor_version = g_minor_version;
    lgl_reset();
    if (api == LGL_API_OPENGL_ES) {
      snprintf(
        g_error, sizeof(g_error),
        "OpenGL ES 3.0 is required; the active context is %d.%d.",
        major_version, minor_version);
    } else {
      snprintf(
        g_error, sizeof(g_error),
        "OpenGL 3.3 is required; the active context is %d.%d.",
        major_version, minor_version);
    }
    return false;
  }

  g_api = api;
  g_loaded = true;
  return true;
}

bool lgl_is_loaded(void) {
  return g_loaded;
}

LglApi lgl_api(void) {
  return g_api;
}

int lgl_version_major(void) {
  return g_major_version;
}

int lgl_version_minor(void) {
  return g_minor_version;
}

const char* lgl_last_error(void) {
  return g_error;
}
