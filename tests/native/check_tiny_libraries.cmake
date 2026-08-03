# Copyright (C) 2026 Michael Binder and contributors
#
# This file is part of OpenShadowFlare.
#
# OpenShadowFlare is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
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

file(GLOB_RECURSE tiny_sources
  "${PROJECT_ROOT}/thirdparty/twl/*.c"
  "${PROJECT_ROOT}/thirdparty/twl/*.h"
  "${PROJECT_ROOT}/thirdparty/tal/*.c"
  "${PROJECT_ROOT}/thirdparty/tal/*.h")

foreach(source IN LISTS tiny_sources)
  file(READ "${source}" contents)
  if(contents MATCHES "(^|[^A-Za-z_])(malloc|calloc|realloc|free)[ \t\r\n]*\\(")
    message(FATAL_ERROR "Allocator call found in ${source}")
  endif()
  if(contents MATCHES "(^|[^A-Za-z_])(lwl|lal)_[A-Za-z0-9_]*[ \t\r\n]*\\(")
    message(FATAL_ERROR "LWL/LAL dependency found in ${source}")
  endif()
endforeach()

foreach(common_source
    "${PROJECT_ROOT}/thirdparty/twl/twl.c"
    "${PROJECT_ROOT}/thirdparty/tal/tal.c")
  file(READ "${common_source}" contents)
  if(contents MATCHES "#include[ \t]*[<\"](stdio|stdlib|string|math)\\.h[>\"]")
    message(FATAL_ERROR "Hosted libc dependency found in ${common_source}")
  endif()
endforeach()
