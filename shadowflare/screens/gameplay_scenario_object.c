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

#include "screens/gameplay_scenario_object.h"

#include "core/coordinates.h"
#include "core/memory_budget.h"

static uint16_t sf_scenario_object_strength(int32_t value) {
  return value < 0 ? 0u : value > UINT16_MAX ? UINT16_MAX : (uint16_t) value;
}

static uint16_t sf_scenario_object_opacity(int32_t value) {
  return value < 0 ? 0u : value > 1000 ? 1000u : (uint16_t) value;
}

static uint16_t sf_scenario_object_frame(
    const SfScenarioObject *object,
    const SfCafSelectedAnimation *animation) {
  if (!animation || animation->frame_count == 0u) return 0u;
  if (animation->looping)
    return (uint16_t) (object->animation_frame % animation->frame_count);
  return object->animation_frame >= animation->frame_count
    ? (uint16_t) (animation->frame_count - 1u)
    : (uint16_t) object->animation_frame;
}

static bool sf_scenario_object_image_visible(
    const SfNjpPatternImage *image, SfScreenPoint anchor) {
  const int32_t left = anchor.x + image->x;
  const int32_t top = anchor.y + image->y;
  return left < (int32_t) SF_FRAME_WIDTH &&
    top < (int32_t) SF_FRAME_HEIGHT &&
    left + image->image.width > 0 && top + image->image.height > 0;
}

static SfScreenPoint sf_scenario_object_anchor(
    const SfScenarioObject *object, const SfWorldRenderView *view) {
  SfScreenPoint anchor = sf_world_to_screen(object->position);
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y + object->display_height;
  return anchor;
}

bool sf_gameplay_scenario_object_visible(
    const SfScenarioObjectAssets *assets, const SfScenarioObject *object,
    const SfWorldRenderView *view, bool shadow) {
  const SfScenarioObjectVisual *visual;
  SfScreenPoint anchor;
  if (!assets || !object || !view ||
      !sf_scenario_object_draw_requested(object)) return false;
  visual = sf_scenario_object_visual(assets, object->resource_id);
  if (!visual) return false;
  anchor = sf_scenario_object_anchor(object, view);
  if (object->visual_mode != 0u) {
    const SfNjpDecodedResource *resource = shadow
      ? &visual->static_shadows : &visual->static_artwork;
    const SfNjpDecodedPattern *pattern =
      object->static_pattern >= 0 && object->static_pattern <= UINT8_MAX
        ? sf_njp_decoded_pattern(
            resource, (uint8_t) object->static_pattern) : NULL;
    if (shadow) anchor.y += object->display_height;
    return pattern && pattern->bounds.valid &&
      anchor.x + pattern->bounds.x < (int32_t) SF_FRAME_WIDTH &&
      anchor.y + pattern->bounds.y < (int32_t) SF_FRAME_HEIGHT &&
      anchor.x + pattern->bounds.x + pattern->bounds.width > 0 &&
      anchor.y + pattern->bounds.y + pattern->bounds.height > 0;
  }
  {
    const SfCafSelectedAnimation *animation = sf_scenario_object_animation(
      visual, object->animation_chart, object->direction);
    const uint16_t frame = sf_scenario_object_frame(object, animation);
    uint8_t part;
    if (!animation || animation->frame_count == 0u) return false;
    for (part = 0u; part < animation->part_count; ++part) {
      const uint8_t source_part = animation->parts[part].source_index;
      const SfCafCell *cell = &animation->parts[part].cells[frame];
      const SfNjpSparsePattern *pattern;
      if (source_part >= SF_MCT_PERSON_PART_LIMIT ||
          (object->enabled_parts & (uint8_t) (1u << source_part)) == 0u ||
          cell->pattern < 0 || (shadow != ((cell->status & 8) != 0)))
        continue;
      pattern = sf_njp_sparse_pattern(
        &visual->animation_artwork, cell->pattern);
      if (pattern && sf_scenario_object_image_visible(&pattern->image, anchor))
        return true;
    }
  }
  return false;
}

