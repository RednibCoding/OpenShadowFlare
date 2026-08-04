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

#include "screens/title_screen.h"

#include "core/memory_budget.h"

#include <string.h>

typedef struct SfPoint {
  int16_t x;
  int16_t y;
} SfPoint;

static const SfPoint sf_smoke_positions[SF_TITLE_SMOKE_COUNT] = {
  {222, 282}, {535, 208}, {122, 472}, {562, 60}, {62, 54},
  {547, 350}, {53, 377}, {566, 294}, {113, 473}, {420, 463}
};

static bool sf_rect_intersects(SfRect first, SfRect second) {
  return first.x < second.x + second.width &&
    second.x < first.x + first.width &&
    first.y < second.y + second.height &&
    second.y < first.y + first.height;
}

static bool sf_smoke_frame(
    const SfTitleAssets *assets, unsigned smoke, int16_t animation_frame,
    uint8_t *pattern, const SfCafFrame **cell) {
  const SfTitleSmokeAsset *asset;
  const SfCafFrame *selected;
  if (animation_frame < 0 || smoke >= SF_TITLE_SMOKE_COUNT) return false;
  asset = &assets->smoke[smoke];
  if (animation_frame >= asset->animation.frame_count) return false;
  selected = &asset->animation.frames[animation_frame];
  if (selected->pattern < 0 || selected->pattern >= asset->images.frame_count)
    return false;
  if (asset->images.frames[selected->pattern].blank) return false;
  *pattern = (uint8_t) selected->pattern;
  *cell = selected;
  return true;
}

static SfRect sf_smoke_bounds(
    const SfTitleAssets *assets, unsigned smoke, int16_t animation_frame) {
  SfRect result = {0, 0, 0, 0};
  const SfCafFrame *cell;
  uint8_t pattern;
  if (sf_smoke_frame(assets, smoke, animation_frame, &pattern, &cell)) {
    const SfNjpCompressedFrame *frame =
      &assets->smoke[smoke].images.frames[pattern];
    (void) cell;
    result.x = (int16_t) (sf_smoke_positions[smoke].x + frame->x);
    result.y = (int16_t) (sf_smoke_positions[smoke].y + frame->y);
    result.width = (int16_t) frame->width;
    result.height = (int16_t) frame->height;
  }
  return result;
}

static void sf_draw_smoke(
    SfTitleScreen *title, SfRenderer *renderer,
    const SfTitleAssets *assets, const SfGame *game,
    unsigned smoke, const SfRect *clip) {
  SfNjpPatternImage image;
  const SfCafFrame *cell;
  uint8_t pattern;
  SfRect bounds;
  const int16_t animation_frame = game->title.smoke_frame[smoke];
  if (!sf_smoke_frame(assets, smoke, animation_frame, &pattern, &cell)) return;
  bounds = sf_smoke_bounds(assets, smoke, animation_frame);
  if (clip && !sf_rect_intersects(bounds, *clip)) return;
  if (!sf_njp_decode_frame(
        &assets->smoke[smoke].images, pattern,
        title->decode_scratch, title->decode_scratch_size, &image)) return;
  sf_renderer_draw_indexed(
    renderer, &image.image,
    sf_smoke_positions[smoke].x + image.x,
    sf_smoke_positions[smoke].y + image.y,
    game->title.scene_brightness, cell->opacity,
    cell->additive ? SF_BLEND_ADDITIVE : SF_BLEND_TRANSLUCENT, clip);
}

