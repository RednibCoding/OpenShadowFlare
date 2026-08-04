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

#include "data/ai_control.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define SF_AI_CONTROL_FILE_LIST_LIMIT 256u
#define SF_AI_CONTROL_FILE_ACTION_LIMIT 4096u
#define SF_AI_CONTROL_FILE_NAME_LIMIT (1024u * 1024u)
#define SF_AI_CONTROL_STORED_ACTION_BYTES 64u

typedef struct SfAiRequestedName {
  char text[SF_AI_CONTROL_NAME_CAPACITY];
  bool found;
} SfAiRequestedName;

typedef struct SfAiScan {
  SfAiRequestedName *requests;
  SfAiControlCatalog *catalog;
  uint16_t request_count;
  uint16_t control_index;
  uint16_t action_index;
  bool fill;
} SfAiScan;

static bool sf_ai_read_bytes(FILE *file, void *data, size_t size) {
  return size == 0u || fread(data, 1u, size, file) == size;
}

static bool sf_ai_read_i32(FILE *file, int32_t *value) {
  uint8_t bytes[4];
  uint32_t parsed;
  if (!value || !sf_ai_read_bytes(file, bytes, sizeof(bytes))) return false;
  parsed = (uint32_t) bytes[0] |
    ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) |
    ((uint32_t) bytes[3] << 24u);
  *value = (int32_t) parsed;
  return true;
}

static bool sf_ai_skip(FILE *file, uint64_t bytes) {
  return bytes <= (uint64_t) LONG_MAX && fseek(file, (long) bytes, SEEK_CUR) == 0;
}

static int sf_ai_requested_name(
    const SfAiRequestedName *requests, uint16_t count, const char *name) {
  uint16_t index;
  for (index = 0u; index < count; ++index) {
    if (strcmp(requests[index].text, name) == 0) return (int) index;
  }
  return -1;
}

static bool sf_ai_build_requests(
    const SfMctScenario *scenario, SfAiRequestedName *requests,
    uint16_t *count) {
  uint16_t enemy_index;
  *count = 0u;
  for (enemy_index = 0u; enemy_index < scenario->enemy_count; ++enemy_index) {
    const char *name = scenario->enemies[enemy_index].ai_control_name;
    size_t length;
    if (!name[0] || sf_ai_requested_name(requests, *count, name) >= 0)
      continue;
    length = strlen(name);
    if (length >= SF_AI_CONTROL_NAME_CAPACITY ||
        *count >= SF_MCT_ENEMY_LIMIT) return false;
    memcpy(requests[*count].text, name, length + 1u);
    ++*count;
  }
  return true;
}

static bool sf_ai_read_name(FILE *file, char *name) {
  int32_t stored_length;
  if (!sf_ai_read_i32(file, &stored_length) || stored_length < 0 ||
      (uint32_t) stored_length > SF_AI_CONTROL_FILE_NAME_LIMIT) return false;
  name[0] = '\0';
  if ((uint32_t) stored_length >= SF_AI_CONTROL_NAME_CAPACITY)
    return sf_ai_skip(file, (uint32_t) stored_length);
  if (!sf_ai_read_bytes(file, name, (size_t) stored_length)) return false;
  name[stored_length] = '\0';
  return true;
}

static bool sf_ai_read_action(FILE *file, SfAiAction *action) {
  uint8_t index;
  if (!sf_ai_read_i32(file, &action->action_number)) return false;
  for (index = 0u; index < SF_AI_ACTION_PARAMETER_COUNT; ++index)
    if (!sf_ai_read_i32(file, &action->parameters[index])) return false;
  for (index = 0u; index < SF_AI_ACTION_CONDITION_COUNT; ++index)
    if (!sf_ai_read_i32(file, &action->conditions[index])) return false;
  return true;
}

static bool sf_ai_scan_list(
    FILE *file, int32_t version, SfAiScan *scan) {
  char name[SF_AI_CONTROL_NAME_CAPACITY];
  int request;
  int32_t walk_point_speed = 10;
  uint8_t event;
  SfAiControl *control = NULL;
  if (!sf_ai_read_name(file, name) ||
      (version > 0 && !sf_ai_read_i32(file, &walk_point_speed))) return false;
  request = name[0]
    ? sf_ai_requested_name(scan->requests, scan->request_count, name) : -1;
  if (request >= 0) {
    scan->requests[request].found = true;
    if (scan->fill) {
      size_t length = strlen(name);
      control = &scan->catalog->controls[scan->control_index];
      memcpy(control->name, name, length + 1u);
      control->walk_point_speed = walk_point_speed;
    }
  }
  for (event = 0u; event < SF_AI_CONTROL_EVENT_COUNT; ++event) {
    int32_t stored_count;
    uint16_t action;
    if (!sf_ai_read_i32(file, &stored_count) || stored_count < 0 ||
        stored_count > (int32_t) SF_AI_CONTROL_FILE_ACTION_LIMIT ||
        scan->action_index > UINT16_MAX - (uint16_t) stored_count)
      return false;
    if (request < 0) {
      if (!sf_ai_skip(
            file, (uint64_t) (uint32_t) stored_count *
              SF_AI_CONTROL_STORED_ACTION_BYTES)) return false;
      continue;
    }
    if (control) {
      control->events[event].first_action = scan->action_index;
      control->events[event].action_count = (uint16_t) stored_count;
    }
    for (action = 0u; action < (uint16_t) stored_count; ++action) {
      if (scan->fill) {
        if (!sf_ai_read_action(
              file, &scan->catalog->actions[scan->action_index])) return false;
      } else if (!sf_ai_skip(file, SF_AI_CONTROL_STORED_ACTION_BYTES)) {
        return false;
      }
      ++scan->action_index;
    }
  }
  if (request >= 0) ++scan->control_index;
  return true;
}

