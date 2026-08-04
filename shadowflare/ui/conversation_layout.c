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

#include "ui/conversation_layout.h"

#include "core/coordinates.h"

#include <string.h>

static const SfScenarioActor *sf_conversation_actor(
    const SfWorldState *world) {
  uint8_t index;
  const int32_t id = world->actor_script_state.message_actor_id;
  for (index = 0u; index < world->actors.count; ++index)
    if (world->actors.actors[index].id == id)
      return &world->actors.actors[index];
  return NULL;
}

static const SfMctPerson *sf_conversation_person(
    const SfGameplayAssets *assets, int32_t actor_id) {
  uint8_t index;
  for (index = 0u; index < assets->scenario.people_count; ++index)
    if (assets->scenario.people[index].id == actor_id)
      return &assets->scenario.people[index];
  return NULL;
}

static bool sf_shift_jis_lead(uint8_t value) {
  return (value >= 0x80u && value <= 0x9fu) || value >= 0xe0u;
}

static bool sf_conversation_append(
    SfConversationLayout *layout, char value) {
  if ((size_t) layout->text_length + 1u >= sizeof(layout->text)) return false;
  layout->text[layout->text_length++] = value;
  layout->text[layout->text_length] = '\0';
  return true;
}

static bool sf_conversation_text(
    SfConversationLayout *layout, const char *source, bool choices_enabled,
    int *maximum_columns, int *line_count) {
  int line = 0;
  int column = 0;
  int maximum = 0;
  int choice_line = 0;
  int choice_column = 0;
  uint16_t choice_offset = 0u;
  bool inside_choice = false;
  size_t index;
  for (index = 0u; source[index] != '\0'; ++index) {
    const uint8_t byte = (uint8_t) source[index];
    if (byte == '\r') continue;
    if (choices_enabled && byte == '~') {
      if (inside_choice) {
        SfConversationChoice *choice;
        if (layout->choice_count >= SF_CONVERSATION_CHOICE_LIMIT) return false;
        choice = &layout->choices[layout->choice_count++];
        choice->byte_offset = choice_offset;
        choice->byte_length = (uint16_t) (layout->text_length - choice_offset);
        choice->line = (int16_t) choice_line;
        choice->column = (int16_t) choice_column;
        choice->length = (int16_t) (column - choice_column);
      } else {
        choice_line = line;
        choice_column = column;
        choice_offset = layout->text_length;
      }
      inside_choice = !inside_choice;
      continue;
    }
    if (byte == '\n') {
      if (!sf_conversation_append(layout, '\n')) return false;
      if (column > maximum) maximum = column;
      ++line;
      column = 0;
      continue;
    }
    if (byte == 0x81u && (uint8_t) source[index + 1u] == 0x40u) {
      if (!sf_conversation_append(layout, ' ') ||
          !sf_conversation_append(layout, ' ')) return false;
      ++index;
      column += 2;
      continue;
    }
    if (!sf_conversation_append(layout, (char) byte)) return false;
    if (sf_shift_jis_lead(byte) && source[index + 1u] != '\0') {
      if (!sf_conversation_append(layout, source[++index])) return false;
      column += 2;
    } else {
      ++column;
    }
  }
  if (inside_choice) return false;
  if (column > maximum) maximum = column;
  *maximum_columns = maximum;
  *line_count = line + (column > 0 || line == 0 ? 1 : 0);
  return true;
}

bool sf_conversation_layout_build(
    const SfGameplayAssets *assets, const SfWorldState *world,
    const SfWorldRenderView *view, uint16_t interpolation,
    SfConversationLayout *layout) {
  const SfScenarioActor *actor;
  const SfMctPerson *person;
  const SfIndexedImage *font;
  const char *text;
  SfScreenPoint anchor;
  int columns;
  int lines;
  if (!assets || !world || !view || !layout ||
      !world->actor_script_state.message_active ||
      assets->font.image_count == 0u) return false;
  actor = sf_conversation_actor(world);
  if (!actor) return false;
  person = sf_conversation_person(assets, actor->id);
  text = sf_scenario_actor_script_message_text(
    &world->actor_script_state, world->script);
  if (!person || !text) return false;
  memset(layout, 0, sizeof(*layout));
  font = &assets->font.images[0].image;
  layout->cell_width = (int16_t) (font->width / 16u);
  layout->cell_height = (int16_t) (font->height / 16u);
  if (layout->cell_width <= 0 || layout->cell_height <= 0 ||
      !sf_conversation_text(
        layout, text,
        world->actor_script_state.message_selection_pending,
        &columns, &lines)) return false;
  layout->width = (int16_t) (columns * layout->cell_width + 8);
  layout->height = (int16_t) (lines * layout->cell_height + 8);
  anchor = sf_world_to_screen(
    sf_scenario_actor_render_position(actor, interpolation));
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y + person->label_height;
  layout->x = (int16_t) (anchor.x + 12 - layout->width / 2);
  layout->y = (int16_t) (anchor.y - 16 - layout->height);
  return true;
}

int sf_conversation_choice_at(
    const SfConversationLayout *layout, int screen_x, int screen_y) {
  uint8_t index;
  if (!layout) return -1;
  for (index = 0u; index < layout->choice_count; ++index) {
    const SfConversationChoice *choice = &layout->choices[index];
    const int left = layout->x + 4 + choice->column * layout->cell_width;
    const int top = layout->y + 4 + choice->line * layout->cell_height;
    if (screen_x >= left &&
        screen_x < left + choice->length * layout->cell_width &&
        screen_y >= top && screen_y < top + layout->cell_height)
      return index;
  }
  return -1;
}
