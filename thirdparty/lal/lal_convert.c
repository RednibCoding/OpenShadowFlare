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

#include "lal_internal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef LAL_USE_LINEAR_RESAMPLER
#define LAL_USE_LINEAR_RESAMPLER 0
#endif

#if !LAL_USE_LINEAR_RESAMPLER
enum {
  LAL_RESAMPLE_PHASES = 256,
  LAL_RESAMPLE_TAPS = 32
};

static const double LAL_PI = 3.14159265358979323846;
static const double LAL_MONO_GAIN = 0.70710678118654752440;
static const double LAL_MONO_LIMIT_START = 0.80;
static const double LAL_MONO_MAX_INPUT = 1.41421356237309504880;
#endif

static uint16_t read_u16_le(const uint8_t *p) {
  return (uint16_t) ((uint16_t) p[0] | ((uint16_t) p[1] << 8));
}

static int16_t pcm_sample(
    const uint8_t *data,
    size_t frame,
    int channel,
    int channels,
    int bits_per_sample,
    int frame_stride) {
  const uint8_t *sample;
  int source_channel;

  source_channel = channels == 1 ? 0 : channel;
  sample = data + frame * (size_t) frame_stride +
           (size_t) source_channel * (size_t) (bits_per_sample / 8);
  if (bits_per_sample == 8) {
    return (int16_t) (((int) sample[0] - 128) * 256);
  }
  return (int16_t) read_u16_le(sample);
}

#if !LAL_USE_LINEAR_RESAMPLER
static double smooth_limit(double sample) {
  double magnitude;
  double sign;
  double position;
  double limited;

  sign = sample < 0.0 ? -1.0 : 1.0;
  magnitude = fabs(sample) / 32768.0;
  if (magnitude <= LAL_MONO_LIMIT_START) {
    return sample;
  }
  if (magnitude >= LAL_MONO_MAX_INPUT) {
    return sign * 32767.0;
  }

  position = (magnitude - LAL_MONO_LIMIT_START) /
    (LAL_MONO_MAX_INPUT - LAL_MONO_LIMIT_START);
  limited =
    (2.0 * position * position * position -
      3.0 * position * position + 1.0) * LAL_MONO_LIMIT_START +
    (position * position * position -
      2.0 * position * position + position) *
      (LAL_MONO_MAX_INPUT - LAL_MONO_LIMIT_START) +
    (-2.0 * position * position * position +
      3.0 * position * position);
  return sign * limited * 32768.0;
}

static double source_sample(
    const uint8_t *data,
    size_t frame,
    int channel,
    int channels,
    int bits_per_sample,
    int frame_stride,
    bool force_mono) {
  if (force_mono && channels == 2) {
    double left;
    double right;

    left = pcm_sample(
      data, frame, 0, channels, bits_per_sample, frame_stride);
    right = pcm_sample(
      data, frame, 1, channels, bits_per_sample, frame_stride);
    return smooth_limit((left + right) * LAL_MONO_GAIN);
  }
  return pcm_sample(
    data, frame, channel, channels, bits_per_sample, frame_stride);
}

static double sinc(double value) {
  if (fabs(value) < 1.0e-12) {
    return 1.0;
  }
  value *= LAL_PI;
  return sin(value) / value;
}

static void build_resample_kernel(
    double kernel[LAL_RESAMPLE_PHASES][LAL_RESAMPLE_TAPS],
    uint32_t input_rate,
    uint32_t output_rate) {
  double cutoff;
  int phase;

  cutoff = (double) output_rate / (double) input_rate;
  if (cutoff > 1.0) {
    cutoff = 1.0;
  }
  cutoff *= 0.98;

  for (phase = 0; phase < LAL_RESAMPLE_PHASES; ++phase) {
    double fraction;
    double sum;
    int tap;

    fraction = (double) phase / (double) LAL_RESAMPLE_PHASES;
    sum = 0.0;
    for (tap = 0; tap < LAL_RESAMPLE_TAPS; ++tap) {
      double distance;
      double window;
      double weight;

      distance = (double) (tap - (LAL_RESAMPLE_TAPS / 2 - 1)) - fraction;
      window = 0.5 + 0.5 * cos(
        LAL_PI * distance / (double) (LAL_RESAMPLE_TAPS / 2));
      weight = cutoff * sinc(cutoff * distance) * window;
      kernel[phase][tap] = weight;
      sum += weight;
    }
    if (fabs(sum) > 1.0e-12) {
      for (tap = 0; tap < LAL_RESAMPLE_TAPS; ++tap) {
        kernel[phase][tap] /= sum;
      }
    }
  }
}

