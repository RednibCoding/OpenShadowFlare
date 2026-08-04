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

#ifndef SHADOWFLARE_UI_GAMEPLAY_STATUS_H
#define SHADOWFLARE_UI_GAMEPLAY_STATUS_H

#include "assets/gameplay_assets.h"
#include "game/player.h"
#include "render/renderer.h"
#include "ui/gameplay_character_panel.h"

void sf_gameplay_status_draw(
  SfRenderer *renderer, const SfGameplayAssets *assets,
  const SfPlayerState *player, const SfGameplayCharacterPanelUi *panel,
  const SfRect *clip);

#endif