static void sf_scenario_object_draw_static(
    SfRenderer *renderer, const SfScenarioObjectVisual *visual,
    const SfScenarioObject *object, SfScreenPoint anchor,
    bool shadow, bool hovered, const SfRect *clip) {
  const SfNjpDecodedResource *resource = shadow
    ? &visual->static_shadows : &visual->static_artwork;
  const SfNjpDecodedPattern *pattern =
    object->static_pattern >= 0 && object->static_pattern <= UINT8_MAX
      ? sf_njp_decoded_pattern(
          resource, (uint8_t) object->static_pattern) : NULL;
  uint16_t opacity;
  uint16_t red;
  uint16_t green;
  uint16_t blue;
  SfBlendMode blend;
  uint8_t reference;
  if (!pattern || pattern->palette >= resource->palette_count) return;
  if (shadow) {
    anchor.y += object->display_height;
    opacity = 500u;
    red = green = blue = 1000u;
    blend = SF_BLEND_TRANSLUCENT;
  } else {
    opacity = sf_scenario_object_opacity(object->draw_strength);
    red = sf_scenario_object_strength(
      object->red_strength + (hovered ? 300 : 0));
    green = sf_scenario_object_strength(
      object->green_strength + (hovered ? 300 : 0));
    blue = sf_scenario_object_strength(
      object->blue_strength + (hovered ? 300 : 0));
    blend = (object->display_status & 0x10) != 0
      ? SF_BLEND_ADDITIVE : SF_BLEND_MASKED;
  }
  for (reference = 0u; reference < pattern->reference_count; ++reference) {
    const SfNjpDecodedReference *item =
      &resource->references[pattern->first_reference + reference];
    SfIndexedImage image;
    if (item->part >= resource->part_count) continue;
    image = resource->parts[item->part].image;
    image.palette = resource->palettes[pattern->palette];
    sf_renderer_draw_indexed_tinted(
      renderer, &image, anchor.x + item->x, anchor.y + item->y,
      red, green, blue, opacity, blend, clip);
  }
}

static void sf_scenario_object_draw_animated(
    SfRenderer *renderer, const SfScenarioObjectVisual *visual,
    const SfScenarioObject *object, SfScreenPoint anchor,
    bool shadow, bool hovered, const SfRect *clip) {
  const SfCafSelectedAnimation *animation = sf_scenario_object_animation(
    visual, object->animation_chart, object->direction);
  const uint16_t frame = sf_scenario_object_frame(object, animation);
  uint8_t priority;
  if (!animation || animation->frame_count == 0u) return;
  for (priority = animation->priority_count; priority > 0u; --priority) {
    uint8_t part;
    for (part = 0u; part < animation->part_count; ++part) {
      const uint8_t source_part = animation->parts[part].source_index;
      const SfCafCell *cell = &animation->parts[part].cells[frame];
      const SfNjpSparsePattern *pattern;
      uint16_t opacity;
      uint16_t red;
      uint16_t green;
      uint16_t blue;
      SfBlendMode blend;
      if (source_part >= SF_MCT_PERSON_PART_LIMIT ||
          (object->enabled_parts & (uint8_t) (1u << source_part)) == 0u ||
          cell->priority != (int16_t) (priority - 1u) || cell->pattern < 0 ||
          (shadow != ((cell->status & 8) != 0))) continue;
      pattern = sf_njp_sparse_pattern(
        &visual->animation_artwork, cell->pattern);
      if (!pattern) continue;
      if (shadow) {
        opacity = 500u;
        red = green = blue = 1000u;
        blend = SF_BLEND_TRANSLUCENT;
      } else {
        opacity = sf_scenario_object_opacity(
          cell->transparency *
            sf_scenario_object_opacity(object->draw_strength) / 1000);
        red = sf_scenario_object_strength(
          sf_scenario_object_part_red(object, source_part, hovered));
        green = sf_scenario_object_strength(
          sf_scenario_object_part_green(object, source_part, hovered));
        blue = sf_scenario_object_strength(
          sf_scenario_object_part_blue(object, source_part, hovered));
        blend = ((cell->status | object->display_status) & 0x10) != 0
          ? SF_BLEND_ADDITIVE : SF_BLEND_MASKED;
      }
      sf_renderer_draw_indexed_tinted(
        renderer, &pattern->image.image,
        anchor.x + pattern->image.x, anchor.y + pattern->image.y,
        red, green, blue, opacity, blend, clip);
    }
  }
}

void sf_gameplay_scenario_object_draw(
    SfRenderer *renderer, const SfScenarioObjectAssets *assets,
    const SfScenarioObject *object, const SfWorldRenderView *view,
    bool shadow, bool hovered, const SfRect *clip) {
  const SfScenarioObjectVisual *visual;
  const SfScreenPoint anchor = object && view
    ? sf_scenario_object_anchor(object, view) : (SfScreenPoint) {0, 0};
  if (!renderer || !assets || !object || !view ||
      !sf_scenario_object_draw_requested(object)) return;
  visual = sf_scenario_object_visual(assets, object->resource_id);
  if (!visual) return;
  if (object->visual_mode != 0u)
    sf_scenario_object_draw_static(
      renderer, visual, object, anchor, shadow, hovered, clip);
  else
    sf_scenario_object_draw_animated(
      renderer, visual, object, anchor, shadow, hovered, clip);
}

