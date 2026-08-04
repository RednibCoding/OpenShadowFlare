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
 */

#include "backend.h"

#include <stdint.h>

TwlResult twl_backend_prepare_frame(Twl *twl, const TwlSurface *surface) {
  TwlAndroid *android = twl ? (TwlAndroid *) twl->backend : NULL;
  const uint16_t *src;
  size_t src_stride;
  uint32_t y;
  uint32_t x;
  if (!android || !surface || !surface->pixels) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  if (surface->format != TWL_PIXEL_RGB555 ||
      surface->width != android->frame_width ||
      surface->height != android->frame_height) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  if (!android->window_ready || !android->gl_ready) {
    return TWL_RESULT_OK;
  }

  src = (const uint16_t *) surface->pixels;
  src_stride = surface->stride_bytes / sizeof(uint16_t);
  for (y = 0; y < android->frame_height; ++y) {
    const uint16_t *source_row = src + (size_t) y * src_stride;
    uint16_t *dest_row = android->staging + (size_t) y * android->frame_width;
    for (x = 0; x < android->frame_width; ++x) {
      dest_row[x] = (uint16_t) ((source_row[x] << 1) | 1u);
    }
  }

  glBindTexture(GL_TEXTURE_2D, android->texture);
  glTexSubImage2D(
    GL_TEXTURE_2D, 0, 0, 0, (GLsizei) android->frame_width,
    (GLsizei) android->frame_height, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1,
    android->staging);
  return TWL_RESULT_OK;
}

TwlResult twl_backend_display_frame(Twl *twl) {
  TwlAndroid *android = twl ? (TwlAndroid *) twl->backend : NULL;
  if (!android) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  if (!android->window_ready || !android->gl_ready) {
    return TWL_RESULT_OK;
  }

  glViewport(0, 0, android->surface_width, android->surface_height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glViewport(
    android->view_x, android->view_y, android->view_w, android->view_h);
  glUseProgram(android->program);
  glBindBuffer(GL_ARRAY_BUFFER, android->vbo);
  glEnableVertexAttribArray((GLuint) android->attr_pos);
  glVertexAttribPointer(
    (GLuint) android->attr_pos, 2, GL_FLOAT, GL_FALSE,
    (GLsizei) (4 * sizeof(GLfloat)), (const void *) 0);
  glEnableVertexAttribArray((GLuint) android->attr_uv);
  glVertexAttribPointer(
    (GLuint) android->attr_uv, 2, GL_FLOAT, GL_FALSE,
    (GLsizei) (4 * sizeof(GLfloat)),
    (const void *) (2 * sizeof(GLfloat)));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, android->texture);
  glUniform1i(android->uniform_tex, 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  eglSwapBuffers(android->display, android->surface);
  return TWL_RESULT_OK;
}
