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

#include "assets/gameplay_assets.h"

#include "assets/retail_paths.h"
#include "data/pattern_list.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

typedef struct SfGameplaySelection {
  uint8_t selected[SF_PATTERN_LIST_LIMIT][SF_NJP_PATTERN_FILE_LIMIT];
  uint8_t counts[SF_PATTERN_LIST_LIMIT];
} SfGameplaySelection;

static bool sf_gameplay_relative_path(
    char *path, size_t capacity, const char *format,
    const char *text, int32_t number) {
  const int length = text
    ? snprintf(path, capacity, format, text)
    : snprintf(path, capacity, format, number);
  return length >= 0 && (size_t) length < capacity;
}

static bool sf_gameplay_path(
    char *path, size_t capacity, const char *root,
    const char *format, const char *text, int32_t number) {
  char relative[SF_RETAIL_PATH_CAPACITY];
  return sf_gameplay_relative_path(
      relative, sizeof(relative), format, text, number) &&
    sf_retail_path_join(path, capacity, root, relative);
}

static bool sf_gameplay_map_stem(
    const char *map_path, char *stem, size_t capacity) {
  const char *name = map_path;
  const char *cursor;
  const char *extension = NULL;
  size_t length;
  if (!map_path || !stem || capacity == 0u) return false;
  for (cursor = map_path; *cursor; ++cursor) {
    if (*cursor == '/' || *cursor == '\\') name = cursor + 1;
    else if (*cursor == '.') extension = cursor;
  }
  if (!extension || extension <= name) extension = cursor;
  length = (size_t) (extension - name);
  if (length == 0u || length >= capacity) return false;
  for (cursor = name; cursor < extension; ++cursor) {
    if (!isalnum((unsigned char) *cursor) && *cursor != '_') return false;
  }
  memcpy(stem, name, length);
  stem[length] = '\0';
  return true;
}

static bool sf_gameplay_ends_with_shadow(const char *name) {
  const size_t length = name ? strlen(name) : 0u;
  return length >= 4u && name[length - 4u] == '.' &&
    tolower((unsigned char) name[length - 3u]) == 's' &&
    tolower((unsigned char) name[length - 2u]) == 'd' &&
    tolower((unsigned char) name[length - 1u]) == 'w';
}

static void sf_gameplay_select(
    SfGameplaySelection *selection, uint8_t set, uint8_t pattern) {
  if (!selection->selected[set][pattern]) {
    selection->selected[set][pattern] = 1u;
    ++selection->counts[set];
  }
}

static bool sf_gameplay_select_ground(
    SfGameplaySelection *selection, const SfPatternList *patterns,
    const SfGroundMap *ground) {
  int32_t y;
  for (y = 0; y < ground->height; ++y) {
    int32_t x;
    for (x = 0; x < ground->width; ++x) {
      const SfGroundCell *cell = sf_ground_cell(ground, x, y);
      if (!cell) return false;
      if (cell->pattern_set == SF_GROUND_EMPTY_PATTERN ||
          cell->pattern == SF_GROUND_EMPTY_PATTERN) continue;
      if (cell->pattern_set >= patterns->count ||
          cell->pattern >= SF_NJP_PATTERN_FILE_LIMIT) return false;
      sf_gameplay_select(
        selection, cell->pattern_set, cell->pattern);
    }
  }
  return true;
}

static bool sf_gameplay_select_objects(
    SfGameplaySelection *selection, const SfPatternList *patterns,
    const SfObjectMap *objects) {
  uint16_t object_index;
  for (object_index = 0u; object_index < objects->count; ++object_index) {
    const SfMapObject *object = &objects->objects[object_index];
    if (object->pattern < 0 ||
        object->pattern >= (int32_t) SF_NJP_PATTERN_FILE_LIMIT ||
        object->pattern_set < 0 || object->pattern_set >= patterns->count)
      continue;
    sf_gameplay_select(
      selection, (uint8_t) object->pattern_set, (uint8_t) object->pattern);
    if ((object->status & 8) != 0 &&
        object->pattern_set + 1 < patterns->count &&
        sf_gameplay_ends_with_shadow(
          patterns->names[object->pattern_set + 1]))
      sf_gameplay_select(
        selection, (uint8_t) (object->pattern_set + 1),
        (uint8_t) object->pattern);
  }
  return true;
}

