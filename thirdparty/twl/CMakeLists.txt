# Copyright (C) 2026 Michael Binder and contributors
#
# This file is part of TWL.
#
# TWL is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# TWL is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
#
# You should have received a copy of the GNU General Public License along with
# TWL. If not, see <https://www.gnu.org/licenses/>.

add_library(twl STATIC twl.c)
add_library(Twl::Twl ALIAS twl)

target_include_directories(twl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(twl PRIVATE c_std_11)
set_target_properties(twl PROPERTIES C_EXTENSIONS OFF)

if(MSVC)
  target_compile_options(twl PRIVATE /W4)
else()
  target_compile_options(twl PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EMSCRIPTEN)
  target_sources(
    twl
    PRIVATE
      backends/web/input.c
      backends/web/presentation.c
      backends/web/window.c
  )
elseif(WIN32)
  target_sources(
    twl
    PRIVATE
      backends/windows/input.c
      backends/windows/presentation.c
      backends/windows/window.c
  )
  target_link_libraries(twl PRIVATE user32 gdi32 xinput9_1_0)
elseif(APPLE)
  target_sources(
    twl
    PRIVATE
      backends/macos/input.c
      backends/macos/presentation.c
      backends/macos/support.c
      backends/macos/window.c
  )
  find_library(TWL_APPKIT_FRAMEWORK AppKit REQUIRED)
  find_library(TWL_GAMECONTROLLER_FRAMEWORK GameController REQUIRED)
  find_library(TWL_OPENGL_FRAMEWORK OpenGL REQUIRED)
  target_link_libraries(
    twl
    PRIVATE
      "${TWL_APPKIT_FRAMEWORK}"
      "${TWL_GAMECONTROLLER_FRAMEWORK}"
      "${TWL_OPENGL_FRAMEWORK}"
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_sources(
    twl
    PRIVATE
      backends/linux/input.c
      backends/linux/presentation.c
      backends/linux/window.c
  )
  find_package(X11 REQUIRED)
  find_package(OpenGL REQUIRED)
  target_link_libraries(twl PRIVATE X11::X11 OpenGL::GL)
elseif(NINTENDO_SWITCH)
  target_sources(
    twl
    PRIVATE
      backends/switch/window.c
      backends/switch/input.c
      backends/switch/presentation.c
  )
else()
  target_sources(twl PRIVATE backends/null/backend.c)
endif()
