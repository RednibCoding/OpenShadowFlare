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

#include "game/gameplay_service.h"

void sf_gameplay_service_clear(SfGameplayServiceRequest *request) {
  if (!request) return;
  request->kind = SF_GAMEPLAY_SERVICE_NONE;
  request->argument = 0;
}

bool sf_gameplay_service_request(
    SfGameplayServiceRequest *request,
    SfGameplayServiceKind kind, int32_t argument) {
  if (!request || kind == SF_GAMEPLAY_SERVICE_NONE ||
      request->kind != SF_GAMEPLAY_SERVICE_NONE) return false;
  request->kind = kind;
  request->argument = argument;
  return true;
}

SfGameplayServiceRequest sf_gameplay_service_take(
    SfGameplayServiceRequest *request) {
  SfGameplayServiceRequest result = {SF_GAMEPLAY_SERVICE_NONE, 0};
  if (!request) return result;
  result = *request;
  sf_gameplay_service_clear(request);
  return result;
}