static bool sf_gameplay_load_patterns(
    SfGameplayAssets *assets, const SfGameplaySelection *selection,
    const SfPatternList *patterns, const char *data_root, SfArena *arena) {
  uint8_t set;
  uint8_t set_count = 0u;
  for (set = 0u; set < patterns->count; ++set) {
    if (selection->counts[set] > 0u) ++set_count;
  }
  if (set_count == 0u || set_count > SF_GAMEPLAY_PATTERN_SET_LIMIT)
    return false;
  assets->pattern_sets = (SfGameplayPatternSet *) sf_arena_push_zero(
    arena, (size_t) set_count * sizeof(*assets->pattern_sets), sizeof(void *));
  if (!assets->pattern_sets) return false;
  for (set = 0u; set < patterns->count; ++set) {
    SfGameplayPatternSet *output;
    uint8_t indices[SF_NJP_DECODED_PATTERN_LIMIT];
    uint8_t index;
    uint8_t count = 0u;
    char path[SF_RETAIL_PATH_CAPACITY];
    if (selection->counts[set] == 0u) continue;
    if (selection->counts[set] > SF_NJP_DECODED_PATTERN_LIMIT) return false;
    for (index = 0u; index < SF_NJP_PATTERN_FILE_LIMIT; ++index) {
      if (selection->selected[set][index]) indices[count++] = index;
    }
    output = &assets->pattern_sets[assets->pattern_set_count];
    if (!sf_gameplay_path(
          path, sizeof(path), data_root,
          sf_retail_world_paths.pattern_format,
          patterns->names[set], 0) ||
        !sf_njp_load_decoded_patterns(
          path, indices, count, arena, &output->resource)) return false;
    output->source_index = set;
    ++assets->pattern_set_count;
  }
  return assets->pattern_set_count == set_count;
}

static bool sf_gameplay_add_player_item(
    SfItemReference *items, uint8_t *count, uint8_t capacity,
    uint8_t category, int32_t definition_id) {
  uint8_t index;
  for (index = 0u; index < *count; ++index)
    if (items[index].category == category &&
        items[index].definition_id == definition_id) return true;
  if (*count >= capacity) return false;
  items[*count].category = category;
  items[*count].definition_id = definition_id;
  ++*count;
  return true;
}

static bool sf_gameplay_load_player(
    SfGameplayAssets *assets, const char *data_root, uint8_t gender,
    const uint8_t *appearance_parts, uint8_t appearance_part_count,
    const SfItemReference *visible_items, uint8_t visible_item_count,
    SfArena *arena) {
  SfItemReference items[SF_GROUND_ITEM_DEFINITION_LIMIT];
  uint8_t count = 0u;
  uint8_t index;
  for (index = 0u; index < visible_item_count; ++index)
    if (visible_items[index].category <= 1u && !sf_gameplay_add_player_item(
          items, &count, SF_GROUND_ITEM_DEFINITION_LIMIT,
          visible_items[index].category,
          visible_items[index].definition_id)) return false;
  for (index = 0u; index < assets->ground_items.definition_count; ++index) {
    const SfItemGroundDefinition *definition =
      &assets->ground_items.definitions[index];
    if (definition->category <= 1u && !sf_gameplay_add_player_item(
          items, &count, SF_GROUND_ITEM_DEFINITION_LIMIT,
          definition->category,
          definition->definition_id)) return false;
  }
  return sf_player_assets_load(
    &assets->player, data_root, gender,
    appearance_parts, appearance_part_count, items, count, arena);
}

