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

#include <stddef.h>
#include <stdint.h>

size_t tal_internal_align_up(size_t value, size_t alignment) {
  const size_t remainder = value % alignment;
  return remainder == 0u ? value : value + alignment - remainder;
}

void tal_internal_zero(void *memory, size_t size) {
  uint8_t *bytes = (uint8_t *) memory;
  size_t index;
  for (index = 0u; index < size; ++index) bytes[index] = 0u;
}

static bool tal_config_valid(const TalConfig *config) {
  return config &&
    (config->output_mode == TAL_OUTPUT_MANUAL ||
     config->output_mode == TAL_OUTPUT_DEVICE) &&
    config->sample_rate > 0u && config->mix_block_frames > 0u &&
    config->max_voices > 0u && config->max_voices <= 256u &&
    (config->channels == 1u || config->channels == 2u);
}

TalConfig tal_config_default(void) {
  TalConfig config;
  config.output_mode = TAL_OUTPUT_DEVICE;
  config.sample_rate = 44100u;
  config.mix_block_frames = 512u;
  config.max_voices = 16u;
  config.channels = 2u;
  return config;
}

TalPlayOptions tal_play_options_default(void) {
  TalPlayOptions options;
  options.volume_q15 = TAL_GAIN_FULL;
  options.pan_q15 = TAL_PAN_CENTER;
  options.playback_rate_q16 = TAL_RATE_NORMAL;
  options.loop = false;
  return options;
}

size_t tal_memory_alignment(void) {
  const size_t common_alignment = _Alignof(max_align_t);
  const size_t backend_alignment = tal_backend_memory_alignment();
  return backend_alignment > common_alignment
           ? backend_alignment
           : common_alignment;
}

size_t tal_memory_required(const TalConfig *config) {
  size_t size;
  size_t sample_count;
  if (!tal_config_valid(config)) return 0u;
  size = tal_internal_align_up(sizeof(Tal), _Alignof(TalVoiceSlot));
  if (config->max_voices >
      (SIZE_MAX - size) / sizeof(TalVoiceSlot)) return 0u;
  size += (size_t) config->max_voices * sizeof(TalVoiceSlot);
  size = tal_internal_align_up(size, _Alignof(int16_t));
  if (config->mix_block_frames >
      SIZE_MAX / (size_t) config->channels) return 0u;
  sample_count =
    (size_t) config->mix_block_frames * (size_t) config->channels;
  if (sample_count > (SIZE_MAX - size) / sizeof(int16_t)) return 0u;
  size += sample_count * sizeof(int16_t);
  if (config->output_mode == TAL_OUTPUT_DEVICE) {
    size = tal_internal_align_up(size, tal_backend_memory_alignment());
    if (tal_backend_memory_required(config) > SIZE_MAX - size) return 0u;
    size += tal_backend_memory_required(config);
  }
  return size;
}

TalResult tal_init(
    void *memory, size_t memory_size, const TalConfig *config, Tal **out_tal) {
  uint8_t *bytes;
  size_t required;
  size_t offset;
  Tal *tal;
  TalResult result;
  if (out_tal) *out_tal = NULL;
  required = tal_memory_required(config);
  if (!memory || !out_tal || required == 0u)
    return TAL_RESULT_INVALID_ARGUMENT;
  if ((uintptr_t) memory % tal_memory_alignment() != 0u)
    return TAL_RESULT_MISALIGNED_MEMORY;
  if (memory_size < required) return TAL_RESULT_INSUFFICIENT_MEMORY;

  tal_internal_zero(memory, required);
  bytes = (uint8_t *) memory;
  tal = (Tal *) memory;
  tal->config = *config;
  tal->master_volume_q15 = TAL_GAIN_FULL;
  offset = tal_internal_align_up(sizeof(Tal), _Alignof(TalVoiceSlot));
  tal->voices = (TalVoiceSlot *) (bytes + offset);
  offset += (size_t) config->max_voices * sizeof(TalVoiceSlot);
  offset = tal_internal_align_up(offset, _Alignof(int16_t));
  tal->mix_buffer = (int16_t *) (bytes + offset);
  offset += (size_t) config->mix_block_frames *
            (size_t) config->channels * sizeof(int16_t);
  if (config->output_mode == TAL_OUTPUT_DEVICE) {
    offset = tal_internal_align_up(offset, tal_backend_memory_alignment());
    tal->backend = bytes + offset;
    tal->backend_size = required - offset;
    result = tal_backend_init(
      tal, tal->backend, tal->backend_size, config);
    if (result != TAL_RESULT_OK) return result;
    tal->backend_ready = true;
  }
  *out_tal = tal;
  return TAL_RESULT_OK;
}

