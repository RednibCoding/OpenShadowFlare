#include "lal.h"
#include "lgl.h"
#include "lwl.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int check_rgba_layout(void) {
  LwlColor color = {1, 2, 3, 4};
  const uint8_t *bytes = (const uint8_t *) &color;
  return sizeof(color) == 4 &&
         offsetof(LwlColor, r) == 0 &&
         offsetof(LwlColor, g) == 1 &&
         offsetof(LwlColor, b) == 2 &&
         offsetof(LwlColor, a) == 3 &&
         bytes[0] == 1 &&
         bytes[1] == 2 &&
         bytes[2] == 3 &&
         bytes[3] == 4;
}

static int check_pcm_conversion(void) {
  const int16_t samples[] = {-32768, 32767};
  LalPcmFormat format;
  LalConfig config;
  LalSound *sound;
  uint32_t expected_rate;
  size_t expected_frames;
  int result;

  config = lal_config_default();
  memset(&format, 0, sizeof(format));
  format.sample_rate = 22050;
  format.channels = 1;
  format.bits_per_sample = 16;
  expected_rate = config.maximum_sample_rate < format.sample_rate
    ? config.maximum_sample_rate
    : format.sample_rate;
  expected_frames =
    (2u * expected_rate + format.sample_rate / 2u) /
      format.sample_rate;
  sound = lal_sound_create_pcm(samples, sizeof(samples), &format);
  result = sound != NULL &&
           lal_sound_frame_count(sound) == expected_frames &&
           lal_sound_sample_rate(sound) == expected_rate &&
           lal_sound_channel_count(sound) == 1 &&
           lal_sound_memory_usage_bytes(sound) >= sizeof(samples) &&
           lal_sound_duration(sound) > 0.0;
  lal_sound_destroy(sound);
  return result;
}

static int check_stereo_storage(void) {
  const int16_t samples[] = {-12000, 4000, 12000, -4000};
  LalPcmFormat format;
  LalConfig config;
  LalSound *sound;
  uint16_t expected_channels;
  int result;

  config = lal_config_default();
  memset(&format, 0, sizeof(format));
  format.sample_rate = 16000;
  format.channels = 2;
  format.bits_per_sample = 16;
  expected_channels = config.force_mono ? 1 : 2;
  sound = lal_sound_create_pcm(samples, sizeof(samples), &format);
  result = sound != NULL &&
           lal_sound_channel_count(sound) == expected_channels;
  lal_sound_destroy(sound);
  return result;
}

static void *unavailable_gl_function(
    const char *name, void *user_data) {
  (void) name;
  (void) user_data;
  return NULL;
}

int main(void) {
  if (!check_rgba_layout()) {
    fprintf(stderr, "LWL framebuffer is not byte-ordered RGBA.\n");
    return 1;
  }
  if (!check_pcm_conversion() || !check_stereo_storage()) {
    fprintf(stderr, "LAL in-memory PCM conversion failed: %s\n",
            lal_last_error());
    return 1;
  }
  if (lgl_load(NULL, NULL) ||
      strstr(lgl_last_error(), "callback") == NULL) {
    fprintf(stderr, "LGL rejected-loader diagnostics failed.\n");
    return 1;
  }
  if (lgl_load_for_api(
          unavailable_gl_function,
          NULL,
          (LglApi) 99) ||
      strstr(lgl_last_error(), "Unknown") == NULL) {
    fprintf(stderr, "LGL rejected-API diagnostics failed.\n");
    return 1;
  }
  return 0;
}
