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

#ifndef SHADOWFLARE_ASSETS_RETAIL_PATHS_H
#define SHADOWFLARE_ASSETS_RETAIL_PATHS_H

#include <stdbool.h>
#include <stddef.h>

#define SF_RETAIL_PATH_CAPACITY 1024u

typedef struct SfRetailTitlePaths {
  const char *artwork;
  const char *smoke_artwork_format;
  const char *smoke_animation_format;
} SfRetailTitlePaths;

typedef struct SfRetailMenuPaths {
  const char *music;
  const char *common_sounds;
} SfRetailMenuPaths;

typedef struct SfRetailCharacterCreatePaths {
  const char *artwork;
  const char *font;
} SfRetailCharacterCreatePaths;

typedef struct SfRetailSavePaths {
  const char *data_format;
  const char *preview_format;
} SfRetailSavePaths;

typedef struct SfRetailWorldPaths {
  const char *scenario_format;
  const char *scenario_script_format;
  const char *ground_format;
  const char *objects_format;
  const char *pattern_list_format;
  const char *pattern_format;
} SfRetailWorldPaths;

typedef struct SfRetailPlayerPaths {
  const char *animation_format;
  const char *artwork_format;
  const char *shadow_format;
} SfRetailPlayerPaths;

typedef struct SfRetailPeoplePaths {
  const char *animation_format;
  const char *artwork_format;
  const char *shadow_format;
} SfRetailPeoplePaths;

typedef struct SfRetailObjectPaths {
  const char *static_artwork_format;
  const char *static_artwork_alternate_format;
  const char *static_shadow_format;
  const char *static_shadow_alternate_format;
  const char *animation_artwork_format;
  const char *animation_format;
} SfRetailObjectPaths;

typedef struct SfRetailCompanionPaths {
  const char *animation_format;
  const char *artwork_format;
  const char *shadow_format;
} SfRetailCompanionPaths;

typedef struct SfRetailGroundItemPaths {
  const char *animation_format;
  const char *artwork_format;
  const char *shadow_format;
} SfRetailGroundItemPaths;

typedef struct SfRetailGamePaths {
  const char *parameter_tables;
  const char *item_database;
  const char *common_sounds;
  const char *font;
  const char *speech_frame;
  const char *hud;
  const char *status;
  const char *magic_icons;
  const char *magic_bar_icons;
  const char *inventory_item_format;
} SfRetailGamePaths;

extern const SfRetailTitlePaths sf_retail_title_paths;
extern const SfRetailMenuPaths sf_retail_menu_paths;
extern const SfRetailCharacterCreatePaths sf_retail_character_create_paths;
extern const SfRetailSavePaths sf_retail_save_paths;
extern const SfRetailWorldPaths sf_retail_world_paths;
extern const SfRetailPlayerPaths sf_retail_player_paths;
extern const SfRetailPeoplePaths sf_retail_people_paths;
extern const SfRetailObjectPaths sf_retail_object_paths;
extern const SfRetailCompanionPaths sf_retail_companion_paths;
extern const SfRetailGroundItemPaths sf_retail_ground_item_paths;
extern const SfRetailGamePaths sf_retail_game_paths;

bool sf_retail_path_join(
  char *path, size_t capacity, const char *root, const char *relative);
bool sf_retail_root_find(
  char *root, size_t capacity,
  const char *executable_path, const char *requested_root);

#endif
