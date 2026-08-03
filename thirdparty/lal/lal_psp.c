#include "lal_internal.h"

#include <pspaudio.h>
#include <pspkernel.h>
#include <pspthreadman.h>
#include <psputils.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum {
  LAL_PSP_BUFFER_COUNT = 2,
  LAL_PSP_OUTPUT_FRAMES = 1024,
  LAL_PSP_MIX_FRAMES = LAL_PSP_OUTPUT_FRAMES / 4,
  LAL_PSP_THREAD_PRIORITY = 0x20,
  LAL_PSP_THREAD_STACK_SIZE = 0x4000
};

static int16_t g_buffers[LAL_PSP_BUFFER_COUNT]
    [LAL_PSP_OUTPUT_FRAMES * LAL_OUTPUT_CHANNELS]
    __attribute__((aligned(64)));
static int16_t g_mix_buffer[LAL_PSP_MIX_FRAMES * LAL_OUTPUT_CHANNELS]
    __attribute__((aligned(64)));
static SceUID g_audio_channel = -1;
static SceUID g_mutex = -1;
static SceUID g_thread = -1;
static bool g_running;

void lal_platform_shutdown(void);

static int lal_psp_thread(SceSize args, void *argp) {
  int buffer_index = 0;

  (void) args;
  (void) argp;
  for (;;) {
    int16_t *buffer;
    bool running;
    size_t frame;

    lal_platform_lock();
    running = g_running;
    if (running) {
      lal_mix_frames(g_mix_buffer, LAL_PSP_MIX_FRAMES);
    }
    lal_platform_unlock();
    if (!running) {
      break;
    }

    buffer = g_buffers[buffer_index];
    for (frame = 0; frame < LAL_PSP_OUTPUT_FRAMES; ++frame) {
      const size_t source = frame / 4;
      buffer[frame * LAL_OUTPUT_CHANNELS] =
          g_mix_buffer[source * LAL_OUTPUT_CHANNELS];
      buffer[frame * LAL_OUTPUT_CHANNELS + 1] =
          g_mix_buffer[source * LAL_OUTPUT_CHANNELS + 1];
    }
    sceKernelDcacheWritebackRange(
        buffer,
        LAL_PSP_OUTPUT_FRAMES * LAL_OUTPUT_CHANNELS * sizeof(*buffer));
    if (sceAudioOutputBlocking(
            g_audio_channel, PSP_AUDIO_VOLUME_MAX, buffer) < 0) {
      lal_platform_lock();
      g_running = false;
      lal_platform_unlock();
      break;
    }
    buffer_index = (buffer_index + 1) % LAL_PSP_BUFFER_COUNT;
  }
  return 0;
}

bool lal_platform_init(void) {
  g_mutex = sceKernelCreateSema("osf_lal", 0, 1, 1, NULL);
  if (g_mutex < 0) {
    lal_set_error("Could not create the PSP audio mutex.");
    return false;
  }

  g_audio_channel = sceAudioChReserve(
      PSP_AUDIO_NEXT_CHANNEL,
      LAL_PSP_OUTPUT_FRAMES,
      PSP_AUDIO_FORMAT_STEREO);
  if (g_audio_channel < 0) {
    lal_set_error("Could not reserve a PSP audio channel.");
    lal_platform_shutdown();
    return false;
  }

  memset(g_buffers, 0, sizeof(g_buffers));
  g_running = true;
  g_thread = sceKernelCreateThread(
      "osf_lal", lal_psp_thread, LAL_PSP_THREAD_PRIORITY,
      LAL_PSP_THREAD_STACK_SIZE,
      THREAD_ATTR_USER | THREAD_ATTR_VFPU,
      NULL);
  if (g_thread < 0 || sceKernelStartThread(g_thread, 0, NULL) < 0) {
    lal_set_error("Could not start the PSP audio thread.");
    lal_platform_shutdown();
    return false;
  }
  return true;
}

void lal_platform_shutdown(void) {
  if (g_mutex >= 0) {
    lal_platform_lock();
    g_running = false;
    lal_platform_unlock();
  }
  if (g_thread >= 0) {
    sceKernelWaitThreadEnd(g_thread, NULL);
    sceKernelDeleteThread(g_thread);
    g_thread = -1;
  }
  if (g_audio_channel >= 0) {
    sceAudioChRelease(g_audio_channel);
    g_audio_channel = -1;
  }
  if (g_mutex >= 0) {
    sceKernelDeleteSema(g_mutex);
    g_mutex = -1;
  }
  g_running = false;
}

void lal_platform_lock(void) {
  if (g_mutex >= 0) {
    sceKernelWaitSema(g_mutex, 1, NULL);
  }
}

void lal_platform_unlock(void) {
  if (g_mutex >= 0) {
    sceKernelSignalSema(g_mutex, 1);
  }
}
