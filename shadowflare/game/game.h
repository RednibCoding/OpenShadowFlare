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

#ifndef SHADOWFLARE_GAME_GAME_H
#define SHADOWFLARE_GAME_GAME_H

#include <stdbool.h>
#include <stdint.h>

#include "game/input.h"
#include "game/world.h"

#define SF_GAME_TITLE_ENTRY_COUNT 3u
#define SF_GAME_TITLE_SMOKE_COUNT 10u

typedef enum SfGameMode {
  SF_GAME_MODE_TITLE = 0,
  SF_GAME_MODE_CHARACTER_SELECT,
  SF_GAME_MODE_LOAD_GAME,
  SF_GAME_MODE_LOADING,
  SF_GAME_MODE_GAMEPLAY
} SfGameMode;

typedef enum SfGameSoundEvent {
  SF_GAME_SOUND_TITLE_CUE = 1u << 0u,
  SF_GAME_SOUND_MENU_MOVE = 1u << 1u,
  SF_GAME_SOUND_TITLE_CONFIRM = 1u << 2u,
  SF_GAME_SOUND_TITLE_MUSIC = 1u << 3u,
  SF_GAME_SOUND_MENU_CONFIRM = 1u << 4u
} SfGameSoundEvent;

typedef struct SfGameConfig {
  uint8_t title_smoke_frame_count[SF_GAME_TITLE_SMOKE_COUNT];
  uint8_t saved_game_file_slots[6];
  uint8_t saved_game_genders[6];
  uint8_t saved_game_count;
  bool next_save_available;
} SfGameConfig;

typedef struct SfTitleState {
  int32_t smoke_delay[SF_GAME_TITLE_SMOKE_COUNT];
  int16_t smoke_frame[SF_GAME_TITLE_SMOKE_COUNT];
  uint16_t menu_brightness[SF_GAME_TITLE_ENTRY_COUNT];
  uint32_t random_state;
  int32_t animation_frame;
  int16_t transition_timer;
  int16_t previous_pointer_x;
  int16_t previous_pointer_y;
  uint16_t scene_brightness;
  uint8_t fade_steps_remaining;
  uint8_t music_delay_frames;
  uint8_t selection;
  uint8_t sound_events;
  bool menu_visible[SF_GAME_TITLE_ENTRY_COUNT];
  bool selection_armed;
  bool transition_started;
  bool title_sound_started;
  bool music_started;
} SfTitleState;

typedef struct SfCharacterCreateState {
  int32_t fade_value;
  int32_t fade_target;
  int16_t character_transition_counter;
  int16_t rendered_transition_counter;
  int16_t launch_counter;
  int16_t previous_pointer_x;
  int16_t previous_pointer_y;
  uint16_t background_brightness;
  uint16_t mode_brightness;
  uint8_t fade_steps_remaining;
  uint8_t screen;
  uint8_t selection;
  uint8_t gender;
  uint8_t name_length;
  uint8_t sound_events;
  char name[16];
  bool input_latch;
  bool name_entry_active;
  bool name_confirm_hovered;
} SfCharacterCreateState;

typedef struct SfLoadGameState {
  int32_t fade_value;
  int32_t fade_target;
  int32_t hover_animation;
  int16_t launch_counter;
  int16_t click_cooldown;
  int16_t previous_pointer_x;
  int16_t previous_pointer_y;
  int16_t dialog_previous_pointer_x;
  int16_t dialog_previous_pointer_y;
  uint16_t background_brightness;
  uint8_t fade_steps_remaining;
  uint8_t hovered_slots;
  uint8_t screen;
  uint8_t selection;
  uint8_t selected_save;
  uint8_t dialog_selection;
  uint8_t sound_events;
  int8_t selected_file_slot;
  int8_t delete_request;
  bool brightness_increasing;
  bool input_latch;
} SfLoadGameState;

typedef struct SfGame {
  SfGameConfig config;
  SfTitleState title;
  SfCharacterCreateState character_create;
  SfLoadGameState load_game;
  SfWorldState world;
  SfGameMode mode;
  uint32_t ticks;
  uint8_t character_select_argument;
  uint8_t player_gender;
  bool quit_requested;
} SfGame;

void sf_game_init(SfGame *game, const SfGameConfig *config);
void sf_game_update(SfGame *game, const SfGameInput *input);
void sf_game_saved_catalog_changed(
  SfGame *game, const uint8_t *file_slots, const uint8_t *genders,
  uint8_t saved_game_count);

#endif
