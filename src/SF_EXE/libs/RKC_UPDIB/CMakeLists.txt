add_library(
  osf_rkc_updib STATIC
  njp.cpp
)
add_library(OpenShadowFlare::RKC_UPDIB ALIAS osf_rkc_updib)

target_compile_features(osf_rkc_updib PUBLIC cxx_std_17)
target_include_directories(
  osf_rkc_updib
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(
  osf_rkc_updib
  PRIVATE
    OpenShadowFlare::RK_FUNCTION
)
set_target_properties(osf_rkc_updib PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
  target_compile_options(osf_rkc_updib PRIVATE /W4 /permissive-)
else()
  target_compile_options(osf_rkc_updib PRIVATE -Wall -Wextra -Wpedantic)
endif()
