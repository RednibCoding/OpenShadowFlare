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

#include "screens/character_create_screen.h"

#include <string.h>

static void sf_character_draw_pattern(
    SfRenderer *renderer, const SfNjpDecodedResource *resource,
    uint8_t source_pattern, int offset_x, int offset_y,
    uint16_t brightness) {
  const SfNjpDecodedPattern *pattern =
    sf_njp_decoded_pattern(resource, source_pattern);
  uint8_t reference;
  if (!pattern || pattern->palette >= resource->palette_count) return;
  for (reference = 0u; reference < pattern->reference_count; ++reference) {
    const SfNjpDecodedReference *item =
      &resource->references[pattern->first_reference + reference];
    SfIndexedImage image;
    if (item->part >= resource->part_count) continue;
    image = resource->parts[item->part].image;
    image.palette = resource->palettes[pattern->palette];
    sf_renderer_draw_indexed(
      renderer, &image, item->x + offset_x, item->y + offset_y,
      brightness, 1000u, SF_BLEND_MASKED, NULL);
  }
}

static uint16_t sf_character_brightness(const SfCharacterCreateState *state) {
  return state->background_brightness < state->mode_brightness
    ? state->background_brightness : state->mode_brightness;
}

static int sf_character_name_cells(const SfCharacterCreateState *state) {
  uint8_t index = 0u;
  int cells = 0;
  while (index < state->name_length) {
    const uint8_t first = (uint8_t) state->name[index];
    uint8_t length = 1u;
    if ((first & 0xe0u) == 0xc0u) length = 2u;
    else if ((first & 0xf0u) == 0xe0u) length = 3u;
    else if ((first & 0xf8u) == 0xf0u) length = 4u;
    if (index + length > state->name_length) break;
    cells += length == 1u ? 1 : 2;
    index = (uint8_t) (index + length);
  }
  return cells;
}

static void sf_character_draw_name(
    SfRenderer *renderer, const SfCharacterCreateAssets *assets,
    const SfCharacterCreateState *state, uint16_t brightness) {
  const SfIndexedImage *font = &assets->font.images[0].image;
  const int name_cells = sf_character_name_cells(state);
  const int caret_column = name_cells > 20 ? 20 : name_cells;
  SfRect rectangle = {190, 407, 130, 20};
  sf_renderer_fill_rect(
    renderer, rectangle, sf_rgb555(8u, 8u, 8u));
  rectangle.x = (int16_t) (194 + caret_column * 6);
  rectangle.y = 411;
  rectangle.width = 6;
  rectangle.height = 12;
  sf_renderer_fill_rect(renderer, rectangle, sf_rgb555(16u, 0u, 0u));
  sf_renderer_draw_text(
    renderer, font, state->name, 194, 411,
    sf_rgb555(31u, 31u, 31u), brightness);
}

static void sf_character_draw_choice(
    SfRenderer *renderer, const SfCharacterCreateAssets *assets,
    const SfCharacterCreateState *state, uint16_t brightness) {
  int male_x = 0;
  int female_x = 0;
  uint16_t male_brightness = brightness;
  uint16_t female_brightness = brightness;
  const int transition = state->rendered_transition_counter;
  if (transition >= 2000 && transition <= 2020) {
    const int phase = transition - 2000;
    if (state->gender == 1u) {
      male_x = 97 - phase * 97 / 20;
      female_brightness = (uint16_t) (brightness * phase / 20);
    } else {
      female_x = -97 + phase * 97 / 20;
      male_brightness = (uint16_t) (brightness * phase / 20);
    }
  }
  sf_character_draw_pattern(
    renderer, &assets->artwork, state->selection == 0u ? 8u : 7u,
    male_x, 0, male_brightness);
  sf_character_draw_pattern(
    renderer, &assets->artwork, 11u, male_x, 0,
    (uint16_t) (male_brightness / 2u));
  sf_character_draw_pattern(
    renderer, &assets->artwork, state->selection == 1u ? 10u : 9u,
    female_x, 0, female_brightness);
  sf_character_draw_pattern(
    renderer, &assets->artwork, 12u, female_x, 0,
    (uint16_t) (female_brightness / 2u));
  sf_character_draw_pattern(
    renderer, &assets->artwork, 34u, 0, 0, brightness);
}

