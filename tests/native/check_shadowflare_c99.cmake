# Copyright (C) 2026 Michael Binder and contributors
#
# This file is part of OpenShadowFlare.
#
# OpenShadowFlare is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenShadowFlare is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# details.
#
# You should have received a copy of the GNU General Public License along with
# OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.

if(NOT DEFINED PROJECT_ROOT)
  message(FATAL_ERROR "PROJECT_ROOT was not provided")
endif()

file(GLOB_RECURSE shadowflare_sources
  "${PROJECT_ROOT}/shadowflare/*.c"
  "${PROJECT_ROOT}/shadowflare/*.h")
file(GLOB_RECURSE shadowflare_cpp
  "${PROJECT_ROOT}/shadowflare/*.cc"
  "${PROJECT_ROOT}/shadowflare/*.cpp"
  "${PROJECT_ROOT}/shadowflare/*.cxx"
  "${PROJECT_ROOT}/shadowflare/*.hpp")
if(shadowflare_cpp)
  message(FATAL_ERROR "C++ source found in the C99 game: ${shadowflare_cpp}")
endif()

foreach(source IN LISTS shadowflare_sources)
  file(RELATIVE_PATH relative_source
    "${PROJECT_ROOT}/shadowflare" "${source}")
  file(READ "${source}" contents)
  if(contents MATCHES "#include[ \t]*[<\"](windows|X11|emscripten|mach|objc)/?[^>\"]*[>\"]")
    message(FATAL_ERROR "Platform header leaked into ${source}")
  endif()
  if(contents MATCHES "(^|[^A-Za-z_])(malloc|calloc|realloc|free)[ \t\r\n]*\\(")
    message(FATAL_ERROR "Heap allocation found in ${source}")
  endif()
  if(contents MATCHES "(^|[^A-Za-z_])(lwl|lal)_[A-Za-z0-9_]*[ \t\r\n]*\\(")
    message(FATAL_ERROR "Legacy LWL/LAL dependency found in ${source}")
  endif()
  if(contents MATCHES "#include[ \t]*[<\"]src/SF_EXE/")
    message(FATAL_ERROR "SF_EXE implementation leaked into ${source}")
  endif()
  if(NOT relative_source MATCHES "^runtime/" AND
      contents MATCHES "#include[ \t]*[<\"](twl|tal)\\.h[>\"]")
    message(FATAL_ERROR "TWL/TAL escaped the runtime boundary: ${source}")
  endif()
  if(relative_source MATCHES "^core/" AND
      contents MATCHES "#include[ \t]*\"(game|render|runtime)/")
    message(FATAL_ERROR "Core depends on a higher layer: ${source}")
  endif()
  if(relative_source MATCHES "^game/" AND
      contents MATCHES "#include[ \t]*\"(render|runtime)/")
    message(FATAL_ERROR "Game rules depend on rendering/runtime: ${source}")
  endif()
  if(relative_source MATCHES "^render/" AND
      contents MATCHES "#include[ \t]*\"runtime/")
    message(FATAL_ERROR "Rendering depends on runtime integration: ${source}")
  endif()
  if(relative_source MATCHES "^screens/" AND
      contents MATCHES "#include[ \t]*\"runtime/")
    message(FATAL_ERROR "A screen depends on runtime integration: ${source}")
  endif()
  if(contents MATCHES "(^|[^A-Za-z_])(_Alignof|_Static_assert|alignas)[^A-Za-z_]")
    message(FATAL_ERROR "Post-C99 language feature found in ${source}")
  endif()
  if(contents MATCHES "(^|[^A-Za-z0-9_])(float|double)([^A-Za-z0-9_]|$)")
    message(FATAL_ERROR "Floating-point type found in ${source}")
  endif()
endforeach()
