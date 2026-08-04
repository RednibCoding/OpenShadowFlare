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

#ifndef SHADOWFLARE_UI_GAMEPLAY_COMPANION_HUD_H
#define SHADOWFLARE_UI_GAMEPLAY_COMPANION_HUD_H

#include "assets/gameplay_assets.h"
#include "game/companion.h"
#include "render/renderer.h"

#include <stdint.h>

void sf_gameplay_companion_hud_draw(
  SfRenderer *renderer, const SfGameplayAssets *assets,
  const SfCompanionState *companion, uint32_t ticks,
  const SfRect *clip);

#endif
