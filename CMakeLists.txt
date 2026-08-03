cmake_minimum_required(VERSION 3.20)

project(
  OpenShadowFlare
  VERSION 0.1.0
  LANGUAGES C CXX
)

if(PSP OR NINTENDO_SWITCH)
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
endif()

include(CTest)

option(OPENSHADOWFLARE_BUILD_EXE "Build the portable ShadowFlare executable" ON)
option(
  OPENSHADOWFLARE_ENABLE_DEBUG_TOOLS
  "Compile the F12 debug menu and runtime profiling tools"
  ON
)

if(PSP)
  set(OPENSHADOWFLARE_ENABLE_DEBUG_TOOLS OFF CACHE BOOL "" FORCE)
  set(OPENSHADOWFLARE_DEFAULT_PRESENTATION_BACKEND "gu")
elseif(NINTENDO_SWITCH)
  set(OPENSHADOWFLARE_DEFAULT_PRESENTATION_BACKEND "nxfb")
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
  PROPERTY STRINGS lgl gu nxfb
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
