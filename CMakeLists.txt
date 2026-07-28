cmake_minimum_required(VERSION 3.20)

project(
  OpenShadowFlare
  VERSION 0.1.0
  LANGUAGES C CXX
)

include(CTest)

option(OPENSHADOWFLARE_BUILD_EXE "Build the portable ShadowFlare executable" ON)

add_subdirectory(thirdparty/lwl)
add_subdirectory(thirdparty/lal)
add_subdirectory(thirdparty/lgl)

if(OPENSHADOWFLARE_BUILD_EXE)
  add_subdirectory(src/SF_EXE)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests/native)
endif()