static void sf_draw_region(
    SfTitleScreen *title, SfRenderer *renderer,
    const SfTitleAssets *assets, const SfGame *game, const SfRect *clip) {
  unsigned entry;
  unsigned smoke;
  if (clip) sf_renderer_fill_rect(renderer, *clip, 0u);
  else sf_renderer_clear(renderer, 0u);
  sf_renderer_draw_indexed(
    renderer, &assets->artwork.images[0].image,
    assets->artwork.images[0].x, assets->artwork.images[0].y,
    game->title.scene_brightness, 1000u, SF_BLEND_MASKED, clip);
  for (entry = 0u; entry < SF_GAME_TITLE_ENTRY_COUNT; ++entry) {
    if (!game->title.menu_visible[entry]) continue;
    sf_renderer_draw_indexed(
      renderer, &assets->artwork.images[entry + 1u].image,
      assets->artwork.images[entry + 1u].x,
      assets->artwork.images[entry + 1u].y,
      game->title.menu_brightness[entry], 1000u, SF_BLEND_MASKED, clip);
  }
  for (smoke = 0u; smoke < SF_TITLE_SMOKE_COUNT; ++smoke)
    sf_draw_smoke(title, renderer, assets, game, smoke, clip);
}

bool sf_title_screen_init(
    SfTitleScreen *title, void *scratch, size_t scratch_size,
    size_t required_scratch_size) {
  unsigned smoke;
  if (!title || !scratch || required_scratch_size == 0u ||
      required_scratch_size > scratch_size) return false;
  memset(title, 0, sizeof(*title));
  title->decode_scratch = scratch;
  title->decode_scratch_size = scratch_size;
  for (smoke = 0u; smoke < SF_GAME_TITLE_SMOKE_COUNT; ++smoke)
    title->previous_smoke_frame[smoke] = -2;
  return true;
}

void sf_title_screen_draw(
    SfTitleScreen *title, SfRenderer *renderer,
    const SfTitleAssets *assets, const SfGame *game) {
  unsigned entry;
  unsigned smoke;
  uint8_t visible = 0u;
  bool full;
  if (!title || !renderer || !assets || !assets->loaded || !game ||
      game->mode != SF_GAME_MODE_TITLE) return;
  sf_dirty_clear(&title->dirty);
  full = !title->initialized ||
    title->previous_scene_brightness != game->title.scene_brightness;
  for (entry = 0u; entry < SF_GAME_TITLE_ENTRY_COUNT; ++entry) {
    if (game->title.menu_visible[entry]) visible |= (uint8_t) (1u << entry);
    if (title->initialized &&
        (title->previous_menu_brightness[entry] !=
           game->title.menu_brightness[entry] ||
         ((title->previous_menu_visible >> entry) & 1u) !=
           (game->title.menu_visible[entry] ? 1u : 0u))) {
      const SfNjpPatternImage *image = &assets->artwork.images[entry + 1u];
      SfRect rectangle = {
        image->x, image->y,
        (int16_t) image->image.width, (int16_t) image->image.height};
      sf_dirty_add(&title->dirty, rectangle, SF_FRAME_WIDTH, SF_FRAME_HEIGHT);
    }
  }
  for (smoke = 0u; smoke < SF_GAME_TITLE_SMOKE_COUNT; ++smoke) {
    if (title->initialized && title->previous_smoke_frame[smoke] !=
        game->title.smoke_frame[smoke]) {
      sf_dirty_add(&title->dirty,
        sf_smoke_bounds(assets, smoke, title->previous_smoke_frame[smoke]),
        SF_FRAME_WIDTH, SF_FRAME_HEIGHT);
      sf_dirty_add(&title->dirty,
        sf_smoke_bounds(assets, smoke, game->title.smoke_frame[smoke]),
        SF_FRAME_WIDTH, SF_FRAME_HEIGHT);
    }
  }
  if (title->dirty.full) full = true;
  if (full) {
    sf_draw_region(title, renderer, assets, game, NULL);
  } else {
    uint8_t region;
    for (region = 0u; region < title->dirty.count; ++region)
      sf_draw_region(
        title, renderer, assets, game, &title->dirty.rectangles[region]);
  }
  title->previous_scene_brightness = game->title.scene_brightness;
  title->previous_menu_visible = visible;
  for (entry = 0u; entry < SF_GAME_TITLE_ENTRY_COUNT; ++entry)
    title->previous_menu_brightness[entry] =
      game->title.menu_brightness[entry];
  for (smoke = 0u; smoke < SF_GAME_TITLE_SMOKE_COUNT; ++smoke)
    title->previous_smoke_frame[smoke] = game->title.smoke_frame[smoke];
  title->initialized = true;
}
