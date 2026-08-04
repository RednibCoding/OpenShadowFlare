cmake_minimum_required(VERSION 3.20)

project(
  OpenShadowFlare
  VERSION 0.1.0
  LANGUAGES C CXX
)

if(NINTENDO_SWITCH)
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
endif()

include(CTest)

option(OPENSHADOWFLARE_BUILD_EXE "Build the portable ShadowFlare executable" ON)
option(
  OPENSHADOWFLARE_BUILD_C99_GAME
  "Build the small C99 ShadowFlare rewrite"
  ON
)
option(
  OPENSHADOWFLARE_ENABLE_DEBUG_TOOLS
  "Compile the F12 debug menu and runtime profiling tools"
  ON
)

if(NINTENDO_SWITCH)
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
  PROPERTY STRINGS lgl nxfb
)

if(OPENSHADOWFLARE_BUILD_EXE OR BUILD_TESTING)
  add_subdirectory(thirdparty/lwl)
  add_subdirectory(thirdparty/lal)
endif()
add_subdirectory(thirdparty/twl)
add_subdirectory(thirdparty/tal)
if((OPENSHADOWFLARE_BUILD_EXE AND
    OPENSHADOWFLARE_PRESENTATION_BACKEND STREQUAL "lgl") OR BUILD_TESTING)
  add_subdirectory(thirdparty/lgl)
endif()

if(OPENSHADOWFLARE_BUILD_EXE)
  add_subdirectory(src/SF_EXE)
endif()

if(OPENSHADOWFLARE_BUILD_C99_GAME)
  add_subdirectory(shadowflare)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests/native)
endif()
