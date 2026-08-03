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

#include "runtime/application.h"

#include "assets/title_assets.h"
#include "core/arena.h"
#include "core/memory_budget.h"
#include "game/game.h"
#include "render/renderer.h"
#include "screens/title_screen.h"

#include "tal.h"
#include "twl.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define SF_FRAME_MICROSECONDS UINT64_C(33333)
#define SF_MAX_FRAME_MICROSECONDS UINT64_C(100000)

static void sf_read_event(
    Twl *window, const TwlEvent *event,
    SfGameInput *input, bool *running) {
  if (event->type == TWL_EVENT_POINTER_MOVE ||
      event->type == TWL_EVENT_POINTER_DOWN ||
      event->type == TWL_EVENT_POINTER_UP) {
    uint32_t width;
    uint32_t height;
    twl_get_display_size(window, &width, &height);
    if (width > 0u && height > 0u) {
      int x = (int) ((int64_t) event->x * SF_FRAME_WIDTH / width);
      int y = (int) ((int64_t) event->y * SF_FRAME_HEIGHT / height);
      if (x < 0) x = 0;
      if (y < 0) y = 0;
      if (x >= (int) SF_FRAME_WIDTH) x = SF_FRAME_WIDTH - 1;
      if (y >= (int) SF_FRAME_HEIGHT) y = SF_FRAME_HEIGHT - 1;
      input->pointer_x = (int16_t) x;
      input->pointer_y = (int16_t) y;
    }
    if (event->type == TWL_EVENT_POINTER_DOWN && event->button == 1u)
      input->pointer_primary_pressed = true;
  }
  if (event->type == TWL_EVENT_QUIT) {
    *running = false;
    return;
  }
  if (event->type == TWL_EVENT_KEY_DOWN && !event->repeat) {
    if (event->key == TWL_KEY_UP) input->up_pressed = true;
    if (event->key == TWL_KEY_DOWN) input->down_pressed = true;
    if (event->key == TWL_KEY_RETURN) input->confirm_pressed = true;
    if (event->key == TWL_KEY_ESCAPE) input->cancel_pressed = true;
  }
  if (event->type == TWL_EVENT_CONTROLLER_BUTTON_DOWN) {
    if (event->controller_button == TWL_CONTROLLER_BUTTON_DPAD_UP)
      input->up_pressed = true;
    if (event->controller_button == TWL_CONTROLLER_BUTTON_DPAD_DOWN)
      input->down_pressed = true;
    if (event->controller_button == TWL_CONTROLLER_BUTTON_SOUTH)
      input->confirm_pressed = true;
    if (event->controller_button == TWL_CONTROLLER_BUTTON_EAST ||
        event->controller_button == TWL_CONTROLLER_BUTTON_BACK)
      input->cancel_pressed = true;
  }
}

static void sf_clear_input(SfGameInput *input) {
  input->pointer_primary_pressed = false;
  input->up_pressed = false;
  input->down_pressed = false;
  input->confirm_pressed = false;
  input->cancel_pressed = false;
}

static void sf_play_pcm(Tal *audio, const SfPcmU8 *source, bool loop) {
  TalPcm pcm;
  TalPlayOptions options;
  TalVoice voice;
  if (!audio || !source || !source->samples || source->frame_count == 0u)
    return;
  pcm.samples = source->samples;
  pcm.frame_count = source->frame_count;
  pcm.sample_rate = source->sample_rate;
  pcm.channels = 1u;
  pcm.format = TAL_SAMPLE_U8;
  options = tal_play_options_default();
  options.loop = loop;
  (void) tal_play(audio, &pcm, &options, &voice);
}

static void sf_play_title_events(
    Tal *audio, const SfTitleAssets *assets, uint8_t events) {
  if ((events & SF_GAME_SOUND_TITLE_CUE) != 0u)
    sf_play_pcm(audio, &assets->sounds[SF_TITLE_SOUND_CUE], false);
  if ((events & SF_GAME_SOUND_MENU_MOVE) != 0u)
    sf_play_pcm(audio, &assets->sounds[SF_TITLE_SOUND_MOVE], false);
  if ((events & SF_GAME_SOUND_TITLE_CONFIRM) != 0u)
    sf_play_pcm(audio, &assets->sounds[SF_TITLE_SOUND_CONFIRM], false);
  if ((events & SF_GAME_SOUND_TITLE_MUSIC) != 0u)
    sf_play_pcm(audio, &assets->music, true);
}