static bool sf_ai_scan_file(FILE *file, SfAiScan *scan) {
  static const uint8_t signature[12] = {
    'R', 'K', 'C', '_', 'A', 'I', 'D', 'A', 'T', 'A', ' ', 'v'
  };
  uint8_t header[16];
  int32_t list_count;
  int32_t event_count;
  int32_t version;
  int32_t list;
  if (!sf_ai_read_bytes(file, header, sizeof(header)) ||
      memcmp(header, signature, sizeof(signature)) != 0 ||
      header[12] < '0' || header[12] > '9' ||
      header[13] < '0' || header[13] > '9' ||
      header[14] < '0' || header[14] > '9' || header[15] != 0x1au)
    return false;
  version = (header[12] - '0') * 100 +
    (header[13] - '0') * 10 + (header[14] - '0');
  if (!sf_ai_read_i32(file, &list_count) ||
      !sf_ai_read_i32(file, &event_count) || list_count < 0 ||
      list_count > (int32_t) SF_AI_CONTROL_FILE_LIST_LIMIT ||
      event_count != (int32_t) SF_AI_CONTROL_EVENT_COUNT) return false;
  for (list = 0; list < list_count; ++list) {
    if (!sf_ai_scan_list(file, version, scan)) return false;
  }
  return fgetc(file) == EOF && !ferror(file);
}

bool sf_ai_control_catalog_load(
    const char *path, const SfMctScenario *scenario,
    SfArena *arena, SfAiControlCatalog *catalog) {
  SfAiRequestedName requests[SF_MCT_ENEMY_LIMIT];
  SfAiScan scan;
  FILE *file = NULL;
  size_t mark;
  uint16_t request_count;
  uint16_t request;
  bool success = false;
  if (!path || !scenario || !arena || !catalog) return false;
  mark = sf_arena_mark(arena);
  memset(catalog, 0, sizeof(*catalog));
  memset(requests, 0, sizeof(requests));
  if (!sf_ai_build_requests(scenario, requests, &request_count)) goto done;
  if (request_count == 0u) {
    success = true;
    goto done;
  }
  memset(&scan, 0, sizeof(scan));
  scan.requests = requests;
  scan.request_count = request_count;
  scan.catalog = catalog;
  file = fopen(path, "rb");
  if (!file || !sf_ai_scan_file(file, &scan)) goto done;
  fclose(file);
  file = NULL;
  for (request = 0u; request < request_count; ++request) {
    if (!requests[request].found) goto done;
    requests[request].found = false;
  }
  catalog->control_count = scan.control_index;
  catalog->action_count = scan.action_index;
  catalog->controls = (SfAiControl *) sf_arena_push_zero(
    arena, (size_t) catalog->control_count * sizeof(*catalog->controls),
    sizeof(void *));
  catalog->actions = (SfAiAction *) sf_arena_push_zero(
    arena, (size_t) catalog->action_count * sizeof(*catalog->actions),
    sizeof(int32_t));
  if (!catalog->controls || (catalog->action_count > 0u && !catalog->actions))
    goto done;
  memset(&scan, 0, sizeof(scan));
  scan.requests = requests;
  scan.request_count = request_count;
  scan.catalog = catalog;
  scan.fill = true;
  file = fopen(path, "rb");
  if (!file || !sf_ai_scan_file(file, &scan) ||
      scan.control_index != catalog->control_count ||
      scan.action_index != catalog->action_count) goto done;
  success = true;
done:
  if (file) fclose(file);
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(catalog, 0, sizeof(*catalog));
  }
  return success;
}

const SfAiControl *sf_ai_control_find(
    const SfAiControlCatalog *catalog, const char *name) {
  uint16_t index;
  if (!catalog || !name) return NULL;
  for (index = 0u; index < catalog->control_count; ++index) {
    if (strcmp(catalog->controls[index].name, name) == 0)
      return &catalog->controls[index];
  }
  return NULL;
}

const SfAiAction *sf_ai_control_action(
    const SfAiControlCatalog *catalog, const SfAiControl *control,
    uint8_t event, uint16_t action_index) {
  const SfAiEvent *selected;
  if (!catalog || !control || event >= SF_AI_CONTROL_EVENT_COUNT)
    return NULL;
  selected = &control->events[event];
  if (action_index >= selected->action_count ||
      selected->first_action + action_index >= catalog->action_count)
    return NULL;
  return &catalog->actions[selected->first_action + action_index];
}
