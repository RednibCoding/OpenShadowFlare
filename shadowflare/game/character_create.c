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

#include "game/character_create.h"

#include "game/title.h"

#include <string.h>

typedef struct SfCharacterCreateRectangle {
  int16_t left;
  int16_t right;
  int16_t top;
  int16_t bottom;
} SfCharacterCreateRectangle;

static const SfCharacterCreateRectangle sf_character_rectangles[2] = {
  {0xa7, 0x12f, 0x6a, 0x17b},
  {0x185, 0x1ea, 0x6a, 0x177}
};

static const SfCharacterCreateRectangle sf_mode_rectangles[3] = {
  {0xdb, 0x1a5, 0xcf, 0xdc},
  {0xdd, 0x1a2, 0xf1, 0xfe},
  {0x11b, 0x165, 0x116, 0x123}
};

static const SfCharacterCreateRectangle sf_network_rectangles[3] = {
  {0xf0, 0x18f, 0xcf, 0xdc},
  {0xee, 0x191, 0xf1, 0xfe},
  {0x11b, 0x165, 0x116, 0x123}
};

static const SfCharacterCreateRectangle sf_back_rectangle = {
  0x188, 0x1d2, 0x1c2, 0x1cf};
static const SfCharacterCreateRectangle sf_exit_rectangle = {
  0x23f, 0x279, 7, 0x14};
static const SfCharacterCreateRectangle sf_name_confirm_rectangle = {
  0x237, 0x25b, 0x1c3, 0x1ce};

static bool sf_character_inside(
    const SfCharacterCreateRectangle *rectangle, int16_t x, int16_t y) {
  return x > rectangle->left && x < rectangle->right &&
    y > rectangle->top && y < rectangle->bottom;
}

static void sf_character_erase_name(SfCharacterCreateState *state) {
  uint8_t index;
  if (state->name_length == 0u) return;
  index = (uint8_t) (state->name_length - 1u);
  while (index > 0u &&
      ((uint8_t) state->name[index] & 0xc0u) == 0x80u) --index;
  state->name_length = index;
  state->name[index] = '\0';
}

static void sf_character_append_name(
    SfCharacterCreateState *state, const SfGameInput *input) {
  uint8_t source = 0u;
  while (source < input->text_length && state->name_length < 15u) {
    const uint8_t first = (uint8_t) input->text[source];
    uint8_t length = 1u;
    if ((first & 0xe0u) == 0xc0u) length = 2u;
    else if ((first & 0xf0u) == 0xe0u) length = 3u;
    else if ((first & 0xf8u) == 0xf0u) length = 4u;
    if (source + length > input->text_length ||
        state->name_length + length > 15u) break;
    memcpy(state->name + state->name_length, input->text + source, length);
    state->name_length = (uint8_t) (state->name_length + length);
    source = (uint8_t) (source + length);
  }
  state->name[state->name_length] = '\0';
}

static void sf_character_begin_name(SfCharacterCreateState *state) {
  state->gender = state->selection == 0u ? 1u : 0u;
  state->character_transition_counter = 1000;
  state->rendered_transition_counter = 1000;
  state->screen = 1u;
  state->name_entry_active = true;
  state->input_latch = true;
  state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
}

void sf_character_create_state_init(SfGame *game) {
  SfCharacterCreateState *state;
  if (!game) return;
  state = &game->character_create;
  memset(state, 0, sizeof(*state));
  state->fade_steps_remaining = 20u;
  state->fade_target = 0;
  state->selection = 0u;
  state->gender = 1u;
  state->input_latch = true;
}

static void sf_character_update_transition(SfCharacterCreateState *state) {
  state->rendered_transition_counter = state->character_transition_counter;
  if ((state->character_transition_counter >= 1000 &&
       state->character_transition_counter < 1020) ||
      (state->character_transition_counter >= 2000 &&
       state->character_transition_counter < 2020)) {
    ++state->character_transition_counter;
  } else if (state->character_transition_counter == 1020 ||
             state->character_transition_counter == 2020) {
    state->character_transition_counter = 0;
  }
}

static void sf_character_update_name(
    SfCharacterCreateState *state, const SfGameInput *input) {
  if (input->backspace_pressed) sf_character_erase_name(state);
  sf_character_append_name(state, input);
  state->name_confirm_hovered = sf_character_inside(
    &sf_name_confirm_rectangle, input->pointer_x, input->pointer_y);
  if (input->cancel_pressed ||
      (input->pointer_primary_pressed && sf_character_inside(
        &sf_back_rectangle, input->pointer_x, input->pointer_y))) {
    state->name_entry_active = false;
    state->screen = 0u;
    state->input_latch = true;
    state->character_transition_counter = 2000;
    state->rendered_transition_counter = 2000;
    state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
    return;
  }
  if (state->name_length > 0u &&
      (input->confirm_pressed ||
       (input->pointer_primary_pressed && state->name_confirm_hovered))) {
    state->name_entry_active = false;
    state->screen = 10u;
    state->selection = 0u;
    state->input_latch = true;
    state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
  }
}

