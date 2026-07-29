add_library(
  osf_rkc_dbfcontrol STATIC
  software_backend.cpp
)
add_library(OpenShadowFlare::RKC_DBFCONTROL ALIAS osf_rkc_dbfcontrol)

target_compile_features(osf_rkc_dbfcontrol PUBLIC cxx_std_17)
target_include_directories(
  osf_rkc_dbfcontrol
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(
  osf_rkc_dbfcontrol
  PUBLIC
    OpenShadowFlare::Gapi
  PRIVATE
    OpenShadowFlare::RKC_DIB
    OpenShadowFlare::RKC_UPDIB
)
set_target_properties(osf_rkc_dbfcontrol PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
  target_compile_options(osf_rkc_dbfcontrol PRIVATE /W4 /permissive-)
else()
  target_compile_options(
    osf_rkc_dbfcontrol
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
  )
endif()
