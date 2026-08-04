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

#ifndef SHADOWFLARE_DATA_ITEM_DECODE_H
#define SHADOWFLARE_DATA_ITEM_DECODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef bool (*SfItemDecodedByte)(
  void *user, size_t offset, uint8_t value);

bool sf_item_decode_stream(
  const char *path, SfItemDecodedByte output, void *user);

#endif
