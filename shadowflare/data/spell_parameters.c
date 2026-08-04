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

#include "data/spell_parameters.h"

#include "data/table.h"

#include <string.h>

typedef struct SfSpellParameterScan {
  SfSpellParameters *parameters;
  uint8_t found;
} SfSpellParameterScan;

static bool sf_spell_parameter_number(
    void *user, int32_t table, int32_t row,
    int32_t column, int32_t value) {
  SfSpellParameterScan *scan = (SfSpellParameterScan *) user;
  if (row < 0 || row >= (int32_t) SF_PLAYER_SPELL_COUNT || column < 0)
    return true;
  if (table == 16 && column < (int32_t) SF_SPELL_EFFECTIVE_LEVEL_LIMIT) {
    scan->parameters->mana[row][column] = value;
    if (row == 21 && column == 29) scan->found |= 1u;
  } else if (table == 17 &&
             column < (int32_t) SF_SPELL_EFFECTIVE_LEVEL_LIMIT) {
    scan->parameters->effect[row][column] = value;
    if (row == 21 && column == 29) scan->found |= 2u;
  } else if (table == 27 && column < (int32_t) SF_SPELL_LEVEL_LIMIT) {
    scan->parameters->threshold[row][column] = value;
    if (row == 21 && column == 19) scan->found |= 4u;
  }
  return true;
}

static bool sf_spell_parameter_text(
    void *user, int32_t table, int32_t row, int32_t column,
    uint32_t byte_index, uint32_t byte_count, uint8_t value) {
  SfSpellParameterScan *scan = (SfSpellParameterScan *) user;
  int32_t spell;
  char *text;
  if (table < 600 || table >= 600 + (int32_t) SF_PLAYER_SPELL_COUNT ||
      row < 0 || row >= (int32_t) SF_SPELL_DESCRIPTION_LINE_LIMIT ||
      column != 0) return true;
  spell = table - 600;
  text = scan->parameters->descriptions[spell][row];
  if (byte_index + 1u < SF_SPELL_DESCRIPTION_TEXT_CAPACITY)
    text[byte_index] = (char) value;
  if (byte_index + 1u == byte_count ||
      byte_index + 2u == SF_SPELL_DESCRIPTION_TEXT_CAPACITY) {
    uint32_t end = byte_count;
    if (end >= SF_SPELL_DESCRIPTION_TEXT_CAPACITY)
      end = SF_SPELL_DESCRIPTION_TEXT_CAPACITY - 1u;
    text[end] = '\0';
    if (byte_count == 2u && (uint8_t) text[0] == 0x81u &&
        (uint8_t) text[1] == 0x40u) {
      text[0] = '\0';
      text[1] = '\0';
    }
    if (scan->parameters->description_lines[spell] <= (uint8_t) row)
      scan->parameters->description_lines[spell] = (uint8_t) row + 1u;
  }
  return true;
}

bool sf_spell_parameters_load(
    const char *path, SfSpellParameters *parameters) {
  SfSpellParameterScan scan;
  if (!path || !parameters) return false;
  memset(parameters, 0, sizeof(*parameters));
  scan.parameters = parameters;
  scan.found = 0u;
  return sf_table_scan(
      path, sf_spell_parameter_number, &scan,
      sf_spell_parameter_text, &scan) && scan.found == 7u;
}

static int32_t sf_spell_parameter_value(
    const int32_t *values, int32_t spell, int32_t level,
    uint8_t level_limit) {
  int32_t value;
  if (!values || spell < 0 || spell >= (int32_t) SF_PLAYER_SPELL_COUNT ||
      level <= 0) return 0;
  if (level > level_limit) level = level_limit;
  value = values[spell * level_limit + level - 1];
  return value < 0 ? 0 : value;
}

int32_t sf_spell_mana(
    const SfSpellParameters *parameters, int32_t spell, int32_t level) {
  return parameters ? sf_spell_parameter_value(
    &parameters->mana[0][0], spell, level, SF_SPELL_EFFECTIVE_LEVEL_LIMIT) : 0;
}

int32_t sf_spell_effect(
    const SfSpellParameters *parameters, int32_t spell, int32_t level) {
  return parameters ? sf_spell_parameter_value(
    &parameters->effect[0][0], spell, level,
    SF_SPELL_EFFECTIVE_LEVEL_LIMIT) : 0;
}

int32_t sf_spell_threshold(
    const SfSpellParameters *parameters, int32_t spell, int32_t level) {
  return parameters ? sf_spell_parameter_value(
    &parameters->threshold[0][0], spell, level, SF_SPELL_LEVEL_LIMIT) : 0;
}
