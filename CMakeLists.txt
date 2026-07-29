cmake_minimum_required(VERSION 3.20)

project(
  OpenShadowFlare
  VERSION 0.1.0
  LANGUAGES C CXX
)

include(CTest)

option(OPENSHADOWFLARE_BUILD_EXE "Build the portable ShadowFlare executable" ON)
set(
  OPENSHADOWFLARE_PRESENTATION_BACKEND
  "lgl"
  CACHE STRING
  "Surface presentation backend used by the portable executable"
)
set_property(
  CACHE OPENSHADOWFLARE_PRESENTATION_BACKEND
  PROPERTY STRINGS lgl
)

add_subdirectory(thirdparty/lwl)
add_subdirectory(thirdparty/lal)
if(OPENSHADOWFLARE_PRESENTATION_BACKEND STREQUAL "lgl" OR BUILD_TESTING)
  add_subdirectory(thirdparty/lgl)
endif()

if(OPENSHADOWFLARE_BUILD_EXE)
  add_subdirectory(src/SF_EXE)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests/native)
endif()