static int16_t clamp_sample(double sample) {
  if (sample < -32768.0) {
    return -32768;
  }
  if (sample > 32767.0) {
    return 32767;
  }
  return (int16_t) (sample < 0.0 ? sample - 0.5 : sample + 0.5);
}
#endif

#if LAL_USE_LINEAR_RESAMPLER
static int32_t linear_source_sample(
    const uint8_t *data,
    size_t frame,
    int channel,
    int channels,
    int bits_per_sample,
    int frame_stride,
    bool force_mono) {
  if (force_mono && channels == 2) {
    return (
      (int32_t) pcm_sample(
        data, frame, 0, channels, bits_per_sample, frame_stride) +
      (int32_t) pcm_sample(
        data, frame, 1, channels, bits_per_sample, frame_stride)) / 2;
  }
  return pcm_sample(
    data, frame, channel, channels, bits_per_sample, frame_stride);
}
#endif

static bool validate_input(
    const uint8_t *sample_data,
    size_t sample_size,
    uint32_t sample_rate,
    uint16_t channels,
    uint16_t bits_per_sample,
    uint16_t *frame_stride) {
  uint16_t packed_stride;

  packed_stride = (uint16_t) (channels * (bits_per_sample / 8));
  if (sample_data == NULL || sample_size == 0) {
    lal_set_error("PCM sample data is empty.");
    return false;
  }
  if (channels != 1 && channels != 2) {
    lal_set_error("Only mono and stereo PCM data is supported.");
    return false;
  }
  if (bits_per_sample != 8 && bits_per_sample != 16) {
    lal_set_error("Only 8-bit and 16-bit PCM data is supported.");
    return false;
  }
  if (sample_rate == 0) {
    lal_set_error("PCM sample rate must be greater than zero.");
    return false;
  }
  if (*frame_stride == 0) {
    *frame_stride = packed_stride;
  }
  if (*frame_stride < packed_stride) {
    lal_set_error("PCM frame stride is smaller than one sample frame.");
    return false;
  }
  return true;
}

