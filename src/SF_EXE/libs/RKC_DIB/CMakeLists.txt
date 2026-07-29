add_library(
  osf_rkc_dib STATIC
  bitmap.cpp
)
add_library(OpenShadowFlare::RKC_DIB ALIAS osf_rkc_dib)

target_compile_features(osf_rkc_dib PUBLIC cxx_std_17)
target_include_directories(
  osf_rkc_dib
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(
  osf_rkc_dib PUBLIC OpenShadowFlare::Gapi)
set_target_properties(osf_rkc_dib PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
  target_compile_options(osf_rkc_dib PRIVATE /W4 /permissive-)
else()
  target_compile_options(osf_rkc_dib PRIVATE -Wall -Wextra -Wpedantic)
endif()
