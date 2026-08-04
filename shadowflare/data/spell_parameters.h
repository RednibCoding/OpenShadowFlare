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

#ifndef SHADOWFLARE_DATA_SPELL_PARAMETERS_H
#define SHADOWFLARE_DATA_SPELL_PARAMETERS_H

#include "game/player_magic.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_SPELL_EFFECTIVE_LEVEL_LIMIT 30u
#define SF_SPELL_LEVEL_LIMIT 20u
#define SF_SPELL_DESCRIPTION_LINE_LIMIT 8u
#define SF_SPELL_DESCRIPTION_TEXT_CAPACITY 80u

typedef struct SfSpellParameters {
  int32_t mana[SF_PLAYER_SPELL_COUNT][SF_SPELL_EFFECTIVE_LEVEL_LIMIT];
  int32_t effect[SF_PLAYER_SPELL_COUNT][SF_SPELL_EFFECTIVE_LEVEL_LIMIT];
  int32_t threshold[SF_PLAYER_SPELL_COUNT][SF_SPELL_LEVEL_LIMIT];
  char descriptions[SF_PLAYER_SPELL_COUNT]
    [SF_SPELL_DESCRIPTION_LINE_LIMIT][SF_SPELL_DESCRIPTION_TEXT_CAPACITY];
  uint8_t description_lines[SF_PLAYER_SPELL_COUNT];
} SfSpellParameters;

bool sf_spell_parameters_load(
  const char *path, SfSpellParameters *parameters);
int32_t sf_spell_mana(
  const SfSpellParameters *parameters, int32_t spell, int32_t level);
int32_t sf_spell_effect(
  const SfSpellParameters *parameters, int32_t spell, int32_t level);
int32_t sf_spell_threshold(
  const SfSpellParameters *parameters, int32_t spell, int32_t level);

#endif
