add_library(
  osf_rkc_rpgscrn STATIC
  caf.cpp
  coordinates.cpp
  display_order.cpp
  ground_map.cpp
  judgement.cpp
  object_map.cpp
)
add_library(OpenShadowFlare::RKC_RPGSCRN ALIAS osf_rkc_rpgscrn)

target_compile_features(osf_rkc_rpgscrn PUBLIC cxx_std_17)
target_include_directories(
  osf_rkc_rpgscrn
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(
  osf_rkc_rpgscrn
  PRIVATE
    OpenShadowFlare::RK_FUNCTION
)
set_target_properties(osf_rkc_rpgscrn PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
  target_compile_options(osf_rkc_rpgscrn PRIVATE /W4 /permissive-)
else()
  target_compile_options(osf_rkc_rpgscrn PRIVATE -Wall -Wextra -Wpedantic)
endif()
