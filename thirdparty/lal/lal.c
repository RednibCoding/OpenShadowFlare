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

#include "lal.h"
#include "lal_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct LalSound {
  int16_t *samples;
  size_t frame_count;
};

typedef struct {
  const LalSound *sound;
  double position;
  float volume;
  float pan;
  float playback_rate;
  uint32_t generation;
  bool loop;
  bool active;
} LalVoiceState;

static LalVoiceState g_voices[LAL_MAX_VOICES];
static float g_master_volume = 1.0f;
static bool g_initialized;
static char g_error[256];

static uint16_t read_u16_le(const uint8_t *p) {
  return (uint16_t) ((uint16_t) p[0] | ((uint16_t) p[1] << 8));
}

static uint32_t read_u32_le(const uint8_t *p) {
  return (uint32_t) p[0] |
         ((uint32_t) p[1] << 8) |
         ((uint32_t) p[2] << 16) |
         ((uint32_t) p[3] << 24);
}

static float clamp_volume(float volume) {
  if (volume != volume) {
    return 0.0f;
  }
  if (volume < 0.0f) {
    return 0.0f;
  }
  if (volume > 1.0f) {
    return 1.0f;
  }
  return volume;
}

static float clamp_pan(float pan) {
  if (pan != pan) {
    return 0.0f;
  }
  if (pan < -1.0f) {
    return -1.0f;
  }
  if (pan > 1.0f) {
    return 1.0f;
  }
  return pan;
}

static float clamp_playback_rate(float playback_rate) {
  if (playback_rate != playback_rate) {
    return 1.0f;
  }
  if (playback_rate < 0.01f) {
    return 0.01f;
  }
  if (playback_rate > 16.0f) {
    return 16.0f;
  }
  return playback_rate;
}

void lal_set_error(const char *message) {
  if (message == NULL) {
    g_error[0] = '\0';
    return;
  }
  snprintf(g_error, sizeof(g_error), "%s", message);
}

const char *lal_last_error(void) {
  return g_error;
}

bool lal_init(void) {
  if (g_initialized) {
    return true;
  }

  memset(g_voices, 0, sizeof(g_voices));
  g_master_volume = 1.0f;
  lal_set_error(NULL);

  if (!lal_platform_init()) {
    if (g_error[0] == '\0') {
      lal_set_error("Could not initialize the platform audio device.");
    }
    return false;
  }

  g_initialized = true;
  return true;
}

void lal_shutdown(void) {
  if (!g_initialized) {
    return;
  }

  lal_platform_shutdown();
  memset(g_voices, 0, sizeof(g_voices));
  g_initialized = false;
}

static bool read_file(
    const char *path, uint8_t **data_out, size_t *size_out) {
  FILE *file;
  long file_size;
  uint8_t *data;
  size_t bytes_read;

  file = fopen(path, "rb");
  if (file == NULL) {
    lal_set_error("Could not open WAV file.");
    return false;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    lal_set_error("Could not seek WAV file.");
    return false;
  }

  file_size = ftell(file);
  if (file_size < 0 ||
      (uintmax_t) file_size > (uintmax_t) SIZE_MAX) {
    fclose(file);
    lal_set_error("WAV file is too large.");
    return false;
  }

  if (fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    lal_set_error("Could not rewind WAV file.");
    return false;
  }

  data = (uint8_t *) malloc((size_t) file_size);
  if (data == NULL && file_size != 0) {
    fclose(file);
    lal_set_error("Out of memory while reading WAV file.");
    return false;
  }

  bytes_read = fread(data, 1, (size_t) file_size, file);
  fclose(file);
  if (bytes_read != (size_t) file_size) {
    free(data);
    lal_set_error("Could not read the complete WAV file.");
    return false;
  }

  *data_out = data;
  *size_out = bytes_read;
  return true;
}

