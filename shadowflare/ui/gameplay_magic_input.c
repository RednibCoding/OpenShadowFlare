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

#include "ui/gameplay_magic_input.h"

#include "ui/gameplay_magic_layout.h"

static int8_t sf_magic_panel_spell_at(
    uint8_t page, int32_t x, int32_t y) {
  uint8_t row;
  if (x < 24 || x >= 56) return -1;
  for (row = 0u; row < 6u; ++row) {
    const int top = 56 + row * 48;
    const int spell = page * 6 + row;
    if (y >= top && y < top + 32 &&
        spell < (int) SF_PLAYER_SPELL_COUNT) return (int8_t) spell;
  }
  return -1;
}

static int8_t sf_magic_panel_slot_at(int32_t x, int32_t y) {
  uint8_t slot;
  if (y < 356 || y >= 388) return -1;
  for (slot = 0u; slot < SF_PLAYER_MAGIC_BAR_SLOT_COUNT; ++slot) {
    const int left = 29 + slot * 32;
    if (x >= left && x < left + 32) return (int8_t) slot;
  }
  return -1;
}

static void sf_magic_feedback(SfGameInput *input, uint16_t sample) {
  input->interface_sound = sample;
  input->pointer_over_gameplay_ui = true;
}

bool sf_gameplay_magic_input_resolve(
    SfGameplayCharacterPanelUi *panel, const SfPlayerState *player,
    bool left_panel, bool right_panel, SfGameInput *input) {
  SfGameplayMagicRegion slots[SF_PLAYER_MAGIC_BAR_SLOT_COUNT];
  SfGameplayMagicRegion target;
  bool changed = false;
  uint8_t slot;
  if (!panel || !player || !input) return false;
  if (input->pointer_active &&
      (panel->pointer_x != input->pointer_x ||
       panel->pointer_y != input->pointer_y)) {
    panel->pointer_x = input->pointer_x;
    panel->pointer_y = input->pointer_y;
    if (panel->tab == SF_GAMEPLAY_CHARACTER_TAB_MAGIC) changed = true;
  }
  if (panel->held_spell >= 0 && !input->pointer_primary_down) {
    const int8_t target_slot = sf_magic_panel_slot_at(
      input->pointer_x, input->pointer_y);
    if (target_slot >= 0 && sf_player_magic_learned(
          &player->magic, panel->held_spell)) {
      input->magic_action = SF_MAGIC_ACTION_ASSIGN;
      input->magic_spell = panel->held_spell;
      input->magic_bar_slot = target_slot;
      sf_magic_feedback(input, 58u);
    }
    panel->held_spell = -1;
    changed = true;
  }
  if (panel->tab == SF_GAMEPLAY_CHARACTER_TAB_MAGIC &&
      input->pointer_primary_pressed) {
    const int8_t spell = sf_magic_panel_spell_at(
      panel->magic_page, input->pointer_x, input->pointer_y);
    const int8_t panel_slot = sf_magic_panel_slot_at(
      input->pointer_x, input->pointer_y);
    input->pointer_over_gameplay_ui = input->pointer_active &&
      input->pointer_x < 320 && input->pointer_y < 412;
    if (panel->magic_page > 0u &&
        sf_gameplay_magic_region_contains(
          (SfGameplayMagicRegion) {16, 335, 33, 16},
          input->pointer_x, input->pointer_y)) {
      --panel->magic_page;
      sf_magic_feedback(input, 58u);
      changed = true;
    } else if (panel->magic_page < 3u &&
        sf_gameplay_magic_region_contains(
          (SfGameplayMagicRegion) {270, 335, 34, 16},
          input->pointer_x, input->pointer_y)) {
      ++panel->magic_page;
      sf_magic_feedback(input, 58u);
      changed = true;
    } else if (spell >= 0 &&
        sf_player_magic_learned(&player->magic, spell)) {
      panel->held_spell = spell;
      sf_magic_feedback(input, 57u);
      changed = true;
    } else if (panel_slot >= 0) {
      const int32_t assigned = sf_player_magic_bar_slot(
        &player->magic, panel_slot);
      if (sf_player_magic_learned(&player->magic, assigned)) {
        panel->held_spell = (int8_t) assigned;
        sf_magic_feedback(input, 57u);
        changed = true;
      }
    }
  }
  if (input->pointer_primary_pressed && !(left_panel && right_panel)) {
    sf_gameplay_magic_bar_layout(
      &player->magic, left_panel, right_panel, slots, &target);
    for (slot = 0u; slot < SF_PLAYER_MAGIC_BAR_SLOT_COUNT; ++slot) {
      const int32_t spell = sf_player_magic_bar_slot(&player->magic, slot);
      if (!sf_gameplay_magic_region_contains(
            slots[slot], input->pointer_x, input->pointer_y)) continue;
      input->pointer_over_gameplay_ui = true;
      if (sf_player_magic_learned(&player->magic, spell) &&
          player->magic.selected_spell != spell) {
        input->magic_action = SF_MAGIC_ACTION_SELECT;
        input->magic_spell = (int8_t) spell;
        sf_magic_feedback(input, 58u);
        changed = true;
      }
      return changed;
    }
    if (sf_gameplay_magic_region_contains(
          target, input->pointer_x, input->pointer_y)) {
      input->magic_action = SF_MAGIC_ACTION_TOGGLE_TARGETING;
      sf_magic_feedback(input, 58u);
      changed = true;
    }
  }
  return changed;
}
