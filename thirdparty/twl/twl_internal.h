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

#ifndef TWL_INTERNAL_H
#define TWL_INTERNAL_H

#include "twl.h"

struct Twl {
  TwlConfig config;
  TwlEvent *events;
  TwlControllerState *controllers;
  size_t event_head;
  size_t event_tail;
  size_t event_count;
  void *backend;
  size_t backend_size;
  uint32_t display_width;
  uint32_t display_height;
  bool backend_ready;
};

size_t twl_internal_align_up(size_t value, size_t alignment);
void twl_internal_zero(void *memory, size_t size);
void twl_internal_push_event(Twl *twl, const TwlEvent *event);
void twl_internal_set_display_size(
  Twl *twl, uint32_t width, uint32_t height);
void twl_internal_set_controller_connected(
  Twl *twl, uint32_t controller_index, bool connected);
void twl_internal_set_controller_button(
  Twl *twl, uint32_t controller_index,
  TwlControllerButton button, bool pressed);
void twl_internal_set_controller_axis(
  Twl *twl, uint32_t controller_index,
  TwlControllerAxis axis, int16_t value);

size_t twl_backend_memory_alignment(void);
size_t twl_backend_memory_required(const TwlConfig *config);
TwlResult twl_backend_init(
  Twl *twl, void *memory, size_t memory_size, const TwlConfig *config);
void twl_backend_shutdown(Twl *twl);
void twl_backend_pump_events(Twl *twl);
TwlResult twl_backend_present(Twl *twl, const TwlSurface *surface);
uint64_t twl_backend_time_microseconds(const Twl *twl);
void twl_backend_sleep_microseconds(Twl *twl, uint64_t duration);

#endif
