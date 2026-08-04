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

#include "ui/conversation_input.h"

#include "ui/conversation_layout.h"

void sf_conversation_input_resolve(
    const SfGameplayAssets *assets, const SfWorldState *world,
    SfGameInput *input) {
  SfConversationLayout layout;
  SfWorldRenderView view;
  if (!input) return;
  input->conversation_choices_resolved = false;
  input->pointed_conversation_option = -1;
  input->conversation_option_count = 0u;
  if (!assets || !world ||
      !world->actor_script_state.message_selection_pending) return;
  sf_world_render_view(world, 1000u, &view);
  if (!sf_conversation_layout_build(
        assets, world, &view, 1000u, &layout)) return;
  input->conversation_choices_resolved = true;
  input->conversation_option_count = layout.choice_count;
  if (input->pointer_active)
    input->pointed_conversation_option = (int8_t) sf_conversation_choice_at(
      &layout, input->pointer_x, input->pointer_y);
}