static int16_t pcm_sample(
    const uint8_t *data, size_t frame, int channel,
    int channels, int bits_per_sample, int block_align) {
  const uint8_t *sample;
  int source_channel;

  source_channel = channels == 1 ? 0 : channel;
  sample = data + frame * (size_t) block_align +
           (size_t) source_channel * (size_t) (bits_per_sample / 8);

  if (bits_per_sample == 8) {
    return (int16_t) (((int) sample[0] - 128) * 256);
  }

  return (int16_t) read_u16_le(sample);
}

static LalSound *create_converted_pcm(
    const uint8_t *sample_data, size_t sample_size,
    uint32_t sample_rate, uint16_t channels,
    uint16_t bits_per_sample, uint16_t frame_stride) {
  size_t input_frames;
  size_t output_frames;
  size_t frame;
  LalSound *sound;
  uint16_t packed_stride;

  packed_stride = (uint16_t) (channels * (bits_per_sample / 8));
  if (sample_data == NULL || sample_size == 0) {
    lal_set_error("PCM sample data is empty.");
    return NULL;
  }
  if (channels != 1 && channels != 2) {
    lal_set_error("Only mono and stereo PCM data is supported.");
    return NULL;
  }
  if (bits_per_sample != 8 && bits_per_sample != 16) {
    lal_set_error("Only 8-bit and 16-bit PCM data is supported.");
    return NULL;
  }
  if (sample_rate == 0) {
    lal_set_error("PCM sample rate must be greater than zero.");
    return NULL;
  }
  if (frame_stride == 0) {
    frame_stride = packed_stride;
  }
  if (frame_stride < packed_stride) {
    lal_set_error("PCM frame stride is smaller than one sample frame.");
    return NULL;
  }

  input_frames = sample_size / frame_stride;
  if (input_frames == 0) {
    lal_set_error("PCM data contains no complete sample frames.");
    return NULL;
  }
  {
    uint64_t computed_output_frames =
      ((uint64_t) input_frames * LAL_OUTPUT_SAMPLE_RATE + sample_rate / 2) /
      sample_rate;
    if (computed_output_frames == 0 ||
        computed_output_frames >
          (uint64_t) (SIZE_MAX / (2 * sizeof(int16_t)))) {
      lal_set_error("Converted PCM sample count is invalid.");
      return NULL;
    }
    output_frames = (size_t) computed_output_frames;
  }

  sound = (LalSound *) calloc(1, sizeof(*sound));
  if (sound == NULL) {
    lal_set_error("Out of memory while creating sound.");
    return NULL;
  }
  sound->samples = (int16_t *) malloc(
    output_frames * 2 * sizeof(*sound->samples));
  if (sound->samples == NULL) {
    free(sound);
    lal_set_error("Out of memory while converting PCM samples.");
    return NULL;
  }
  sound->frame_count = output_frames;

  for (frame = 0; frame < output_frames; ++frame) {
    uint64_t source_position;
    size_t source_frame;
    size_t next_frame;
    uint32_t fraction;
    int channel;

    source_position = (uint64_t) frame * sample_rate;
    source_frame = (size_t) (source_position / LAL_OUTPUT_SAMPLE_RATE);
    fraction = (uint32_t) (source_position % LAL_OUTPUT_SAMPLE_RATE);
    if (source_frame >= input_frames) {
      source_frame = input_frames - 1;
    }
    next_frame = source_frame + 1 < input_frames
      ? source_frame + 1
      : source_frame;

    for (channel = 0; channel < 2; ++channel) {
      int32_t first;
      int32_t second;
      int32_t converted;

      first = pcm_sample(
        sample_data, source_frame, channel, channels,
        bits_per_sample, frame_stride);
      second = pcm_sample(
        sample_data, next_frame, channel, channels,
        bits_per_sample, frame_stride);
      converted = (int32_t) (
        ((int64_t) first * (LAL_OUTPUT_SAMPLE_RATE - fraction) +
         (int64_t) second * fraction) /
        LAL_OUTPUT_SAMPLE_RATE);
      sound->samples[frame * 2 + (size_t) channel] = (int16_t) converted;
    }
  }

  return sound;
}

