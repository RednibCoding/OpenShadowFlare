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

#include "ui/scenario_label.h"

#include "core/coordinates.h"

#define SF_SCENARIO_LABEL_CELL_WIDTH 6
#define SF_SCENARIO_LABEL_CELL_HEIGHT 12
#define SF_SCENARIO_LABEL_MARGIN 3

static bool sf_scenario_label_shift_jis_lead(uint8_t value) {
  return (value >= 0x80u && value <= 0x9fu) || value >= 0xe0u;
}

static void sf_scenario_label_measure(
    const char *text, int *width, int *height) {
  int columns = 0;
  int maximum = 0;
  int lines = 0;
  bool content_after_break = false;
  uint16_t index;
  for (index = 0u; text && text[index] != '\0'; ++index) {
    const uint8_t byte = (uint8_t) text[index];
    if (byte == '\r') continue;
    if (byte == '\n') {
      if (columns > maximum) maximum = columns;
      columns = 0;
      ++lines;
      content_after_break = false;
    } else if (sf_scenario_label_shift_jis_lead(byte) &&
               text[index + 1u] != '\0') {
      columns += 2;
      content_after_break = true;
      ++index;
    } else {
      ++columns;
      content_after_break = true;
    }
  }
  if (columns > maximum) maximum = columns;
  if (content_after_break || lines == 0) ++lines;
  *width = maximum * SF_SCENARIO_LABEL_CELL_WIDTH;
  *height = lines * SF_SCENARIO_LABEL_CELL_HEIGHT;
}

static bool sf_scenario_label_intersects(
    SfRect first, const SfRect *second) {
  return !second || (first.x < second->x + second->width &&
    first.x + first.width > second->x &&
    first.y < second->y + second->height &&
    first.y + first.height > second->y);
}

bool sf_scenario_label_bounds(
    const SfGameplayAssets *assets, const SfWorldState *world,
    const SfWorldRenderView *view, uint8_t index, SfRect *bounds) {
  const SfScenarioLabel *label;
  const SfScsMessage *message;
  const char *text;
  SfScreenPoint anchor;
  int width;
  int height;
  if (!assets || !world || !world->script || !view || !bounds ||
      index >= world->scenario_labels.count) return false;
  label = &world->scenario_labels.labels[index];
  message = sf_scs_message(world->script, label->message_id);
  text = sf_scs_message_text(world->script, message);
  if (!text || !text[0]) return false;
  sf_scenario_label_measure(text, &width, &height);
  if (width <= 0 || height <= 0) return false;
  anchor = sf_world_to_screen(label->anchor);
  bounds->x = (int16_t) (
    anchor.x - view->camera_x + label->offset_x - width / 2 -
      SF_SCENARIO_LABEL_MARGIN);
  bounds->y = (int16_t) (
    anchor.y - view->camera_y + label->offset_y - height -
      SF_SCENARIO_LABEL_MARGIN);
  bounds->width = (int16_t) (width + SF_SCENARIO_LABEL_MARGIN * 2);
  bounds->height = (int16_t) (height + SF_SCENARIO_LABEL_MARGIN * 2);
  return true;
}

static uint8_t sf_scenario_label_channel(int32_t value) {
  return value <= 0 ? 0u : value >= 255 ? 31u : (uint8_t) (value >> 3u);
}

static uint16_t sf_scenario_label_opacity(int32_t value) {
  return value <= 0 ? 0u : value >= 1000 ? 1000u : (uint16_t) value;
}

void sf_scenario_labels_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfWorldState *world, const SfWorldRenderView *view,
    const SfRect *clip) {
  const SfIndexedImage *font;
  uint8_t index;
  if (!renderer || !assets || !world || !view ||
      assets->font.image_count == 0u) return;
  font = &assets->font.images[0].image;
  for (index = 0u; index < world->scenario_labels.count; ++index) {
    const SfScenarioLabel *label = &world->scenario_labels.labels[index];
    const SfScsMessage *message = sf_scs_message(
      world->script, label->message_id);
    const char *text = sf_scs_message_text(world->script, message);
    SfRect bounds;
    uint16_t color;
    if (!text || !sf_scenario_label_bounds(
          assets, world, view, index, &bounds) ||
        !sf_scenario_label_intersects(bounds, clip)) continue;
    color = sf_rgb555(
      sf_scenario_label_channel(label->red),
      sf_scenario_label_channel(label->green),
      sf_scenario_label_channel(label->blue));
    sf_renderer_fill_rect_blended(
      renderer, bounds, 0u,
      sf_scenario_label_opacity(label->background_opacity));
    sf_renderer_draw_text(
      renderer, font, text, bounds.x + SF_SCENARIO_LABEL_MARGIN + 1,
      bounds.y + SF_SCENARIO_LABEL_MARGIN + 1, 0u, 1000u);
    sf_renderer_draw_text(
      renderer, font, text, bounds.x + SF_SCENARIO_LABEL_MARGIN,
      bounds.y + SF_SCENARIO_LABEL_MARGIN, color, 1000u);
  }
}
