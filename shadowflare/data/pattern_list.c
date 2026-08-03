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

#include "data/pattern_list.h"

#include <stdio.h>
#include <string.h>

bool sf_pattern_list_load(const char *path, SfPatternList *list) {
  FILE *file;
  char line[SF_PATTERN_NAME_CAPACITY + 2u];
  if (!path || !list) return false;
  memset(list, 0, sizeof(*list));
  file = fopen(path, "rb");
  if (!file) return false;
  while (fgets(line, sizeof(line), file)) {
    size_t length = strcspn(line, "\r\n");
    if (list->count >= SF_PATTERN_LIST_LIMIT ||
        length == 0u || length >= SF_PATTERN_NAME_CAPACITY ||
        (line[length] != '\r' && line[length] != '\n' &&
         line[length] != '\0')) {
      fclose(file);
      memset(list, 0, sizeof(*list));
      return false;
    }
    memcpy(list->names[list->count], line, length);
    list->names[list->count][length] = '\0';
    ++list->count;
  }
  if (ferror(file) || list->count == 0u) {
    fclose(file);
    memset(list, 0, sizeof(*list));
    return false;
  }
  fclose(file);
  return true;
}
