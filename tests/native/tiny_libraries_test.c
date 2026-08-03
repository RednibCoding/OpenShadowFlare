/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of OpenShadowFlare.
 *
 * OpenShadowFlare is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * OpenShadowFlare is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
 */

#include "tal.h"
#include "twl.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TEST_CHECK(condition, message) \
  do { \
    if (!(condition)) { \
      fputs(message "\n", stderr); \
      return 0; \
    } \
  } while (0)

typedef union {
  void *pointer_alignment;
  uint64_t integer_alignment;
  long double floating_point_alignment;
  uint8_t bytes[16384];
} AlignedStorage;

static int test_twl(void) {
  AlignedStorage storage;
  TwlConfig config = twl_config_default();
  TwlSurface surface;
  TwlControllerState controller;
  Twl *twl = NULL;
  uint16_t pixels[4] = {0u, 31u, 992u, 31744u};
  size_t required;

  config.display_mode = TWL_DISPLAY_HEADLESS;
  config.event_capacity = 8u;
  config.controller_capacity = 2u;
  required = twl_memory_required(&config);
  TEST_CHECK(
    required > 0u && required <= sizeof(storage.bytes),
    "TWL returned an invalid memory requirement");
  TEST_CHECK(
    twl_init(storage.bytes, required - 1u, &config, &twl) ==
      TWL_RESULT_INSUFFICIENT_MEMORY,
    "TWL did not reject insufficient caller memory");
  TEST_CHECK(
    twl_init(storage.bytes, required, &config, &twl) == TWL_RESULT_OK,
    "TWL headless initialization failed");
  TEST_CHECK(
    twl_controller_state(twl, 0u, &controller) && !controller.connected,
    "TWL controller state was not initialized");
  surface.pixels = pixels;
  surface.width = 2u;
  surface.height = 2u;
  surface.stride_bytes = 2u * sizeof(uint16_t);
  surface.format = TWL_PIXEL_RGB555;
  TEST_CHECK(
    twl_present(twl, &surface) == TWL_RESULT_OK,
    "TWL rejected a valid RGB555 surface");
  TEST_CHECK(
    twl_display_frame(twl) == TWL_RESULT_INVALID_ARGUMENT,
    "TWL displayed a frame that had not been prepared");
  TEST_CHECK(
    twl_prepare_frame(twl, &surface) == TWL_RESULT_OK &&
      twl_display_frame(twl) == TWL_RESULT_OK,
    "TWL rejected split frame preparation and display");
  twl_shutdown(twl);
  return 1;
}

static int test_tal(void) {
  AlignedStorage storage;
  TalConfig config = tal_config_default();
  TalPlayOptions options = tal_play_options_default();
  static const int16_t samples[] = {0, 12000, -12000, 6000};
  static const uint8_t samples_u8[] = {0u, 128u, 255u, 128u};
  TalPcm pcm;
  TalVoice voice;
  Tal *tal = NULL;
  int16_t output[8];
  size_t required;

  config.output_mode = TAL_OUTPUT_MANUAL;
  config.sample_rate = 16000u;
  config.mix_block_frames = 4u;
  config.max_voices = 4u;
  config.channels = 2u;
  required = tal_memory_required(&config);
  TEST_CHECK(
    required > 0u && required <= sizeof(storage.bytes),
    "TAL returned an invalid memory requirement");
  TEST_CHECK(
    tal_init(storage.bytes, required, &config, &tal) == TAL_RESULT_OK,
    "TAL manual initialization failed");

  pcm.samples = samples;
  pcm.frame_count = 4u;
  pcm.sample_rate = 16000u;
  pcm.channels = 1u;
  pcm.format = TAL_SAMPLE_S16;
  TEST_CHECK(
    tal_play(tal, &pcm, &options, &voice) == TAL_RESULT_OK &&
      voice != TAL_INVALID_VOICE,
    "TAL could not start a valid PCM voice");
  TEST_CHECK(
    tal_render(tal, output, 4u) == TAL_RESULT_OK,
    "TAL could not render a manual block");
  TEST_CHECK(
    output[0] == 0 && output[1] == 0 &&
      output[2] == 12000 && output[3] == 12000 &&
      output[4] == -12000 && output[5] == -12000 &&
      output[6] == 6000 && output[7] == 6000,
    "TAL rendered unexpected PCM samples");
  TEST_CHECK(
    !tal_voice_playing(tal, voice),
    "TAL voice remained active after its final frame");
  pcm.samples = samples_u8;
  pcm.format = TAL_SAMPLE_U8;
  TEST_CHECK(
    tal_play(tal, &pcm, &options, &voice) == TAL_RESULT_OK,
    "TAL could not start unsigned 8-bit PCM");
  TEST_CHECK(
    tal_render(tal, output, 4u) == TAL_RESULT_OK &&
      output[0] == -32768 && output[1] == -32768 &&
      output[2] == 0 && output[3] == 0 &&
      output[4] == 32512 && output[5] == 32512 &&
      output[6] == 0 && output[7] == 0,
    "TAL rendered unexpected unsigned 8-bit PCM samples");
  tal_shutdown(tal);
  return 1;
}

int main(void) {
  return test_twl() && test_tal() ? 0 : 1;
}
