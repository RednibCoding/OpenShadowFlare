add_library(
  osf_rk_function STATIC
  rclib_lz.cpp
)
add_library(OpenShadowFlare::RK_FUNCTION ALIAS osf_rk_function)

target_compile_features(osf_rk_function PUBLIC cxx_std_17)
target_include_directories(
  osf_rk_function
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
set_target_properties(osf_rk_function PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
  target_compile_options(osf_rk_function PRIVATE /W4 /permissive-)
else()
  target_compile_options(osf_rk_function PRIVATE -Wall -Wextra -Wpedantic)
endif()