static uint8_t sf_scenario_object_pixel(
    const SfIndexedImage *image, uint16_t x, uint16_t y) {
  const uint16_t source_y = image->bottom_up
    ? (uint16_t) (image->height - y - 1u) : y;
  const uint8_t *row = image->pixels + (size_t) source_y * image->stride;
  if (image->bits_per_pixel == 8u) return row[x];
  if (image->bits_per_pixel == 4u) {
    const uint8_t packed = row[x >> 1u];
    return (uint8_t) ((packed >> ((x & 1u) ? 0u : 4u)) & 15u);
  }
  return (uint8_t) ((row[x >> 3u] >> (7u - (x & 7u))) & 1u);
}

static bool sf_scenario_object_image_hit(
    const SfIndexedImage *image, int left, int top,
    int pointer_x, int pointer_y, int half_size, bool *exact) {
  int first_x = pointer_x - half_size;
  int first_y = pointer_y - half_size;
  int last_x = pointer_x + half_size + 1;
  int last_y = pointer_y + half_size + 1;
  int y;
  bool hit = false;
  if (first_x < left) first_x = left;
  if (first_y < top) first_y = top;
  if (last_x > left + image->width) last_x = left + image->width;
  if (last_y > top + image->height) last_y = top + image->height;
  for (y = first_y; y < last_y; ++y) {
    int x;
    for (x = first_x; x < last_x; ++x) {
      if (sf_scenario_object_pixel(
            image, (uint16_t) (x - left),
            (uint16_t) (y - top)) == 0u) continue;
      hit = true;
      if (exact && x == pointer_x && y == pointer_y) *exact = true;
    }
  }
  return hit;
}

bool sf_gameplay_scenario_object_pixel_hit(
    const SfScenarioObjectAssets *assets, const SfScenarioObject *object,
    const SfWorldRenderView *view, int pointer_x, int pointer_y,
    int half_size, bool *exact) {
  const SfScenarioObjectVisual *visual;
  SfScreenPoint anchor;
  bool hit = false;
  if (exact) *exact = false;
  if (!assets || !object || !view ||
      !sf_scenario_object_draw_requested(object)) return false;
  visual = sf_scenario_object_visual(assets, object->resource_id);
  if (!visual) return false;
  anchor = sf_scenario_object_anchor(object, view);
  if (object->visual_mode != 0u) {
    const SfNjpDecodedPattern *pattern =
      object->static_pattern >= 0 && object->static_pattern <= UINT8_MAX
        ? sf_njp_decoded_pattern(
            &visual->static_artwork, (uint8_t) object->static_pattern) : NULL;
    uint8_t reference;
    if (!pattern) return false;
    for (reference = 0u; reference < pattern->reference_count; ++reference) {
      const SfNjpDecodedReference *item = &visual->static_artwork.references[
        pattern->first_reference + reference];
      if (item->part < visual->static_artwork.part_count &&
          sf_scenario_object_image_hit(
            &visual->static_artwork.parts[item->part].image,
            anchor.x + item->x, anchor.y + item->y,
            pointer_x, pointer_y, half_size, exact)) hit = true;
    }
    return hit;
  }
  {
    const SfCafSelectedAnimation *animation = sf_scenario_object_animation(
      visual, object->animation_chart, object->direction);
    const uint16_t frame = sf_scenario_object_frame(object, animation);
    uint8_t part;
    if (!animation || animation->frame_count == 0u) return false;
    for (part = 0u; part < animation->part_count; ++part) {
      const uint8_t source_part = animation->parts[part].source_index;
      const SfCafCell *cell = &animation->parts[part].cells[frame];
      const SfNjpSparsePattern *pattern;
      if (source_part >= SF_MCT_PERSON_PART_LIMIT ||
          (object->enabled_parts & (uint8_t) (1u << source_part)) == 0u ||
          cell->pattern < 0 || (cell->status & 8) != 0) continue;
      pattern = sf_njp_sparse_pattern(
        &visual->animation_artwork, cell->pattern);
      if (pattern && sf_scenario_object_image_hit(
            &pattern->image.image,
            anchor.x + pattern->image.x, anchor.y + pattern->image.y,
            pointer_x, pointer_y, half_size, exact)) hit = true;
    }
  }
  return hit;
}
