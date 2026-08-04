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

#include "ui/gameplay_magic.h"

#include "ui/gameplay_magic_layout.h"
#include "ui/gameplay_status_pattern.h"

#include <stdio.h>
#include <string.h>

static const char *const sf_spell_names[SF_PLAYER_SPELL_COUNT] = {
  "Transport", "Fire Ball", "Ice Bolt", "Plasma", "Hell Fire",
  "Ice Blast", "Heal", "Moon", "Berserker", "Energy Shield",
  "Earth Spear", "Flame Strike", "Dread Deathscythe",
  "Lightning Storm", "Medusa", "Sonic Blade", "Mud Javelin",
  "Identify", "Magic Shield", "Counter Burst", "Explosion",
  "Elemental Strike"
};

static const char *const sf_spell_effect_labels[SF_PLAYER_SPELL_COUNT] = {
  "", "Attack.", "Attack.", "Attack.", "Attack.", "Attack.", "Heal.",
  "", "", "Def.", "Attack.", "Attack.", "Attack.", "Attack.",
  "Attack.", "Attack.", "Attack.", "", "Shield.", "RefPer.",
  "Attack.", "Attack."
};

static void sf_magic_text(
    SfRenderer *renderer, const SfIndexedImage *font,
    const char *text, int x, int y, uint16_t color) {
  sf_renderer_draw_text(renderer, font, text, x + 1, y + 1, 0u, 1000u);
  sf_renderer_draw_text(renderer, font, text, x, y, color, 1000u);
}

static void sf_magic_number(
    SfRenderer *renderer, const SfIndexedImage *font,
    int32_t value, int x, int y, uint16_t color) {
  char text[16];
  const int length = snprintf(text, sizeof(text), "%d", (int) value);
  if (length > 0 && (size_t) length < sizeof(text))
    sf_magic_text(renderer, font, text, x, y, color);
}

static int32_t sf_magic_level(
    const SfPlayerMagicState *magic, int32_t spell) {
  int32_t level = magic->levels[spell];
  if (level < 1) level = 1;
  if (level > 20) level = 20;
  return level;
}

static void sf_magic_experience(
    SfRenderer *renderer, const SfIndexedImage *font,
    const SfGameplayAssets *assets, const SfPlayerMagicState *magic,
    int32_t spell, int32_t level, int y, uint16_t color) {
  char text[32];
  int length;
  if (level >= 20) {
    sf_magic_text(renderer, font, "Max", 264, y, color);
    return;
  }
  length = snprintf(
    text, sizeof(text), "%d/%d", (int) magic->experience[spell],
    (int) sf_spell_threshold(assets->spell_parameters, spell, level));
  if (length > 0 && (size_t) length < sizeof(text))
    sf_magic_text(renderer, font, text, 264, y, color);
}

static int32_t sf_magic_hovered_spell(
    const SfGameplayCharacterPanelUi *panel) {
  uint8_t row;
  if (panel->pointer_x <= 59 || panel->pointer_x >= 228) return -1;
  for (row = 0u; row < 6u; ++row) {
    const int top = 66 + row * 48;
    const int spell = panel->magic_page * 6 + row;
    if (panel->pointer_y >= top && panel->pointer_y < top + 12 &&
        spell < (int) SF_PLAYER_SPELL_COUNT) return spell;
  }
  return -1;
}

static void sf_magic_description_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfPlayerState *player, const SfGameplayCharacterPanelUi *panel) {
  const int32_t spell = sf_magic_hovered_spell(panel);
  const SfIndexedImage *font;
  uint8_t lines;
  uint8_t line;
  size_t longest = 0u;
  int width;
  int height;
  int x;
  int y;
  if (!sf_player_magic_learned(&player->magic, spell) ||
      assets->font.image_count == 0u) return;
  if (!assets->spell_parameters) return;
  lines = assets->spell_parameters->description_lines[spell];
  if (lines == 0u) return;
  for (line = 0u; line < lines; ++line) {
    const size_t length = strlen(
      assets->spell_parameters->descriptions[spell][line]);
    if (length > longest) longest = length;
  }
  font = &assets->font.images[0].image;
  width = (int) longest * 6 + 8;
  height = lines * 12 + 8;
  x = panel->pointer_x - width / 2;
  y = panel->pointer_y + 8;
  if (x < 1) x = 1;
  if (x + width > 639) x = 639 - width;
  if (y < 1) y = 1;
  if (y + height > 479) y = 479 - height;
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) x, (int16_t) y, (int16_t) width, (int16_t) height}, 0u);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x - 1), (int16_t) (y - 1),
    (int16_t) (width + 2), 1}, sf_rgb555(31u, 31u, 31u));
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x - 1), (int16_t) (y + height),
    (int16_t) (width + 2), 1}, sf_rgb555(31u, 31u, 31u));
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x - 1), (int16_t) (y - 1), 1,
    (int16_t) (height + 2)}, sf_rgb555(31u, 31u, 31u));
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x + width), (int16_t) (y - 1), 1,
    (int16_t) (height + 2)}, sf_rgb555(31u, 31u, 31u));
  for (line = 0u; line < lines; ++line)
    sf_renderer_draw_text(
      renderer, font, assets->spell_parameters->descriptions[spell][line],
      x + 4, y + 4 + line * 12, sf_rgb555(28u, 28u, 28u), 1000u);
}

