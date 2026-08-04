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

#include "assets/menu_assets.h"
#include "assets/retail_paths.h"
#include "core/arena.h"
#include "core/memory_budget.h"
#include "data/save.h"
#include "game/game.h"
#include "render/renderer.h"
#include "runtime/screen_runtime.h"

#if defined(SF_ENABLE_PROFILING)
#include "runtime/profiler.h"
#endif

#include "tal.h"
#include "twl.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if !defined(SF_PRESENTATION_HZ)
#define SF_PRESENTATION_HZ 60
#endif
#if SF_PRESENTATION_HZ != 30 && SF_PRESENTATION_HZ != 60
#error "SF_PRESENTATION_HZ must be 30 or 60"
#endif

#define SF_UPDATE_MICROSECONDS UINT64_C(33333)
#define SF_RENDER_MICROSECONDS \
  ((UINT64_C(1000000) + SF_PRESENTATION_HZ / 2u) / SF_PRESENTATION_HZ)
#define SF_MAX_FRAME_MICROSECONDS UINT64_C(100000)

static void sf_read_event(
    Twl *window, const TwlEvent *event,
    SfGameInput *input, bool *running) {
  if (event->type == TWL_EVENT_POINTER_MOVE ||
      event->type == TWL_EVENT_POINTER_DOWN ||
      event->type == TWL_EVENT_POINTER_UP) {
    input->pointer_active = true;
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
    if (event->button == 1u) {
      if (event->type == TWL_EVENT_POINTER_DOWN) {
        input->pointer_primary_pressed = true;
        input->pointer_primary_down = true;
      }
      if (event->type == TWL_EVENT_POINTER_UP)
        input->pointer_primary_down = false;
    }
    if (event->button == 3u && event->type == TWL_EVENT_POINTER_DOWN)
      input->pointer_secondary_pressed = true;
  }
  if (event->type == TWL_EVENT_QUIT) {
    *running = false;
    return;
  }
  if (event->type == TWL_EVENT_KEY_DOWN && !event->repeat) {
    if (event->key == TWL_KEY_UP) input->up_pressed = true;
    if (event->key == TWL_KEY_DOWN) input->down_pressed = true;
    if (event->key == TWL_KEY_LEFT) input->left_pressed = true;
    if (event->key == TWL_KEY_RIGHT) input->right_pressed = true;
    if (event->key == TWL_KEY_RETURN) input->confirm_pressed = true;
    if (event->key == TWL_KEY_ESCAPE) input->cancel_pressed = true;
    if (event->key == TWL_KEY_BACKSPACE) input->backspace_pressed = true;
    if (event->key == TWL_KEY_DELETE) input->delete_pressed = true;
    if (event->key == TWL_KEY_R) input->pace_toggle_pressed = true;
    if (event->key == TWL_KEY_SPACE) input->companion_toggle_pressed = true;
    if (event->key == TWL_KEY_I) input->inventory_pressed = true;
    if (event->key == TWL_KEY_S) input->status_pressed = true;
    if (event->key == TWL_KEY_M) input->magic_pressed = true;
    if (event->key == TWL_KEY_X) input->special_items_pressed = true;
    if (event->key >= TWL_KEY_1 && event->key <= TWL_KEY_8) {
      input->belt_pocket_pressed = (int8_t) (event->key - TWL_KEY_1);
      input->belt_pocket_key_pressed = true;
    }
  }
  if (event->type == TWL_EVENT_TEXT && input->text_length < 15u) {
    uint8_t encoded[4];
    uint8_t length = 0u;
    const uint32_t codepoint = event->codepoint;
    if (codepoint >= 0x20u && codepoint <= 0x7eu) {
      encoded[0] = (uint8_t) codepoint;
      length = 1u;
    } else if (codepoint >= 0x80u && codepoint <= 0x7ffu) {
      encoded[0] = (uint8_t) (0xc0u | (codepoint >> 6u));
      encoded[1] = (uint8_t) (0x80u | (codepoint & 0x3fu));
      length = 2u;
    } else if (codepoint <= 0xffffu &&
               (codepoint < 0xd800u || codepoint > 0xdfffu)) {
      encoded[0] = (uint8_t) (0xe0u | (codepoint >> 12u));
      encoded[1] = (uint8_t) (0x80u | ((codepoint >> 6u) & 0x3fu));
      encoded[2] = (uint8_t) (0x80u | (codepoint & 0x3fu));
      length = 3u;
    } else if (codepoint <= 0x10ffffu) {
      encoded[0] = (uint8_t) (0xf0u | (codepoint >> 18u));
      encoded[1] = (uint8_t) (0x80u | ((codepoint >> 12u) & 0x3fu));
      encoded[2] = (uint8_t) (0x80u | ((codepoint >> 6u) & 0x3fu));
      encoded[3] = (uint8_t) (0x80u | (codepoint & 0x3fu));
      length = 4u;
    }
    if (length > 0u && length <= 15u - input->text_length) {
      memcpy(input->text + input->text_length, encoded, length);
      input->text_length = (uint8_t) (input->text_length + length);
      input->text[input->text_length] = '\0';
    }
  }
  if (event->type == TWL_EVENT_CONTROLLER_BUTTON_DOWN) {
    if (event->controller_button == TWL_CONTROLLER_BUTTON_DPAD_UP)
      input->up_pressed = true;
    if (event->controller_button == TWL_CONTROLLER_BUTTON_DPAD_DOWN)
      input->down_pressed = true;
    if (event->controller_button == TWL_CONTROLLER_BUTTON_DPAD_LEFT)
      input->left_pressed = true;
    if (event->controller_button == TWL_CONTROLLER_BUTTON_DPAD_RIGHT)
      input->right_pressed = true;
    if (event->controller_button == TWL_CONTROLLER_BUTTON_SOUTH)
      input->confirm_pressed = true;
    if (event->controller_button == TWL_CONTROLLER_BUTTON_LEFT_SHOULDER)
      input->companion_toggle_pressed = true;
    if (event->controller_button == TWL_CONTROLLER_BUTTON_EAST ||
        event->controller_button == TWL_CONTROLLER_BUTTON_BACK)
      input->cancel_pressed = true;
  }
}

