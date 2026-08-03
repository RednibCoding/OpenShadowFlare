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

#include <alsa/asoundlib.h>

typedef struct {
  snd_pcm_t *pcm;
  uint32_t pending_frames;
  uint32_t pending_offset;
} TalAlsa;

size_t tal_backend_memory_alignment(void) {
  return _Alignof(TalAlsa);
}

size_t tal_backend_memory_required(const TalConfig *config) {
  (void) config;
  return sizeof(TalAlsa);
}

TalResult tal_backend_init(
    Tal *tal, void *memory, size_t memory_size, const TalConfig *config) {
  TalAlsa *alsa;
  int result;
  (void) tal;
  if (!memory || memory_size < sizeof(TalAlsa) || !config)
    return TAL_RESULT_INVALID_ARGUMENT;
  alsa = (TalAlsa *) memory;
  result = snd_pcm_open(
    &alsa->pcm, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (result < 0) return TAL_RESULT_BACKEND_UNAVAILABLE;
  result = snd_pcm_set_params(
    alsa->pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
    config->channels, config->sample_rate, 1, 50000u);
  if (result < 0) {
    snd_pcm_close(alsa->pcm);
    alsa->pcm = NULL;
    return TAL_RESULT_BACKEND_FAILURE;
  }
  return TAL_RESULT_OK;
}

void tal_backend_shutdown(Tal *tal) {
  TalAlsa *alsa = tal ? (TalAlsa *) tal->backend : NULL;
  if (!alsa || !alsa->pcm) return;
  snd_pcm_drop(alsa->pcm);
  snd_pcm_close(alsa->pcm);
  alsa->pcm = NULL;
}

static TalResult tal_alsa_write_pending(Tal *tal, TalAlsa *alsa) {
  int16_t *samples = tal_internal_mix_buffer(tal) +
    (size_t) alsa->pending_offset * tal->config.channels;
  snd_pcm_sframes_t written = snd_pcm_writei(
    alsa->pcm, samples, (snd_pcm_uframes_t) alsa->pending_frames);
  if (written < 0) {
    if (written == -EAGAIN) return TAL_RESULT_OK;
    if (snd_pcm_recover(alsa->pcm, (int) written, 1) < 0)
      return TAL_RESULT_BACKEND_FAILURE;
    return TAL_RESULT_OK;
  }
  alsa->pending_frames -= (uint32_t) written;
  alsa->pending_offset += (uint32_t) written;
  if (alsa->pending_frames == 0u) alsa->pending_offset = 0u;
  return TAL_RESULT_OK;
}

TalResult tal_backend_update(Tal *tal) {
  TalAlsa *alsa = tal ? (TalAlsa *) tal->backend : NULL;
  unsigned iteration;
  if (!alsa || !alsa->pcm) return TAL_RESULT_BACKEND_FAILURE;
  for (iteration = 0u; iteration < 4u; ++iteration) {
    snd_pcm_sframes_t available;
    TalResult result;
    if (alsa->pending_frames > 0u) {
      result = tal_alsa_write_pending(tal, alsa);
      if (result != TAL_RESULT_OK || alsa->pending_frames > 0u) return result;
    }
    available = snd_pcm_avail_update(alsa->pcm);
    if (available < 0) {
      if (available == -EAGAIN) return TAL_RESULT_OK;
      if (snd_pcm_recover(alsa->pcm, (int) available, 1) < 0)
        return TAL_RESULT_BACKEND_FAILURE;
      continue;
    }
    if (available == 0) return TAL_RESULT_OK;
    alsa->pending_frames = (uint32_t) available;
    if (alsa->pending_frames > tal->config.mix_block_frames)
      alsa->pending_frames = tal->config.mix_block_frames;
    alsa->pending_offset = 0u;
    result = tal_internal_render(
      tal, tal_internal_mix_buffer(tal), alsa->pending_frames);
    if (result != TAL_RESULT_OK) return result;
  }
  return TAL_RESULT_OK;
}

void tal_backend_lock(Tal *tal) {
  (void) tal;
}

void tal_backend_unlock(Tal *tal) {
  (void) tal;
}
