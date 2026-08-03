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

#ifndef TAL_H
#define TAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Tal Tal;
typedef uint32_t TalVoice;

#define TAL_INVALID_VOICE ((TalVoice) 0)
#define TAL_GAIN_SILENT 0u
#define TAL_GAIN_FULL 32768u
#define TAL_PAN_LEFT (-32767)
#define TAL_PAN_CENTER 0
#define TAL_PAN_RIGHT 32767
#define TAL_RATE_NORMAL 65536u

typedef enum {
  TAL_RESULT_OK = 0,
  TAL_RESULT_INVALID_ARGUMENT,
  TAL_RESULT_MISALIGNED_MEMORY,
  TAL_RESULT_INSUFFICIENT_MEMORY,
  TAL_RESULT_NO_FREE_VOICE,
  TAL_RESULT_BACKEND_UNAVAILABLE,
  TAL_RESULT_BACKEND_FAILURE
} TalResult;

typedef enum {
  TAL_OUTPUT_MANUAL = 0,
  TAL_OUTPUT_DEVICE
} TalOutputMode;

typedef enum {
  TAL_SAMPLE_S16 = 0,
  TAL_SAMPLE_U8
} TalSampleFormat;

typedef struct {
  const void *samples;
  uint32_t frame_count;
  uint32_t sample_rate;
  uint8_t channels;
  TalSampleFormat format;
} TalPcm;

typedef struct {
  uint16_t volume_q15;
  int16_t pan_q15;
  uint32_t playback_rate_q16;
  bool loop;
} TalPlayOptions;

typedef struct {
  TalOutputMode output_mode;
  uint32_t sample_rate;
  uint32_t mix_block_frames;
  uint16_t max_voices;
  uint8_t channels;
} TalConfig;

TalConfig tal_config_default(void);
TalPlayOptions tal_play_options_default(void);
size_t tal_memory_alignment(void);
size_t tal_memory_required(const TalConfig *config);

TalResult tal_init(
  void *memory, size_t memory_size, const TalConfig *config, Tal **out_tal);
void tal_shutdown(Tal *tal);

TalResult tal_play(
  Tal *tal, const TalPcm *pcm, const TalPlayOptions *options,
  TalVoice *out_voice);
bool tal_voice_playing(const Tal *tal, TalVoice voice);
TalResult tal_voice_stop(Tal *tal, TalVoice voice);
TalResult tal_voice_set_volume(
  Tal *tal, TalVoice voice, uint16_t volume_q15);
TalResult tal_voice_set_pan(Tal *tal, TalVoice voice, int16_t pan_q15);
void tal_stop_all(Tal *tal);

void tal_set_master_volume(Tal *tal, uint16_t volume_q15);
uint16_t tal_master_volume(const Tal *tal);

TalResult tal_render(Tal *tal, int16_t *output, uint32_t frame_count);
TalResult tal_update(Tal *tal);

#ifdef __cplusplus
}
#endif

#endif
