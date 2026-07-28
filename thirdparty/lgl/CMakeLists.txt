# Copyright (C) 2026 Michael Binder and contributors
#
# This file is part of LGL.
#
# LGL is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# LGL is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
#
# You should have received a copy of the GNU General Public License along with
# LGL. If not, see <https://www.gnu.org/licenses/>.

add_library(lgl STATIC lgl.c)
add_library(Lgl::Lgl ALIAS lgl)

target_include_directories(
  lgl
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_features(lgl PUBLIC c_std_99)
set_target_properties(lgl PROPERTIES C_EXTENSIONS OFF)

if(MSVC)
  target_compile_options(lgl PRIVATE /W4)
else()
  target_compile_options(lgl PRIVATE -Wall -Wextra -Wpedantic)
endif()
