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

#ifndef SHADOWFLARE_RENDER_DEPTH_H
#define SHADOWFLARE_RENDER_DEPTH_H

#include "core/bounds.h"
#include "core/coordinates.h"

#include <stddef.h>
#include <stdint.h>

typedef struct SfDepthEntry {
  SfWorldPoint position;
  SfObjectBounds judgement;
  uint16_t source_index;
  int16_t status;
} SfDepthEntry;

int sf_depth_class(int16_t status);
void sf_depth_sort(SfDepthEntry *entries, size_t count);

#endif
