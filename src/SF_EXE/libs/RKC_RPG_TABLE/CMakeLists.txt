add_library(
  osf_rkc_rpg_table STATIC
  table_database.cpp
)
add_library(
  OpenShadowFlare::RKC_RPG_TABLE
  ALIAS osf_rkc_rpg_table
)

target_compile_features(osf_rkc_rpg_table PUBLIC cxx_std_17)
target_include_directories(
  osf_rkc_rpg_table
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
set_target_properties(
  osf_rkc_rpg_table
  PROPERTIES
    CXX_EXTENSIONS OFF
)

if(MSVC)
  target_compile_options(
    osf_rkc_rpg_table PRIVATE /W4 /permissive-)
else()
  target_compile_options(
    osf_rkc_rpg_table PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(
  osf_rkc_rpg_table
  PRIVATE
    OpenShadowFlare::RK_FUNCTION
)
