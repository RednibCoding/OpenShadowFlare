# Copyright (C) 2026 Michael Binder and contributors
#
# This file is part of OpenShadowFlare.
#
# OpenShadowFlare is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenShadowFlare is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# details.
#
# You should have received a copy of the GNU General Public License along with
# OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.

function(sf_configure_c99_target target)
  target_compile_features(${target} PUBLIC c_std_99)
  target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
  set_target_properties(
    ${target}
    PROPERTIES
      C_EXTENSIONS OFF
      C_STANDARD 99
      C_STANDARD_REQUIRED ON
  )
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

add_library(
  shadowflare_core STATIC
  assets/character_create_assets.c
  assets/companion_assets.c
  assets/ground_item_assets.c
  assets/ground_item_requests.c
  assets/inventory_item_assets.c
  assets/gameplay_assets.c
  assets/gameplay_sound_assets.c
  assets/load_game_assets.c
  assets/menu_assets.c
  assets/player_assets.c
  assets/retail_paths.c
  assets/scenario_actor_assets.c
  assets/scenario_object_assets.c
  assets/title_assets.c
  core/arena.c
  core/coordinates.c
  core/retail_random.c
  data/caf.c
  data/companion_parameters.c
  data/bmp.c
  data/gnd.c
  data/item.c
  data/item_decode.c
  data/item_ground.c
  data/item_records.c
  data/mct.c
  data/mct_reader.c
  data/mct_sections.c
  data/njp.c
  data/njp_stream.c
  data/njp_stream_reader.c
  data/njp_sparse.c
  data/obl.c
  data/pattern_list.c
  data/player_parameters.c
  data/rclib.c
  data/save.c
  data/save_game.c
  data/save_game_progress.c
  data/save_payload.c
  data/save_player.c
  data/save_player_items.c
  data/scs.c
  data/spell_parameters.c
  data/table.c
  data/transport.c
  data/voc.c
  game/character_create.c
  game/companion.c
  game/belt.c
  game/collision.c
  game/equipment.c
  game/game.c
  game/gameplay_service.c
  game/ground_item.c
  game/inventory.c
  game/item_condition.c
  game/load_game.c
  game/movement.c
  game/player.c
  game/player_companion.c
  game/player_magic.c
  game/player_elements.c
  game/player_item.c
  game/player_loadout.c
  game/player_save.c
  game/player_profile.c
  game/route.c
  game/route_tables.c
  game/scenario_actor.c
  game/scenario_effect.c
  game/scenario_label.c
  game/scenario_object.c
  game/sound_event.c
  game/special_items.c
  game/title.c
  game/world.c
  game/world_conversation.c
  game/world_inventory.c
  game/world_interaction.c
  game/world_magic.c
  game/world_save.c
  game/world_script.c
  game/world_transport.c
  interpreter/scenario_actor_script.c
  interpreter/scenario_script_commands.c
  interpreter/scenario_script_values.c
  render/depth.c
  render/dirty.c
  render/framebuffer.c
  render/renderer.c
  screens/character_create_screen.c
  screens/gameplay_actor.c
  screens/gameplay_companion.c
  screens/gameplay_ground_item.c
  screens/gameplay_scene.c
  screens/gameplay_screen.c
  screens/gameplay_object_visual.c
  screens/gameplay_player.c
  screens/gameplay_scenario_object.c
  screens/load_game_screen.c
  screens/title_screen.c
  ui/actor_nameplate.c
  ui/conversation_bubble.c
  ui/conversation_input.c
  ui/conversation_layout.c
  ui/gameplay_character_panel.c
  ui/gameplay_companion_hud.c
  ui/gameplay_companion_hud_input.c
  ui/gameplay_belt.c
  ui/gameplay_equipment_layout.c
  ui/gameplay_hud.c
  ui/gameplay_hud_input.c
  ui/gameplay_inventory.c
  ui/gameplay_inventory_input.c
  ui/gameplay_magic_input.c
  ui/gameplay_magic_layout.c
  ui/gameplay_magic.c
  ui/gameplay_panels_input.c
  ui/gameplay_item_information.c
  ui/gameplay_item_icon.c
  ui/gameplay_item_condition.c
  ui/gameplay_status_pattern.c
  ui/gameplay_status.c
  ui/gameplay_special_items.c
  ui/gameplay_service_controller.c
  ui/gameplay_transport.c
  ui/ground_item_nameplate.c
  ui/scenario_object_nameplate.c
  ui/scenario_label.c
  ui/world_pointer.c
  ui/world_pointer_overlay.c
)
add_library(ShadowFlare::Core ALIAS shadowflare_core)
sf_configure_c99_target(shadowflare_core)

add_executable(
  osf
  main.c
  runtime/application.c
  runtime/screen_runtime.c
)
add_executable(ShadowFlare::Game ALIAS osf)
sf_configure_c99_target(osf)
set(
  OPENSHADOWFLARE_C99_PRESENTATION_HZ
  "60"
  CACHE STRING
  "Presentation rate for the C99 game (30 or 60 Hz)"
)
set_property(
  CACHE OPENSHADOWFLARE_C99_PRESENTATION_HZ PROPERTY STRINGS 30 60)
if(NOT OPENSHADOWFLARE_C99_PRESENTATION_HZ STREQUAL "30" AND
   NOT OPENSHADOWFLARE_C99_PRESENTATION_HZ STREQUAL "60")
  message(FATAL_ERROR "C99 presentation rate must be 30 or 60 Hz")
endif()
target_compile_definitions(
  osf PRIVATE SF_PRESENTATION_HZ=${OPENSHADOWFLARE_C99_PRESENTATION_HZ})
target_link_libraries(
  osf
  PRIVATE
    ShadowFlare::Core
    Tal::Tal
    Twl::Twl
)

if(OPENSHADOWFLARE_ENABLE_DEBUG_TOOLS)
  add_library(shadowflare_profiler STATIC runtime/profiler.c)
  add_library(ShadowFlare::Profiler ALIAS shadowflare_profiler)
  sf_configure_c99_target(shadowflare_profiler)
  target_link_libraries(osf PRIVATE ShadowFlare::Profiler)
  target_compile_definitions(osf PRIVATE SF_ENABLE_PROFILING=1)

  add_executable(
    osf-measure
    ../utils/osf-measure.c
    runtime/screen_runtime.c
  )
  sf_configure_c99_target(osf-measure)
  target_link_libraries(
    osf-measure
    PRIVATE
      ShadowFlare::Core
      Tal::Tal
      Twl::Twl
  )
endif()
