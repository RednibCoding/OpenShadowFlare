# Copyright (C) 2026 Michael Binder and contributors
#
# This file is part of LAL.
#
# LAL is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# LAL is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
#
# You should have received a copy of the GNU General Public License along with
# LAL. If not, see <https://www.gnu.org/licenses/>.

add_library(lal STATIC
  lal.c
)
add_library(Lal::Lal ALIAS lal)

target_include_directories(
  lal
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_features(lal PUBLIC c_std_99)
set_target_properties(
  lal
  PROPERTIES
    C_EXTENSIONS OFF
)

if(MSVC)
  target_compile_options(lal PRIVATE /W4)
else()
  target_compile_options(lal PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EMSCRIPTEN)
  target_sources(lal PRIVATE lal_webaudio.c)
elseif(WIN32)
  target_sources(lal PRIVATE lal_waveout.c)
  target_link_libraries(lal PRIVATE winmm)
elseif(APPLE)
  target_sources(lal PRIVATE lal_audioqueue.c)
  find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
  find_package(Threads REQUIRED)
  target_link_libraries(
    lal
    PRIVATE
      "${AUDIOTOOLBOX_FRAMEWORK}"
      Threads::Threads
  )
elseif(NINTENDO_SWITCH)
  target_sources(lal PRIVATE lal_switch.c)
elseif(WIIU OR CMAKE_SYSTEM_NAME STREQUAL "WiiU")
  target_sources(lal PRIVATE lal_wiiu.c)
  set(_lal_wiiu_portlibs "$ENV{DEVKITPRO}/portlibs/wiiu")
  find_path(
    LAL_WIIU_SDL2_INCLUDE_DIR
    NAMES SDL.h
    PATHS "${_lal_wiiu_portlibs}/include/SDL2"
    NO_DEFAULT_PATH
    REQUIRED
  )
  find_library(
    LAL_WIIU_SDL2_LIBRARY
    NAMES SDL2
    PATHS "${_lal_wiiu_portlibs}/lib"
    NO_DEFAULT_PATH
    REQUIRED
  )
  target_include_directories(lal PRIVATE "${LAL_WIIU_SDL2_INCLUDE_DIR}")
  target_link_libraries(lal PRIVATE "${LAL_WIIU_SDL2_LIBRARY}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_sources(lal PRIVATE lal_alsa.c)
  find_package(ALSA REQUIRED)
  find_package(Threads REQUIRED)
  target_link_libraries(
    lal
    PRIVATE
      ALSA::ALSA
      Threads::Threads
  )
else()
  message(FATAL_ERROR "LAL has no backend for this platform")
endif()
