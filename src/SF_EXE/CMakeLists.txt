function(osf_configure_game_library target)
  target_compile_features(${target} PUBLIC cxx_std_17)
  target_include_directories(
    ${target}
    PUBLIC
      "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      ${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

include(cmake/configure_platform.cmake)
include(cmake/configure_presentation.cmake)

add_library(osf_gapi STATIC gapi/gapi.cpp)
add_library(OpenShadowFlare::Gapi ALIAS osf_gapi)
osf_configure_game_library(osf_gapi)

add_subdirectory(libs/RK_FUNCTION)
add_subdirectory(libs/RKC_DIB)
add_subdirectory(libs/RKC_DSOUND)
add_subdirectory(libs/RKC_UPDIB)
add_subdirectory(libs/RKC_RPGSCRN)
add_subdirectory(libs/RKC_RPG_AICONTROL)
add_subdirectory(libs/RKC_RPG_SCRIPT)
add_subdirectory(libs/RKC_RPG_TABLE)
add_subdirectory(libs/RKC_DBFCONTROL)

add_library(
  osf_core STATIC
  core/command_line.cpp
  core/game_config.cpp
  core/retail_random.cpp
)
add_library(OpenShadowFlare::Core ALIAS osf_core)
osf_configure_game_library(osf_core)

add_library(
  osf_items STATIC
  items/item_database.cpp
  items/item_audio.cpp
  items/item_condition.cpp
  items/item_instance_values.cpp
  items/item_grid.cpp
  items/item_information.cpp
  items/item_instance_factory.cpp
  items/new_player_loadout.cpp
  items/player_belt.cpp
  items/player_equipment.cpp
  items/player_inventory.cpp
  items/player_special_items.cpp
)
add_library(OpenShadowFlare::Items ALIAS osf_items)
osf_configure_game_library(osf_items)
target_link_libraries(
  osf_items
  PRIVATE
    OpenShadowFlare::RK_FUNCTION
)

add_library(
  osf_resources STATIC
  resources/character_visual_resource.cpp
  resources/effect_pattern_resource.cpp
  resources/effect_visual_resource.cpp
  resources/item_inventory_resource.cpp
  resources/item_world_resource.cpp
  resources/object_visual_resource.cpp
  resources/retail_filesystem.cpp
  resources/save_catalog.cpp
)
add_library(OpenShadowFlare::Resources ALIAS osf_resources)
osf_configure_game_library(osf_resources)
target_link_libraries(
  osf_resources
  PUBLIC
    OpenShadowFlare::RKC_RPGSCRN
    OpenShadowFlare::RKC_UPDIB
)

add_library(
  osf_states STATIC
  states/game_state.cpp
  states/gameplay_debug_menu.cpp
  states/gameplay_inventory.cpp
  states/gameplay_magic.cpp
  states/gameplay_map.cpp
  states/gameplay_mission_list.cpp
  states/gameplay_options_menu.cpp
  states/gameplay_status.cpp
  states/gameplay_state.cpp
  states/gameplay_transport.cpp
  states/save_slot.cpp
  states/character_select_state.cpp
  states/character_select/network_flow.cpp
  states/character_select/new_character_flow.cpp
  states/character_select/saved_game_flow.cpp
  states/title_state.cpp
)
add_library(OpenShadowFlare::States ALIAS osf_states)
osf_configure_game_library(osf_states)
target_link_libraries(
  osf_states
  PUBLIC
    OpenShadowFlare::Core
    OpenShadowFlare::Items
)

add_library(
  osf_world STATIC
  world/actor_direction.cpp
  world/companion_actor.cpp
  world/companion_attack_action.cpp
  world/companion_attack_impact.cpp
  world/companion_damage_receiver.cpp
  world/companion_profile.cpp
  world/companion_respawn.cpp
  world/companion_target_selector.cpp
  world/combat_damage.cpp
  world/combat_effect_actor.cpp
  world/combat_hit_chance.cpp
  world/enemy_actor.cpp
  world/enemy_ai_action.cpp
  world/enemy_ai_evaluator.cpp
  world/enemy_damage_receiver.cpp
  world/enemy_death_rewards.cpp
  world/enemy_direct_impact.cpp
  world/enemy_effect_controller.cpp
  world/enemy_effect_impact.cpp
  world/enemy_presentation.cpp
  world/enemy_presentation_audio.cpp
  world/enemy_target_selector.cpp
  world/generic_effect_actor.cpp
  world/ground_item.cpp
  world/map_exploration.cpp
  world/miss_effect_actor.cpp
  world/mission_catalog.cpp
  world/movement_controller.cpp
  world/movement_destination_selector.cpp
  world/npc_actor.cpp
  world/player_appearance.cpp
  world/player_actor.cpp
  world/player_attack_action.cpp
  world/player_attack_impact.cpp
  world/player_attack_target.cpp
  world/player_ranged_attack.cpp
  world/player_combat_defense.cpp
  world/player_counter_burst.cpp
  world/player_damage_receiver.cpp
  world/player_data.cpp
  world/player_energy_shield.cpp
  world/player_heal_spell.cpp
  world/player_item_controller.cpp
  world/player_level_up_notice.cpp
  world/player_magic_shield.cpp
  world/player_magic.cpp
  world/player_moon_spell.cpp
  world/player_resource_rate.cpp
  world/player_runtime_profile.cpp
  world/player_spell_action.cpp
  world/player_spell_cast.cpp
  world/player_spell_parameters.cpp
  world/player_sustained_spell.cpp
  world/player_transport_spell.cpp
  world/player_voice.cpp
  world/quest_state.cpp
  world/retail_save_file.cpp
  world/retail_save_items.cpp
  world/retail_save_magic.cpp
  world/retail_save_preview.cpp
  world/retail_save_progress.cpp
  world/runtime_effect_actor.cpp
  world/runtime_effect_system.cpp
  world/runtime_effect_target.cpp
  world/scenario_data.cpp
  world/scenario_entity_state.cpp
  world/scenario_object_actor.cpp
  world/scenario_world.cpp
  world/transport_catalog.cpp
  world/script/scenario_script_runtime.cpp
  world/world_pointer.cpp
  world/world_scene.cpp
  world/world_scene_companion_combat.cpp
  world/world_scene_combat.cpp
  world/world_scene_enemy_combat.cpp
  world/world_scene_effects.cpp
  world/world_scene_interaction.cpp
  world/world_scene_items.cpp
  world/world_scene_loading.cpp
  world/world_scene_moon.cpp
  world/world_scene_player_profile.cpp
  world/world_scene_player_resources.cpp
  world/world_scene_script.cpp
  world/world_scene_transport_spell.cpp
)
add_library(OpenShadowFlare::World ALIAS osf_world)
osf_configure_game_library(osf_world)
target_link_libraries(
  osf_world
  PUBLIC
    OpenShadowFlare::Core
    OpenShadowFlare::Gapi
    OpenShadowFlare::Items
    OpenShadowFlare::Resources
    OpenShadowFlare::RKC_DIB
    OpenShadowFlare::RKC_RPGSCRN
    OpenShadowFlare::RKC_RPG_AICONTROL
    OpenShadowFlare::RKC_RPG_SCRIPT
    OpenShadowFlare::RKC_RPG_TABLE
    OpenShadowFlare::RKC_UPDIB
)

add_library(
  osf_ui STATIC
  ui/conversation_layout.cpp
  ui/player_level_up_notice_input.cpp
  ui/player_level_up_notice_layout.cpp
  ui/quest_notice_layout.cpp
)
add_library(OpenShadowFlare::Ui ALIAS osf_ui)
osf_configure_game_library(osf_ui)
target_link_libraries(
  osf_ui
  PUBLIC
    OpenShadowFlare::World
    OpenShadowFlare::RKC_RPGSCRN
)

add_library(
  osf_render STATIC
  render/character_renderer.cpp
  render/character_select_renderer.cpp
  render/enemy_nameplate_renderer.cpp
  render/gameplay_debug_renderer.cpp
  render/gameplay_help_renderer.cpp
  render/gameplay_hud_renderer.cpp
  render/gameplay_inventory_renderer.cpp
  render/gameplay_map_renderer.cpp
  render/gameplay_magic_renderer.cpp
  render/gameplay_mission_list_renderer.cpp
  render/gameplay_options_renderer.cpp
  render/gameplay_overlay_renderer.cpp
  render/gameplay_renderer.cpp
  render/gameplay_status_renderer.cpp
  render/gameplay_transport_renderer.cpp
  render/item_information_renderer.cpp
  render/loading_renderer.cpp
  render/player_level_up_notice_renderer.cpp
  render/quest_notice_renderer.cpp
  render/system_cursor_renderer.cpp
  render/title_renderer.cpp
)
add_library(OpenShadowFlare::Render ALIAS osf_render)
osf_configure_game_library(osf_render)
target_link_libraries(
  osf_render
  PUBLIC
    OpenShadowFlare::Gapi
    OpenShadowFlare::States
    OpenShadowFlare::Ui
    OpenShadowFlare::World
    OpenShadowFlare::RKC_DIB
    OpenShadowFlare::RKC_RPGSCRN
    OpenShadowFlare::RKC_UPDIB
)

add_library(sf_game_core INTERFACE)
add_library(OpenShadowFlare::GameCore ALIAS sf_game_core)
target_link_libraries(
  sf_game_core
  INTERFACE
    OpenShadowFlare::Core
    OpenShadowFlare::Items
    OpenShadowFlare::Resources
    OpenShadowFlare::States
    OpenShadowFlare::Ui
    OpenShadowFlare::World
    OpenShadowFlare::Render
)

add_executable(
  ShadowFlare_rebuilt
  runtime/audio_system.cpp
  runtime/frontend_assets.cpp
  runtime/game_runtime.cpp
  runtime/gameplay_ui_controller.cpp
  runtime/input_adapter.cpp
  runtime/main.cpp
  runtime/runtime_renderer.cpp
  runtime/state_bindings.cpp
)

target_compile_features(ShadowFlare_rebuilt PRIVATE cxx_std_17)
set_target_properties(ShadowFlare_rebuilt PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  ShadowFlare_rebuilt
  PRIVATE
    OpenShadowFlare::GameCore
    OpenShadowFlare::RKC_DBFCONTROL
    OpenShadowFlare::RKC_DSOUND
    Lwl::Lwl
    Lal::Lal
)

osf_configure_platform(ShadowFlare_rebuilt)
osf_configure_presentation(ShadowFlare_rebuilt)

if(MINGW)
  target_link_options(
    ShadowFlare_rebuilt
    PRIVATE
      -static-libgcc
      -static-libstdc++
  )
endif()

if(MSVC)
  target_compile_options(ShadowFlare_rebuilt PRIVATE /W4 /permissive-)
else()
  target_compile_options(
    ShadowFlare_rebuilt
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
  )
endif()
