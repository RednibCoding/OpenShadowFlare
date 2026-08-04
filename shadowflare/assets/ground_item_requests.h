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

#ifndef SHADOWFLARE_ASSETS_GROUND_ITEM_REQUESTS_H
#define SHADOWFLARE_ASSETS_GROUND_ITEM_REQUESTS_H

#include "assets/ground_item_assets.h"

typedef struct SfGroundItemResourceRequest {
  int32_t resource_id;
  uint16_t charts[SF_GROUND_ITEM_DEFINITION_LIMIT];
  uint8_t chart_count;
} SfGroundItemResourceRequest;

bool sf_ground_item_collect_definitions(
  const SfScsScript *script, SfItemGroundDefinition *definitions,
  uint8_t *definition_count);
bool sf_ground_item_collect_resources(
  const SfItemGroundDefinition *definitions, uint8_t definition_count,
  SfGroundItemResourceRequest *requests, uint8_t *request_count);

#endif