static LalSound *decode_wav(const uint8_t *file_data, size_t file_size) {
  const uint8_t *format_data;
  const uint8_t *sample_data;
  size_t format_size;
  size_t sample_size;
  size_t offset;
  uint16_t audio_format;
  uint16_t channels;
  uint32_t sample_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;

  format_data = NULL;
  sample_data = NULL;
  format_size = 0;
  sample_size = 0;

  if (file_size < 12 ||
      memcmp(file_data, "RIFF", 4) != 0 ||
      memcmp(file_data + 8, "WAVE", 4) != 0) {
    lal_set_error("File is not a RIFF/WAVE file.");
    return NULL;
  }

  offset = 12;
  while (offset + 8 <= file_size) {
    const uint8_t *chunk;
    uint32_t chunk_size_u32;
    size_t chunk_size;
    size_t data_offset;
    size_t next_offset;

    chunk = file_data + offset;
    chunk_size_u32 = read_u32_le(chunk + 4);
    chunk_size = (size_t) chunk_size_u32;
    data_offset = offset + 8;

    if (chunk_size > file_size - data_offset) {
      lal_set_error("WAV file contains a truncated chunk.");
      return NULL;
    }

    if (memcmp(chunk, "fmt ", 4) == 0) {
      format_data = file_data + data_offset;
      format_size = chunk_size;
    } else if (memcmp(chunk, "data", 4) == 0) {
      sample_data = file_data + data_offset;
      sample_size = chunk_size;
    }

    next_offset = data_offset + chunk_size;
    if ((chunk_size & 1u) != 0u) {
      if (next_offset == file_size) {
        break;
      }
      ++next_offset;
    }
    offset = next_offset;
  }

  if (format_data == NULL || format_size < 16 || sample_data == NULL) {
    lal_set_error("WAV file is missing its format or sample-data chunk.");
    return NULL;
  }

  audio_format = read_u16_le(format_data);
  channels = read_u16_le(format_data + 2);
  sample_rate = read_u32_le(format_data + 4);
  block_align = read_u16_le(format_data + 12);
  bits_per_sample = read_u16_le(format_data + 14);

  if (audio_format != 1) {
    lal_set_error("Only uncompressed PCM WAV files are supported.");
    return NULL;
  }
  return create_converted_pcm(
    sample_data, sample_size, sample_rate, channels,
    bits_per_sample, block_align);
}

LalSound *lal_sound_load_wav(const char *path) {
  uint8_t *file_data;
  size_t file_size;
  LalSound *sound;

  if (path == NULL || path[0] == '\0') {
    lal_set_error("No WAV path was provided.");
    return NULL;
  }

  lal_set_error(NULL);
  if (!read_file(path, &file_data, &file_size)) {
    return NULL;
  }

  sound = decode_wav(file_data, file_size);
  free(file_data);
  return sound;
}

LalSound *lal_sound_load_wav_memory(
    const void *data, size_t byte_count) {
  if (data == NULL || byte_count == 0) {
    lal_set_error("WAV memory image is empty.");
    return NULL;
  }
  lal_set_error(NULL);
  return decode_wav((const uint8_t *) data, byte_count);
}

LalSound *lal_sound_create_pcm(
    const void *samples, size_t byte_count,
    const LalPcmFormat *format) {
  if (format == NULL) {
    lal_set_error("No PCM format was provided.");
    return NULL;
  }
  lal_set_error(NULL);
  return create_converted_pcm(
    (const uint8_t *) samples, byte_count,
    format->sample_rate, format->channels,
    format->bits_per_sample, format->frame_stride_bytes);
}

void lal_sound_destroy(LalSound *sound) {
  int index;

  if (sound == NULL) {
    return;
  }

  if (g_initialized) {
    lal_platform_lock();
    for (index = 0; index < LAL_MAX_VOICES; ++index) {
      if (g_voices[index].active && g_voices[index].sound == sound) {
        g_voices[index].active = false;
        g_voices[index].sound = NULL;
      }
    }
    lal_platform_unlock();
  }

  free(sound->samples);
  free(sound);
}

size_t lal_sound_frame_count(const LalSound *sound) {
  return sound == NULL ? 0 : sound->frame_count;
}

