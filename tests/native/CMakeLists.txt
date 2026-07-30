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

add_test(
  NAME source_boundaries
  COMMAND
    "${CMAKE_COMMAND}"
    "-DPROJECT_ROOT=${PROJECT_SOURCE_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_source_boundaries.cmake"
)

if(TARGET sf_game_core)
  add_executable(game_core_test game_core_test.cpp)
  target_link_libraries(
    game_core_test
    PRIVATE
      OpenShadowFlare::States
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
