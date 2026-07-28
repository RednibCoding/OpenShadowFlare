add_library(
  sf_game_core
  core/command_line.cpp
  core/game_config.cpp
  core/retail_random.cpp
  gapi/bitmap.cpp
  gapi/gapi.cpp
  gapi/njp.cpp
  gapi/software_backend.cpp
  render/character_select_renderer.cpp
  states/game_state.cpp
  states/menu_states.cpp
  states/save_catalog.cpp
)
add_library(OpenShadowFlare::GameCore ALIAS sf_game_core)

target_compile_features(sf_game_core PUBLIC cxx_std_17)
target_include_directories(
  sf_game_core
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
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
