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

#ifndef TAL_INTERNAL_H
#define TAL_INTERNAL_H

#include "tal.h"

typedef struct {
  TalPcm pcm;
  uint32_t frame;
  uint32_t fraction_q16;
  uint32_t step_q16;
  uint16_t generation;
  uint16_t volume_q15;
  uint16_t left_gain_q15;
  uint16_t right_gain_q15;
  int16_t pan_q15;
  bool loop;
  bool active;
} TalVoiceSlot;

struct Tal {
  TalConfig config;
  TalVoiceSlot *voices;
  int16_t *mix_buffer;
  void *backend;
  size_t backend_size;
  uint16_t master_volume_q15;
  bool backend_ready;
};

size_t tal_internal_align_up(size_t value, size_t alignment);
void tal_internal_zero(void *memory, size_t size);
TalResult tal_internal_render(
  Tal *tal, int16_t *output, uint32_t frame_count);
int16_t *tal_internal_mix_buffer(Tal *tal);

size_t tal_backend_memory_alignment(void);
size_t tal_backend_memory_required(const TalConfig *config);
TalResult tal_backend_init(
  Tal *tal, void *memory, size_t memory_size, const TalConfig *config);
void tal_backend_shutdown(Tal *tal);
TalResult tal_backend_update(Tal *tal);

#endif
