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

#include "ui/gameplay_companion_hud.h"

#include "ui/gameplay_hud.h"

void sf_gameplay_companion_hud_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfCompanionState *companion, uint32_t ticks,
    const SfRect *clip) {
  int width;
  uint16_t strength = 1000u;
  SfRect bar_clip;
  if (!renderer || !assets || !companion || !companion->valid) return;
  sf_gameplay_hud_draw_pattern(
    renderer, &assets->hud, 30u, 0, 0, clip);
  width = sf_gameplay_hud_bar_width(
    companion->current_life, companion->profile.values[3], 109);
  if (width > 0) {
    if (width * 100 / 109 < 30 && ticks % 4u < 2u) strength = 1500u;
    bar_clip = (SfRect) {
      (int16_t) (110 - width), 396, (int16_t) width, 11};
    sf_gameplay_hud_draw_pattern_strength(
      renderer, &assets->hud, 29u, 0, 0, strength, &bar_clip);
  }
  sf_gameplay_hud_draw_pattern(
    renderer, &assets->hud, companion->inactive ? 32u : 31u,
    0, 0, clip);
}
