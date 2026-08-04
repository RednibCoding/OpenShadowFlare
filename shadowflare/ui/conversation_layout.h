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

#ifndef SHADOWFLARE_UI_CONVERSATION_LAYOUT_H
#define SHADOWFLARE_UI_CONVERSATION_LAYOUT_H

#include "assets/gameplay_assets.h"
#include "game/world.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SF_CONVERSATION_TEXT_CAPACITY 1024u
#define SF_CONVERSATION_CHOICE_LIMIT 16u

typedef struct SfConversationChoice {
  uint16_t byte_offset;
  uint16_t byte_length;
  int16_t line;
  int16_t column;
  int16_t length;
} SfConversationChoice;

typedef struct SfConversationLayout {
  char text[SF_CONVERSATION_TEXT_CAPACITY];
  SfConversationChoice choices[SF_CONVERSATION_CHOICE_LIMIT];
  uint16_t text_length;
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
  int16_t cell_width;
  int16_t cell_height;
  uint8_t choice_count;
} SfConversationLayout;

bool sf_conversation_layout_build(
  const SfGameplayAssets *assets, const SfWorldState *world,
  const SfWorldRenderView *view, uint16_t interpolation,
  SfConversationLayout *layout);
int sf_conversation_choice_at(
  const SfConversationLayout *layout, int screen_x, int screen_y);

#endif