void tal_shutdown(Tal *tal) {
  if (!tal) return;
  if (tal->backend_ready) {
    tal_backend_shutdown(tal);
    tal->backend_ready = false;
  }
  tal_stop_all(tal);
}

static TalVoice tal_voice_handle(uint16_t index, uint16_t generation) {
  return ((uint32_t) generation << 16u) | ((uint32_t) index + 1u);
}

static TalVoiceSlot *tal_voice_slot(Tal *tal, TalVoice voice) {
  const uint32_t encoded_index = voice & UINT32_C(0xffff);
  const uint16_t generation = (uint16_t) (voice >> 16u);
  TalVoiceSlot *slot;
  if (!tal || encoded_index == 0u ||
      encoded_index > tal->config.max_voices) return NULL;
  slot = &tal->voices[encoded_index - 1u];
  return slot->active && slot->generation == generation ? slot : NULL;
}

static void tal_update_voice_gains(const Tal *tal, TalVoiceSlot *slot) {
  uint32_t left = slot->volume_q15;
  uint32_t right = slot->volume_q15;
  if (slot->pan_q15 > 0)
    left = left * (uint32_t) (32767 - slot->pan_q15) / 32767u;
  else if (slot->pan_q15 < 0)
    right = right * (uint32_t) (32767 + slot->pan_q15) / 32767u;
  left = left * tal->master_volume_q15 / TAL_GAIN_FULL;
  right = right * tal->master_volume_q15 / TAL_GAIN_FULL;
  slot->left_gain_q15 = (uint16_t) left;
  slot->right_gain_q15 = (uint16_t) right;
}

TalResult tal_play(
    Tal *tal, const TalPcm *pcm, const TalPlayOptions *options,
    TalVoice *out_voice) {
  TalPlayOptions defaults;
  uint16_t index;
  if (out_voice) *out_voice = TAL_INVALID_VOICE;
  if (!tal || !pcm || !pcm->samples || pcm->frame_count == 0u ||
      pcm->sample_rate == 0u || (pcm->channels != 1u && pcm->channels != 2u) ||
      !out_voice) return TAL_RESULT_INVALID_ARGUMENT;
  if (!options) {
    defaults = tal_play_options_default();
    options = &defaults;
  }
  if (options->volume_q15 > TAL_GAIN_FULL ||
      options->pan_q15 == INT16_MIN ||
      options->playback_rate_q16 == 0u) return TAL_RESULT_INVALID_ARGUMENT;
  for (index = 0u; index < tal->config.max_voices; ++index) {
    TalVoiceSlot *slot = &tal->voices[index];
    if (!slot->active) {
      uint16_t generation = (uint16_t) (slot->generation + 1u);
      if (generation == 0u) generation = 1u;
      tal_internal_zero(slot, sizeof(*slot));
      slot->pcm = *pcm;
      slot->generation = generation;
      slot->volume_q15 = options->volume_q15;
      slot->pan_q15 = options->pan_q15;
      tal_update_voice_gains(tal, slot);
      slot->loop = options->loop;
      slot->step_q16 = (uint32_t)
        (((uint64_t) pcm->sample_rate * options->playback_rate_q16) /
         tal->config.sample_rate);
      if (slot->step_q16 == 0u) slot->step_q16 = 1u;
      slot->active = true;
      *out_voice = tal_voice_handle(index, generation);
      return TAL_RESULT_OK;
    }
  }
  return TAL_RESULT_NO_FREE_VOICE;
}

bool tal_voice_playing(const Tal *tal, TalVoice voice) {
  return tal_voice_slot((Tal *) tal, voice) != NULL;
}

TalResult tal_voice_stop(Tal *tal, TalVoice voice) {
  TalVoiceSlot *slot = tal_voice_slot(tal, voice);
  if (!slot) return TAL_RESULT_INVALID_ARGUMENT;
  slot->active = false;
  return TAL_RESULT_OK;
}

TalResult tal_voice_set_volume(
    Tal *tal, TalVoice voice, uint16_t volume_q15) {
  TalVoiceSlot *slot;
  if (volume_q15 > TAL_GAIN_FULL) return TAL_RESULT_INVALID_ARGUMENT;
  slot = tal_voice_slot(tal, voice);
  if (!slot) return TAL_RESULT_INVALID_ARGUMENT;
  slot->volume_q15 = volume_q15;
  tal_update_voice_gains(tal, slot);
  return TAL_RESULT_OK;
}

TalResult tal_voice_set_pan(Tal *tal, TalVoice voice, int16_t pan_q15) {
  if (pan_q15 == INT16_MIN) return TAL_RESULT_INVALID_ARGUMENT;
  TalVoiceSlot *slot = tal_voice_slot(tal, voice);
  if (!slot) return TAL_RESULT_INVALID_ARGUMENT;
  slot->pan_q15 = pan_q15;
  tal_update_voice_gains(tal, slot);
  return TAL_RESULT_OK;
}