static void sf_clear_input(SfGameInput *input) {
  input->pointer_over_gameplay_ui = false;
  input->world_view_offset_x = 0;
  input->world_pointer_resolved = false;
  input->pointed_actor_id = -1;
  input->pointed_enemy_id = -1;
  input->pointed_scenario_object_id = -1;
  input->pointed_ground_item_id = -1;
  input->conversation_choices_resolved = false;
  input->pointed_conversation_option = -1;
  input->conversation_option_count = 0u;
  input->pointer_primary_pressed = false;
  input->pointer_secondary_pressed = false;
  input->up_pressed = false;
  input->down_pressed = false;
  input->left_pressed = false;
  input->right_pressed = false;
  input->confirm_pressed = false;
  input->cancel_pressed = false;
  input->backspace_pressed = false;
  input->delete_pressed = false;
  input->pace_toggle_pressed = false;
  input->companion_toggle_pressed = false;
  input->inventory_pressed = false;
  input->status_pressed = false;
  input->magic_pressed = false;
  input->special_items_pressed = false;
  input->transport_destination = -1;
  input->transport_selected = false;
  input->belt_pocket_pressed = -1;
  input->belt_pocket_key_pressed = false;
  input->text_length = 0u;
  input->text[0] = '\0';
}

static bool sf_play_pcm(Tal *audio, const SfPcmU8 *source, bool loop) {
  TalPcm pcm;
  TalPlayOptions options;
  TalVoice voice;
  if (!audio || !source || !source->samples || source->frame_count == 0u)
    return false;
  pcm.samples = source->samples;
  pcm.frame_count = source->frame_count;
  pcm.sample_rate = source->sample_rate;
  pcm.channels = 1u;
  pcm.format = TAL_SAMPLE_U8;
  options = tal_play_options_default();
  options.loop = loop;
  return tal_play(audio, &pcm, &options, &voice) == TAL_RESULT_OK;
}

