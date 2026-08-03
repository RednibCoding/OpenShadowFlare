#include "lal_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int convert(
    const int16_t *samples,
    size_t frame_count,
    uint32_t sample_rate,
    uint16_t channels,
    uint32_t output_rate,
    int force_mono,
    LalConvertedPcm *output) {
  return lal_convert_pcm(
    (const uint8_t *) samples,
    frame_count * channels * sizeof(*samples),
    sample_rate,
    channels,
    16,
    0,
    output_rate,
    force_mono != 0,
    output);
}

static int check_mono_passthrough(void) {
  const int16_t input[] = {-12000, -1, 0, 1, 12000};
  LalConvertedPcm output;
  size_t index;
  int result;

  if (!convert(input, 5, 16000, 1, 16000, 1, &output)) {
    return 0;
  }
  result = output.frame_count == 5 &&
           output.sample_rate == 16000 &&
           output.channels == 1;
  for (index = 0; result && index < 5; ++index) {
    result = output.samples[index] == input[index];
  }
  free(output.samples);
  return result;
}

static int check_constant_power_mono(void) {
  int16_t input[64 * 2];
  LalConvertedPcm output;
  size_t frame;
  int result;

  for (frame = 0; frame < 64; ++frame) {
    input[frame * 2] = 12000;
    input[frame * 2 + 1] = 0;
  }
  if (!convert(input, 64, 16000, 2, 16000, 1, &output)) {
    return 0;
  }
  result = output.channels == 1;
  for (frame = 0; result && frame < output.frame_count; ++frame) {
    result = output.samples[frame] >= 8484 &&
             output.samples[frame] <= 8486;
  }
  free(output.samples);
  return result;
}

static int check_mono_peak_control(void) {
  int16_t input[64 * 2];
  LalConvertedPcm output;
  size_t frame;
  int result;

  for (frame = 0; frame < 64; ++frame) {
    input[frame * 2] = 30000;
    input[frame * 2 + 1] = 30000;
  }
  if (!convert(input, 64, 16000, 2, 16000, 1, &output)) {
    return 0;
  }
  result = output.channels == 1;
  for (frame = 0; result && frame < output.frame_count; ++frame) {
    result = output.samples[frame] > 30000 &&
             output.samples[frame] < 32767;
  }
  free(output.samples);
  return result;
}

static int check_antialiased_downsampling(void) {
  int16_t input[2205];
  LalConvertedPcm output;
  size_t frame;
  int64_t magnitude_sum;
  size_t measured_frames;
  int result;

  for (frame = 0; frame < 2205; ++frame) {
    input[frame] = (frame & 1u) == 0u ? 20000 : -20000;
  }
  if (!convert(input, 2205, 22050, 1, 16000, 0, &output)) {
    return 0;
  }

  magnitude_sum = 0;
  measured_frames = 0;
  for (frame = 16; frame + 16 < output.frame_count; ++frame) {
    int sample;

    sample = output.samples[frame];
    magnitude_sum += sample < 0 ? -sample : sample;
    ++measured_frames;
  }
  result = output.sample_rate == 16000 &&
           output.channels == 1 &&
           measured_frames != 0 &&
           magnitude_sum / (int64_t) measured_frames < 1000;
  free(output.samples);
  return result;
}

int main(void) {
  if (!check_mono_passthrough()) {
    fprintf(stderr, "LAL changed mono PCM that required no conversion.\n");
    return 1;
  }
  if (!check_constant_power_mono()) {
    fprintf(stderr, "LAL did not preserve stereo energy while folding mono.\n");
    return 1;
  }
  if (!check_mono_peak_control()) {
    fprintf(stderr, "LAL did not smoothly contain forced-mono peaks.\n");
    return 1;
  }
  if (!check_antialiased_downsampling()) {
    fprintf(
      stderr,
      "LAL did not reject frequencies above the output Nyquist limit.\n");
    return 1;
  }
  return 0;
}
