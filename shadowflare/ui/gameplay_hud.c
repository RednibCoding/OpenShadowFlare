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

#include "ui/gameplay_hud.h"

#define SF_HUD_LIFE_PATTERN 0u
#define SF_HUD_MANA_PATTERN 3u
#define SF_HUD_LEFT_FRAME_PATTERN 7u
#define SF_HUD_RIGHT_FRAME_PATTERN 8u
#define SF_HUD_RUN_PATTERN 10u
#define SF_HUD_WALK_PATTERN 11u
#define SF_HUD_EXPERIENCE_PATTERN 14u
#define SF_HUD_EXPERIENCE_FRAME_PATTERN 15u
#define SF_HUD_FIRST_DIGIT_PATTERN 19u

static bool sf_hud_intersection(
    SfRect first, const SfRect *second, SfRect *result) {
  int left = first.x;
  int top = first.y;
  int right = first.x + first.width;
  int bottom = first.y + first.height;
  if (second) {
    if (left < second->x) left = second->x;
    if (top < second->y) top = second->y;
    if (right > second->x + second->width)
      right = second->x + second->width;
    if (bottom > second->y + second->height)
      bottom = second->y + second->height;
  }
  if (left >= right || top >= bottom) return false;
  *result = (SfRect) {
    (int16_t) left, (int16_t) top,
    (int16_t) (right - left), (int16_t) (bottom - top)};
  return true;
}

void sf_gameplay_hud_draw_pattern_strength(
    SfRenderer *renderer, const SfNjpDecodedResource *hud,
    uint8_t source_pattern, int x, int y, uint16_t strength,
    const SfRect *clip) {
  const SfNjpDecodedPattern *pattern =
    sf_njp_decoded_pattern(hud, source_pattern);
  uint8_t reference;
  if (!pattern || pattern->palette >= hud->palette_count) return;
  for (reference = 0u; reference < pattern->reference_count; ++reference) {
    const SfNjpDecodedReference *item =
      &hud->references[pattern->first_reference + reference];
    SfIndexedImage image;
    if (item->part >= hud->part_count) continue;
    image = hud->parts[item->part].image;
    image.palette = hud->palettes[pattern->palette];
    sf_renderer_draw_indexed_tinted(
      renderer, &image, x + item->x, y + item->y,
      strength, strength, strength, 1000u, SF_BLEND_MASKED, clip);
  }
}

void sf_gameplay_hud_draw_pattern(
    SfRenderer *renderer, const SfNjpDecodedResource *hud,
    uint8_t source_pattern, int x, int y, const SfRect *clip) {
  sf_gameplay_hud_draw_pattern_strength(
    renderer, hud, source_pattern, x, y, 1000u, clip);
}

int sf_gameplay_hud_bar_width(int32_t current, int32_t maximum, int width) {
  int64_t result;
  if (current <= 0 || maximum <= 0) return 0;
  if (current >= maximum) return width;
  result = (int64_t) current * width / maximum;
  return result < 1 ? 1 : (int) result;
}

static void sf_hud_draw_bar(
    SfRenderer *renderer, const SfNjpDecodedResource *hud,
    uint8_t image, SfRect bounds, const SfRect *clip) {
  SfRect draw_clip;
  if (bounds.width <= 0 || !sf_hud_intersection(
        bounds, clip, &draw_clip)) return;
  sf_gameplay_hud_draw_pattern(renderer, hud, image, 0, 0, &draw_clip);
}

static void sf_hud_draw_level(
    SfRenderer *renderer, const SfNjpDecodedResource *hud,
    int32_t level, const SfRect *clip) {
  static const int16_t positions[3][3] = {
    {60, 0, 0}, {65, 56, 0}, {69, 60, 51}
  };
  int digits;
  int divisor = 1;
  int index;
  if (level < 0) level = 0;
  if (level > 999) level = 999;
  digits = level > 99 ? 3 : level > 9 ? 2 : 1;
  for (index = 0; index < digits; ++index) {
    const uint8_t digit = (uint8_t) ((level / divisor) % 10);
    sf_gameplay_hud_draw_pattern(
      renderer, hud, (uint8_t) (SF_HUD_FIRST_DIGIT_PATTERN + digit),
      positions[digits - 1][index], 465, clip);
    divisor *= 10;
  }
}

void sf_gameplay_hud_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfPlayerState *player, const SfRect *clip) {
  SfRect reserved = {0, 412, 640, 68};
  SfRect background;
  int width;
  if (!renderer || !assets || !player || assets->hud.pattern_count != 22u ||
      !sf_hud_intersection(reserved, clip, &background)) return;
  sf_renderer_fill_rect(renderer, background, 0u);
  sf_gameplay_hud_draw_pattern(
    renderer, &assets->hud, SF_HUD_LEFT_FRAME_PATTERN, 0, 0, clip);
  sf_gameplay_hud_draw_pattern(
    renderer, &assets->hud, SF_HUD_RIGHT_FRAME_PATTERN, 0, 0, clip);
  sf_gameplay_hud_draw_pattern(
    renderer, &assets->hud,
    player->pace == SF_PLAYER_PACE_RUN ?
      SF_HUD_RUN_PATTERN : SF_HUD_WALK_PATTERN,
    0, 0, clip);
  sf_hud_draw_level(renderer, &assets->hud, player->level, clip);
  width = sf_gameplay_hud_bar_width(
    player->current_life, player->initial_parameters.values[2], 206);
  sf_hud_draw_bar(renderer, &assets->hud, SF_HUD_LIFE_PATTERN,
    (SfRect) {81, 425, (int16_t) width, 12}, clip);
  width = sf_gameplay_hud_bar_width(
    player->current_mana, player->initial_parameters.values[3], 206);
  sf_hud_draw_bar(renderer, &assets->hud, SF_HUD_MANA_PATTERN,
    (SfRect) {106, 452, (int16_t) width, 12}, clip);
  sf_gameplay_hud_draw_pattern(
    renderer, &assets->hud, SF_HUD_EXPERIENCE_FRAME_PATTERN, 0, 0, clip);
  width = sf_gameplay_hud_bar_width(
    player->experience,
    player->initial_parameters.experience_threshold, 109);
  sf_hud_draw_bar(renderer, &assets->hud, SF_HUD_EXPERIENCE_PATTERN,
    (SfRect) {530, 395, (int16_t) width, 9}, clip);
}
