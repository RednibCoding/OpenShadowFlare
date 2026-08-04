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

#ifndef SHADOWFLARE_GAME_GAMEPLAY_SERVICE_H
#define SHADOWFLARE_GAME_GAMEPLAY_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum SfGameplayServiceKind {
  SF_GAMEPLAY_SERVICE_NONE = 0,
  SF_GAMEPLAY_SERVICE_TOGGLE_SPECIAL_ITEMS
} SfGameplayServiceKind;

typedef struct SfGameplayServiceRequest {
  SfGameplayServiceKind kind;
  int32_t argument;
} SfGameplayServiceRequest;

void sf_gameplay_service_clear(SfGameplayServiceRequest *request);
bool sf_gameplay_service_request(
  SfGameplayServiceRequest *request,
  SfGameplayServiceKind kind, int32_t argument);
SfGameplayServiceRequest sf_gameplay_service_take(
  SfGameplayServiceRequest *request);

#endif