static void sf_play_menu_events(
    Tal *audio, const SfMenuAssets *assets, uint8_t events,
    bool *music_started) {
  if ((events & SF_GAME_SOUND_TITLE_CUE) != 0u)
    (void) sf_play_pcm(
      audio, &assets->sounds[SF_MENU_SOUND_TITLE_CUE], false);
  if ((events & SF_GAME_SOUND_MENU_MOVE) != 0u)
    (void) sf_play_pcm(audio, &assets->sounds[SF_MENU_SOUND_MOVE], false);
  if ((events & SF_GAME_SOUND_TITLE_CONFIRM) != 0u)
    (void) sf_play_pcm(
      audio, &assets->sounds[SF_MENU_SOUND_TITLE_CONFIRM], false);
  if ((events & SF_GAME_SOUND_MENU_CONFIRM) != 0u)
    (void) sf_play_pcm(audio, &assets->sounds[SF_MENU_SOUND_CONFIRM], false);
  if ((events & SF_GAME_SOUND_TITLE_MUSIC) != 0u && !*music_started)
    *music_started = sf_play_pcm(audio, &assets->music, true);
}

static void sf_play_world_events(
    Tal *audio, const SfGameplayAssets *assets,
    const SfWorldState *world) {
  uint8_t index;
  if (!assets || !world) return;
  for (index = 0u; index < world->ground_items.sound_count; ++index)
    (void) sf_play_pcm(audio, sf_ground_item_sound(
      &assets->ground_items, world->ground_items.sound_samples[index]),
      false);
  for (index = 0u; index < world->sounds.count; ++index)
    (void) sf_play_pcm(audio, sf_gameplay_sound(
      &assets->sounds, world->sounds.samples[index]), false);
}

static bool sf_menu_game_mode(SfGameMode mode) {
  return mode == SF_GAME_MODE_CHARACTER_SELECT ||
    mode == SF_GAME_MODE_LOAD_GAME;
}

