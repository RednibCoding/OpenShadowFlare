#include "lal_internal.h"

#include <aaudio/AAudio.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static AAudioStream *g_stream;
static pthread_mutex_t g_mutex;
static bool g_mutex_ready;
static bool g_running;

static aaudio_data_callback_result_t lal_aaudio_callback(
    AAudioStream *stream, void *user_data, void *audio_data,
    int32_t frame_count) {
  bool running;
  (void) stream;
  (void) user_data;

  pthread_mutex_lock(&g_mutex);
  running = g_running;
  if (running) {
    lal_mix_frames((int16_t *) audio_data, (size_t) frame_count);
  }
  pthread_mutex_unlock(&g_mutex);
  if (!running) {
    memset(audio_data, 0, (size_t) frame_count * LAL_OUTPUT_CHANNELS *
                         sizeof(int16_t));
  }
  return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

bool lal_platform_init(void) {
  AAudioStreamBuilder *builder = NULL;
  aaudio_result_t result;

  if (pthread_mutex_init(&g_mutex, NULL) != 0) {
    lal_set_error("Could not create the Android audio mutex.");
    return false;
  }
  g_mutex_ready = true;
  result = AAudio_createStreamBuilder(&builder);
  if (result != AAUDIO_OK) {
    lal_set_error("Could not create the Android audio stream builder.");
    lal_platform_shutdown();
    return false;
  }
  AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
  AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
  AAudioStreamBuilder_setSampleRate(builder, LAL_OUTPUT_SAMPLE_RATE);
  AAudioStreamBuilder_setChannelCount(builder, LAL_OUTPUT_CHANNELS);
  AAudioStreamBuilder_setPerformanceMode(
      builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
  AAudioStreamBuilder_setDataCallback(builder, lal_aaudio_callback, NULL);
  result = AAudioStreamBuilder_openStream(builder, &g_stream);
  AAudioStreamBuilder_delete(builder);
  if (result != AAUDIO_OK || !g_stream) {
    lal_set_error("Could not open the Android audio output stream.");
    lal_platform_shutdown();
    return false;
  }
  if (AAudioStream_getSampleRate(g_stream) != LAL_OUTPUT_SAMPLE_RATE ||
      AAudioStream_getChannelCount(g_stream) != LAL_OUTPUT_CHANNELS ||
      AAudioStream_getFormat(g_stream) != AAUDIO_FORMAT_PCM_I16) {
    lal_set_error("Android audio did not accept 44.1 kHz stereo PCM.");
    lal_platform_shutdown();
    return false;
  }
  g_running = true;
  result = AAudioStream_requestStart(g_stream);
  if (result != AAUDIO_OK) {
    lal_set_error("Could not start the Android audio output stream.");
    lal_platform_shutdown();
    return false;
  }
  return true;
}

void lal_platform_shutdown(void) {
  if (g_mutex_ready) {
    pthread_mutex_lock(&g_mutex);
    g_running = false;
    pthread_mutex_unlock(&g_mutex);
  }
  if (g_stream) {
    AAudioStream_requestStop(g_stream);
    AAudioStream_close(g_stream);
    g_stream = NULL;
  }
  if (g_mutex_ready) {
    pthread_mutex_destroy(&g_mutex);
    g_mutex_ready = false;
  }
}

void lal_platform_lock(void) {
  if (g_mutex_ready) {
    pthread_mutex_lock(&g_mutex);
  }
}

void lal_platform_unlock(void) {
  if (g_mutex_ready) {
    pthread_mutex_unlock(&g_mutex);
  }
}
