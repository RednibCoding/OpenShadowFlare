/*
 * Copyright (C) 2026 Michael Binder
 *
 * This file is part of LAL.
 *
 * LAL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * LAL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along with
 * LAL. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LAL_H
#define LAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LalSound LalSound;
typedef uint32_t LalVoice;

#define LAL_INVALID_VOICE ((LalVoice) 0)

enum {
  LAL_SAMPLE_RATE_16000 = 16000,
  LAL_SAMPLE_RATE_22050 = 22050,
  LAL_SAMPLE_RATE_48000 = 48000
};

typedef struct {
  uint32_t maximum_sample_rate;
  bool force_mono;
} LalConfig;

typedef struct {
  uint32_t sample_rate;
  uint16_t channels;
  uint16_t bits_per_sample;
  uint16_t frame_stride_bytes;
} LalPcmFormat;

typedef struct {
  float volume;
  float pan;
  float playback_rate;
  bool loop;
} LalPlayOptions;

LalConfig lal_config_default(void);
bool lal_init(void);
bool lal_init_ex(const LalConfig *config);
void lal_shutdown(void);

LalSound *lal_sound_load_wav(const char *path);
LalSound *lal_sound_load_wav_memory(
  const void *data, size_t byte_count);
LalSound *lal_sound_create_pcm(
  const void *samples, size_t byte_count, const LalPcmFormat *format);
void lal_sound_destroy(LalSound *sound);
size_t lal_sound_frame_count(const LalSound *sound);
double lal_sound_duration(const LalSound *sound);
uint32_t lal_sound_sample_rate(const LalSound *sound);
uint16_t lal_sound_channel_count(const LalSound *sound);
size_t lal_sound_memory_usage_bytes(const LalSound *sound);

LalPlayOptions lal_play_options_default(void);
LalVoice lal_play(const LalSound *sound, float volume, bool loop);
LalVoice lal_play_ex(
  const LalSound *sound, const LalPlayOptions *options);
bool lal_set_voice_volume(LalVoice voice, float volume);
bool lal_set_voice_pan(LalVoice voice, float pan);
bool lal_set_voice_playback_rate(LalVoice voice, float playback_rate);
void lal_stop(LalVoice voice);
bool lal_is_playing(LalVoice voice);
void lal_stop_all(void);

void lal_set_master_volume(float volume);
float lal_master_volume(void);

const char *lal_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
