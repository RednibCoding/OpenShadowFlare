add_library(
  sf_game_core
  src/command_line.cpp
  src/game_config.cpp
  src/game_state.cpp
)
add_library(OpenShadowFlare::GameCore ALIAS sf_game_core)

target_compile_features(sf_game_core PUBLIC cxx_std_17)
target_include_directories(
  sf_game_core
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
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

add_executable(ShadowFlare_rebuilt src/main.cpp)

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