double lal_sound_duration(const LalSound *sound) {
  if (sound == NULL) {
    return 0.0;
  }
  return (double) sound->frame_count / LAL_OUTPUT_SAMPLE_RATE;
}

static LalVoice make_voice_handle(int index, uint32_t generation) {
  return (LalVoice) ((generation << 8) | (uint32_t) (index + 1));
}

static bool decode_voice_handle(
    LalVoice voice, int *index_out, uint32_t *generation_out) {
  uint32_t slot;

  slot = voice & 0xffu;
  if (slot == 0 || slot > LAL_MAX_VOICES) {
    return false;
  }

  *index_out = (int) slot - 1;
  *generation_out = voice >> 8;
  return *generation_out != 0;
}

LalPlayOptions lal_play_options_default(void) {
  LalPlayOptions options;

  options.volume = 1.0f;
  options.pan = 0.0f;
  options.playback_rate = 1.0f;
  options.loop = false;
  return options;
}

LalVoice lal_play(const LalSound *sound, float volume, bool loop) {
  LalPlayOptions options;

  options = lal_play_options_default();
  options.volume = volume;
  options.loop = loop;
  return lal_play_ex(sound, &options);
}

LalVoice lal_play_ex(
    const LalSound *sound, const LalPlayOptions *options) {
  int index;
  LalVoice result;
  LalPlayOptions effective;

  if (!g_initialized || sound == NULL || sound->frame_count == 0) {
    return LAL_INVALID_VOICE;
  }

  effective = options == NULL ? lal_play_options_default() : *options;
  result = LAL_INVALID_VOICE;
  lal_platform_lock();
  for (index = 0; index < LAL_MAX_VOICES; ++index) {
    LalVoiceState *voice;

    voice = &g_voices[index];
    if (voice->active) {
      continue;
    }

    voice->generation = (voice->generation + 1u) & 0x00ffffffu;
    if (voice->generation == 0) {
      voice->generation = 1;
    }
    voice->sound = sound;
    voice->position = 0.0;
    voice->volume = clamp_volume(effective.volume);
    voice->pan = clamp_pan(effective.pan);
    voice->playback_rate =
      clamp_playback_rate(effective.playback_rate);
    voice->loop = effective.loop;
    voice->active = true;
    result = make_voice_handle(index, voice->generation);
    break;
  }
  lal_platform_unlock();
  return result;
}

static LalVoiceState *active_voice(
    LalVoice voice_handle, int *index_out) {
  int index;
  uint32_t generation;

  if (!g_initialized ||
      !decode_voice_handle(voice_handle, &index, &generation) ||
      !g_voices[index].active ||
      g_voices[index].generation != generation) {
    return NULL;
  }
  if (index_out != NULL) {
    *index_out = index;
  }
  return &g_voices[index];
}

bool lal_set_voice_volume(LalVoice voice_handle, float volume) {
  LalVoiceState *voice;

  if (!g_initialized) {
    return false;
  }
  lal_platform_lock();
  voice = active_voice(voice_handle, NULL);
  if (voice != NULL) {
    voice->volume = clamp_volume(volume);
  }
  lal_platform_unlock();
  return voice != NULL;
}

bool lal_set_voice_pan(LalVoice voice_handle, float pan) {
  LalVoiceState *voice;

  if (!g_initialized) {
    return false;
  }
  lal_platform_lock();
  voice = active_voice(voice_handle, NULL);
  if (voice != NULL) {
    voice->pan = clamp_pan(pan);
  }
  lal_platform_unlock();
  return voice != NULL;
}

bool lal_set_voice_playback_rate(
    LalVoice voice_handle, float playback_rate) {
  LalVoiceState *voice;

  if (!g_initialized) {
    return false;
  }
  lal_platform_lock();
  voice = active_voice(voice_handle, NULL);
  if (voice != NULL) {
    voice->playback_rate = clamp_playback_rate(playback_rate);
  }
  lal_platform_unlock();
  return voice != NULL;
}