bool sf_gameplay_assets_load(
    SfGameplayAssets *assets, const char *data_root,
    int32_t scenario_id, int32_t entry_key, uint8_t player_gender,
    int32_t player_level,
    int32_t companion_type, int32_t companion_level,
    const uint8_t *appearance_parts, uint8_t appearance_part_count,
    const SfItemReference *visible_items, uint8_t visible_item_count,
    const SfItemReference *retained_items, uint8_t retained_item_count,
    SfArena *arena) {
  SfPatternList patterns;
  SfGameplaySelection selection;
  const SfMctEntry *entry;
  char map_name[SF_PATTERN_NAME_CAPACITY];
  char path[SF_RETAIL_PATH_CAPACITY];
  static const uint8_t font_pattern = 0u;
  static const uint8_t speech_patterns[5] = {0u, 1u, 2u, 3u, 4u};
  static const uint8_t hud_patterns[22] = {
    0u, 3u, 7u, 8u, 10u, 11u, 14u, 15u,
    19u, 20u, 21u, 22u, 23u, 24u, 25u, 26u, 27u, 28u,
    29u, 30u, 31u, 32u
  };
  static const uint8_t inventory_patterns[37] = {
    0u, 1u, 2u, 3u, 5u, 6u, 14u, 15u, 16u, 32u,
    36u, 37u, 38u, 39u, 40u, 41u, 42u, 43u, 44u, 45u, 46u,
    47u, 48u, 49u, 50u, 51u, 52u, 53u, 54u, 55u, 56u, 57u,
    67u, 69u, 70u, 116u, 117u
  };
  static const uint8_t magic_icon_patterns[23] = {
    0u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u,
    13u, 14u, 15u, 16u, 17u, 18u, 19u, 20u, 21u, 22u, 23u
  };
  static const uint8_t magic_bar_patterns[24] = {
    2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u,
    14u, 15u, 16u, 17u, 18u, 19u, 20u, 21u, 22u, 23u, 24u, 25u
  };
  size_t mark;
  bool success = false;
  if (!assets || !data_root || scenario_id < 0 || !arena) return false;
  mark = sf_arena_mark(arena);
  memset(assets, 0, sizeof(*assets));
  memset(&selection, 0, sizeof(selection));
  assets->script = (SfScsScript *) sf_arena_push_zero(
    arena, sizeof(*assets->script), sizeof(int32_t));
  assets->spell_parameters = (SfSpellParameters *) sf_arena_push_zero(
    arena, sizeof(*assets->spell_parameters), sizeof(int32_t));
  if (!assets->script || !assets->spell_parameters) goto done;
  if (!sf_gameplay_path(
        path, sizeof(path), data_root,
        sf_retail_world_paths.scenario_format, NULL, scenario_id) ||
      !sf_mct_load(path, arena, &assets->scenario)) goto done;
  if (!sf_gameplay_path(
        path, sizeof(path), data_root,
        sf_retail_world_paths.scenario_script_format, NULL, scenario_id) ||
      !sf_scs_load(path, assets->script)) goto done;
  entry = sf_mct_find_entry(&assets->scenario, entry_key);
  if (!entry || !sf_gameplay_map_stem(
        assets->scenario.map_path, map_name, sizeof(map_name))) goto done;
  assets->entry = *entry;
  if (!sf_gameplay_path(
        path, sizeof(path), data_root,
        sf_retail_world_paths.ground_format, map_name, 0) ||
      !sf_gnd_load(path, arena, &assets->ground) ||
      !sf_gameplay_path(
        path, sizeof(path), data_root,
        sf_retail_world_paths.objects_format, map_name, 0) ||
      !sf_obl_load(path, arena, &assets->objects) ||
      !sf_gameplay_path(
        path, sizeof(path), data_root,
        sf_retail_world_paths.pattern_list_format, map_name, 0) ||
      !sf_pattern_list_load(path, &patterns)) goto done;
  if (!sf_gameplay_select_ground(
        &selection, &patterns, &assets->ground) ||
      !sf_gameplay_select_objects(
        &selection, &patterns, &assets->objects) ||
      !sf_gameplay_load_patterns(
        assets, &selection, &patterns, data_root, arena) ||
      !sf_retail_path_join(
        path, sizeof(path), data_root, sf_retail_game_paths.font) ||
      !sf_njp_load_selected(
        path, &font_pattern, 1u, arena, &assets->font) ||
      !sf_retail_path_join(
        path, sizeof(path), data_root, sf_retail_game_paths.speech_frame) ||
      !sf_njp_load_selected(
        path, speech_patterns, 5u, arena, &assets->speech_frame) ||
      !sf_retail_path_join(
        path, sizeof(path), data_root, sf_retail_game_paths.hud) ||
      !sf_njp_load_decoded_patterns(
        path, hud_patterns, 22u, arena, &assets->hud) ||
      !sf_retail_path_join(
        path, sizeof(path), data_root, sf_retail_game_paths.status) ||
      !sf_njp_stream_decoded_patterns(
        path, inventory_patterns, 37u, arena, &assets->inventory_panel) ||
      !sf_retail_path_join(
        path, sizeof(path), data_root, sf_retail_game_paths.magic_icons) ||
      !sf_njp_stream_decoded_patterns(
        path, magic_icon_patterns, 23u, arena, &assets->magic_icons) ||
      !sf_retail_path_join(
        path, sizeof(path), data_root,
        sf_retail_game_paths.magic_bar_icons) ||
      !sf_njp_stream_decoded_patterns(
        path, magic_bar_patterns, 24u, arena, &assets->magic_bar_icons) ||
      !sf_retail_path_join(
        path, sizeof(path), data_root,
        sf_retail_game_paths.parameter_tables) ||
      !sf_player_initial_parameters_load(
        path, player_gender, player_level, &assets->player_parameters) ||
      !sf_companion_profile_load(
        path, companion_type, companion_level, &assets->companion_profile) ||
      !sf_spell_parameters_load(path, assets->spell_parameters) ||
      !sf_gameplay_sound_assets_load(
        &assets->sounds, data_root, arena) ||
      !sf_ground_item_assets_load(
        &assets->ground_items, data_root, assets->script,
        retained_items, retained_item_count, arena) ||
      !sf_gameplay_load_player(
        assets, data_root, player_gender,
        appearance_parts, appearance_part_count,
        visible_items, visible_item_count, arena) ||
      !sf_companion_assets_load(
        &assets->companion, data_root, &assets->companion_profile, arena) ||
      !sf_scenario_actor_assets_load(
        &assets->actors, data_root, &assets->scenario, arena) ||
      !sf_scenario_object_assets_load(
        &assets->scenario_objects, data_root, &assets->scenario, arena) ||
      !sf_inventory_item_assets_load(
        &assets->inventory_items, data_root,
        assets->ground_items.definitions,
        assets->ground_items.definition_count, arena)) goto done;
  assets->memory_bytes = sf_arena_mark(arena) - mark;
  success = true;
done:
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(assets, 0, sizeof(*assets));
  }
  return success;
}

const SfNjpDecodedResource *sf_gameplay_pattern_set(
    const SfGameplayAssets *assets, uint8_t source_index) {
  uint8_t index;
  if (!assets) return NULL;
  for (index = 0u; index < assets->pattern_set_count; ++index) {
    if (assets->pattern_sets[index].source_index == source_index)
      return &assets->pattern_sets[index].resource;
  }
  return NULL;
}
