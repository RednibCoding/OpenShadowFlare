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
target_compile_features(twl PUBLIC c_std_11)
set_target_properties(twl PROPERTIES C_EXTENSIONS OFF)

if(MSVC)
  target_compile_options(twl PRIVATE /W4)
else()
  target_compile_options(twl PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EMSCRIPTEN)
  target_sources(twl PRIVATE twl_web.c)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_sources(twl PRIVATE twl_x11.c)
  find_package(X11 REQUIRED)
  find_package(OpenGL REQUIRED)
  target_link_libraries(twl PRIVATE X11::X11 OpenGL::GL)
else()
  target_sources(twl PRIVATE twl_null.c)
endif()
