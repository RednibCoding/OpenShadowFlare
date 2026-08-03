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

function(sf_configure_c99_target target)
  target_compile_features(${target} PUBLIC c_std_99)
  target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
  set_target_properties(
    ${target}
    PROPERTIES
      C_EXTENSIONS OFF
      C_STANDARD 99
      C_STANDARD_REQUIRED ON
  )
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

add_library(
  shadowflare_core STATIC
  assets/character_create_assets.c
  assets/menu_assets.c
  assets/retail_paths.c
  assets/title_assets.c
  core/arena.c
  data/caf.c
  data/njp.c
  data/rclib.c
  data/voc.c
  game/character_create.c
  game/game.c
  game/title.c
  render/dirty.c
  render/framebuffer.c
  render/renderer.c
  screens/character_create_screen.c
  screens/title_screen.c
)
add_library(ShadowFlare::Core ALIAS shadowflare_core)
sf_configure_c99_target(shadowflare_core)

add_executable(
  osf
  main.c
  runtime/application.c
  runtime/screen_runtime.c
)
add_executable(ShadowFlare::Game ALIAS osf)
sf_configure_c99_target(osf)
target_link_libraries(
  osf
  PRIVATE
    ShadowFlare::Core
    Tal::Tal
    Twl::Twl
)
