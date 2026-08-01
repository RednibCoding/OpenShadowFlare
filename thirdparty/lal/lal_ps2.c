/*
 * LAL backend for the Sony PlayStation 2 (audsrv).
 *
 * The SPU2 is driven through ps2sdk's audsrv module, which runs a streaming
 * thread on the EE side and a ring buffer in the IOP. lal_platform_init
 * requests the 44100 Hz / 16-bit / stereo output format and registers a
 * fill-buffer callback: whenever audsrv's ring buffer has room for one more
 * chunk, the callback pulls that many frames out of the mixer through
 * lal_mix_frames and enqueues them for playback.
 *
 * The callback runs on audsrv's own thread while the game thread mutates
 * the mixer through the LAL API, so the platform lock is implemented with a
 * binary kernel semaphore.
 */

#include "lal_internal.h"

#include <audsrv.h>
#include <kernel.h>
#include <loadfile.h>

#include <stdbool.h>
#include <stdint.h>

#define LAL_PS2_CHUNK_FRAMES 1024
#define LAL_PS2_CHUNK_BYTES                                                     \
  (LAL_PS2_CHUNK_FRAMES * LAL_OUTPUT_CHANNELS * 2)

static int16_t g_mix_buffer[LAL_PS2_CHUNK_FRAMES * LAL_OUTPUT_CHANNELS];
static int g_mutex = -1;
static bool g_started;

static int on_fill(void *arg) {
  (void) arg;
  lal_platform_lock();
  lal_mix_frames(g_mix_buffer, LAL_PS2_CHUNK_FRAMES);
  lal_platform_unlock();
  audsrv_play_audio((const char *) g_mix_buffer, LAL_PS2_CHUNK_BYTES);
  return 0;
}

bool lal_platform_init(void) {
  struct audsrv_fmt_t format;
  ee_sema_t semaphore;
  int result;

  if (g_started) {
    return true;
  }

  semaphore.attr = 0;
  semaphore.option = 0;
  semaphore.init_count = 1;
  semaphore.max_count = 1;
  g_mutex = CreateSema(&semaphore);
  if (g_mutex < 0) {
    lal_set_error("Could not create the PS2 audio mutex.");
    return false;
  }

  if (SifLoadModule("cdrom0:\\AUDSRV.IRX;1", 0, NULL) < 0) {
    DeleteSema(g_mutex);
    g_mutex = -1;
    lal_set_error("Could not load the PS2 audio driver module.");
    return false;
  }

  if (audsrv_init() != 0) {
    DeleteSema(g_mutex);
    g_mutex = -1;
    lal_set_error("Could not initialize the PS2 audio driver.");
    return false;
  }

  format.freq = LAL_OUTPUT_SAMPLE_RATE;
  format.bits = 16;
  format.channels = LAL_OUTPUT_CHANNELS;
  if (audsrv_set_format(&format) != 0) {
    audsrv_quit();
    DeleteSema(g_mutex);
    g_mutex = -1;
    lal_set_error("The PS2 audio driver does not support the output format.");
    return false;
  }

  result = audsrv_on_fillbuf(LAL_PS2_CHUNK_BYTES, on_fill, NULL);
  if (result != 0) {
    audsrv_quit();
    DeleteSema(g_mutex);
    g_mutex = -1;
    lal_set_error("Could not register the PS2 audio callback.");
    return false;
  }

  g_started = true;
  return true;
}

void lal_platform_shutdown(void) {
  if (!g_started) {
    return;
  }
  g_started = false;
  audsrv_quit();
  if (g_mutex >= 0) {
    DeleteSema(g_mutex);
    g_mutex = -1;
  }
}

void lal_platform_lock(void) {
  if (g_mutex >= 0) {
    WaitSema(g_mutex);
  }
}

void lal_platform_unlock(void) {
  if (g_mutex >= 0) {
    SignalSema(g_mutex);
  }
}
