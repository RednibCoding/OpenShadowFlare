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

#include "ui/conversation_bubble.h"

#include "ui/conversation_layout.h"

static void sf_conversation_horizontal_edges(
    SfRenderer *renderer, int x, int y, int width, int height) {
  const uint16_t gray160 = sf_rgb555(20u, 20u, 20u);
  const uint16_t gray224 = sf_rgb555(28u, 28u, 28u);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x + 9), (int16_t) y, (int16_t) (width - 18), 2}, 0u);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x + 9), (int16_t) (y + 2),
    (int16_t) (width - 18), 1}, gray160);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x + 9), (int16_t) (y + 3),
    (int16_t) (width - 18), 1}, gray224);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x + 9), (int16_t) (y + height - 2),
    (int16_t) (width - 18), 2}, 0u);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x + 9), (int16_t) (y + height - 3),
    (int16_t) (width - 18), 1}, gray160);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x + 9), (int16_t) (y + height - 4),
    (int16_t) (width - 18), 1}, gray224);
}

static void sf_conversation_vertical_edges(
    SfRenderer *renderer, int x, int y, int width, int height) {
  const uint16_t gray160 = sf_rgb555(20u, 20u, 20u);
  const uint16_t gray224 = sf_rgb555(28u, 28u, 28u);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) x, (int16_t) (y + 9), 2, (int16_t) (height - 18)}, 0u);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x + 2), (int16_t) (y + 9),
    1, (int16_t) (height - 18)}, gray160);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x + 3), (int16_t) (y + 9),
    1, (int16_t) (height - 18)}, gray224);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x + width - 2), (int16_t) (y + 9),
    2, (int16_t) (height - 18)}, 0u);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x + width - 3), (int16_t) (y + 9),
    1, (int16_t) (height - 18)}, gray160);
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (x + width - 4), (int16_t) (y + 9),
    1, (int16_t) (height - 18)}, gray224);
}

static void sf_conversation_pattern(
    SfRenderer *renderer, const SfNjpPatternImage *pattern, int x, int y) {
  sf_renderer_draw_indexed(
    renderer, &pattern->image, x + pattern->x, y + pattern->y,
    1000u, 1000u, SF_BLEND_MASKED, NULL);
}

static void sf_conversation_choices(
    SfRenderer *renderer, const SfIndexedImage *font,
    SfConversationLayout *layout, int selected) {
  uint8_t index;
  for (index = 0u; index < layout->choice_count; ++index) {
    const SfConversationChoice *choice = &layout->choices[index];
    const uint16_t end = (uint16_t) (choice->byte_offset + choice->byte_length);
    const char saved = layout->text[end];
    const uint16_t color = index == selected
      ? sf_rgb555(31u, 0u, 0u) : sf_rgb555(12u, 12u, 12u);
    layout->text[end] = '\0';
    sf_renderer_draw_text(
      renderer, font, &layout->text[choice->byte_offset],
      layout->x + 4 + choice->column * layout->cell_width,
      layout->y + 4 + choice->line * layout->cell_height,
      color, 1000u);
    layout->text[end] = saved;
  }
}

void sf_conversation_bubble_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfWorldState *world, const SfWorldRenderView *view,
    uint16_t interpolation) {
  SfConversationLayout layout;
  const SfIndexedImage *font;
  int frame_x;
  int frame_y;
  int frame_width;
  int frame_height;
  if (!renderer || !assets || !world || !view ||
      assets->speech_frame.image_count < 5u ||
      !sf_conversation_layout_build(
        assets, world, view, interpolation, &layout)) return;
  font = &assets->font.images[0].image;
  frame_x = layout.x - 9;
  frame_y = layout.y - 9;
  frame_width = layout.width + 18;
  frame_height = layout.height + 18;
  sf_renderer_fill_rect(renderer, (SfRect) {
    (int16_t) (frame_x + 4), (int16_t) (frame_y + 4),
    (int16_t) (frame_width - 8), (int16_t) (frame_height - 8)},
    sf_rgb555(31u, 31u, 31u));
  sf_conversation_horizontal_edges(
    renderer, frame_x, frame_y, frame_width, frame_height);
  sf_conversation_vertical_edges(
    renderer, frame_x, frame_y, frame_width, frame_height);
  sf_conversation_pattern(
    renderer, &assets->speech_frame.images[0], frame_x, frame_y);
  sf_conversation_pattern(
    renderer, &assets->speech_frame.images[2],
    frame_x + frame_width - 9, frame_y);
  sf_conversation_pattern(
    renderer, &assets->speech_frame.images[1],
    frame_x, frame_y + frame_height - 9);
  sf_conversation_pattern(
    renderer, &assets->speech_frame.images[3],
    frame_x + frame_width - 9, frame_y + frame_height - 9);
  sf_conversation_pattern(
    renderer, &assets->speech_frame.images[4],
    layout.x + layout.width / 2 - 5,
    layout.y + layout.height + 5);
  sf_renderer_draw_text(
    renderer, font, layout.text,
    layout.x + 4, layout.y + 4, 0u, 1000u);
  sf_conversation_choices(
    renderer, font, &layout,
    world->actor_script_state.selected_option);
}
