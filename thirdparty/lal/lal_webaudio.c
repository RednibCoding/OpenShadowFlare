/*
 * LAL backend for Emscripten / WebAssembly (Web Audio API).
 *
 * A ScriptProcessorNode runs on the browser main thread and pulls interleaved
 * 16-bit stereo frames straight from lal_mix_frames each audio quantum, then
 * converts them to Web Audio's planar float32 output. Because both the mixer
 * pull and every LAL API mutation happen on the single main thread, no lock
 * is needed and no pthreads/SharedArrayBuffer are required -- so the page
 * does NOT need Cross-Origin-Opener/Embedder-Policy headers.
 *
 * The AudioContext is created at LAL's own 44100 Hz output rate to avoid a
 * resample, and is resumed on the first user gesture to satisfy the browser
 * autoplay policy.
 */

#include "lal_internal.h"

#include <emscripten.h>

#include <stdbool.h>
#include <stdint.h>

#define LAL_WEB_BLOCK_FRAMES 2048
#define LAL_WEB_MAX_BLOCK_FRAMES 8192

static int16_t g_mix_buffer[LAL_WEB_MAX_BLOCK_FRAMES * LAL_OUTPUT_CHANNELS];
static bool g_started;

EMSCRIPTEN_KEEPALIVE
int16_t *lal_web_mix(int frame_count) {
  if (frame_count < 0) {
    frame_count = 0;
  }
  if (frame_count > LAL_WEB_MAX_BLOCK_FRAMES) {
    frame_count = LAL_WEB_MAX_BLOCK_FRAMES;
  }
  lal_mix_frames(g_mix_buffer, (size_t) frame_count);
  return g_mix_buffer;
}

EM_JS(int, lal_web_audio_start, (int sample_rate, int block_frames), {
  try {
    if (Module['lalAudio'] && Module['lalAudio'].ctx) {
      return 1;
    }
    var AudioContextClass = window.AudioContext || window.webkitAudioContext;
    if (!AudioContextClass) {
      return 0;
    }
    var ctx = new AudioContextClass({ sampleRate: sample_rate });
    var node = ctx.createScriptProcessor(block_frames, 0, 2);
    node.onaudioprocess = function(event) {
      var frames = event.outputBuffer.length;
      var ptr = _lal_web_mix(frames);
      var base = ptr >> 1;
      var heap = HEAP16;   /* re-read each call in case memory grew */
      var left = event.outputBuffer.getChannelData(0);
      var right = event.outputBuffer.getChannelData(1);
      for (var i = 0; i < frames; ++i) {
        left[i] = heap[base + i * 2] / 32768;
        right[i] = heap[base + i * 2 + 1] / 32768;
      }
    };
    node.connect(ctx.destination);
    Module['lalAudio'] = { ctx: ctx, node: node };

    var resume = function() {
      if (ctx.state !== 'running') {
        ctx.resume();
      }
    };
    ['pointerdown', 'keydown', 'touchstart'].forEach(function(name) {
      window.addEventListener(name, resume, { passive: true });
    });
    return 1;
  } catch (error) {
    return 0;
  }
})

EM_JS(void, lal_web_audio_stop, (void), {
  var audio = Module['lalAudio'];
  if (!audio) {
    return;
  }
  try {
    if (audio.node) {
      audio.node.disconnect();
      audio.node.onaudioprocess = null;
    }
    if (audio.ctx) {
      audio.ctx.close();
    }
  } catch (error) {
  }
  Module['lalAudio'] = null;
})

bool lal_platform_init(void) {
  if (g_started) {
    return true;
  }
  if (!lal_web_audio_start(LAL_OUTPUT_SAMPLE_RATE, LAL_WEB_BLOCK_FRAMES)) {
    lal_set_error("Could not start the Web Audio output.");
    return false;
  }
  g_started = true;
  return true;
}

void lal_platform_shutdown(void) {
  if (!g_started) {
    return;
  }
  lal_web_audio_stop();
  g_started = false;
}

void lal_platform_lock(void) {}

void lal_platform_unlock(void) {}
