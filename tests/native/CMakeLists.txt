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

if(TARGET sf_game_core)
  add_executable(game_core_test game_core_test.cpp)
  target_link_libraries(game_core_test PRIVATE OpenShadowFlare::GameCore)
  target_compile_features(game_core_test PRIVATE cxx_std_17)
  set_target_properties(game_core_test PROPERTIES CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(game_core_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(game_core_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME game_core COMMAND game_core_test)
endif()
