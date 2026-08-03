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

#include <emscripten.h>

typedef struct {
  Tal *tal;
} TalWeb;

EMSCRIPTEN_KEEPALIVE
int16_t *tal_web_mix(TalWeb *web, int frame_count) {
  uint32_t frames;
  if (!web || !web->tal || frame_count <= 0) return NULL;
  frames = (uint32_t) frame_count;
  if (frames > web->tal->config.mix_block_frames)
    frames = web->tal->config.mix_block_frames;
  tal_internal_render(
    web->tal, tal_internal_mix_buffer(web->tal), frames);
  return tal_internal_mix_buffer(web->tal);
}

EM_JS(int, tal_web_start,
      (TalWeb *web, int sample_rate, int channels, int block_frames), {
  try {
    var AudioContextClass = window.AudioContext || window.webkitAudioContext;
    if (!AudioContextClass) return 0;
    if (!Module.talDevices) Module.talDevices = {};
    var context = new AudioContextClass({ sampleRate: sample_rate });
    var node = context.createScriptProcessor(block_frames, 0, channels);
    node.onaudioprocess = function(event) {
      var frames = event.outputBuffer.length;
      var mixedFrames = Math.min(frames, block_frames);
      var pointer = _tal_web_mix(web, mixedFrames);
      var input = HEAP16;
      var base = pointer >> 1;
      for (var channel = 0; channel < channels; ++channel) {
        var output = event.outputBuffer.getChannelData(channel);
        for (var frame = 0; frame < mixedFrames; ++frame)
          output[frame] = input[base + frame * channels + channel] / 32768;
        for (var silentFrame = mixedFrames; silentFrame < frames; ++silentFrame)
          output[silentFrame] = 0;
      }
    };
    node.connect(context.destination);
    var resume = function() {
      if (context.state !== 'running') context.resume();
    };
    ['pointerdown', 'keydown', 'touchstart'].forEach(function(name) {
      window.addEventListener(name, resume, { passive: true });
    });
    Module.talDevices[web] = {
      context: context, node: node, resume: resume
    };
    return 1;
  } catch (error) {
    return 0;
  }
})

EM_JS(void, tal_web_stop, (TalWeb *web), {
  var devices = Module.talDevices;
  var device = devices ? devices[web] : null;
  if (!device) return;
  try {
    device.node.disconnect();
    device.node.onaudioprocess = null;
    device.context.close();
    ['pointerdown', 'keydown', 'touchstart'].forEach(function(name) {
      window.removeEventListener(name, device.resume);
    });
  } catch (error) {
  }
  delete devices[web];
})

size_t tal_backend_memory_alignment(void) {
  return _Alignof(TalWeb);
}

size_t tal_backend_memory_required(const TalConfig *config) {
  (void) config;
  return sizeof(TalWeb);
}

TalResult tal_backend_init(
    Tal *tal, void *memory, size_t memory_size, const TalConfig *config) {
  TalWeb *web;
  if (!tal || !memory || memory_size < sizeof(TalWeb) || !config)
    return TAL_RESULT_INVALID_ARGUMENT;
  web = (TalWeb *) memory;
  web->tal = tal;
  return tal_web_start(
           web, (int) config->sample_rate, (int) config->channels,
           (int) config->mix_block_frames)
           ? TAL_RESULT_OK
           : TAL_RESULT_BACKEND_UNAVAILABLE;
}

void tal_backend_shutdown(Tal *tal) {
  TalWeb *web = tal ? (TalWeb *) tal->backend : NULL;
  if (!web) return;
  tal_web_stop(web);
  web->tal = NULL;
}

TalResult tal_backend_update(Tal *tal) {
  (void) tal;
  return TAL_RESULT_OK;
}

void tal_backend_lock(Tal *tal) {
  (void) tal;
}

void tal_backend_unlock(Tal *tal) {
  (void) tal;
}
