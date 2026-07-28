add_library(
  osf_rkc_rpg_script STATIC
  script_data.cpp
  script_engine.cpp
)
add_library(
  OpenShadowFlare::RKC_RPG_SCRIPT
  ALIAS
  osf_rkc_rpg_script
)

target_compile_features(osf_rkc_rpg_script PUBLIC cxx_std_17)
target_include_directories(
  osf_rkc_rpg_script
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
set_target_properties(
  osf_rkc_rpg_script
  PROPERTIES
    CXX_EXTENSIONS OFF
)

if(MSVC)
  target_compile_options(
    osf_rkc_rpg_script PRIVATE /W4 /permissive-)
else()
  target_compile_options(
    osf_rkc_rpg_script PRIVATE -Wall -Wextra -Wpedantic)
endif()
