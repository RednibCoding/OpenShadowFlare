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

#define SF_GAME_TITLE_ENTRY_COUNT 3u
#define SF_GAME_TITLE_SMOKE_COUNT 10u

typedef enum SfGameMode {
  SF_GAME_MODE_TITLE = 0,
  SF_GAME_MODE_CHARACTER_SELECT
} SfGameMode;

typedef enum SfGameSoundEvent {
  SF_GAME_SOUND_TITLE_CUE = 1u << 0u,
  SF_GAME_SOUND_MENU_MOVE = 1u << 1u,
  SF_GAME_SOUND_TITLE_CONFIRM = 1u << 2u,
  SF_GAME_SOUND_TITLE_MUSIC = 1u << 3u
} SfGameSoundEvent;

typedef struct SfGameInput {
  int16_t pointer_x;
  int16_t pointer_y;
  bool pointer_primary_pressed;
  bool up_pressed;
  bool down_pressed;
  bool confirm_pressed;
  bool cancel_pressed;
} SfGameInput;

typedef struct SfGameConfig {
  uint8_t title_smoke_frame_count[SF_GAME_TITLE_SMOKE_COUNT];
  bool saved_game_exists;
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

typedef struct SfGame {
  SfGameConfig config;
  SfTitleState title;
  SfGameMode mode;
  uint32_t ticks;
  uint8_t character_select_argument;
  bool quit_requested;
} SfGame;

void sf_game_init(SfGame *game, const SfGameConfig *config);
void sf_game_update(SfGame *game, const SfGameInput *input);

#endif
