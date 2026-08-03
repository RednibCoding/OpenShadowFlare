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

typedef union {
  max_align_t alignment;
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
  if (required == 0u || required > sizeof(storage.bytes)) return 0;
  if (twl_init(
        storage.bytes, required - 1u, &config, &twl) !=
      TWL_RESULT_INSUFFICIENT_MEMORY) return 0;
  if (twl_init(storage.bytes, required, &config, &twl) != TWL_RESULT_OK)
    return 0;
  if (!twl_controller_state(twl, 0u, &controller) || controller.connected)
    return 0;
  surface.pixels = pixels;
  surface.width = 2u;
  surface.height = 2u;
  surface.stride_bytes = 2u * sizeof(uint16_t);
  surface.format = TWL_PIXEL_RGB555;
  if (twl_present(twl, &surface) != TWL_RESULT_OK) return 0;
  twl_shutdown(twl);
  return 1;
}

static int test_tal(void) {
  AlignedStorage storage;
  TalConfig config = tal_config_default();
  TalPlayOptions options = tal_play_options_default();
  static const int16_t samples[] = {0, 12000, -12000, 6000};
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
  if (required == 0u || required > sizeof(storage.bytes)) return 0;
  if (tal_init(storage.bytes, required, &config, &tal) != TAL_RESULT_OK)
    return 0;

  pcm.samples = samples;
  pcm.frame_count = 4u;
  pcm.sample_rate = 16000u;
  pcm.channels = 1u;
  if (tal_play(tal, &pcm, &options, &voice) != TAL_RESULT_OK ||
      voice == TAL_INVALID_VOICE) return 0;
  if (tal_render(tal, output, 4u) != TAL_RESULT_OK) return 0;
  if (output[0] != 0 || output[1] != 0 ||
      output[2] != 12000 || output[3] != 12000 ||
      output[4] != -12000 || output[5] != -12000 ||
      output[6] != 6000 || output[7] != 6000) return 0;
  if (tal_voice_playing(tal, voice)) return 0;
  tal_shutdown(tal);
  return 1;
}

int main(void) {
  return test_twl() && test_tal() ? 0 : 1;
}
