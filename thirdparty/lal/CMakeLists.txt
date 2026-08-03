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
  lal_convert.c
)
add_library(Lal::Lal ALIAS lal)

set(
  LAL_MAXIMUM_SAMPLE_RATE
  "44100"
  CACHE STRING
  "Highest sample rate retained by LAL (11025, 12000, 16000, 22050, 44100, or 48000)"
)
set_property(
  CACHE LAL_MAXIMUM_SAMPLE_RATE
  PROPERTY STRINGS 11025 12000 16000 22050 44100 48000
)
if(NOT LAL_MAXIMUM_SAMPLE_RATE MATCHES "^(11025|12000|16000|22050|44100|48000)$")
  message(FATAL_ERROR
    "LAL_MAXIMUM_SAMPLE_RATE must be 11025, 12000, 16000, 22050, 44100, or 48000")
endif()
option(
  LAL_FORCE_MONO
  "Downmix retained LAL sounds to mono"
  OFF
)
if(LAL_FORCE_MONO)
  set(LAL_DEFAULT_FORCE_MONO_VALUE 1)
else()
  set(LAL_DEFAULT_FORCE_MONO_VALUE 0)
endif()

target_include_directories(
  lal
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_features(lal PUBLIC c_std_99)
if(NOT MSVC)
  target_link_libraries(lal PRIVATE m)
endif()
target_compile_definitions(
  lal
  PRIVATE
    LAL_DEFAULT_MAXIMUM_SAMPLE_RATE=${LAL_MAXIMUM_SAMPLE_RATE}
    LAL_DEFAULT_FORCE_MONO=${LAL_DEFAULT_FORCE_MONO_VALUE}
)
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
elseif(PSP)
  target_sources(lal PRIVATE lal_psp.c)
  # The PSP hardware output is fixed at 44.1 kHz. The port retains audio at
  # 11.025 kHz, then expands each mixed frame for the hardware output. Its
  # startup sound bank contains 160 clips, so use the bounded integer
  # resampler rather than the desktop sinc filter before the first frame.
  target_compile_definitions(
    lal
    PRIVATE
      LAL_OUTPUT_SAMPLE_RATE=11025
      LAL_USE_LINEAR_RESAMPLER=1
  )
elseif(NINTENDO_SWITCH)
  target_sources(lal PRIVATE lal_switch.c)
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
