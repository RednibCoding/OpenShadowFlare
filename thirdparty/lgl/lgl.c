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

#include "lgl.h"

#include <stdio.h>
#include <string.h>

LglGetIntegervProc lglGetIntegerv;
LglViewportProc lglViewport;
LglClearColorProc lglClearColor;
LglClearProc lglClear;

static bool g_loaded;
static int g_major_version;
static int g_minor_version;
static char g_error[128];

void lgl_reset(void) {
  lglGetIntegerv = NULL;
  lglViewport = NULL;
  lglClearColor = NULL;
  lglClear = NULL;
  g_loaded = false;
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
  char error[sizeof(g_error)];

  lgl_reset();
  if (load == NULL) {
    snprintf(g_error, sizeof(g_error), "No OpenGL load callback was supplied.");
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
        load, user_data, "glClear")) {
    snprintf(error, sizeof(error), "%s", g_error);
    lgl_reset();
    snprintf(g_error, sizeof(g_error), "%s", error);
    return false;
  }

  lglGetIntegerv(LGL_MAJOR_VERSION, &g_major_version);
  lglGetIntegerv(LGL_MINOR_VERSION, &g_minor_version);
  if (g_major_version < 3 ||
      (g_major_version == 3 && g_minor_version < 3)) {
    int major_version = g_major_version;
    int minor_version = g_minor_version;
    lgl_reset();
    snprintf(
      g_error, sizeof(g_error),
      "OpenGL 3.3 is required; the active context is %d.%d.",
      major_version, minor_version);
    return false;
  }

  g_loaded = true;
  return true;
}

bool lgl_is_loaded(void) {
  return g_loaded;
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
