# Copyright (C) 2026 Michael Binder and contributors
#
# This file is part of LWL.
#
# LWL is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# LWL is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
#
# You should have received a copy of the GNU General Public License along with
# LWL. If not, see <https://www.gnu.org/licenses/>.

add_library(lwl STATIC)
add_library(Lwl::Lwl ALIAS lwl)

target_include_directories(
  lwl
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_features(lwl PUBLIC c_std_11)
set_target_properties(lwl PROPERTIES C_EXTENSIONS OFF)

if(MSVC)
  target_compile_options(lwl PRIVATE /W4)
else()
  target_compile_options(lwl PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EMSCRIPTEN)
  target_sources(lwl PRIVATE lwl_emscripten.c)
  target_link_options(
    lwl
    PUBLIC
      "-sMAX_WEBGL_VERSION=2"
      "-sGL_ENABLE_GET_PROC_ADDRESS=1"
  )
elseif(NINTENDO_3DS)
  target_sources(lwl PRIVATE lwl_n3ds.c)
elseif(WIN32)
  target_sources(lwl PRIVATE lwl_win32.c)
  target_link_libraries(
    lwl
    PRIVATE
      user32
      gdi32
      shell32
      ole32
      opengl32
  )
elseif(APPLE)
  target_sources(lwl PRIVATE lwl_macos.c)
  find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
  find_library(COREGRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
  find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
  target_link_libraries(
    lwl
    PRIVATE
      "${APPKIT_FRAMEWORK}"
      "${COREGRAPHICS_FRAMEWORK}"
      "${OPENGL_FRAMEWORK}"
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_sources(lwl PRIVATE lwl_x11.c)
  find_package(OpenGL REQUIRED)
  find_package(X11 REQUIRED)
  target_compile_definitions(
    lwl
    PRIVATE
      LWL_HAVE_XSHM
      LWL_HAVE_XINERAMA
  )
  target_link_libraries(
    lwl
    PRIVATE
      OpenGL::GL
      X11::X11
      X11::Xcursor
      X11::Xext
      X11::Xinerama
      "${CMAKE_DL_LIBS}"
  )
else()
  message(FATAL_ERROR "LWL has no backend for this platform")
endif()
