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

#include "core/arena.h"
#include "core/memory_budget.h"
#include "game/game.h"
#include "render/title.h"

#include "tal.h"
#include "twl.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_UPDATE_MICROSECONDS UINT64_C(16667)
#define SF_MAX_FRAME_MICROSECONDS UINT64_C(100000)

static void sf_read_event(
    const TwlEvent *event, SfGameInput *input, bool *running) {
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
  input->up_pressed = false;
  input->down_pressed = false;
  input->confirm_pressed = false;
  input->cancel_pressed = false;
}

int sf_application_run(
    void *main_memory, size_t main_memory_size,
    void *video_memory, size_t video_memory_size) {
  SfArena main_arena;
  SfArena video_arena;
  SfFramebuffer framebuffer;
  SfGame *game;
  SfGameInput input;
  TwlConfig window_config;
  TalConfig audio_config;
  TwlSurface surface;
  Twl *window;
  Tal *audio;
  void *window_memory;
  void *audio_memory;
  void *frame_memory;
  size_t window_bytes;
  size_t audio_bytes;
  uint64_t previous_time;
  uint64_t accumulator = 0u;
  bool running = true;

  if (!main_memory || main_memory_size > SF_MAIN_ARENA_BYTES ||
      !video_memory || video_memory_size > SF_VIDEO_MEMORY_LIMIT_BYTES)
    return 1;
  sf_arena_init(&main_arena, main_memory, main_memory_size);
  sf_arena_init(&video_arena, video_memory, video_memory_size);

  window_config = twl_config_default();
  window_config.title = "ShadowFlare";
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
  frame_memory = sf_arena_push(
    &video_arena, SF_FRAMEBUFFER_BYTES, sizeof(uint16_t));
  if (!game || !frame_memory || !sf_framebuffer_init(
        &framebuffer, frame_memory, SF_FRAMEBUFFER_BYTES,
        SF_FRAME_WIDTH, SF_FRAME_HEIGHT)) {
    if (audio) tal_shutdown(audio);
    twl_shutdown(window);
    return 3;
  }

  sf_game_init(game);
  sf_clear_input(&input);
  surface.pixels = framebuffer.pixels;
  surface.width = framebuffer.width;
  surface.height = framebuffer.height;
  surface.stride_bytes =
    (size_t) framebuffer.stride * sizeof(uint16_t);
  surface.format = TWL_PIXEL_RGB555;
  previous_time = twl_time_microseconds(window);

  while (running && !game->quit_requested) {
    TwlEvent event;
    uint64_t now;
    uint64_t elapsed;
    twl_pump_events(window);
    while (twl_poll_event(window, &event))
      sf_read_event(&event, &input, &running);

    now = twl_time_microseconds(window);
    elapsed = now >= previous_time ? now - previous_time : 0u;
    previous_time = now;
    if (elapsed > SF_MAX_FRAME_MICROSECONDS)
      elapsed = SF_MAX_FRAME_MICROSECONDS;
    accumulator += elapsed;
    while (accumulator >= SF_UPDATE_MICROSECONDS) {
      sf_game_update(game, &input);
      sf_clear_input(&input);
      accumulator -= SF_UPDATE_MICROSECONDS;
    }

    sf_title_render(game, &framebuffer);
    if (twl_present(window, &surface) != TWL_RESULT_OK) {
      running = false;
    }
    if (audio) (void) tal_update(audio);
  }

  if (audio) tal_shutdown(audio);
  twl_shutdown(window);
  return 0;
}
