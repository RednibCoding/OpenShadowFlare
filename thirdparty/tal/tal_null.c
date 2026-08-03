/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of TAL.
 *
 * TAL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TAL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with TAL. If not, see <https://www.gnu.org/licenses/>.
 */

#include "tal_internal.h"

size_t tal_backend_memory_alignment(void) {
  return 1u;
}

size_t tal_backend_memory_required(const TalConfig *config) {
  (void) config;
  return 0u;
}

TalResult tal_backend_init(
    Tal *tal, void *memory, size_t memory_size, const TalConfig *config) {
  (void) tal;
  (void) memory;
  (void) memory_size;
  (void) config;
  return TAL_RESULT_BACKEND_UNAVAILABLE;
}

void tal_backend_shutdown(Tal *tal) {
  (void) tal;
}

TalResult tal_backend_update(Tal *tal) {
  (void) tal;
  return TAL_RESULT_BACKEND_UNAVAILABLE;
}

void tal_backend_lock(Tal *tal) {
  (void) tal;
}

void tal_backend_unlock(Tal *tal) {
  (void) tal;
}
