/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of TWL.
 *
 * TWL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TWL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with TWL. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TWL_WIIU_BACKEND_H
#define TWL_WIIU_BACKEND_H

#include "twl_internal.h"

#include <gx2/sampler.h>
#include <gx2/texture.h>
#include <gx2r/buffer.h>
#include <whb/gfx.h>

/* Compiled GFD shader bundled with the .wuhb. See shaders/README.md for how it
 * is produced from shaders/texture_blit.{vsh,psh}. */
#ifndef TWL_WIIU_SHADER_PATH
#define TWL_WIIU_SHADER_PATH "/vol/content/texture_blit.gsh"
#endif

typedef struct {
  WHBGfxShaderGroup shader;
  GX2RBuffer position_buffer;
  GX2RBuffer texcoord_buffer;
  GX2Texture texture;
  GX2Sampler sampler;
  uint32_t frame_width;
  uint32_t frame_height;
  int32_t pointer_x;
  int32_t pointer_y;
  bool touching;
  bool proc_ready;
  bool gfx_ready;
  bool shader_ready;
  bool texture_ready;
  bool vpad_ready;
  bool quit_pushed;
} TwlWiiU;

#endif
