# Copyright (C) 2026 Michael Binder and contributors
#
# This file is part of TAL.
#
# TAL is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# TAL is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
#
# You should have received a copy of the GNU General Public License along with
# TAL. If not, see <https://www.gnu.org/licenses/>.

add_library(tal STATIC tal.c)
add_library(Tal::Tal ALIAS tal)

target_include_directories(tal PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(tal PRIVATE c_std_11)
set_target_properties(tal PROPERTIES C_EXTENSIONS OFF)

if(MSVC)
  target_compile_options(tal PRIVATE /W4)
else()
  target_compile_options(tal PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EMSCRIPTEN)
  target_sources(tal PRIVATE tal_web.c)
elseif(WIN32)
  target_sources(tal PRIVATE tal_winmm.c)
  target_link_libraries(tal PRIVATE winmm)
elseif(APPLE)
  target_sources(tal PRIVATE tal_coreaudio.c)
  find_library(TAL_AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
  find_library(TAL_AUDIOUNIT_FRAMEWORK AudioUnit REQUIRED)
  find_package(Threads REQUIRED)
  target_link_libraries(
    tal
    PRIVATE
      "${TAL_AUDIOTOOLBOX_FRAMEWORK}"
      "${TAL_AUDIOUNIT_FRAMEWORK}"
      Threads::Threads
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_sources(tal PRIVATE tal_alsa.c)
  find_package(ALSA REQUIRED)
  target_link_libraries(tal PRIVATE ALSA::ALSA)
elseif(NINTENDO_SWITCH)
  target_sources(tal PRIVATE tal_switch.c)
else()
  target_sources(tal PRIVATE tal_null.c)
endif()
