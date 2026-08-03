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

#ifndef SHADOWFLARE_CORE_ARENA_H
#define SHADOWFLARE_CORE_ARENA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct SfArena {
  uint8_t *memory;
  size_t capacity;
  size_t used;
  size_t peak;
} SfArena;

void sf_arena_init(SfArena *arena, void *memory, size_t capacity);
void *sf_arena_push(SfArena *arena, size_t size, size_t alignment);
void *sf_arena_push_zero(SfArena *arena, size_t size, size_t alignment);
size_t sf_arena_mark(const SfArena *arena);
bool sf_arena_rewind(SfArena *arena, size_t mark);
size_t sf_arena_remaining(const SfArena *arena);

#endif
