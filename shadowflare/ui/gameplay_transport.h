/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of OpenShadowFlare.
 *
 * OpenShadowFlare is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenShadowFlare is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SHADOWFLARE_UI_GAMEPLAY_TRANSPORT_H
#define SHADOWFLARE_UI_GAMEPLAY_TRANSPORT_H

#include "assets/gameplay_assets.h"
#include "game/input.h"
#include "interpreter/scenario_actor_script.h"
#include "render/renderer.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_GAMEPLAY_TRANSPORT_ROWS_PER_PAGE 10u

typedef struct SfGameplayTransportUi {
  int32_t service_argument;
  int16_t hovered_destination;
  uint8_t page;
  bool active;
} SfGameplayTransportUi;

void sf_gameplay_transport_init(SfGameplayTransportUi *transport);
void sf_gameplay_transport_open(
  SfGameplayTransportUi *transport, int32_t service_argument);
void sf_gameplay_transport_close(SfGameplayTransportUi *transport);
bool sf_gameplay_transport_input_resolve(
  SfGameplayTransportUi *transport,
  const SfTransportCatalog *catalog,
  const SfScenarioProgressState *progress,
  SfGameInput *input);
void sf_gameplay_transport_draw(
  SfRenderer *renderer, const SfGameplayAssets *assets,
  const SfScenarioProgressState *progress,
  const SfGameplayTransportUi *transport, const SfRect *clip);

#endif
