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

#ifndef SHADOWFLARE_DATA_PATTERN_LIST_H
#define SHADOWFLARE_DATA_PATTERN_LIST_H

#include <stdbool.h>
#include <stdint.h>

#define SF_PATTERN_LIST_LIMIT 80u
#define SF_PATTERN_NAME_CAPACITY 64u

typedef struct SfPatternList {
  char names[SF_PATTERN_LIST_LIMIT][SF_PATTERN_NAME_CAPACITY];
  uint8_t count;
} SfPatternList;

bool sf_pattern_list_load(const char *path, SfPatternList *list);

#endif
