add_library(
  osf_rkc_dsound STATIC
  voc.cpp
  voc_player.cpp
)
add_library(OpenShadowFlare::RKC_DSOUND ALIAS osf_rkc_dsound)

target_compile_features(osf_rkc_dsound PUBLIC cxx_std_17)
target_include_directories(
  osf_rkc_dsound
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(osf_rkc_dsound PUBLIC Lal::Lal)
set_target_properties(osf_rkc_dsound PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
  target_compile_options(osf_rkc_dsound PRIVATE /W4 /permissive-)
else()
  target_compile_options(osf_rkc_dsound PRIVATE -Wall -Wextra -Wpedantic)
endif()