int sf_application_run(
    void *main_memory, size_t main_memory_size,
    void *video_memory, size_t video_memory_size,
    const char *executable_path, const char *requested_data_root) {
  SfArena main_arena;
  SfArena video_arena;
  SfRenderer renderer;
  const SfFramebuffer *framebuffer;
  SfScreenRuntime *screen_runtime;
  SfMenuAssets *menu_assets;
  const SfTitleAssets *title_assets;
  SfGame *game;
  SfGameConfig game_config;
  SfSaveCatalog save_catalog;
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
  uint64_t next_update;
  uint64_t next_render;
  char data_root[SF_RETAIL_PATH_CAPACITY];
  SfGameMode audio_mode = SF_GAME_MODE_TITLE;
  bool running = true;
  bool menu_music_started = false;
#if defined(SF_ENABLE_PROFILING)
  SfRuntimeProfiler profiler;
#endif

  if (!main_memory || main_memory_size > SF_MAIN_ARENA_BYTES ||
      !video_memory || video_memory_size > SF_VIDEO_MEMORY_LIMIT_BYTES)
    return 1;
  if (!sf_retail_root_find(
        data_root, sizeof(data_root),
        executable_path, requested_data_root)) {
    fprintf(stderr,
      "Could not find the retail ShadowFlare data. Put osf in the original "
      "game folder, keep the development copy in tmp/ShadowFlare, or pass "
      "the game folder as the first argument.\n");
    return 1;
  }
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
  menu_assets = (SfMenuAssets *) sf_arena_push_zero(
    &main_arena, sizeof(*menu_assets), sizeof(void *));
  screen_runtime = (SfScreenRuntime *) sf_arena_push_zero(
    &main_arena, sizeof(*screen_runtime), sizeof(void *));
  decode_scratch = sf_arena_push(
    &main_arena, SF_TITLE_DECODE_SCRATCH_BYTES, 4u);
  if (!game || !menu_assets || !screen_runtime || !decode_scratch ||
      !sf_menu_assets_load(menu_assets, data_root, &main_arena) ||
      !sf_screen_runtime_init(
        screen_runtime, &main_arena, data_root,
        decode_scratch, SF_TITLE_DECODE_SCRATCH_BYTES) ||
      !sf_screen_runtime_load(screen_runtime, game)) {
    fprintf(stderr, "Could not load the retail menu assets from '%s'.\n",
      data_root);
    if (audio) tal_shutdown(audio);
    twl_shutdown(window);
    return 3;
  }
  frame_memory = sf_arena_push(
    &video_arena, SF_FRAMEBUFFER_BYTES, sizeof(uint16_t));
  if (!frame_memory || !sf_renderer_init(
        &renderer, frame_memory, SF_FRAMEBUFFER_BYTES,
        SF_FRAME_WIDTH, SF_FRAME_HEIGHT)) {
    if (audio) tal_shutdown(audio);
    twl_shutdown(window);
    return 4;
  }
  framebuffer = sf_renderer_framebuffer(&renderer);
  title_assets = sf_screen_runtime_title_assets(screen_runtime);
  if (!title_assets) {
    if (audio) tal_shutdown(audio);
    twl_shutdown(window);
    return 4;
  }

  if (!sf_save_catalog_load(data_root, &save_catalog)) {
    if (audio) tal_shutdown(audio);
    twl_shutdown(window);
    return 4;
  }
  memset(&game_config, 0, sizeof(game_config));
  game_config.saved_game_count = save_catalog.count;
  game_config.next_save_available = save_catalog.count < SF_SAVE_SLOT_COUNT;
  {
    unsigned smoke;
    unsigned save;
    for (save = 0u; save < SF_SAVE_SLOT_COUNT; ++save) {
      game_config.saved_game_file_slots[save] = save < save_catalog.count
        ? save_catalog.entries[save].file_slot : UINT8_MAX;
      game_config.saved_game_genders[save] = save < save_catalog.count &&
        save_catalog.entries[save].gender == 1 ? 1u : 0u;
    }
    for (smoke = 0u; smoke < SF_TITLE_SMOKE_COUNT; ++smoke)
      game_config.title_smoke_frame_count[smoke] =
        title_assets->smoke[smoke].animation.frame_count;
  }
  sf_game_init(game, &game_config);
  memset(&input, 0, sizeof(input));
  sf_clear_input(&input);
  surface.pixels = framebuffer->pixels;
  surface.width = framebuffer->width;
  surface.height = framebuffer->height;
  surface.stride_bytes =
    (size_t) framebuffer->stride * sizeof(uint16_t);
  surface.format = TWL_PIXEL_RGB555;
  next_update = twl_time_microseconds(window);
  next_render = next_update;
#if defined(SF_ENABLE_PROFILING)
  sf_profiler_init(&profiler, next_update);
#endif

  while (running && !game->quit_requested) {
    TwlEvent event;
    uint64_t now;
    uint64_t next_wake;
    twl_pump_events(window);
    while (twl_poll_event(window, &event))
      sf_read_event(window, &event, &input, &running);
    if (!running) break;

    now = twl_time_microseconds(window);
    next_wake = next_update < next_render ? next_update : next_render;
    if (now < next_wake) {
      twl_sleep_microseconds(window, next_wake - now);
      now = twl_time_microseconds(window);
    }
    {
      unsigned updates = 0u;
      while (now >= next_update && updates < 3u) {
        sf_screen_runtime_resolve_input(screen_runtime, game, &input);
        sf_game_update(game, &input);
        sf_play_menu_events(
          audio, menu_assets,
          (uint8_t) (game->title.sound_events |
            game->character_create.sound_events |
            game->load_game.sound_events),
          &menu_music_started);
        if (game->mode == SF_GAME_MODE_GAMEPLAY)
          sf_play_world_events(
            audio, sf_screen_runtime_gameplay_assets(screen_runtime),
            &game->world);
        sf_clear_input(&input);
        next_update += SF_UPDATE_MICROSECONDS;
        ++updates;
      }
      if (now > next_update + SF_MAX_FRAME_MICROSECONDS)
        next_update = now + SF_UPDATE_MICROSECONDS;
    }

    if (sf_menu_game_mode(audio_mode) && !sf_menu_game_mode(game->mode)) {
      if (audio) tal_stop_all(audio);
      menu_music_started = false;
    }
    if (!sf_menu_game_mode(audio_mode) && sf_menu_game_mode(game->mode) &&
        !menu_music_started) {
      menu_music_started = sf_play_pcm(audio, &menu_assets->music, true);
    }
    audio_mode = game->mode;

    if ((!screen_runtime->loaded ||
         screen_runtime->loaded_mode != game->mode) &&
        !sf_screen_runtime_load(screen_runtime, game)) {
      fprintf(stderr, "Could not load assets for game mode %d.\n",
        (int) game->mode);
      if (sf_game_recover_saved_game_load_failure(game) &&
          sf_screen_runtime_load(screen_runtime, game)) continue;
      running = false;
      continue;
    }
    if (!sf_screen_runtime_prepare(screen_runtime, game)) {
      fprintf(stderr, "Could not prepare assets for game mode %d.\n",
        (int) game->mode);
      running = false;
      continue;
    }
    if (now > next_render + SF_MAX_FRAME_MICROSECONDS) next_render = now;
    if (now < next_render) {
      if (audio) (void) tal_update(audio);
      continue;
    }
    {
      const uint64_t previous_update =
        next_update >= SF_UPDATE_MICROSECONDS
          ? next_update - SF_UPDATE_MICROSECONDS : 0u;
      uint64_t interpolation_time = now > previous_update
        ? now - previous_update : 0u;
      uint16_t interpolation;
#if defined(SF_ENABLE_PROFILING)
      const uint64_t fill_started = twl_time_microseconds(window);
      uint64_t fill_finished;
      uint64_t present_prepared;
      uint64_t frame_finished;
#endif
      if (interpolation_time > SF_UPDATE_MICROSECONDS)
        interpolation_time = SF_UPDATE_MICROSECONDS;
#if SF_PRESENTATION_HZ == 30
      interpolation = 1000u;
#else
      interpolation = (uint16_t) (
        interpolation_time * 1000u / SF_UPDATE_MICROSECONDS);
#endif
      sf_screen_runtime_draw(
        screen_runtime, &renderer, game, interpolation);
#if defined(SF_ENABLE_PROFILING)
      fill_finished = twl_time_microseconds(window);
#endif
      if (twl_prepare_frame(window, &surface) != TWL_RESULT_OK)
        running = false;
#if defined(SF_ENABLE_PROFILING)
      present_prepared = twl_time_microseconds(window);
#endif
      if (running && twl_display_frame(window) != TWL_RESULT_OK)
        running = false;
#if defined(SF_ENABLE_PROFILING)
      frame_finished = twl_time_microseconds(window);
      if (running) {
        sf_profiler_record_frame(
          &profiler, frame_finished,
          (uint32_t) (fill_finished - fill_started),
          (uint32_t) (present_prepared - fill_finished),
          main_arena.used, video_arena.used);
      }
#endif
    }
    next_render += SF_RENDER_MICROSECONDS;
    if (audio) (void) tal_update(audio);
  }

  if (audio) tal_shutdown(audio);
  twl_shutdown(window);
  return 0;
}
