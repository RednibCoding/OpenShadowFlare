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

#include "assets/retail_paths.h"

#include <stdio.h>
#include <string.h>

const SfRetailTitlePaths sf_retail_title_paths = {
  "System/Title/Pattern/Title.njp",
  "System/Title/Pattern/Smoke%02u.Njp",
  "System/Title/Pattern/Smoke%02u.Caf"
};

const SfRetailMenuPaths sf_retail_menu_paths = {
  "System/Title/Music/BGM00.Voc",
  "System/Game/Voice/Voice00.Voc"
};

const SfRetailCharacterCreatePaths sf_retail_character_create_paths = {
  "System/Select/Pattern/Select.njp",
  "System/Common/Pattern/Font00.njp"
};

const SfRetailSavePaths sf_retail_save_paths = {
  "Save/%04u.Ssv",
  "Save/%04u.Bmp"
};

const SfRetailWorldPaths sf_retail_world_paths = {
  "Scenario/%08d/Scenario.Mct",
  "Scenario/%08d/Scenario.Scs",
  "Map/Ground/%s.Gnd",
  "Map/Object/%s.Obl",
  "Map/Pattern/%s.Lst",
  "Map/Pattern/%s"
};

const SfRetailPlayerPaths sf_retail_player_paths = {
  "Player/%s/Animation00.Caf",
  "Player/%s/Animation00.Njp",
  "Player/%s/Animation00.Sdw"
};

const SfRetailPeoplePaths sf_retail_people_paths = {
  "Character/PEOPLE/%08d/Animation.Caf",
  "Character/PEOPLE/%08d/Animation.Njp",
  "Character/PEOPLE/%08d/Animation.Sdw"
};

const SfRetailObjectPaths sf_retail_object_paths = {
  "Character/OBJECT/%08d/Pattern.Njp",
  "Character/OBJECT/%08d/Pattern.njp",
  "Character/OBJECT/%08d/Pattern.Sdw",
  "Character/OBJECT/%08d/Pattern.sdw",
  "Character/OBJECT/%08d/Animation.Njp",
  "Character/OBJECT/%08d/Animation.Caf"
};

const SfRetailCompanionPaths sf_retail_companion_paths = {
  "Character/PARTNER/%08d/Animation.Caf",
  "Character/PARTNER/%08d/Animation.Njp",
  "Character/PARTNER/%08d/Animation.Sdw"
};

const SfRetailGroundItemPaths sf_retail_ground_item_paths = {
  "Character/ITEM/%08d/Animation.Caf",
  "Character/ITEM/%08d/Animation.Njp",
  "Character/ITEM/%08d/Animation.Sdw"
};

const SfRetailGamePaths sf_retail_game_paths = {
  "System/Game/Parameter/Table.Tbd",
  "System/Game/Parameter/Item.Ibn",
  "System/Game/Voice/Voice00.Voc",
  "System/Common/Pattern/Font00.njp",
  "System/Game/Pattern/Hukidasi.njp",
  "System/Game/Pattern/Bar.njp",
  "System/Game/Pattern/Status.njp",
  "System/Game/Pattern/MagicIcon.njp",
  "System/Game/Pattern/MagicBarIcon.njp",
  "System/Game/Pattern/Item%04u.njp"
};

static bool sf_retail_path_copy(
    char *destination, size_t capacity, const char *source) {
  const size_t length = source ? strlen(source) : 0u;
  if (!destination || capacity == 0u || !source || length >= capacity)
    return false;
  memcpy(destination, source, length + 1u);
  return true;
}

bool sf_retail_path_join(
    char *path, size_t capacity, const char *root, const char *relative) {
  size_t root_length;
  bool separator_needed;
  int length;
  if (!path || capacity == 0u || !root || !relative) return false;
  root_length = strlen(root);
  separator_needed = root_length > 0u &&
    root[root_length - 1u] != '/' && root[root_length - 1u] != '\\';
  length = snprintf(
    path, capacity, "%s%s%s", root,
    separator_needed ? "/" : "", relative);
  return length >= 0 && (size_t) length < capacity;
}

static bool sf_retail_root_has_assets(const char *root) {
  char title_path[SF_RETAIL_PATH_CAPACITY];
  FILE *file;
  if (!root || !sf_retail_path_join(
        title_path, sizeof(title_path), root,
        sf_retail_title_paths.artwork)) return false;
  file = fopen(title_path, "rb");
  if (!file) return false;
  fclose(file);
  return true;
}

static bool sf_retail_try_root(
    char *root, size_t capacity, const char *candidate) {
  return sf_retail_root_has_assets(candidate) &&
    sf_retail_path_copy(root, capacity, candidate);
}

static bool sf_retail_executable_directory(
    char *directory, size_t capacity, const char *executable_path) {
  const char *separator = NULL;
  const char *cursor;
  size_t length;
  if (!executable_path || !executable_path[0])
    return sf_retail_path_copy(directory, capacity, ".");
  for (cursor = executable_path; *cursor; ++cursor) {
    if (*cursor == '/' || *cursor == '\\') separator = cursor;
  }
  if (!separator) return sf_retail_path_copy(directory, capacity, ".");
  length = (size_t) (separator - executable_path);
  if (length == 0u) length = 1u;
  if (length >= capacity) return false;
  memcpy(directory, executable_path, length);
  directory[length] = '\0';
  return true;
}

bool sf_retail_root_find(
    char *root, size_t capacity,
    const char *executable_path, const char *requested_root) {
  char executable_directory[SF_RETAIL_PATH_CAPACITY];
  char candidate[SF_RETAIL_PATH_CAPACITY];
  if (!root || capacity == 0u) return false;
  root[0] = '\0';
  if (requested_root)
    return sf_retail_try_root(root, capacity, requested_root);
  if (sf_retail_executable_directory(
        executable_directory, sizeof(executable_directory), executable_path)) {
    if (sf_retail_try_root(root, capacity, executable_directory)) return true;
    if (sf_retail_path_join(
          candidate, sizeof(candidate), executable_directory,
          "../../../../tmp/ShadowFlare") &&
        sf_retail_try_root(root, capacity, candidate)) return true;
  }
  if (sf_retail_try_root(root, capacity, ".")) return true;
  return sf_retail_try_root(root, capacity, "tmp/ShadowFlare");
}
