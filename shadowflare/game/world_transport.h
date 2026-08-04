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

#ifndef SHADOWFLARE_GAME_WORLD_TRANSPORT_H
#define SHADOWFLARE_GAME_WORLD_TRANSPORT_H

#include "game/world.h"

#include <stdbool.h>
#include <stdint.h>

bool sf_world_transport_activate(
  SfWorldState *world, int32_t destination_index);
bool sf_world_travel_apply_local(SfWorldState *world);

#endif
