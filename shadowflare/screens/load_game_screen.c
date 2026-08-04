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

#include "screens/load_game_screen.h"

#include <stdio.h>
#include <string.h>

static void sf_load_draw_pattern(
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

static void sf_load_draw_text(
    SfRenderer *renderer, const SfIndexedImage *font, const char *text,
    int x, int y, uint16_t color, uint16_t brightness) {
  sf_renderer_draw_text(
    renderer, font, text, x + 1, y + 1, 0u, brightness);
  sf_renderer_draw_text(renderer, font, text, x, y, color, brightness);
}

static const char *sf_load_job_name(int32_t job, int32_t gender) {
  if (job == 5) return "Hunter";
  if (job == 6) return "Warrior";
  if (job == 9) return gender == 1 ? "Wizard" : "Witch";
  if (job == 16) return "Mercenary";
  return "";
}

static void sf_load_draw_summary(
    SfRenderer *renderer, const SfIndexedImage *font,
    const SfSaveSummary *summary, int x, int y, bool selected,
    uint16_t brightness) {
  const uint16_t label = selected
    ? sf_rgb555(28u, 24u, 16u) : sf_rgb555(14u, 12u, 8u);
  const uint16_t value = selected
    ? sf_rgb555(28u, 28u, 28u) : sf_rgb555(14u, 14u, 14u);
  char text[48];
  sf_load_draw_text(renderer, font, "Level.", x + 39, y + 12,
    label, brightness);
  (void) snprintf(text, sizeof(text), "       %d", summary->level);
  sf_load_draw_text(renderer, font, text, x + 39, y + 12,
    value, brightness);
  sf_load_draw_text(renderer, font, "Job.", x + 104, y + 12,
    label, brightness);
  (void) snprintf(text, sizeof(text), "     %s",
    sf_load_job_name(summary->job, summary->gender));
  sf_load_draw_text(renderer, font, text, x + 104, y + 12,
    value, brightness);
  sf_load_draw_text(renderer, font, "Sex.", x + 200, y + 12,
    label, brightness);
  (void) snprintf(text, sizeof(text), "     %s",
    summary->gender == 1 ? "Male" : "Female");
  sf_load_draw_text(renderer, font, text, x + 200, y + 12,
    value, brightness);
  sf_load_draw_text(renderer, font, "Name.", x + 39, y + 32,
    label, brightness);
  (void) snprintf(text, sizeof(text), "      %s", summary->name);
  sf_load_draw_text(renderer, font, text, x + 39, y + 32,
    value, brightness);
  sf_load_draw_text(renderer, font, "HP.", x + 39, y + 52,
    label, brightness);
  (void) snprintf(text, sizeof(text), "    %d", summary->life);
  sf_load_draw_text(renderer, font, text, x + 39, y + 52,
    value, brightness);
  sf_load_draw_text(renderer, font, "MP.", x + 100, y + 52,
    label, brightness);
  (void) snprintf(text, sizeof(text), "    %d", summary->mana);
  sf_load_draw_text(renderer, font, text, x + 100, y + 52,
    value, brightness);
  sf_load_draw_text(renderer, font, "EXP.", x + 160, y + 52,
    label, brightness);
  (void) snprintf(text, sizeof(text), "     %d", summary->experience);
  sf_load_draw_text(renderer, font, text, x + 160, y + 52,
    value, brightness);
}

static void sf_load_draw_slots(
    SfRenderer *renderer, const SfLoadGameAssets *assets,
    const SfLoadGameState *state, uint16_t brightness) {
  static const uint8_t hover_frames[8] = {3u, 2u, 1u, 0u, 0u, 1u, 2u, 3u};
  const SfIndexedImage *font = &assets->font.images[0].image;
  uint8_t index;
  for (index = 0u; index < SF_SAVE_SLOT_COUNT; ++index) {
    const int x = 32 + (index & 1u) * 304;
    const int y = 188 + (index >> 1u) * 88;
    const bool selected = index == state->selection;
    const uint16_t item_brightness = selected
      ? brightness : (uint16_t) (brightness / 2u);
    uint8_t number_frame = selected ? 0u : 3u;
    const bool hovered = state->brightness_increasing &&
      state->launch_counter == 0 && state->hover_animation > 28 &&
      (state->hovered_slots & (uint8_t) (1u << index)) != 0u;
    if (hovered) {
      const int rendered = state->hover_animation > 0
        ? state->hover_animation - 1 : 0;
      number_frame = hover_frames[(rendered / 4) & 7];
    }
    sf_load_draw_pattern(
      renderer, &assets->artwork, 40u, x, y, item_brightness);
    sf_load_draw_pattern(
      renderer, &assets->artwork,
      (uint8_t) (42u + index * 4u + number_frame),
      x, y + 28, item_brightness);
    if (index < assets->catalog.count) {
      sf_load_draw_summary(
        renderer, font, &assets->catalog.entries[index],
        x, y, selected, brightness);
    } else {
      sf_load_draw_text(
        renderer, font, "No Data", x + 39, y + 12,
        sf_rgb555(14u, 14u, 14u), brightness);
    }
  }
}

static void sf_load_draw_mode(
    SfRenderer *renderer, const SfLoadGameAssets *assets,
    const SfLoadGameState *state) {
  static const uint8_t mode_normal[3] = {22u, 24u, 20u};
  static const uint8_t mode_selected[3] = {23u, 25u, 21u};
  static const uint8_t network_normal[3] = {26u, 28u, 20u};
  static const uint8_t network_selected[3] = {27u, 29u, 21u};
  const uint8_t *normal = state->screen == 11u
    ? network_normal : mode_normal;
  const uint8_t *selected = state->screen == 11u
    ? network_selected : mode_selected;
  uint8_t entry;
  sf_load_draw_pattern(renderer, &assets->artwork, 38u, 0, 0, 1000u);
  for (entry = 0u; entry < 3u; ++entry)
    sf_load_draw_pattern(
      renderer, &assets->artwork,
      state->dialog_selection == entry ? selected[entry] : normal[entry],
      0, 0, 1000u);
}

static void sf_load_draw_delete(
    SfRenderer *renderer, const SfLoadGameAssets *assets,
    const SfLoadGameState *state) {
  const SfIndexedImage *font = &assets->font.images[0].image;
  char text[48];
  const SfSaveSummary *summary = state->selection < assets->catalog.count
    ? &assets->catalog.entries[state->selection] : NULL;
  sf_load_draw_pattern(renderer, &assets->artwork, 38u, 0, 0, 1000u);
  (void) snprintf(text, sizeof(text), "No. %u  %s",
    (unsigned) state->selection + 1u, summary ? summary->name : "");
  sf_load_draw_text(renderer, font, text, 240, 190,
    sf_rgb555(31u, 31u, 31u), 1000u);
  sf_load_draw_text(renderer, font, "Are you sure you want to delete",
    215, 210, sf_rgb555(31u, 31u, 31u), 1000u);
  sf_load_draw_text(renderer, font, "this saved data?",
    268, 230, sf_rgb555(31u, 31u, 31u), 1000u);
  sf_load_draw_pattern(
    renderer, &assets->artwork,
    state->dialog_selection == 0u ? 15u : 14u, 0, 0, 1000u);
  sf_load_draw_pattern(
    renderer, &assets->artwork,
    state->dialog_selection == 1u ? 17u : 16u, 0, 0, 1000u);
}

static bool sf_load_screen_changed(
    const SfLoadGameScreen *screen, const SfLoadGameAssets *assets,
    const SfLoadGameState *state) {
  return !screen->initialized ||
    memcmp(&screen->previous, state, sizeof(*state)) != 0 ||
    screen->previous_preview_index != assets->preview_catalog_index ||
    screen->previous_save_count != assets->catalog.count;
}

void sf_load_game_screen_init(SfLoadGameScreen *screen) {
  if (!screen) return;
  memset(screen, 0, sizeof(*screen));
  screen->previous_preview_index = -1;
}

void sf_load_game_screen_draw(
    SfLoadGameScreen *screen, SfRenderer *renderer,
    const SfLoadGameAssets *assets, const SfGame *game) {
  const SfLoadGameState *state;
  uint16_t brightness;
  if (!screen || !renderer || !assets || !assets->loaded || !game ||
      game->mode != SF_GAME_MODE_LOAD_GAME) return;
  state = &game->load_game;
  if (!sf_load_screen_changed(screen, assets, state)) return;
  brightness = state->background_brightness;
  sf_renderer_clear(renderer, 0u);
  sf_load_draw_pattern(renderer, &assets->artwork, 41u, 0, 0, brightness);
  sf_load_draw_pattern(renderer, &assets->artwork, 37u, 0, 0, brightness);
  if (assets->preview.pixels)
    sf_renderer_draw_rgb555(renderer, &assets->preview, 224, 60, brightness);
  sf_load_draw_pattern(renderer, &assets->artwork, 39u, 0, 0, brightness);
  sf_load_draw_slots(renderer, assets, state, brightness);
  sf_load_draw_pattern(
    renderer, &assets->artwork,
    state->launch_counter >= 1000 && state->launch_counter < 2000
      ? 3u : 2u, 0, 0, brightness);
  sf_load_draw_pattern(
    renderer, &assets->artwork,
    state->launch_counter >= 2000 ? 1u : 0u, 0, 0, brightness);
  if (assets->catalog.count > 0u) {
    sf_load_draw_pattern(renderer, &assets->artwork, 4u, 0, 0, brightness);
    sf_load_draw_pattern(renderer, &assets->artwork, 6u, 0, 0, brightness);
  }
  if (state->screen == 1u) sf_load_draw_delete(renderer, assets, state);
  else if (state->screen == 10u || state->screen == 11u)
    sf_load_draw_mode(renderer, assets, state);
  screen->previous = *state;
  screen->previous_preview_index = assets->preview_catalog_index;
  screen->previous_save_count = assets->catalog.count;
  screen->initialized = true;
}
