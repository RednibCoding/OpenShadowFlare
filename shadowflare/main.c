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

#include "core/memory_budget.h"
#include "runtime/application.h"

#include <stdint.h>

static uint8_t sf_main_memory[SF_MAIN_ARENA_BYTES];
static uint8_t sf_video_memory[SF_VIDEO_MEMORY_LIMIT_BYTES];

int main(int argument_count, char **arguments) {
  return sf_application_run(
    sf_main_memory, sizeof(sf_main_memory),
    sf_video_memory, sizeof(sf_video_memory),
    argument_count > 0 ? arguments[0] : NULL,
    argument_count > 1 ? arguments[1] : NULL);
}
