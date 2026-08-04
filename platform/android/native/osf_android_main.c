/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of OpenShadowFlare.
 *
 * OpenShadowFlare is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * OpenShadowFlare is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
 */

#include <android_native_app_glue.h>

#include "backends/android/twl_android.h"
#include "core/memory_budget.h"
#include "runtime/application.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static uint8_t g_main_memory[SF_MAIN_ARENA_BYTES];
static uint8_t g_video_memory[SF_VIDEO_MEMORY_LIMIT_BYTES];

void android_main(struct android_app *app) {
  char data_root[512];
  const char *external =
    (app && app->activity) ? app->activity->externalDataPath : NULL;
  const char *requested = NULL;
  if (external) {
    snprintf(data_root, sizeof(data_root), "%s/ShadowFlare", external);
    requested = data_root;
  }
  twl_android_set_app(app);
  sf_application_run(
    g_main_memory, sizeof(g_main_memory),
    g_video_memory, sizeof(g_video_memory),
    NULL, requested);
}
