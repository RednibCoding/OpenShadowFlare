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

#include "game/title.h"

#include <string.h>

typedef struct SfMenuRectangle {
  int16_t left;
  int16_t right;
  int16_t top;
  int16_t bottom;
} SfMenuRectangle;

static const SfMenuRectangle sf_title_menu_rectangles[3] = {
  {0x0d6, 0x1a3, 0x16d, 0x184},
  {0x0d7, 0x1a3, 0x186, 0x19c},
  {0x10e, 0x172, 0x19b, 0x1b9}
};

static uint16_t sf_title_random(SfTitleState *title) {
  title->random_state = title->random_state * UINT32_C(0x343fd) +
    UINT32_C(0x269ec3);
  return (uint16_t) ((title->random_state >> 16u) & 0x7fffu);
}

static bool sf_title_inside(
    const SfMenuRectangle *rectangle, int16_t x, int16_t y) {
  return x > rectangle->left && x < rectangle->right &&
    y > rectangle->top && y < rectangle->bottom;
}

static uint16_t sf_title_transition_brightness(int16_t timer) {
  const int phase = timer % 1000;
  int brightness;
  if (phase <= 5) return 1000u;
  brightness = 1000 + ((5 - phase) * 1000) / 15;
  return brightness < 0 ? 0u : (uint16_t) brightness;
}

void sf_title_state_init(SfGame *game) {
  unsigned smoke;
  SfTitleState *title;
  if (!game) return;
  title = &game->title;
  memset(title, 0, sizeof(*title));
  title->random_state = 1u;
  title->fade_steps_remaining = 20u;
  title->selection_armed = true;
  title->previous_pointer_x = 0;
  title->previous_pointer_y = 0;
  title->menu_visible[0] = game->config.next_save_available;
  title->menu_visible[1] = game->config.saved_game_count > 0u;
  title->menu_visible[2] = true;
  title->selection = title->menu_visible[0] ? 0u :
    title->menu_visible[1] ? 1u : 2u;
  for (smoke = 0u; smoke < SF_GAME_TITLE_SMOKE_COUNT; ++smoke) {
    title->smoke_delay[smoke] = sf_title_random(title) % 0x5a;
    title->smoke_frame[smoke] = -1;
  }
  for (smoke = 0u; smoke < SF_GAME_TITLE_ENTRY_COUNT; ++smoke)
    title->menu_brightness[smoke] = 500u;
  title->menu_brightness[title->selection] = 1000u;
}

static void sf_title_begin_transition(SfTitleState *title, uint8_t entry) {
  if (title->transition_started) return;
  title->transition_started = true;
  title->transition_timer = (int16_t) ((entry + 1u) * 1000u);
  title->sound_events |= SF_GAME_SOUND_TITLE_CONFIRM;
}

void sf_title_state_update(SfGame *game, const SfGameInput *input) {
  SfTitleState *title;
  bool pointer_moved;
  unsigned entry;
  if (!game || !input) return;
  title = &game->title;
  title->sound_events = 0u;

  if (title->transition_timer == 1020 || title->transition_timer == 2020) {
    game->mode = title->transition_timer == 1020
      ? SF_GAME_MODE_CHARACTER_SELECT : SF_GAME_MODE_LOAD_GAME;
    game->character_select_argument = title->transition_timer == 1020 ? 0u : 1u;
    return;
  }
  if (title->transition_timer == 3020) game->quit_requested = true;
  if (title->transition_timer > 0) ++title->transition_timer;

  if (title->fade_steps_remaining > 0u) {
    title->scene_brightness = (uint16_t)
      ((20u - title->fade_steps_remaining) * 50u);
    --title->fade_steps_remaining;
  } else {
    title->scene_brightness =
      sf_title_transition_brightness(title->transition_timer);
    if (!title->title_sound_started) {
      title->sound_events |= SF_GAME_SOUND_TITLE_CUE;
      title->title_sound_started = true;
    }
    if (title->music_delay_frames < 60u) ++title->music_delay_frames;
    if (title->music_delay_frames == 60u && !title->music_started) {
      title->sound_events |= SF_GAME_SOUND_TITLE_MUSIC;
      title->music_started = true;
    }
  }

  for (entry = 0u; entry < SF_GAME_TITLE_ENTRY_COUNT; ++entry)
    title->menu_brightness[entry] = 500u;
  if (!title->transition_started) {
    if (input->up_pressed && title->selection > 0u) {
      do { --title->selection; }
      while (title->selection > 0u &&
             !title->menu_visible[title->selection]);
      title->selection_armed = true;
      title->sound_events |= SF_GAME_SOUND_MENU_MOVE;
    }
    if (input->down_pressed && title->selection < 2u) {
      do { ++title->selection; }
      while (title->selection < 2u &&
             !title->menu_visible[title->selection]);
      title->selection_armed = true;
      title->sound_events |= SF_GAME_SOUND_MENU_MOVE;
    }
  }

  pointer_moved = input->pointer_x != title->previous_pointer_x ||
    input->pointer_y != title->previous_pointer_y;
  for (entry = 0u; entry < SF_GAME_TITLE_ENTRY_COUNT; ++entry) {
    const bool inside = title->menu_visible[entry] && sf_title_inside(
      &sf_title_menu_rectangles[entry], input->pointer_x, input->pointer_y);
    const int transition_start = (int) (entry + 1u) * 1000;
    const bool transition_highlighted =
      title->transition_timer >= transition_start &&
      title->transition_timer < transition_start + 1000;
    if (pointer_moved && title->transition_timer == 0 && inside)
      title->selection_armed = false;
    if (transition_highlighted ||
        (title->transition_timer == 0 && inside && !title->selection_armed)) {
      title->menu_brightness[entry] = 1000u;
      title->selection = (uint8_t) entry;
      if (input->pointer_primary_pressed && title->transition_timer == 0)
        sf_title_begin_transition(title, (uint8_t) entry);
    }
  }
  if (title->selection_armed && title->selection < 3u)
    title->menu_brightness[title->selection] = 1000u;
  if (input->confirm_pressed && !title->transition_started)
    sf_title_begin_transition(title, title->selection);
  if (input->cancel_pressed && !title->transition_started)
    sf_title_begin_transition(title, 2u);

  for (entry = 0u; entry < SF_GAME_TITLE_SMOKE_COUNT; ++entry) {
    const int frame_count = game->config.title_smoke_frame_count[entry];
    const int first = title->smoke_delay[entry];
    const int end = first + frame_count;
    title->smoke_frame[entry] = -1;
    if (frame_count <= 0) continue;
    if (title->animation_frame >= first && title->animation_frame < end)
      title->smoke_frame[entry] = (int16_t) (title->animation_frame - first);
    if (title->animation_frame == end - 1) {
      title->smoke_delay[entry] = title->animation_frame + frame_count + 30 +
        sf_title_random(title) % 100;
    }
  }
  ++title->animation_frame;
  title->previous_pointer_x = input->pointer_x;
  title->previous_pointer_y = input->pointer_y;
}