bool lal_convert_pcm(
    const uint8_t *sample_data,
    size_t sample_size,
    uint32_t sample_rate,
    uint16_t channels,
    uint16_t bits_per_sample,
    uint16_t frame_stride,
    uint32_t maximum_sample_rate,
    bool force_mono,
    LalConvertedPcm *output) {
#if !LAL_USE_LINEAR_RESAMPLER
  double (*kernel)[LAL_RESAMPLE_TAPS];
#endif
  size_t input_frames;
  size_t output_frames;
  uint32_t output_sample_rate;
  uint16_t output_channels;
  size_t frame;

  if (output == NULL) {
    lal_set_error("No PCM conversion output was provided.");
    return false;
  }
  memset(output, 0, sizeof(*output));
  if (!validate_input(
        sample_data,
        sample_size,
        sample_rate,
        channels,
        bits_per_sample,
        &frame_stride)) {
    return false;
  }
  if (maximum_sample_rate == 0) {
    lal_set_error("The maximum PCM sample rate must be greater than zero.");
    return false;
  }

  input_frames = sample_size / frame_stride;
  if (input_frames == 0) {
    lal_set_error("PCM data contains no complete sample frames.");
    return false;
  }
  output_sample_rate = sample_rate < maximum_sample_rate
    ? sample_rate
    : maximum_sample_rate;
  output_channels = force_mono ? 1 : channels;
  {
    uint64_t computed_output_frames;

    computed_output_frames =
      ((uint64_t) input_frames * output_sample_rate + sample_rate / 2) /
      sample_rate;
    if (computed_output_frames == 0 ||
        computed_output_frames >
          (uint64_t) (SIZE_MAX /
            ((size_t) output_channels * sizeof(int16_t)))) {
      lal_set_error("Converted PCM sample count is invalid.");
      return false;
    }
    output_frames = (size_t) computed_output_frames;
  }

  output->samples = (int16_t *) malloc(
    output_frames * (size_t) output_channels * sizeof(*output->samples));
  if (output->samples == NULL) {
    lal_set_error("Out of memory while converting PCM samples.");
    return false;
  }
  output->frame_count = output_frames;
  output->sample_rate = output_sample_rate;
  output->channels = output_channels;

#if LAL_USE_LINEAR_RESAMPLER
  for (frame = 0; frame < output_frames; ++frame) {
    uint64_t source_position;
    size_t source_frame;
    size_t next_frame;
    uint32_t fraction;
    int channel;

    source_position = (uint64_t) frame * sample_rate;
    source_frame = (size_t) (source_position / output_sample_rate);
    fraction = (uint32_t) (source_position % output_sample_rate);
    if (source_frame >= input_frames) {
      source_frame = input_frames - 1;
    }
    next_frame = source_frame + 1 < input_frames
      ? source_frame + 1
      : source_frame;

    for (channel = 0; channel < output_channels; ++channel) {
      const int32_t first = linear_source_sample(
        sample_data,
        source_frame,
        channel,
        channels,
        bits_per_sample,
        frame_stride,
        force_mono);
      const int32_t second = linear_source_sample(
        sample_data,
        next_frame,
        channel,
        channels,
        bits_per_sample,
        frame_stride,
        force_mono);
      const int32_t converted = (int32_t) (
        ((int64_t) first * (output_sample_rate - fraction) +
         (int64_t) second * fraction) /
        output_sample_rate);
      output->samples[
        frame * (size_t) output_channels + (size_t) channel] =
          (int16_t) converted;
    }
  }
  return true;
#else
  kernel = NULL;
  if (output_sample_rate < sample_rate) {
    kernel = (double (*)[LAL_RESAMPLE_TAPS]) malloc(
      sizeof(double) * LAL_RESAMPLE_PHASES * LAL_RESAMPLE_TAPS);
    if (kernel == NULL) {
      free(output->samples);
      memset(output, 0, sizeof(*output));
      lal_set_error("Out of memory while preparing PCM resampling.");
      return false;
    }
    build_resample_kernel(kernel, sample_rate, output_sample_rate);
  }

  for (frame = 0; frame < output_frames; ++frame) {
    uint64_t source_position;
    size_t source_frame;
    uint32_t fraction;
    int channel;

    source_position = (uint64_t) frame * sample_rate;
    source_frame = (size_t) (source_position / output_sample_rate);
    fraction = (uint32_t) (source_position % output_sample_rate);
    if (source_frame >= input_frames) {
      source_frame = input_frames - 1;
    }

    for (channel = 0; channel < output_channels; ++channel) {
      double converted;

      if (kernel == NULL) {
        converted = source_sample(
          sample_data,
          source_frame,
          channel,
          channels,
          bits_per_sample,
          frame_stride,
          force_mono);
      } else {
        size_t phase;
        double weight_sum;
        int tap;

        phase = ((size_t) fraction * LAL_RESAMPLE_PHASES) /
          output_sample_rate;
        if (phase >= LAL_RESAMPLE_PHASES) {
          phase = LAL_RESAMPLE_PHASES - 1;
        }
        converted = 0.0;
        weight_sum = 0.0;
        for (tap = 0; tap < LAL_RESAMPLE_TAPS; ++tap) {
          ptrdiff_t input_frame;
          double weight;

          input_frame = (ptrdiff_t) source_frame + tap -
            (LAL_RESAMPLE_TAPS / 2 - 1);
          if (input_frame < 0 || (size_t) input_frame >= input_frames) {
            continue;
          }
          weight = kernel[phase][tap];
          converted += source_sample(
            sample_data,
            (size_t) input_frame,
            channel,
            channels,
            bits_per_sample,
            frame_stride,
            force_mono) * weight;
          weight_sum += weight;
        }
        if (fabs(weight_sum) > 1.0e-12) {
          converted /= weight_sum;
        }
      }
      output->samples[
        frame * (size_t) output_channels + (size_t) channel] =
          clamp_sample(converted);
    }
  }

  free(kernel);
  return true;
#endif
}