void sf_gameplay_magic_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfPlayerState *player, const SfGameplayCharacterPanelUi *panel,
    const SfRect *clip) {
  const SfIndexedImage *font;
  const uint16_t name = sf_rgb555(28u, 24u, 16u);
  const uint16_t label = sf_rgb555(20u, 20u, 8u);
  const uint16_t value = sf_rgb555(28u, 28u, 28u);
  uint8_t row;
  uint8_t slot;
  if (!renderer || !assets || !player || !panel ||
      panel->tab != SF_GAMEPLAY_CHARACTER_TAB_MAGIC ||
      assets->font.image_count == 0u ||
      (clip && (clip->x >= 320 || clip->x + clip->width <= 0 ||
                clip->y >= 412 || clip->y + clip->height <= 0))) return;
  font = &assets->font.images[0].image;
  sf_renderer_fill_rect(renderer, (SfRect) {0, 0, 320, 412}, 0u);
  sf_gameplay_status_pattern_draw(
    renderer, &assets->inventory_panel, 6u, 0, 0, NULL);
  for (row = 0u; row < 6u; ++row) {
    const int32_t spell = panel->magic_page * 6 + row;
    const int icon_y = 59 + row * 48;
    int32_t level;
    int first_y;
    int second_y;
    if (spell >= (int32_t) SF_PLAYER_SPELL_COUNT) break;
    sf_gameplay_status_pattern_draw(
      renderer, &assets->inventory_panel, 32u, 24, icon_y - 3, NULL);
    if (player->magic.availability[spell] == 3)
      sf_gameplay_status_pattern_draw(
        renderer, &assets->magic_icons, (uint8_t) (spell + 2),
        27, icon_y, NULL);
    else if (player->magic.availability[spell] == 1)
      sf_gameplay_status_pattern_draw_opacity(
        renderer, &assets->magic_icons, (uint8_t) (spell + 2),
        27, icon_y, 300u, NULL);
    if ((player->magic.availability[spell] & 1) == 0) continue;
    level = sf_magic_level(&player->magic, spell);
    first_y = icon_y + 7;
    second_y = first_y + 12;
    sf_magic_text(renderer, font, sf_spell_names[spell], 59, first_y, name);
    sf_magic_text(renderer, font, "Lv.", 192, first_y, label);
    sf_magic_number(renderer, font, level, 216, first_y, value);
    sf_magic_text(renderer, font, "Exp.", 234, first_y, label);
    sf_magic_experience(
      renderer, font, assets, &player->magic, spell, level, first_y, value);
    sf_magic_text(renderer, font, "MP.", 192, second_y, label);
    sf_magic_number(renderer, font,
      sf_spell_mana(assets->spell_parameters, spell, level),
      216, second_y, value);
    if (sf_spell_effect_labels[spell][0] != '\0') {
      sf_magic_text(renderer, font, sf_spell_effect_labels[spell],
        240, second_y, label);
      sf_magic_number(renderer, font,
        sf_spell_effect(assets->spell_parameters, spell, level),
        294, second_y, value);
    }
  }
  if (panel->magic_page > 0u)
    sf_gameplay_status_pattern_draw(
      renderer, &assets->inventory_panel, 69u, 0, 0, NULL);
  if (panel->magic_page < 3u)
    sf_gameplay_status_pattern_draw(
      renderer, &assets->inventory_panel, 70u, 0, 0, NULL);
  for (slot = 0u; slot < SF_PLAYER_MAGIC_BAR_SLOT_COUNT; ++slot) {
    const int32_t spell = player->magic.bar_slots[slot];
    if (sf_player_magic_learned(&player->magic, spell))
      sf_gameplay_status_pattern_draw(
        renderer, &assets->magic_icons, (uint8_t) (spell + 2),
        32 + slot * 32, 359, NULL);
  }
  sf_magic_description_draw(renderer, assets, player, panel);
}

void sf_gameplay_magic_bar_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfPlayerState *player, bool left_panel, bool right_panel,
    const SfRect *clip) {
  SfGameplayMagicRegion slots[SF_PLAYER_MAGIC_BAR_SLOT_COUNT];
  SfGameplayMagicRegion target;
  uint8_t slot;
  if (!renderer || !assets || !player || (left_panel && right_panel)) return;
  sf_gameplay_magic_bar_layout(
    &player->magic, left_panel, right_panel, slots, &target);
  for (slot = 0u; slot < SF_PLAYER_MAGIC_BAR_SLOT_COUNT; ++slot) {
    const int32_t spell = player->magic.bar_slots[slot];
    if (!sf_player_magic_learned(&player->magic, spell))
      sf_gameplay_status_pattern_draw(
        renderer, &assets->magic_bar_icons, 3u,
        slots[slot].x, 392, clip);
    else if (spell == player->magic.selected_spell)
      sf_gameplay_status_pattern_draw(
        renderer, &assets->magic_icons, (uint8_t) (spell + 2),
        slots[slot].x, 382, clip);
    else
      sf_gameplay_status_pattern_draw(
        renderer, &assets->magic_bar_icons, (uint8_t) (spell + 4),
        slots[slot].x, 392, clip);
  }
  sf_gameplay_status_pattern_draw(
    renderer, player->magic.targeting
      ? &assets->magic_icons : &assets->magic_bar_icons,
    player->magic.targeting ? 0u : 2u,
    target.x, player->magic.targeting ? 382 : 392, clip);
}

void sf_gameplay_magic_held_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfGameplayCharacterPanelUi *panel) {
  if (!renderer || !assets || !panel || panel->held_spell < 0) return;
  sf_gameplay_status_pattern_draw(
    renderer, &assets->magic_icons, (uint8_t) (panel->held_spell + 2),
    panel->pointer_x - 13, panel->pointer_y - 13, NULL);
}