void tal_stop_all(Tal *tal) {
  uint16_t index;
  if (!tal) return;
  for (index = 0u; index < tal->config.max_voices; ++index)
    tal->voices[index].active = false;
}

void tal_set_master_volume(Tal *tal, uint16_t volume_q15) {
  uint16_t index;
  if (!tal) return;
  tal->master_volume_q15 =
    volume_q15 > TAL_GAIN_FULL ? TAL_GAIN_FULL : volume_q15;
  for (index = 0u; index < tal->config.max_voices; ++index) {
    if (tal->voices[index].active)
      tal_update_voice_gains(tal, &tal->voices[index]);
  }
}

uint16_t tal_master_volume(const Tal *tal) {
  return tal ? tal->master_volume_q15 : 0u;
}

static int16_t tal_interpolate(
    int16_t first, int16_t second, uint32_t fraction_q16) {
  const int32_t first_weight = (int32_t) (65536u - fraction_q16);
  const int32_t second_weight = (int32_t) fraction_q16;
  return (int16_t)
    (((int32_t) first * first_weight +
      (int32_t) second * second_weight) / 65536);
}

static void tal_voice_samples(
    const TalVoiceSlot *slot, int16_t *left, int16_t *right) {
  uint32_t next_frame = slot->frame + 1u;
  const int16_t *samples = slot->pcm.samples;
  if (next_frame >= slot->pcm.frame_count)
    next_frame = slot->loop ? 0u : slot->frame;
  if (slot->pcm.channels == 1u) {
    *left = tal_interpolate(
      samples[slot->frame], samples[next_frame], slot->fraction_q16);
    *right = *left;
  } else {
    *left = tal_interpolate(
      samples[(size_t) slot->frame * 2u],
      samples[(size_t) next_frame * 2u], slot->fraction_q16);
    *right = tal_interpolate(
      samples[(size_t) slot->frame * 2u + 1u],
      samples[(size_t) next_frame * 2u + 1u], slot->fraction_q16);
  }
}

static void tal_advance_voice(TalVoiceSlot *slot) {
  const uint32_t old_fraction = slot->fraction_q16;
  const uint32_t fraction_step = slot->step_q16 & UINT32_C(0xffff);
  slot->fraction_q16 = (old_fraction + fraction_step) & UINT32_C(0xffff);
  slot->frame += slot->step_q16 >> 16u;
  if (slot->fraction_q16 < old_fraction) ++slot->frame;
  if (slot->frame >= slot->pcm.frame_count) {
    if (slot->loop)
      slot->frame %= slot->pcm.frame_count;
    else
      slot->active = false;
  }
}

static int16_t tal_clip(int32_t value) {
  if (value > 32767) return 32767;
  if (value < -32768) return -32768;
  return (int16_t) value;
}

TalResult tal_internal_render(
    Tal *tal, int16_t *output, uint32_t frame_count) {
  uint32_t frame;
  if (!tal || !output || frame_count > tal->config.mix_block_frames)
    return TAL_RESULT_INVALID_ARGUMENT;
  for (frame = 0u; frame < frame_count; ++frame) {
    int32_t mixed_left = 0;
    int32_t mixed_right = 0;
    uint16_t index;
    for (index = 0u; index < tal->config.max_voices; ++index) {
      TalVoiceSlot *slot = &tal->voices[index];
      int16_t left;
      int16_t right;
      if (!slot->active) continue;
      tal_voice_samples(slot, &left, &right);
      mixed_left +=
        (int32_t) left * (int32_t) slot->left_gain_q15 /
        (int32_t) TAL_GAIN_FULL;
      mixed_right +=
        (int32_t) right * (int32_t) slot->right_gain_q15 /
        (int32_t) TAL_GAIN_FULL;
      tal_advance_voice(slot);
    }
    if (tal->config.channels == 1u) {
      output[frame] = tal_clip((mixed_left + mixed_right) / 2);
    } else {
      output[(size_t) frame * 2u] = tal_clip(mixed_left);
      output[(size_t) frame * 2u + 1u] = tal_clip(mixed_right);
    }
  }
  return TAL_RESULT_OK;
}

TalResult tal_render(Tal *tal, int16_t *output, uint32_t frame_count) {
  return tal_internal_render(tal, output, frame_count);
}

int16_t *tal_internal_mix_buffer(Tal *tal) {
  return tal ? tal->mix_buffer : NULL;
}

TalResult tal_update(Tal *tal) {
  if (!tal) return TAL_RESULT_INVALID_ARGUMENT;
  return tal->backend_ready ? tal_backend_update(tal) : TAL_RESULT_OK;
}
