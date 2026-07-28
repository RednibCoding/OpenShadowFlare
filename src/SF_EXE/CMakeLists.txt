add_subdirectory(libs/RK_FUNCTION)
add_subdirectory(libs/RKC_DIB)
add_subdirectory(libs/RKC_DSOUND)
add_subdirectory(libs/RKC_UPDIB)
add_subdirectory(libs/RKC_RPGSCRN)
add_subdirectory(libs/RKC_DBFCONTROL)

add_library(
  sf_game_core
  core/command_line.cpp
  core/game_config.cpp
  core/retail_random.cpp
  gapi/gapi.cpp
  render/character_select_renderer.cpp
  render/gameplay_renderer.cpp
  render/title_renderer.cpp
  states/game_state.cpp
  states/gameplay_state.cpp
  states/menu_states.cpp
  states/save_catalog.cpp
  world/player_actor.cpp
  world/world_scene.cpp
)
add_library(OpenShadowFlare::GameCore ALIAS sf_game_core)

target_compile_features(sf_game_core PUBLIC cxx_std_17)
target_include_directories(
  sf_game_core
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(
  sf_game_core
  PUBLIC
    OpenShadowFlare::RKC_DIB
    OpenShadowFlare::RKC_UPDIB
    OpenShadowFlare::RKC_RPGSCRN
)
set_target_properties(
  sf_game_core
  PROPERTIES
    CXX_EXTENSIONS OFF
)

if(MSVC)
  target_compile_options(sf_game_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(sf_game_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(
  ShadowFlare_rebuilt
  runtime/lgl_surface_presenter.cpp
  runtime/main.cpp
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
    Lgl::Lgl
)

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
