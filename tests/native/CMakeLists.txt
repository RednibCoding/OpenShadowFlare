add_executable(
  portable_libraries_test
  portable_libraries_test.c
)

target_link_libraries(
  portable_libraries_test
  PRIVATE
    Lal::Lal
    Lgl::Lgl
)
target_include_directories(
  portable_libraries_test
  PRIVATE
    "${PROJECT_SOURCE_DIR}/thirdparty/lwl"
)

add_test(NAME portable_libraries COMMAND portable_libraries_test)

if(OPENSHADOWFLARE_ENABLE_DEBUG_TOOLS)
  add_executable(frame_profiler_test frame_profiler_test.cpp)
  target_link_libraries(
    frame_profiler_test PRIVATE OpenShadowFlare::DebugTools)
  target_compile_features(frame_profiler_test PRIVATE cxx_std_17)
  set_target_properties(
    frame_profiler_test PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      frame_profiler_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      frame_profiler_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME frame_profiler COMMAND frame_profiler_test)
endif()

add_test(
  NAME source_boundaries
  COMMAND
    "${CMAKE_COMMAND}"
    "-DPROJECT_ROOT=${PROJECT_SOURCE_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_source_boundaries.cmake"
)

if(TARGET sf_game_core)
  add_executable(resource_manager_test resource_manager_test.cpp)
  target_link_libraries(
    resource_manager_test PRIVATE OpenShadowFlare::Resources)
  target_compile_features(resource_manager_test PRIVATE cxx_std_17)
  target_compile_definitions(
    resource_manager_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    resource_manager_test PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      resource_manager_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      resource_manager_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME resource_manager COMMAND resource_manager_test)

  add_executable(game_core_test game_core_test.cpp)
  target_link_libraries(
    game_core_test
    PRIVATE
      OpenShadowFlare::States
      OpenShadowFlare::Ui
      OpenShadowFlare::RKC_RPGSCRN
  )
  target_compile_features(game_core_test PRIVATE cxx_std_17)
  set_target_properties(game_core_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(game_core_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(game_core_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME game_core COMMAND game_core_test)

  add_executable(
    gameplay_blackjack_test
    gameplay_blackjack_test.cpp
  )
  target_link_libraries(
    gameplay_blackjack_test
    PRIVATE
      OpenShadowFlare::Render
      OpenShadowFlare::World
  )
  target_compile_features(
    gameplay_blackjack_test PRIVATE cxx_std_17)
  target_compile_definitions(
    gameplay_blackjack_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    gameplay_blackjack_test PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      gameplay_blackjack_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      gameplay_blackjack_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME gameplay_blackjack COMMAND gameplay_blackjack_test)

  add_executable(vendor_test vendor_test.cpp)
  target_link_libraries(
    vendor_test
    PRIVATE
      OpenShadowFlare::States
      OpenShadowFlare::World
  )
  target_compile_features(vendor_test PRIVATE cxx_std_17)
  target_compile_definitions(
    vendor_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(vendor_test PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(vendor_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      vendor_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME vendor COMMAND vendor_test)

  add_executable(player_magic_test player_magic_test.cpp)
  target_link_libraries(
    player_magic_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(player_magic_test PRIVATE cxx_std_17)
  target_compile_definitions(
    player_magic_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    player_magic_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      player_magic_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      player_magic_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME player_magic COMMAND player_magic_test)

  add_executable(
    player_spell_cast_test
    player_spell_cast_test.cpp
  )
  target_link_libraries(
    player_spell_cast_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    player_spell_cast_test PRIVATE cxx_std_17)
  target_compile_definitions(
    player_spell_cast_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    player_spell_cast_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      player_spell_cast_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      player_spell_cast_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME player_spell_cast
    COMMAND player_spell_cast_test)

  add_executable(
    gameplay_magic_test
    gameplay_magic_test.cpp
  )
  target_link_libraries(
    gameplay_magic_test
    PRIVATE
      OpenShadowFlare::States
      OpenShadowFlare::World
  )
  target_compile_features(
    gameplay_magic_test PRIVATE cxx_std_17)
  set_target_properties(
    gameplay_magic_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      gameplay_magic_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      gameplay_magic_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME gameplay_magic
    COMMAND gameplay_magic_test)

  add_executable(
    gameplay_equipment_color_test
    gameplay_equipment_color_test.cpp
  )
  target_link_libraries(
    gameplay_equipment_color_test
    PRIVATE
      OpenShadowFlare::Render
      OpenShadowFlare::States
      OpenShadowFlare::World
  )
  target_compile_definitions(
    gameplay_equipment_color_test
    PRIVATE
      OPENSHADOWFLARE_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
  )
  target_compile_features(
    gameplay_equipment_color_test PRIVATE cxx_std_17)
  set_target_properties(
    gameplay_equipment_color_test PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      gameplay_equipment_color_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      gameplay_equipment_color_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME gameplay_equipment_color
    COMMAND gameplay_equipment_color_test)

  add_executable(
    gameplay_ui_controller_test
    gameplay_ui_controller_test.cpp
    "${PROJECT_SOURCE_DIR}/src/SF_EXE/runtime/audio_system.cpp"
    "${PROJECT_SOURCE_DIR}/src/SF_EXE/runtime/gameplay_ui_controller.cpp"
    "${PROJECT_SOURCE_DIR}/src/SF_EXE/runtime/input_adapter.cpp"
  )
  target_link_libraries(
    gameplay_ui_controller_test
    PRIVATE
      OpenShadowFlare::GameCore
      OpenShadowFlare::RKC_DSOUND
      Lal::Lal
      Lwl::Lwl
  )
  target_compile_features(
    gameplay_ui_controller_test PRIVATE cxx_std_17)
  target_compile_definitions(
    gameplay_ui_controller_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    gameplay_ui_controller_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      gameplay_ui_controller_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      gameplay_ui_controller_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME gameplay_ui_controller
    COMMAND gameplay_ui_controller_test)

  add_executable(movement_test movement_test.cpp)
  target_link_libraries(
    movement_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(movement_test PRIVATE cxx_std_17)
  target_compile_definitions(
    movement_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(movement_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(movement_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      movement_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME movement COMMAND movement_test)

  add_executable(
    player_attack_action_test
    player_attack_action_test.cpp
  )
  target_link_libraries(
    player_attack_action_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    player_attack_action_test PRIVATE cxx_std_17)
  target_compile_definitions(
    player_attack_action_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    player_attack_action_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      player_attack_action_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      player_attack_action_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME player_attack_action
    COMMAND player_attack_action_test)

  add_executable(
    player_increased_power_test
    player_increased_power_test.cpp
  )
  target_link_libraries(
    player_increased_power_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    player_increased_power_test PRIVATE cxx_std_17)
  target_compile_definitions(
    player_increased_power_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    player_increased_power_test PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      player_increased_power_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      player_increased_power_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME player_increased_power
    COMMAND player_increased_power_test)

  add_executable(
    player_land_mine_test
    player_land_mine_test.cpp
  )
  target_link_libraries(
    player_land_mine_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    player_land_mine_test PRIVATE cxx_std_17)
  target_compile_definitions(
    player_land_mine_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    player_land_mine_test PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      player_land_mine_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      player_land_mine_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME player_land_mine COMMAND player_land_mine_test)

  add_executable(
    player_transport_spell_test
    player_transport_spell_test.cpp
  )
  target_link_libraries(
    player_transport_spell_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    player_transport_spell_test PRIVATE cxx_std_17)
  target_compile_definitions(
    player_transport_spell_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    player_transport_spell_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      player_transport_spell_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      player_transport_spell_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME player_transport_spell
    COMMAND player_transport_spell_test)

  add_executable(
    player_attack_impact_test
    player_attack_impact_test.cpp
  )
  target_link_libraries(
    player_attack_impact_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    player_attack_impact_test PRIVATE cxx_std_17)
  target_compile_definitions(
    player_attack_impact_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    player_attack_impact_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      player_attack_impact_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      player_attack_impact_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME player_attack_impact
    COMMAND player_attack_impact_test)

  add_executable(
    player_ranged_attack_test
    player_ranged_attack_test.cpp
  )
  target_link_libraries(
    player_ranged_attack_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    player_ranged_attack_test PRIVATE cxx_std_17)
  target_compile_definitions(
    player_ranged_attack_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    player_ranged_attack_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      player_ranged_attack_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      player_ranged_attack_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME player_ranged_attack
    COMMAND player_ranged_attack_test)

  add_executable(scenario_data_test scenario_data_test.cpp)
  target_link_libraries(
    scenario_data_test
    PRIVATE
      OpenShadowFlare::Render
  )
  target_compile_features(
    scenario_data_test PRIVATE cxx_std_17)
  target_compile_definitions(
    scenario_data_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    scenario_data_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      scenario_data_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      scenario_data_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME scenario_data COMMAND scenario_data_test)

  add_executable(quest_state_test quest_state_test.cpp)
  target_link_libraries(
    quest_state_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    quest_state_test PRIVATE cxx_std_17)
  set_target_properties(
    quest_state_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      quest_state_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      quest_state_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME quest_state COMMAND quest_state_test)

  add_executable(mission_list_test mission_list_test.cpp)
  target_link_libraries(
    mission_list_test
    PRIVATE
      OpenShadowFlare::Render
  )
  target_compile_features(
    mission_list_test PRIVATE cxx_std_17)
  target_compile_definitions(
    mission_list_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    mission_list_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      mission_list_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      mission_list_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME mission_list COMMAND mission_list_test)

  add_executable(gameplay_map_test gameplay_map_test.cpp)
  target_link_libraries(
    gameplay_map_test
    PRIVATE
      OpenShadowFlare::Render
      OpenShadowFlare::RKC_DBFCONTROL
  )
  target_compile_features(
    gameplay_map_test PRIVATE cxx_std_17)
  target_compile_definitions(
    gameplay_map_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    gameplay_map_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      gameplay_map_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      gameplay_map_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME gameplay_map COMMAND gameplay_map_test)

  add_executable(
    gameplay_inventory_test
    gameplay_inventory_test.cpp
  )
  target_link_libraries(
    gameplay_inventory_test
    PRIVATE
      OpenShadowFlare::Render
  )
  target_compile_features(
    gameplay_inventory_test PRIVATE cxx_std_17)
  target_compile_definitions(
    gameplay_inventory_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    gameplay_inventory_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      gameplay_inventory_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      gameplay_inventory_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME gameplay_inventory
    COMMAND gameplay_inventory_test)

  add_executable(item_database_test item_database_test.cpp)
  target_link_libraries(
    item_database_test
    PRIVATE
      OpenShadowFlare::Items
  )
  target_compile_features(
    item_database_test PRIVATE cxx_std_17)
  target_compile_definitions(
    item_database_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    item_database_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      item_database_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      item_database_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME item_database COMMAND item_database_test)

  add_executable(table_database_test table_database_test.cpp)
  target_link_libraries(
    table_database_test
    PRIVATE
      OpenShadowFlare::RKC_RPG_TABLE
  )
  target_compile_features(
    table_database_test PRIVATE cxx_std_17)
  target_compile_definitions(
    table_database_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    table_database_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      table_database_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      table_database_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME table_database COMMAND table_database_test)

  add_executable(ai_control_test ai_control_test.cpp)
  target_link_libraries(
    ai_control_test
    PRIVATE
      OpenShadowFlare::RKC_RPG_AICONTROL
  )
  target_compile_features(
    ai_control_test PRIVATE cxx_std_17)
  target_compile_definitions(
    ai_control_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    ai_control_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      ai_control_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      ai_control_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME ai_control COMMAND ai_control_test)

  add_executable(
    enemy_ai_evaluator_test
    enemy_ai_evaluator_test.cpp
  )
  target_link_libraries(
    enemy_ai_evaluator_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    enemy_ai_evaluator_test PRIVATE cxx_std_17)
  target_compile_definitions(
    enemy_ai_evaluator_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    enemy_ai_evaluator_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      enemy_ai_evaluator_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      enemy_ai_evaluator_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME enemy_ai_evaluator
    COMMAND enemy_ai_evaluator_test)

  add_executable(
    enemy_ai_action_test
    enemy_ai_action_test.cpp
  )
  target_link_libraries(
    enemy_ai_action_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    enemy_ai_action_test PRIVATE cxx_std_17)
  target_compile_definitions(
    enemy_ai_action_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    enemy_ai_action_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      enemy_ai_action_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      enemy_ai_action_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME enemy_ai_action
    COMMAND enemy_ai_action_test)

  add_executable(
    enemy_target_selector_test
    enemy_target_selector_test.cpp
  )
  target_link_libraries(
    enemy_target_selector_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    enemy_target_selector_test PRIVATE cxx_std_17)
  set_target_properties(
    enemy_target_selector_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      enemy_target_selector_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      enemy_target_selector_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME enemy_target_selector
    COMMAND enemy_target_selector_test)

  add_executable(
    enemy_presentation_test
    enemy_presentation_test.cpp
  )
  target_link_libraries(
    enemy_presentation_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    enemy_presentation_test PRIVATE cxx_std_17)
  target_compile_definitions(
    enemy_presentation_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    enemy_presentation_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      enemy_presentation_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      enemy_presentation_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME enemy_presentation
    COMMAND enemy_presentation_test)

  add_executable(
    enemy_reaction_presentation_test
    enemy_reaction_presentation_test.cpp
  )
  target_link_libraries(
    enemy_reaction_presentation_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    enemy_reaction_presentation_test PRIVATE cxx_std_17)
  target_compile_definitions(
    enemy_reaction_presentation_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    enemy_reaction_presentation_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      enemy_reaction_presentation_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      enemy_reaction_presentation_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME enemy_reaction_presentation
    COMMAND enemy_reaction_presentation_test)

  add_executable(
    enemy_presentation_audio_test
    enemy_presentation_audio_test.cpp
  )
  target_link_libraries(
    enemy_presentation_audio_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    enemy_presentation_audio_test PRIVATE cxx_std_17)
  set_target_properties(
    enemy_presentation_audio_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      enemy_presentation_audio_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      enemy_presentation_audio_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME enemy_presentation_audio
    COMMAND enemy_presentation_audio_test)

  add_executable(
    enemy_effect_impact_test
    enemy_effect_impact_test.cpp
  )
  target_link_libraries(
    enemy_effect_impact_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    enemy_effect_impact_test PRIVATE cxx_std_17)
  target_compile_definitions(
    enemy_effect_impact_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    enemy_effect_impact_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      enemy_effect_impact_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      enemy_effect_impact_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME enemy_effect_impact
    COMMAND enemy_effect_impact_test)

  add_executable(
    enemy_effect_controller_test
    enemy_effect_controller_test.cpp
  )
  target_link_libraries(
    enemy_effect_controller_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    enemy_effect_controller_test PRIVATE cxx_std_17)
  target_compile_definitions(
    enemy_effect_controller_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    enemy_effect_controller_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      enemy_effect_controller_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      enemy_effect_controller_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME enemy_effect_controller
    COMMAND enemy_effect_controller_test)

  add_executable(
    runtime_effect_actor_test
    runtime_effect_actor_test.cpp
  )
  target_link_libraries(
    runtime_effect_actor_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    runtime_effect_actor_test PRIVATE cxx_std_17)
  target_compile_definitions(
    runtime_effect_actor_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    runtime_effect_actor_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      runtime_effect_actor_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      runtime_effect_actor_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME runtime_effect_actor
    COMMAND runtime_effect_actor_test)

  add_executable(
    runtime_effect_target_test
    runtime_effect_target_test.cpp
  )
  target_link_libraries(
    runtime_effect_target_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    runtime_effect_target_test PRIVATE cxx_std_17)
  set_target_properties(
    runtime_effect_target_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      runtime_effect_target_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      runtime_effect_target_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME runtime_effect_target
    COMMAND runtime_effect_target_test)

  add_executable(
    runtime_effect_system_test
    runtime_effect_system_test.cpp
  )
  target_link_libraries(
    runtime_effect_system_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    runtime_effect_system_test PRIVATE cxx_std_17)
  target_compile_definitions(
    runtime_effect_system_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    runtime_effect_system_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      runtime_effect_system_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      runtime_effect_system_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME runtime_effect_system
    COMMAND runtime_effect_system_test)

  add_executable(
    runtime_effect_live_test
    runtime_effect_live_test.cpp
  )
  target_link_libraries(
    runtime_effect_live_test
    PRIVATE
      OpenShadowFlare::Render
      OpenShadowFlare::RKC_RPG_TABLE
  )
  target_compile_features(
    runtime_effect_live_test PRIVATE cxx_std_17)
  target_compile_definitions(
    runtime_effect_live_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    runtime_effect_live_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      runtime_effect_live_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      runtime_effect_live_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME runtime_effect_live
    COMMAND runtime_effect_live_test)

  add_executable(
    enemy_direct_impact_test
    enemy_direct_impact_test.cpp
  )
  target_link_libraries(
    enemy_direct_impact_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    enemy_direct_impact_test PRIVATE cxx_std_17)
  set_target_properties(
    enemy_direct_impact_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      enemy_direct_impact_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      enemy_direct_impact_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME enemy_direct_impact
    COMMAND enemy_direct_impact_test)

  add_executable(
    combat_damage_test
    combat_damage_test.cpp
  )
  target_link_libraries(
    combat_damage_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    combat_damage_test PRIVATE cxx_std_17)
  target_compile_definitions(
    combat_damage_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    combat_damage_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      combat_damage_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      combat_damage_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME combat_damage
    COMMAND combat_damage_test)

  add_executable(
    player_combat_defense_test
    player_combat_defense_test.cpp
  )
  target_link_libraries(
    player_combat_defense_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    player_combat_defense_test PRIVATE cxx_std_17)
  target_compile_definitions(
    player_combat_defense_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    player_combat_defense_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      player_combat_defense_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      player_combat_defense_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME player_combat_defense
    COMMAND player_combat_defense_test)

  add_executable(
    player_damage_receiver_test
    player_damage_receiver_test.cpp
  )
  target_link_libraries(
    player_damage_receiver_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    player_damage_receiver_test PRIVATE cxx_std_17)
  target_compile_definitions(
    player_damage_receiver_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    player_damage_receiver_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      player_damage_receiver_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      player_damage_receiver_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME player_damage_receiver
    COMMAND player_damage_receiver_test)

  add_executable(
    companion_damage_receiver_test
    companion_damage_receiver_test.cpp
  )
  target_link_libraries(
    companion_damage_receiver_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    companion_damage_receiver_test PRIVATE cxx_std_17)
  target_compile_definitions(
    companion_damage_receiver_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    companion_damage_receiver_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      companion_damage_receiver_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      companion_damage_receiver_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME companion_damage_receiver
    COMMAND companion_damage_receiver_test)

  add_executable(
    companion_actor_test
    companion_actor_test.cpp
  )
  target_link_libraries(
    companion_actor_test
    PRIVATE
      OpenShadowFlare::Render
  )
  target_compile_features(
    companion_actor_test PRIVATE cxx_std_17)
  target_compile_definitions(
    companion_actor_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    companion_actor_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      companion_actor_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      companion_actor_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME companion_actor
    COMMAND companion_actor_test)

  add_executable(
    enemy_damage_receiver_test
    enemy_damage_receiver_test.cpp
  )
  target_link_libraries(
    enemy_damage_receiver_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    enemy_damage_receiver_test PRIVATE cxx_std_17)
  target_compile_definitions(
    enemy_damage_receiver_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    enemy_damage_receiver_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      enemy_damage_receiver_test
      PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      enemy_damage_receiver_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME enemy_damage_receiver
    COMMAND enemy_damage_receiver_test)

  add_executable(
    movement_destination_test
    movement_destination_test.cpp
  )
  target_link_libraries(
    movement_destination_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    movement_destination_test PRIVATE cxx_std_17)
  set_target_properties(
    movement_destination_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      movement_destination_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      movement_destination_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME movement_destination
    COMMAND movement_destination_test)

  add_executable(player_data_test player_data_test.cpp)
  target_link_libraries(
    player_data_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    player_data_test PRIVATE cxx_std_17)
  target_compile_definitions(
    player_data_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    player_data_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      player_data_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      player_data_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME player_data COMMAND player_data_test)

  add_executable(
    enemy_death_rewards_test
    enemy_death_rewards_test.cpp
  )
  target_link_libraries(
    enemy_death_rewards_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    enemy_death_rewards_test PRIVATE cxx_std_17)
  target_compile_definitions(
    enemy_death_rewards_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    enemy_death_rewards_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      enemy_death_rewards_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      enemy_death_rewards_test
      PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME enemy_death_rewards
    COMMAND enemy_death_rewards_test)

  add_executable(
    first_goblin_combat_test
    first_goblin_combat_test.cpp
  )
  target_link_libraries(
    first_goblin_combat_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    first_goblin_combat_test PRIVATE cxx_std_17)
  target_compile_definitions(
    first_goblin_combat_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    first_goblin_combat_test
    PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      first_goblin_combat_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      first_goblin_combat_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()

  add_test(
    NAME first_goblin_combat
    COMMAND first_goblin_combat_test)

  add_executable(
    episode_one_quest_test
    episode_one_quest_test.cpp
  )
  target_link_libraries(
    episode_one_quest_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_one_quest_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_one_quest_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_one_quest_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_one_quest_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_one_quest_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_one_quest
    COMMAND episode_one_quest_test)

  add_executable(
    episode_one_dusty_ruins_test
    episode_one_dusty_ruins_test.cpp
  )
  target_link_libraries(
    episode_one_dusty_ruins_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_one_dusty_ruins_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_one_dusty_ruins_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_one_dusty_ruins_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_one_dusty_ruins_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_one_dusty_ruins_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_one_dusty_ruins
    COMMAND episode_one_dusty_ruins_test)

  add_executable(
    episode_one_spirit_stone_test
    episode_one_spirit_stone_test.cpp
  )
  target_link_libraries(
    episode_one_spirit_stone_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_one_spirit_stone_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_one_spirit_stone_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_one_spirit_stone_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_one_spirit_stone_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_one_spirit_stone_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_one_spirit_stone
    COMMAND episode_one_spirit_stone_test)

  add_executable(
    episode_one_town_rewards_test
    episode_one_town_rewards_test.cpp
  )
  target_link_libraries(
    episode_one_town_rewards_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_one_town_rewards_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_one_town_rewards_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_one_town_rewards_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_one_town_rewards_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_one_town_rewards_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_one_town_rewards
    COMMAND episode_one_town_rewards_test)

  add_executable(
    episode_one_cold_svalt_route_test
    episode_one_cold_svalt_route_test.cpp
  )
  target_link_libraries(
    episode_one_cold_svalt_route_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_one_cold_svalt_route_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_one_cold_svalt_route_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_one_cold_svalt_route_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_one_cold_svalt_route_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_one_cold_svalt_route_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_one_cold_svalt_route
    COMMAND episode_one_cold_svalt_route_test)

  add_executable(
    episode_one_memorable_ruby_test
    episode_one_memorable_ruby_test.cpp
  )
  target_link_libraries(
    episode_one_memorable_ruby_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_one_memorable_ruby_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_one_memorable_ruby_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_one_memorable_ruby_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_one_memorable_ruby_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_one_memorable_ruby_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_one_memorable_ruby
    COMMAND episode_one_memorable_ruby_test)

  add_executable(
    episode_one_cold_ruins_test
    episode_one_cold_ruins_test.cpp
  )
  target_link_libraries(
    episode_one_cold_ruins_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_one_cold_ruins_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_one_cold_ruins_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_one_cold_ruins_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_one_cold_ruins_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_one_cold_ruins_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_one_cold_ruins
    COMMAND episode_one_cold_ruins_test)

  add_executable(
    episode_one_purgatory_test
    episode_one_purgatory_test.cpp
  )
  target_link_libraries(
    episode_one_purgatory_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_one_purgatory_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_one_purgatory_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_one_purgatory_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_one_purgatory_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_one_purgatory_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_one_purgatory
    COMMAND episode_one_purgatory_test)

  add_executable(
    episode_one_remains_test
    episode_one_remains_test.cpp
  )
  target_link_libraries(
    episode_one_remains_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_one_remains_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_one_remains_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_one_remains_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_one_remains_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_one_remains_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_one_remains
    COMMAND episode_one_remains_test)

  add_executable(
    episode_one_immortal_route_test
    episode_one_immortal_route_test.cpp
  )
  target_link_libraries(
    episode_one_immortal_route_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_one_immortal_route_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_one_immortal_route_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_one_immortal_route_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_one_immortal_route_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_one_immortal_route_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_one_immortal_route
    COMMAND episode_one_immortal_route_test)

  add_executable(
    episode_one_immortal_battle_test
    episode_one_immortal_battle_test.cpp
  )
  target_link_libraries(
    episode_one_immortal_battle_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_one_immortal_battle_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_one_immortal_battle_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_one_immortal_battle_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_one_immortal_battle_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_one_immortal_battle_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_one_immortal_battle
    COMMAND episode_one_immortal_battle_test)

  add_executable(
    episode_two_mining_town_route_test
    episode_two_mining_town_route_test.cpp
  )
  target_link_libraries(
    episode_two_mining_town_route_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_mining_town_route_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_mining_town_route_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_mining_town_route_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_mining_town_route_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_mining_town_route_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_mining_town_route
    COMMAND episode_two_mining_town_route_test)

  add_executable(
    episode_two_forest_of_claws_test
    episode_two_forest_of_claws_test.cpp
  )
  target_link_libraries(
    episode_two_forest_of_claws_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_forest_of_claws_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_forest_of_claws_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_forest_of_claws_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_forest_of_claws_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_forest_of_claws_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_forest_of_claws
    COMMAND episode_two_forest_of_claws_test)

  add_executable(
    episode_two_elf_gate_test
    episode_two_elf_gate_test.cpp
  )
  target_link_libraries(
    episode_two_elf_gate_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_elf_gate_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_elf_gate_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_elf_gate_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_elf_gate_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_elf_gate_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_elf_gate
    COMMAND episode_two_elf_gate_test)

  add_executable(
    episode_two_wizard_tower_test
    episode_two_wizard_tower_test.cpp
  )
  target_link_libraries(
    episode_two_wizard_tower_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_wizard_tower_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_wizard_tower_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_wizard_tower_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_wizard_tower_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_wizard_tower_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_wizard_tower
    COMMAND episode_two_wizard_tower_test)

  add_executable(
    episode_two_seal_crystal_test
    episode_two_seal_crystal_test.cpp
  )
  target_link_libraries(
    episode_two_seal_crystal_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_seal_crystal_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_seal_crystal_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_seal_crystal_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_seal_crystal_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_seal_crystal_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_seal_crystal
    COMMAND episode_two_seal_crystal_test)

  add_executable(
    episode_two_fanann_handoff_test
    episode_two_fanann_handoff_test.cpp
  )
  target_link_libraries(
    episode_two_fanann_handoff_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_fanann_handoff_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_fanann_handoff_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_fanann_handoff_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_fanann_handoff_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_fanann_handoff_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_fanann_handoff
    COMMAND episode_two_fanann_handoff_test)

  add_executable(
    episode_two_yugunos_blockade_test
    episode_two_yugunos_blockade_test.cpp
  )
  target_link_libraries(
    episode_two_yugunos_blockade_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_yugunos_blockade_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_yugunos_blockade_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_yugunos_blockade_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_yugunos_blockade_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_yugunos_blockade_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_yugunos_blockade
    COMMAND episode_two_yugunos_blockade_test)

  add_executable(
    episode_two_yugunos_seal_test
    episode_two_yugunos_seal_test.cpp
  )
  target_link_libraries(
    episode_two_yugunos_seal_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_yugunos_seal_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_yugunos_seal_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_yugunos_seal_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_yugunos_seal_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_yugunos_seal_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_executable(
    episode_two_control_room_test
    episode_two_control_room_test.cpp
  )
  target_link_libraries(
    episode_two_control_room_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_control_room_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_control_room_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_control_room_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_control_room_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_control_room_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_control_room
    COMMAND episode_two_control_room_test)
  add_test(
    NAME episode_two_yugunos_seal
    COMMAND episode_two_yugunos_seal_test)

  add_executable(
    episode_two_power_supply_test
    episode_two_power_supply_test.cpp
  )
  target_link_libraries(
    episode_two_power_supply_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_power_supply_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_power_supply_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_power_supply_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_power_supply_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_power_supply_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_power_supply
    COMMAND episode_two_power_supply_test)

  add_executable(
    episode_two_dragons_test
    episode_two_dragons_test.cpp
  )
  target_link_libraries(
    episode_two_dragons_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_dragons_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_dragons_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_dragons_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_dragons_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_dragons_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_dragons
    COMMAND episode_two_dragons_test)

  add_executable(
    episode_two_central_front_test
    episode_two_central_front_test.cpp
  )
  target_link_libraries(
    episode_two_central_front_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_central_front_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_central_front_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_central_front_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_central_front_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_central_front_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_central_front
    COMMAND episode_two_central_front_test)

  add_executable(
    episode_two_south_camp_test
    episode_two_south_camp_test.cpp
  )
  target_link_libraries(
    episode_two_south_camp_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_south_camp_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_south_camp_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_south_camp_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_south_camp_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_south_camp_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_south_camp
    COMMAND episode_two_south_camp_test)

  add_executable(
    episode_two_sacred_wing_test
    episode_two_sacred_wing_test.cpp
  )
  target_link_libraries(
    episode_two_sacred_wing_test
    PRIVATE
      OpenShadowFlare::World
  )
  target_compile_features(
    episode_two_sacred_wing_test PRIVATE cxx_std_17)
  target_compile_definitions(
    episode_two_sacred_wing_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    episode_two_sacred_wing_test
    PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      episode_two_sacred_wing_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      episode_two_sacred_wing_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME episode_two_sacred_wing
    COMMAND episode_two_sacred_wing_test)

  add_executable(script_engine_test script_engine_test.cpp)
  target_link_libraries(
    script_engine_test
    PRIVATE
      OpenShadowFlare::RKC_RPG_SCRIPT
  )
  target_compile_features(
    script_engine_test PRIVATE cxx_std_17)
  target_compile_definitions(
    script_engine_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    script_engine_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(
      script_engine_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      script_engine_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME script_engine COMMAND script_engine_test)

  add_executable(gapi_test gapi_test.cpp)
  target_link_libraries(
    gapi_test
    PRIVATE
      OpenShadowFlare::Render
      OpenShadowFlare::RKC_DBFCONTROL
  )
  target_compile_features(gapi_test PRIVATE cxx_std_17)
  set_target_properties(gapi_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(gapi_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(gapi_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME gapi COMMAND gapi_test)

  add_executable(
    scenario_presentation_test
    scenario_presentation_test.cpp
  )
  target_link_libraries(
    scenario_presentation_test
    PRIVATE
      OpenShadowFlare::Render
      OpenShadowFlare::World
  )
  target_compile_features(
    scenario_presentation_test PRIVATE cxx_std_17)
  target_compile_definitions(
    scenario_presentation_test
    PRIVATE
      "OPENSHADOWFLARE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  )
  set_target_properties(
    scenario_presentation_test PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(
      scenario_presentation_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(
      scenario_presentation_test PRIVATE
        -Wall -Wextra -Wpedantic)
  endif()
  add_test(
    NAME scenario_presentation
    COMMAND scenario_presentation_test)

  add_executable(menu_states_test menu_states_test.cpp)
  target_link_libraries(
    menu_states_test PRIVATE OpenShadowFlare::Render)
  target_compile_features(menu_states_test PRIVATE cxx_std_17)
  set_target_properties(menu_states_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(menu_states_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(menu_states_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME menu_states COMMAND menu_states_test)

  add_executable(voc_test voc_test.cpp)
  target_link_libraries(
    voc_test
    PRIVATE
      OpenShadowFlare::RKC_DSOUND
  )
  target_compile_features(voc_test PRIVATE cxx_std_17)
  set_target_properties(voc_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(voc_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(voc_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME voc COMMAND voc_test)
endif()
