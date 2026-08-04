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

#ifndef SHADOWFLARE_UI_GAMEPLAY_STATUS_PATTERN_H
#define SHADOWFLARE_UI_GAMEPLAY_STATUS_PATTERN_H

#include "data/njp.h"
#include "render/renderer.h"

#include <stdint.h>

void sf_gameplay_status_pattern_draw(
  SfRenderer *renderer, const SfNjpDecodedResource *resource,
  uint8_t source_pattern, int x, int y, const SfRect *clip);

#endif
