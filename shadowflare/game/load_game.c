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

#include "game/load_game.h"

#include "data/save.h"
#include "game/title.h"

#include <string.h>

typedef struct SfLoadRectangle {
  int16_t left;
  int16_t right;
  int16_t top;
  int16_t bottom;
} SfLoadRectangle;

static const SfLoadRectangle sf_load_slot_rectangles[6] = {
  {32, 319, 188, 264}, {336, 623, 188, 264},
  {32, 319, 276, 352}, {336, 623, 276, 352},
  {32, 319, 364, 440}, {336, 623, 364, 440}
};
static const SfLoadRectangle sf_load_back_rectangle =
  {0x188, 0x1d2, 0x1c2, 0x1cf};
static const SfLoadRectangle sf_load_exit_rectangle =
  {0x23f, 0x279, 7, 0x14};
static const SfLoadRectangle sf_load_continue_rectangle =
  {0x236, 0x25c, 0x1c2, 0x1cf};
static const SfLoadRectangle sf_load_delete_rectangle =
  {0x3d, 0xa9, 0x1c2, 0x1cf};
static const SfLoadRectangle sf_delete_dialog_rectangles[2] = {
  {0xf7, 0x129, 0x113, 0x120},
  {0x15c, 0x183, 0x113, 0x120}
};
static const SfLoadRectangle sf_mode_rectangles[3] = {
  {0xdb, 0x1a5, 0xcf, 0xdc},
  {0xdd, 0x1a2, 0xf1, 0xfe},
  {0x11b, 0x165, 0x116, 0x123}
};
static const SfLoadRectangle sf_network_rectangles[3] = {
  {0xf0, 0x18f, 0xcf, 0xdc},
  {0xee, 0x191, 0xf1, 0xfe},
  {0x11b, 0x165, 0x116, 0x123}
};

static bool sf_load_inside(
    const SfLoadRectangle *rectangle, int16_t x, int16_t y) {
  return x > rectangle->left && x < rectangle->right &&
    y > rectangle->top && y < rectangle->bottom;
}

static void sf_load_begin_choice(SfGame *game) {
  SfLoadGameState *state = &game->load_game;
  state->selected_save = state->selection;
  state->selected_file_slot = (int8_t)
    game->config.saved_game_file_slots[state->selection];
  game->player_gender = game->config.saved_game_genders[state->selection];
  state->screen = 10u;
  state->input_latch = true;
  state->brightness_increasing = false;
  state->dialog_selection = 0u;
  state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
}

void sf_load_game_state_init(SfGame *game) {
  SfLoadGameState *state;
  if (!game) return;
  state = &game->load_game;
  memset(state, 0, sizeof(*state));
  state->fade_steps_remaining = 20u;
  state->brightness_increasing = true;
  state->input_latch = true;
  state->delete_request = -1;
  state->selected_file_slot = -1;
}

static void sf_load_update_delete(
    SfLoadGameState *state, const SfGameInput *input) {
  const bool pointer_moved =
    input->pointer_x != state->dialog_previous_pointer_x ||
    input->pointer_y != state->dialog_previous_pointer_y;
  uint8_t choice;
  if (!state->input_latch) {
    if (input->up_pressed || input->left_pressed)
      state->dialog_selection = 0u;
    if (input->down_pressed || input->right_pressed)
      state->dialog_selection = 1u;
  }
  for (choice = 0u; choice < 2u; ++choice) {
    if (pointer_moved && sf_load_inside(
          &sf_delete_dialog_rectangles[choice],
          input->pointer_x, input->pointer_y)) {
      state->dialog_selection = choice;
    }
  }
  if (!state->input_latch && input->cancel_pressed) {
    state->screen = 0u;
    state->brightness_increasing = true;
    state->input_latch = true;
    state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
  } else if (!state->input_latch &&
      (input->confirm_pressed || input->pointer_primary_pressed)) {
    const bool pointer_choice = sf_load_inside(
      &sf_delete_dialog_rectangles[state->dialog_selection],
      input->pointer_x, input->pointer_y);
    if (input->confirm_pressed || pointer_choice) {
      if (state->dialog_selection == 0u) {
        state->delete_request = (int8_t) state->selection;
        state->selection = 0u;
      }
      state->screen = 0u;
      state->brightness_increasing = true;
      state->input_latch = true;
      state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
    }
  }
  state->dialog_previous_pointer_x = input->pointer_x;
  state->dialog_previous_pointer_y = input->pointer_y;
}

