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

#include "ui/gameplay_status.h"

#include "game/player_elements.h"
#include "game/player_profile.h"
#include "ui/gameplay_status_pattern.h"

#include <stdio.h>

static void sf_status_text(
    SfRenderer *renderer, const SfIndexedImage *font,
    const char *text, int x, int y, uint16_t color) {
  sf_renderer_draw_text(renderer, font, text, x + 1, y + 1, 0u, 1000u);
  sf_renderer_draw_text(renderer, font, text, x, y, color, 1000u);
}

static uint16_t sf_status_value_color(int32_t value, int32_t base) {
  if (value < base) return sf_rgb555(28u, 8u, 8u);
  if (value > base) return sf_rgb555(28u, 24u, 16u);
  return sf_rgb555(28u, 28u, 28u);
}

static void sf_status_number(
    SfRenderer *renderer, const SfIndexedImage *font,
    int32_t value, int right, int y, uint16_t color) {
  char text[16];
  int length;
  if (value < 0) value = 0;
  length = snprintf(text, sizeof(text), "%d", (int) value);
  if (length <= 0 || (size_t) length >= sizeof(text)) return;
  sf_status_text(renderer, font, text, right - length * 8, y, color);
}

static void sf_status_profile_draw(
    SfRenderer *renderer, const SfIndexedImage *font,
    const SfPlayerState *player, const SfPlayerProfile *profile) {
  const int32_t *base = player->initial_parameters.values;
  const uint16_t normal = sf_rgb555(28u, 28u, 28u);
  sf_status_text(
    renderer, font, sf_player_job_name(player->job, player->gender),
    22, 42, normal);
  sf_status_text(renderer, font, player->name, 92, 42, normal);
  sf_status_number(
    renderer, font, player->level, 303, 43,
    player->level == 100 ? sf_rgb555(28u, 24u, 16u) : normal);
  if (player->level == 100)
    sf_status_text(
      renderer, font, "Max", 283, 67, sf_rgb555(28u, 24u, 16u));
  else
    sf_status_number(renderer, font, player->experience, 303, 67, normal);
  sf_status_number(
    renderer, font, player->current_life, 81, 91,
    sf_status_value_color(profile->maximum_life, base[2]));
  sf_status_number(
    renderer, font, profile->maximum_life, 124, 91,
    sf_status_value_color(profile->maximum_life, base[2]));
  sf_status_number(renderer, font, profile->weight_capacity, 95, 115,
    sf_status_value_color(profile->weight_capacity, base[4]));
  sf_status_number(renderer, font, profile->physical_attack, 198, 115,
    sf_status_value_color(profile->physical_attack, base[5]));
  sf_status_number(renderer, font, profile->physical_defense, 303, 115,
    sf_status_value_color(profile->physical_defense, base[6]));
  sf_status_number(renderer, font, profile->hit_rate, 150, 139,
    sf_status_value_color(profile->hit_rate, base[9]));
  sf_status_number(renderer, font, profile->physical_evasion, 303, 139,
    sf_status_value_color(profile->physical_evasion, base[10]));
  sf_status_number(renderer, font, profile->walking_speed, 150, 163,
    sf_status_value_color(profile->walking_speed, base[1]));
  sf_status_number(renderer, font, profile->attack_speed, 303, 163,
    sf_status_value_color(profile->attack_speed, base[0]));
  sf_status_number(
    renderer, font, player->current_mana, 81, 194,
    sf_status_value_color(profile->maximum_mana, base[3]));
  sf_status_number(
    renderer, font, profile->maximum_mana, 124, 194,
    sf_status_value_color(profile->maximum_mana, base[3]));
  sf_status_number(renderer, font, profile->magical_attack, 150, 219,
    sf_status_value_color(profile->magical_attack, base[7]));
  sf_status_number(renderer, font, profile->magical_defense, 303, 219,
    sf_status_value_color(profile->magical_defense, base[8]));
  sf_status_number(renderer, font, profile->magical_hit_rate, 150, 243,
    sf_status_value_color(profile->magical_hit_rate, base[11]));
  sf_status_number(renderer, font, profile->magical_evasion, 303, 243,
    sf_status_value_color(profile->magical_evasion, base[12]));
}

void sf_gameplay_status_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfPlayerState *player, const SfGameplayCharacterPanelUi *panel,
    const SfRect *clip) {
  SfPlayerProfile profile;
  int8_t affinities[SF_PLAYER_ELEMENT_COUNT];
  const SfIndexedImage *font;
  uint8_t element;
  if (!renderer || !assets || !player || !panel ||
      panel->tab != SF_GAMEPLAY_CHARACTER_TAB_STATUS ||
      assets->font.image_count == 0u ||
      (clip && (clip->x >= 320 || clip->x + clip->width <= 0 ||
                clip->y >= 412 || clip->y + clip->height <= 0))) return;
  font = &assets->font.images[0].image;
  sf_renderer_fill_rect(renderer, (SfRect) {0, 0, 320, 412}, 0u);
  sf_gameplay_status_pattern_draw(
    renderer, &assets->inventory_panel, 5u, 0, 0, NULL);
  sf_player_profile_build(
    player, assets->ground_items.definitions,
    assets->ground_items.definition_count, &profile);
  sf_status_profile_draw(renderer, font, player, &profile);
  sf_player_element_affinities(
    player, assets->ground_items.definitions,
    assets->ground_items.definition_count, affinities);
  for (element = 0u; element < SF_PLAYER_ELEMENT_COUNT; ++element)
    sf_gameplay_status_pattern_draw(
      renderer, &assets->inventory_panel,
      (uint8_t) (36 + affinities[element] + 10),
      0, element * 16, NULL);
  sf_gameplay_status_pattern_draw(
    renderer, &assets->inventory_panel, 57u,
    player->element_x * 48 / 20000 + 80,
    330 - player->element_y * 48 / 20000, NULL);
}
