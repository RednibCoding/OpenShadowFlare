/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of TWL.
 *
 * TWL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TWL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with TWL. If not, see <https://www.gnu.org/licenses/>.
 */

#include "twl_internal.h"

size_t twl_backend_memory_alignment(void) {
  return 1u;
}

size_t twl_backend_memory_required(const TwlConfig *config) {
  (void) config;
  return 0u;
}

TwlResult twl_backend_init(
    Twl *twl, void *memory, size_t memory_size, const TwlConfig *config) {
  (void) twl;
  (void) memory;
  (void) memory_size;
  (void) config;
  return TWL_RESULT_BACKEND_UNAVAILABLE;
}

void twl_backend_shutdown(Twl *twl) {
  (void) twl;
}

void twl_backend_pump_events(Twl *twl) {
  (void) twl;
}

TwlResult twl_backend_present(Twl *twl, const TwlSurface *surface) {
  (void) twl;
  (void) surface;
  return TWL_RESULT_BACKEND_UNAVAILABLE;
}

uint64_t twl_backend_time_microseconds(const Twl *twl) {
  (void) twl;
  return 0u;
}