static void sf_character_draw_editor(
    SfRenderer *renderer, const SfCharacterCreateAssets *assets,
    const SfCharacterCreateState *state, uint16_t brightness) {
  const int transition = state->rendered_transition_counter;
  const int phase = transition >= 1000 && transition <= 1020
    ? transition - 1000 : 20;
  const uint16_t other_brightness =
    (uint16_t) (brightness * (20 - phase) / 20);
  int selected_x;
  if (state->gender == 1u) {
    sf_character_draw_pattern(
      renderer, &assets->artwork, 9u, 0, 0, other_brightness);
    sf_character_draw_pattern(
      renderer, &assets->artwork, 12u, 0, 0,
      (uint16_t) (other_brightness / 2u));
    selected_x = phase * 97 / 20;
    sf_character_draw_pattern(
      renderer, &assets->artwork, 8u, selected_x, 0, brightness);
    sf_character_draw_pattern(
      renderer, &assets->artwork, 11u, selected_x, 0,
      (uint16_t) (brightness / 2u));
  } else {
    sf_character_draw_pattern(
      renderer, &assets->artwork, 7u, 0, 0, other_brightness);
    sf_character_draw_pattern(
      renderer, &assets->artwork, 11u, 0, 0,
      (uint16_t) (other_brightness / 2u));
    selected_x = -phase * 97 / 20;
    sf_character_draw_pattern(
      renderer, &assets->artwork, 10u, selected_x, 0, brightness);
    sf_character_draw_pattern(
      renderer, &assets->artwork, 12u, selected_x, 0,
      (uint16_t) (brightness / 2u));
  }
  sf_character_draw_pattern(
    renderer, &assets->artwork, 35u, 0, 0, brightness);
  sf_character_draw_name(renderer, assets, state, brightness);
  if (state->name_length > 0u) {
    sf_character_draw_pattern(
      renderer, &assets->artwork,
      state->name_confirm_hovered && state->launch_counter == 0 ? 5u : 4u,
      0, 0, brightness);
  }
}

static void sf_character_draw_mode(
    SfRenderer *renderer, const SfCharacterCreateAssets *assets,
    const SfCharacterCreateState *state) {
  static const uint8_t normal_patterns[3] = {22u, 24u, 20u};
  static const uint8_t selected_patterns[3] = {23u, 25u, 21u};
  uint8_t entry;
  sf_character_draw_pattern(renderer, &assets->artwork, 38u, 0, 0, 1000u);
  for (entry = 0u; entry < 3u; ++entry) {
    sf_character_draw_pattern(
      renderer, &assets->artwork,
      state->selection == entry
        ? selected_patterns[entry] : normal_patterns[entry],
      0, 0, 1000u);
  }
}

static void sf_character_draw_network(
    SfRenderer *renderer, const SfCharacterCreateAssets *assets,
    const SfCharacterCreateState *state) {
  static const uint8_t normal_patterns[3] = {26u, 28u, 20u};
  static const uint8_t selected_patterns[3] = {27u, 29u, 21u};
  uint8_t entry;
  sf_character_draw_pattern(renderer, &assets->artwork, 38u, 0, 0, 1000u);
  for (entry = 0u; entry < 3u; ++entry) {
    sf_character_draw_pattern(
      renderer, &assets->artwork,
      state->selection == entry
        ? selected_patterns[entry] : normal_patterns[entry],
      0, 0, 1000u);
  }
}

static bool sf_character_screen_changed(
    const SfCharacterCreateScreen *screen,
    const SfCharacterCreateState *state) {
  if (!screen->initialized) return true;
  return screen->previous.background_brightness !=
      state->background_brightness ||
    screen->previous.mode_brightness != state->mode_brightness ||
    screen->previous.rendered_transition_counter !=
      state->rendered_transition_counter ||
    screen->previous.launch_counter != state->launch_counter ||
    screen->previous.screen != state->screen ||
    screen->previous.selection != state->selection ||
    screen->previous.gender != state->gender ||
    screen->previous.name_length != state->name_length ||
    screen->previous.name_confirm_hovered != state->name_confirm_hovered ||
    memcmp(screen->previous.name, state->name, sizeof(state->name)) != 0;
}

void sf_character_create_screen_init(SfCharacterCreateScreen *screen) {
  if (screen) memset(screen, 0, sizeof(*screen));
}

void sf_character_create_screen_draw(
    SfCharacterCreateScreen *screen, SfRenderer *renderer,
    const SfCharacterCreateAssets *assets, const SfGame *game) {
  const SfCharacterCreateState *state;
  uint16_t brightness;
  if (!screen || !renderer || !assets || !assets->loaded || !game ||
      game->mode != SF_GAME_MODE_CHARACTER_SELECT) return;
  state = &game->character_create;
  if (!sf_character_screen_changed(screen, state)) return;
  brightness = sf_character_brightness(state);
  sf_renderer_clear(renderer, 0u);
  sf_character_draw_pattern(
    renderer, &assets->artwork, 41u, 0, 0, brightness);
  sf_character_draw_pattern(
    renderer, &assets->artwork, 36u, 0, 0, brightness);
  sf_character_draw_pattern(
    renderer, &assets->artwork,
    state->launch_counter >= 1000 && state->launch_counter < 2000
      ? 3u : 2u,
    0, 0, brightness);
  sf_character_draw_pattern(
    renderer, &assets->artwork,
    state->launch_counter >= 2000 ? 1u : 0u,
    0, 0, brightness);
  if (state->screen == 0u) {
    sf_character_draw_choice(renderer, assets, state, brightness);
  } else if (state->screen == 1u) {
    sf_character_draw_editor(renderer, assets, state, brightness);
  } else if (state->screen == 10u) {
    sf_character_draw_mode(renderer, assets, state);
  } else if (state->screen == 11u) {
    sf_character_draw_network(renderer, assets, state);
  }
  screen->previous = *state;
  screen->initialized = true;
}