static void sf_load_update_dialog(
    SfLoadGameState *state, const SfGameInput *input,
    const SfLoadRectangle *rectangles, bool network) {
  uint8_t choice;
  const uint8_t previous = state->dialog_selection;
  bool pointer_confirm = false;
  if (!state->input_latch) {
    if ((input->up_pressed || input->left_pressed) &&
        state->dialog_selection > 0u) --state->dialog_selection;
    if ((input->down_pressed || input->right_pressed) &&
        state->dialog_selection < 2u) ++state->dialog_selection;
  }
  for (choice = 0u; choice < 3u; ++choice) {
    if (sf_load_inside(
          &rectangles[choice], input->pointer_x, input->pointer_y)) {
      state->dialog_selection = choice;
      if (!state->input_latch && input->pointer_primary_pressed)
        pointer_confirm = true;
    }
  }
  if (state->dialog_selection != previous)
    state->sound_events |= SF_GAME_SOUND_MENU_MOVE;
  if (!state->input_latch && input->cancel_pressed) {
    state->screen = network ? 10u : 0u;
    state->brightness_increasing = !network;
    state->dialog_selection = 0u;
    state->input_latch = true;
    state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
  } else if (!state->input_latch &&
      (input->confirm_pressed || pointer_confirm)) {
    if (!network && state->dialog_selection == 0u) {
      state->screen = 11u;
      state->dialog_selection = 0u;
    } else if ((!network && state->dialog_selection == 1u) ||
               (network && state->dialog_selection < 2u)) {
      state->screen = 20u;
    } else if (network) {
      state->screen = 10u;
      state->dialog_selection = 0u;
    } else {
      state->screen = 0u;
      state->brightness_increasing = true;
      state->dialog_selection = 0u;
    }
    state->input_latch = true;
    state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
  }
}

static void sf_load_update_slots(
    SfGame *game, const SfGameInput *input, uint8_t save_count) {
  SfLoadGameState *state = &game->load_game;
  uint8_t index;
  const uint8_t previous = state->selection;
  bool hovered = false;
  state->hovered_slots = 0u;
  if (!state->input_latch && save_count > 0u) {
    if (input->up_pressed)
      state->selection = state->selection >= 2u
        ? (uint8_t) (state->selection - 2u) : 0u;
    if (input->down_pressed) {
      uint8_t target = (uint8_t) (state->selection + 2u);
      state->selection = target < save_count
        ? target : (uint8_t) (save_count - 1u);
    }
    if (input->left_pressed && state->selection > 0u) --state->selection;
    if (input->right_pressed && state->selection + 1u < save_count)
      ++state->selection;
  }
  if (state->selection != previous)
    state->sound_events |= SF_GAME_SOUND_MENU_MOVE;
  if (state->click_cooldown > 0) --state->click_cooldown;
  for (index = 0u; index < SF_SAVE_SLOT_COUNT; ++index) {
    const bool inside = sf_load_inside(
      &sf_load_slot_rectangles[index], input->pointer_x, input->pointer_y);
    if (inside) state->hovered_slots |= (uint8_t) (1u << index);
    if (!state->brightness_increasing || state->launch_counter != 0 ||
        state->hover_animation < 28 || !inside) continue;
    ++state->hover_animation;
    hovered = true;
    if (!state->input_latch && input->pointer_primary_pressed &&
        index < save_count) {
      const bool double_click = state->click_cooldown > 0 &&
        state->previous_pointer_x == input->pointer_x &&
        state->previous_pointer_y == input->pointer_y;
      state->selection = index;
      state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
      if (double_click) sf_load_begin_choice(game);
    }
  }
  if (!state->input_latch && input->pointer_primary_pressed) {
    state->click_cooldown = 10;
    state->previous_pointer_x = input->pointer_x;
    state->previous_pointer_y = input->pointer_y;
  }
  if (state->fade_steps_remaining == 0u && state->hover_animation < 28)
    ++state->hover_animation;
  if (state->hover_animation >= 28 && !hovered)
    state->hover_animation = 64;

  if (!state->input_latch && state->launch_counter == 0) {
    if (input->cancel_pressed ||
        (input->pointer_primary_pressed && sf_load_inside(
          &sf_load_back_rectangle, input->pointer_x, input->pointer_y))) {
      state->launch_counter = 1000;
      state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
    } else if (input->pointer_primary_pressed && sf_load_inside(
        &sf_load_exit_rectangle, input->pointer_x, input->pointer_y)) {
      state->launch_counter = 2000;
      state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
    } else if (save_count > 0u &&
        (input->confirm_pressed ||
         (input->pointer_primary_pressed && sf_load_inside(
           &sf_load_continue_rectangle,
           input->pointer_x, input->pointer_y)))) {
      sf_load_begin_choice(game);
    } else if (save_count > 0u &&
        (input->delete_pressed ||
         (input->pointer_primary_pressed && sf_load_inside(
           &sf_load_delete_rectangle,
           input->pointer_x, input->pointer_y)))) {
      state->screen = 1u;
      state->brightness_increasing = false;
      state->dialog_selection = 1u;
      state->input_latch = true;
      state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
    }
  }
}