static void sf_character_update_choice(
    SfCharacterCreateState *state, const SfGameInput *input) {
  const bool pointer_moved =
    input->pointer_x != state->previous_pointer_x ||
    input->pointer_y != state->previous_pointer_y;
  uint8_t choice;
  if (!state->input_latch && state->fade_steps_remaining == 0u) {
    const uint8_t previous = state->selection;
    if (input->up_pressed || input->left_pressed) state->selection = 0u;
    if (input->down_pressed || input->right_pressed) state->selection = 1u;
    if (previous != state->selection)
      state->sound_events |= SF_GAME_SOUND_MENU_MOVE;
  }
  for (choice = 0u; choice < 2u; ++choice) {
    if (pointer_moved && sf_character_inside(
          &sf_character_rectangles[choice],
          input->pointer_x, input->pointer_y)) {
      if (state->selection != choice)
        state->sound_events |= SF_GAME_SOUND_MENU_MOVE;
      state->selection = choice;
    }
    if (!state->input_latch && input->pointer_primary_pressed &&
        sf_character_inside(&sf_character_rectangles[choice],
          input->pointer_x, input->pointer_y)) {
      state->selection = choice;
      sf_character_begin_name(state);
      break;
    }
  }
  if (state->screen == 0u && !state->input_latch && input->confirm_pressed)
    sf_character_begin_name(state);
}

static void sf_character_update_three_choices(
    SfCharacterCreateState *state, const SfGameInput *input,
    const SfCharacterCreateRectangle *rectangles, bool network_screen) {
  uint8_t choice;
  uint8_t previous = state->selection;
  bool pointer_confirm = false;
  if (!state->input_latch) {
    if ((input->up_pressed || input->left_pressed) && state->selection > 0u)
      --state->selection;
    if ((input->down_pressed || input->right_pressed) &&
        state->selection < 2u) ++state->selection;
  }
  for (choice = 0u; choice < 3u; ++choice) {
    if (sf_character_inside(
          &rectangles[choice], input->pointer_x, input->pointer_y)) {
      state->selection = choice;
      if (!state->input_latch && input->pointer_primary_pressed)
        pointer_confirm = true;
    }
  }
  if (previous != state->selection)
    state->sound_events |= SF_GAME_SOUND_MENU_MOVE;
  if (input->cancel_pressed && !state->input_latch) {
    state->selection = 0u;
    state->screen = network_screen ? 10u : 1u;
    state->name_entry_active = !network_screen;
    state->input_latch = true;
    state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
    return;
  }
  if (!state->input_latch && (input->confirm_pressed || pointer_confirm)) {
    if (network_screen && state->selection < 2u) {
      state->screen = 20u;
    } else if (network_screen) {
      state->selection = 0u;
      state->screen = 10u;
    } else if (state->selection == 0u) {
      state->selection = 0u;
      state->screen = 11u;
    } else if (state->selection == 1u) {
      state->screen = 20u;
    } else if (state->selection == 2u) {
      state->selection = 0u;
      state->screen = 1u;
      state->name_entry_active = true;
    }
    state->input_latch = true;
    state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
  }
}

void sf_character_create_state_update(
    SfGame *game, const SfGameInput *input) {
  SfCharacterCreateState *state;
  int phase;
  if (!game || !input) return;
  state = &game->character_create;
  state->sound_events = 0u;
  state->name_confirm_hovered = false;
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
  state->background_brightness = state->fade_value < 0
    ? 0u : (uint16_t) state->fade_value;
  phase = state->launch_counter % 1000;
  state->mode_brightness = state->launch_counter == 0
    ? 1000u : (uint16_t) (phase < 20 ? 1000 - phase * 50 : 0);
  if (state->mode_brightness > state->background_brightness)
    state->mode_brightness = state->background_brightness;
  if (phase > 5) {
    state->fade_value = 1000 + ((5 - phase) * 1000) / 15;
    if (state->fade_value < 0) state->fade_value = 0;
    state->background_brightness = (uint16_t) state->fade_value;
  }

  sf_character_update_transition(state);
  if (state->screen == 1u && state->name_entry_active) {
    sf_character_update_name(state, input);
  } else if (state->screen == 0u) {
    sf_character_update_choice(state, input);
    if (state->launch_counter == 0 && !state->input_latch &&
        (input->cancel_pressed ||
         (input->pointer_primary_pressed && sf_character_inside(
           &sf_back_rectangle, input->pointer_x, input->pointer_y)))) {
      state->launch_counter = 1000;
      state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
    }
    if (state->launch_counter == 0 && !state->input_latch &&
        input->pointer_primary_pressed && sf_character_inside(
          &sf_exit_rectangle, input->pointer_x, input->pointer_y)) {
      state->launch_counter = 2000;
      state->sound_events |= SF_GAME_SOUND_MENU_CONFIRM;
    }
  } else if (state->screen == 10u) {
    sf_character_update_three_choices(
      state, input, sf_mode_rectangles, false);
  } else if (state->screen == 11u) {
    sf_character_update_three_choices(
      state, input, sf_network_rectangles, true);
  } else if (state->screen == 20u) {
    if (state->launch_counter == 0) state->launch_counter = 5010;
    if (state->launch_counter == 5024) {
      game->player_gender = state->gender;
      game->mode = SF_GAME_MODE_LOADING;
    }
  }
  state->previous_pointer_x = input->pointer_x;
  state->previous_pointer_y = input->pointer_y;
  if (state->input_latch) state->input_latch = false;
}