void lal_stop(LalVoice voice_handle) {
  int index;
  uint32_t generation;

  if (!g_initialized ||
      !decode_voice_handle(voice_handle, &index, &generation)) {
    return;
  }

  lal_platform_lock();
  if (g_voices[index].active &&
      g_voices[index].generation == generation) {
    g_voices[index].active = false;
    g_voices[index].sound = NULL;
  }
  lal_platform_unlock();
}

bool lal_is_playing(LalVoice voice_handle) {
  int index;
  uint32_t generation;
  bool playing;

  if (!g_initialized ||
      !decode_voice_handle(voice_handle, &index, &generation)) {
    return false;
  }

  lal_platform_lock();
  playing = g_voices[index].active &&
            g_voices[index].generation == generation;
  lal_platform_unlock();
  return playing;
}

void lal_stop_all(void) {
  int index;

  if (!g_initialized) {
    return;
  }

  lal_platform_lock();
  for (index = 0; index < LAL_MAX_VOICES; ++index) {
    g_voices[index].active = false;
    g_voices[index].sound = NULL;
  }
  lal_platform_unlock();
}

void lal_set_master_volume(float volume) {
  if (!g_initialized) {
    g_master_volume = clamp_volume(volume);
    return;
  }

  lal_platform_lock();
  g_master_volume = clamp_volume(volume);
  lal_platform_unlock();
}

float lal_master_volume(void) {
  float volume;

  if (!g_initialized) {
    return g_master_volume;
  }

  lal_platform_lock();
  volume = g_master_volume;
  lal_platform_unlock();
  return volume;
}

static int16_t clamp_sample(int64_t sample) {
  if (sample < -32768) {
    return -32768;
  }
  if (sample > 32767) {
    return 32767;
  }
  return (int16_t) sample;
}

void lal_mix_frames(int16_t *output, size_t frame_count) {
  size_t frame;

  for (frame = 0; frame < frame_count; ++frame) {
    int64_t left;
    int64_t right;
    int index;

    left = 0;
    right = 0;
    for (index = 0; index < LAL_MAX_VOICES; ++index) {
      LalVoiceState *voice;
      const int16_t *first;
      const int16_t *second;
      size_t first_frame;
      size_t second_frame;
      double fraction;
      float left_sample;
      float right_sample;
      float left_balance;
      float right_balance;
      float gain;

      voice = &g_voices[index];
      if (!voice->active || voice->sound == NULL) {
        continue;
      }

      if (voice->position >= (double) voice->sound->frame_count) {
        if (voice->loop) {
          voice->position = 0.0;
        } else {
          voice->active = false;
          voice->sound = NULL;
          continue;
        }
      }

      first_frame = (size_t) voice->position;
      second_frame = first_frame + 1;
      if (second_frame >= voice->sound->frame_count) {
        second_frame = voice->loop ? 0 : first_frame;
      }
      fraction = voice->position - first_frame;
      first = voice->sound->samples + first_frame * 2;
      second = voice->sound->samples + second_frame * 2;
      left_sample = (float) (
        first[0] + (second[0] - first[0]) * fraction);
      right_sample = (float) (
        first[1] + (second[1] - first[1]) * fraction);
      left_balance = voice->pan > 0.0f ? 1.0f - voice->pan : 1.0f;
      right_balance = voice->pan < 0.0f ? 1.0f + voice->pan : 1.0f;
      gain = voice->volume * g_master_volume;
      left += (int64_t) (left_sample * gain * left_balance);
      right += (int64_t) (right_sample * gain * right_balance);
      voice->position += voice->playback_rate;
      if (voice->position >= (double) voice->sound->frame_count) {
        if (voice->loop) {
          size_t wraps;

          wraps = (size_t) (
            voice->position / (double) voice->sound->frame_count);
          voice->position -=
            wraps * (double) voice->sound->frame_count;
        } else {
          voice->active = false;
          voice->sound = NULL;
        }
      }
    }

    output[frame * 2] = clamp_sample(left);
    output[frame * 2 + 1] = clamp_sample(right);
  }
}
