cmake_minimum_required(VERSION 3.20)

project(
  OpenShadowFlare
  VERSION 0.1.0
  LANGUAGES C CXX
)

if(NINTENDO_SWITCH OR WIIU OR CMAKE_SYSTEM_NAME STREQUAL "WiiU")
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
endif()

include(CTest)

option(OPENSHADOWFLARE_BUILD_EXE "Build the portable ShadowFlare executable" ON)

if(NINTENDO_SWITCH)
  set(OPENSHADOWFLARE_DEFAULT_PRESENTATION_BACKEND "nxfb")
elseif(WIIU OR CMAKE_SYSTEM_NAME STREQUAL "WiiU")
  set(OPENSHADOWFLARE_DEFAULT_PRESENTATION_BACKEND "wiiu")
else()
  set(OPENSHADOWFLARE_DEFAULT_PRESENTATION_BACKEND "lgl")
endif()
set(
  OPENSHADOWFLARE_PRESENTATION_BACKEND
  "${OPENSHADOWFLARE_DEFAULT_PRESENTATION_BACKEND}"
  CACHE STRING
  "Surface presentation backend used by the portable executable"
)
set_property(
  CACHE OPENSHADOWFLARE_PRESENTATION_BACKEND
  PROPERTY STRINGS lgl nxfb wiiu
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