int sf_application_run(
    void *main_memory, size_t main_memory_size,
    void *video_memory, size_t video_memory_size,
    const char *data_root) {
  SfArena main_arena;
  SfArena video_arena;
  SfRenderer renderer;
  const SfFramebuffer *framebuffer;
  SfTitleScreen title_screen;
  SfTitleAssets *title_assets;
  SfGame *game;
  SfGameConfig game_config;
  SfGameInput input;
  TwlConfig window_config;
  TalConfig audio_config;
  TwlSurface surface;
  Twl *window;
  Tal *audio;
  void *window_memory;
  void *audio_memory;
  void *frame_memory;
  void *decode_scratch;
  size_t window_bytes;
  size_t audio_bytes;
  uint64_t next_frame;
  bool running = true;

  if (!main_memory || main_memory_size > SF_MAIN_ARENA_BYTES ||
      !video_memory || video_memory_size > SF_VIDEO_MEMORY_LIMIT_BYTES)
    return 1;
  sf_arena_init(&main_arena, main_memory, main_memory_size);
  sf_arena_init(&video_arena, video_memory, video_memory_size);

  window_config = twl_config_default();
  window_config.title = "OpenShadowFlare";
  window_config.width = SF_FRAME_WIDTH;
  window_config.height = SF_FRAME_HEIGHT;
  window_config.event_capacity = 64u;
  window_config.controller_capacity = 2u;
  window_config.resizable = true;
  window_bytes = twl_memory_required(&window_config);
  window_memory = sf_arena_push(
    &main_arena, window_bytes, twl_memory_alignment());
  if (!window_memory || twl_init(
        window_memory, window_bytes, &window_config, &window) != TWL_RESULT_OK)
    return 2;

  audio_config = tal_config_default();
  audio_config.sample_rate = 11025u;
  audio_config.mix_block_frames = 256u;
  audio_config.max_voices = 8u;
  audio_config.channels = 1u;
  audio_bytes = tal_memory_required(&audio_config);
  audio_memory = sf_arena_push(
    &main_arena, audio_bytes, tal_memory_alignment());
  audio = NULL;
  if (audio_memory)
    (void) tal_init(audio_memory, audio_bytes, &audio_config, &audio);

  game = (SfGame *) sf_arena_push_zero(
    &main_arena, sizeof(*game), sizeof(void *));
  title_assets = (SfTitleAssets *) sf_arena_push_zero(
    &main_arena, sizeof(*title_assets), sizeof(void *));
  decode_scratch = sf_arena_push(
    &main_arena, SF_TITLE_DECODE_SCRATCH_BYTES, 4u);
  if (!game || !title_assets || !decode_scratch ||
      !sf_title_assets_load(
        title_assets, data_root, &main_arena,
        decode_scratch, SF_TITLE_DECODE_SCRATCH_BYTES)) {
    fprintf(stderr, "Could not load the retail title assets from '%s'.\n",
      data_root ? data_root : "");
    if (audio) tal_shutdown(audio);
    twl_shutdown(window);
    return 3;
  }
  frame_memory = sf_arena_push(
    &video_arena, SF_FRAMEBUFFER_BYTES, sizeof(uint16_t));
  if (!frame_memory || !sf_renderer_init(
        &renderer, frame_memory, SF_FRAMEBUFFER_BYTES,
        SF_FRAME_WIDTH, SF_FRAME_HEIGHT) ||
      !sf_title_screen_init(
        &title_screen, decode_scratch, SF_TITLE_DECODE_SCRATCH_BYTES,
        title_assets->decode_scratch_bytes)) {
    if (audio) tal_shutdown(audio);
    twl_shutdown(window);
    return 4;
  }
  framebuffer = sf_renderer_framebuffer(&renderer);

  game_config.saved_game_exists = false;
  game_config.next_save_available = true;
  {
    unsigned smoke;
    for (smoke = 0u; smoke < SF_TITLE_SMOKE_COUNT; ++smoke)
      game_config.title_smoke_frame_count[smoke] =
        title_assets->smoke[smoke].animation.frame_count;
  }
  sf_game_init(game, &game_config);
  input.pointer_x = 0;
  input.pointer_y = 0;
  sf_clear_input(&input);
  surface.pixels = framebuffer->pixels;
  surface.width = framebuffer->width;
  surface.height = framebuffer->height;
  surface.stride_bytes =
    (size_t) framebuffer->stride * sizeof(uint16_t);
  surface.format = TWL_PIXEL_RGB555;
  next_frame = twl_time_microseconds(window);

  while (running && !game->quit_requested) {
    TwlEvent event;
    uint64_t now;
    twl_pump_events(window);
    while (twl_poll_event(window, &event))
      sf_read_event(window, &event, &input, &running);
    if (!running) break;

    now = twl_time_microseconds(window);
    if (now < next_frame) {
      twl_sleep_microseconds(window, next_frame - now);
      now = twl_time_microseconds(window);
    }
    {
      unsigned updates = 0u;
      while (now >= next_frame && updates < 3u) {
        sf_game_update(game, &input);
        sf_play_title_events(audio, title_assets, game->title.sound_events);
        sf_clear_input(&input);
        next_frame += SF_FRAME_MICROSECONDS;
        ++updates;
      }
      if (now > next_frame + SF_MAX_FRAME_MICROSECONDS)
        next_frame = now + SF_FRAME_MICROSECONDS;
    }

    if (game->mode == SF_GAME_MODE_TITLE) {
      sf_title_screen_draw(&title_screen, &renderer, title_assets, game);
    } else {
      sf_renderer_clear(&renderer, 0u);
    }
    if (twl_present(window, &surface) != TWL_RESULT_OK) running = false;
    if (audio) (void) tal_update(audio);
  }

  if (audio) tal_shutdown(audio);
  twl_shutdown(window);
  return 0;
}
