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

#ifndef SHADOWFLARE_DATA_BMP_H
#define SHADOWFLARE_DATA_BMP_H

#include "render/renderer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool sf_bmp_load_rgb555(
  const char *path, uint16_t *pixels, size_t pixel_capacity,
  void *row_scratch, size_t row_scratch_size, SfRgb555Image *image);

#endif
