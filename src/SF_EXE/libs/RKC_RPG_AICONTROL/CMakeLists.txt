add_library(
  osf_rkc_rpg_aicontrol STATIC
  ai_control_database.cpp
)
add_library(
  OpenShadowFlare::RKC_RPG_AICONTROL
  ALIAS osf_rkc_rpg_aicontrol
)

target_compile_features(
  osf_rkc_rpg_aicontrol PUBLIC cxx_std_17)
target_include_directories(
  osf_rkc_rpg_aicontrol
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
set_target_properties(
  osf_rkc_rpg_aicontrol
  PROPERTIES
    CXX_EXTENSIONS OFF
)

if(MSVC)
  target_compile_options(
    osf_rkc_rpg_aicontrol PRIVATE /W4 /permissive-)
else()
  target_compile_options(
    osf_rkc_rpg_aicontrol
    PRIVATE -Wall -Wextra -Wpedantic)
endif()
