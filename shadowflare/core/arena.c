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

#include "core/arena.h"

void sf_arena_init(SfArena *arena, void *memory, size_t capacity) {
  if (!arena) return;
  arena->memory = (uint8_t *) memory;
  arena->capacity = memory ? capacity : 0u;
  arena->used = 0u;
  arena->peak = 0u;
}

void *sf_arena_push(SfArena *arena, size_t size, size_t alignment) {
  size_t remainder;
  size_t padding;
  size_t available;
  uintptr_t address;
  void *result;
  if (!arena || !arena->memory || alignment == 0u) return NULL;
  address = (uintptr_t) (arena->memory + arena->used);
  remainder = (size_t) (address % alignment);
  padding = remainder == 0u ? 0u : alignment - remainder;
  available = arena->capacity - arena->used;
  if (padding > available || size > available - padding) return NULL;
  arena->used += padding;
  result = arena->memory + arena->used;
  arena->used += size;
  if (arena->used > arena->peak) arena->peak = arena->used;
  return result;
}

void *sf_arena_push_zero(SfArena *arena, size_t size, size_t alignment) {
  uint8_t *memory = (uint8_t *) sf_arena_push(arena, size, alignment);
  size_t index;
  if (!memory) return NULL;
  for (index = 0u; index < size; ++index) memory[index] = 0u;
  return memory;
}

size_t sf_arena_mark(const SfArena *arena) {
  return arena ? arena->used : 0u;
}

bool sf_arena_rewind(SfArena *arena, size_t mark) {
  if (!arena || mark > arena->used) return false;
  arena->used = mark;
  return true;
}

size_t sf_arena_remaining(const SfArena *arena) {
  return arena ? arena->capacity - arena->used : 0u;
}