void sf_load_game_state_update(SfGame *game, const SfGameInput *input) {
  SfLoadGameState *state;
  int phase;
  int mode_brightness;
  uint8_t save_count;
  if (!game || !input) return;
  state = &game->load_game;
  state->sound_events = 0u;
  save_count = game->config.saved_game_count > SF_SAVE_SLOT_COUNT
    ? SF_SAVE_SLOT_COUNT : game->config.saved_game_count;
  if (state->selection >= save_count && save_count > 0u)
    state->selection = (uint8_t) (save_count - 1u);

  if (state->launch_counter == 1022) {
    game->mode = SF_GAME_MODE_TITLE;
    sf_title_state_init(game);
    return;
  }
  if (state->launch_counter == 2022) game->quit_requested = true;
  if (state->launch_counter > 0) ++state->launch_counter;
  if (state->fade_steps_remaining > 0u) {
    state->fade_value = (21 - state->fade_steps_remaining) * 50;
    state->fade_target = state->fade_value;
    --state->fade_steps_remaining;
  }
  if (!state->brightness_increasing) {
    if (state->fade_target > 500) state->fade_target -= 80;
  } else if (state->fade_steps_remaining == 0u && state->fade_target < 1000) {
    state->fade_target += 80;
  }
  phase = state->launch_counter % 1000;
  mode_brightness = state->fade_steps_remaining == 0u
    ? 1000 - phase * 100 : state->fade_target;
  if (mode_brightness < 0) mode_brightness = 0;
  if (phase > 5) {
    state->fade_value = 1000 + ((5 - phase) * 1000) / 15;
    if (state->fade_value < 0) state->fade_value = 0;
  }
  if (mode_brightness > state->fade_target)
    mode_brightness = state->fade_target;
  state->background_brightness = (uint16_t) (
    state->fade_value < mode_brightness
      ? state->fade_value : mode_brightness);

  if (state->screen == 1u) {
    sf_load_update_delete(state, input);
  } else if (state->screen == 0u) {
    sf_load_update_slots(game, input, save_count);
  } else if (state->screen == 10u) {
    sf_load_update_dialog(state, input, sf_mode_rectangles, false);
  } else if (state->screen == 11u) {
    sf_load_update_dialog(state, input, sf_network_rectangles, true);
  } else if (state->screen == 20u) {
    if (state->launch_counter == 0) state->launch_counter = 5010;
    if (state->launch_counter == 5024) game->mode = SF_GAME_MODE_LOADING;
  }
  if (state->input_latch) state->input_latch = false;
}
