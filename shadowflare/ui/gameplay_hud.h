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

#ifndef SHADOWFLARE_UI_GAMEPLAY_HUD_H
#define SHADOWFLARE_UI_GAMEPLAY_HUD_H

#include "assets/gameplay_assets.h"
#include "game/player.h"
#include "render/renderer.h"

void sf_gameplay_hud_draw(
  SfRenderer *renderer, const SfGameplayAssets *assets,
  const SfPlayerState *player, const SfRect *clip);
void sf_gameplay_hud_draw_pattern(
  SfRenderer *renderer, const SfNjpDecodedResource *hud,
  uint8_t source_pattern, int x, int y, const SfRect *clip);
void sf_gameplay_hud_draw_pattern_strength(
  SfRenderer *renderer, const SfNjpDecodedResource *hud,
  uint8_t source_pattern, int x, int y, uint16_t strength,
  const SfRect *clip);
int sf_gameplay_hud_bar_width(int32_t current, int32_t maximum, int width);

#endif
