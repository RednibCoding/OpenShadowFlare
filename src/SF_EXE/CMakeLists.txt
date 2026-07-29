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
  items/item_condition.cpp
  items/item_information.cpp
  items/player_equipment.cpp
  items/player_inventory.cpp
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
  resources/item_inventory_resource.cpp
  resources/item_world_resource.cpp
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
  states/gameplay_inventory.cpp
  states/gameplay_map.cpp
  states/gameplay_mission_list.cpp
  states/gameplay_options_menu.cpp
  states/gameplay_state.cpp
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
  world/ground_item.cpp
  world/map_exploration.cpp
  world/mission_catalog.cpp
  world/movement_controller.cpp
  world/npc_actor.cpp
  world/player_appearance.cpp
  world/player_actor.cpp
  world/player_data.cpp
  world/retail_save_file.cpp
  world/retail_save_preview.cpp
  world/quest_state.cpp
  world/scenario_data.cpp
  world/script/scenario_script_runtime.cpp
  world/world_pointer.cpp
  world/world_scene.cpp
  world/world_scene_interaction.cpp
  world/world_scene_items.cpp
  world/world_scene_loading.cpp
  world/world_scene_script.cpp
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
    OpenShadowFlare::RKC_RPG_SCRIPT
    OpenShadowFlare::RKC_RPG_TABLE
    OpenShadowFlare::RKC_UPDIB
)

add_library(
  osf_ui STATIC
  ui/conversation_layout.cpp
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
  render/gameplay_help_renderer.cpp
  render/gameplay_hud_renderer.cpp
  render/gameplay_inventory_renderer.cpp
  render/gameplay_map_renderer.cpp
  render/gameplay_mission_list_renderer.cpp
  render/gameplay_options_renderer.cpp
  render/gameplay_overlay_renderer.cpp
  render/gameplay_renderer.cpp
  render/item_information_renderer.cpp
  render/loading_renderer.cpp
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
