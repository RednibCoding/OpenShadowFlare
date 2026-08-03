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

#include "backend.h"

#include <GL/glext.h>

static const char twl_x11_vertex_shader[] =
  "#version 120\n"
  "varying vec2 texture_position;\n"
  "void main() {\n"
  "  gl_Position = gl_Vertex;\n"
  "  texture_position = gl_MultiTexCoord0.xy;\n"
  "}\n";

static const char twl_x11_fragment_shader[] =
  "#version 120\n"
  "uniform sampler2D framebuffer_texture;\n"
  "uniform float framebuffer_format;\n"
  "varying vec2 texture_position;\n"
  "void main() {\n"
  "  vec4 texel = texture2D(framebuffer_texture, texture_position);\n"
  "  if (framebuffer_format > 1.5) {\n"
  "    gl_FragColor = vec4(texel.rgb, 1.0);\n"
  "    return;\n"
  "  }\n"
  "  float packed_value = floor(texel.r * 65535.0 + 0.5);\n"
  "  float red = mod(packed_value, 32.0) / 31.0;\n"
  "  float green_bits = framebuffer_format > 0.5 ? 64.0 : 32.0;\n"
  "  float green = mod(floor(packed_value / 32.0), green_bits) / (green_bits - 1.0);\n"
  "  float blue_divisor = framebuffer_format > 0.5 ? 2048.0 : 1024.0;\n"
  "  float blue = mod(floor(packed_value / blue_divisor), 32.0) / 31.0;\n"
  "  gl_FragColor = vec4(red, green, blue, 1.0);\n"
  "}\n";

static GLuint twl_x11_compile_shader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  GLint compiled = GL_FALSE;
  if (shader == 0u) {
    return 0u;
  }
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled != GL_TRUE) {
    glDeleteShader(shader);
    return 0u;
  }
  return shader;
}

GLuint twl_x11_create_program(void) {
  GLuint vertex = twl_x11_compile_shader(
    GL_VERTEX_SHADER, twl_x11_vertex_shader);
  GLuint fragment = twl_x11_compile_shader(
    GL_FRAGMENT_SHADER, twl_x11_fragment_shader);
  GLuint program;
  GLint linked = GL_FALSE;
  if (vertex == 0u || fragment == 0u) {
    if (vertex != 0u) glDeleteShader(vertex);
    if (fragment != 0u) glDeleteShader(fragment);
    return 0u;
  }
  program = glCreateProgram();
  glAttachShader(program, vertex);
  glAttachShader(program, fragment);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (linked != GL_TRUE) {
    glDeleteProgram(program);
    return 0u;
  }
  return program;
}

TwlResult twl_x11_presentation_init(TwlX11 *x11) {
  x11->program = twl_x11_create_program();
  if (x11->program == 0u) return TWL_RESULT_BACKEND_FAILURE;
  x11->texture_uniform =
    glGetUniformLocation(x11->program, "framebuffer_texture");
  x11->format_uniform =
    glGetUniformLocation(x11->program, "framebuffer_format");
  glGenTextures(1, &x11->texture);
  glBindTexture(GL_TEXTURE_2D, x11->texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  return TWL_RESULT_OK;
}

void twl_x11_presentation_shutdown(TwlX11 *x11) {
  if (!x11 || !x11->context) return;
  if (x11->texture != 0u) glDeleteTextures(1, &x11->texture);
  if (x11->program != 0u) glDeleteProgram(x11->program);
  x11->texture = 0u;
  x11->program = 0u;
}

TwlResult twl_backend_prepare_frame(Twl *twl, const TwlSurface *surface) {
  TwlX11 *x11 = twl ? (TwlX11 *) twl->backend : NULL;
  GLenum internal_format;
  GLenum source_format;
  GLenum source_type;
  size_t pixel_size;
  if (!x11 || !x11->display || !surface ||
      surface->stride_bytes > (size_t) INT32_MAX) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  pixel_size = surface->format == TWL_PIXEL_XRGB8888 ? 4u : 2u;
  if (surface->stride_bytes % pixel_size != 0u) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  if (surface->format == TWL_PIXEL_XRGB8888) {
    internal_format = GL_RGB8;
    source_format = GL_BGRA;
    source_type = GL_UNSIGNED_INT_8_8_8_8_REV;
  } else {
    internal_format = GL_LUMINANCE16;
    source_format = GL_LUMINANCE;
    source_type = GL_UNSIGNED_SHORT;
  }
  glBindTexture(GL_TEXTURE_2D, x11->texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) pixel_size);
  glPixelStorei(
    GL_UNPACK_ROW_LENGTH, (GLint) (surface->stride_bytes / pixel_size));
  if (x11->texture_width != surface->width ||
      x11->texture_height != surface->height ||
      x11->texture_format != surface->format) {
    glTexImage2D(
      GL_TEXTURE_2D, 0, (GLint) internal_format,
      (GLsizei) surface->width, (GLsizei) surface->height,
      0, source_format, source_type, surface->pixels);
    x11->texture_width = surface->width;
    x11->texture_height = surface->height;
    x11->texture_format = surface->format;
  } else {
    glTexSubImage2D(
      GL_TEXTURE_2D, 0, 0, 0,
      (GLsizei) surface->width, (GLsizei) surface->height,
      source_format, source_type, surface->pixels);
  }
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glViewport(
    0, 0, (GLsizei) twl->display_width, (GLsizei) twl->display_height);
  glUseProgram(x11->program);
  glUniform1i(x11->texture_uniform, 0);
  glUniform1f(x11->format_uniform, (GLfloat) surface->format);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f);
  glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f);
  glEnd();
  return TWL_RESULT_OK;
}

TwlResult twl_backend_display_frame(Twl *twl) {
  TwlX11 *x11 = twl ? (TwlX11 *) twl->backend : NULL;
  if (!x11 || !x11->display) return TWL_RESULT_INVALID_ARGUMENT;
  glXSwapBuffers(x11->display, x11->window);
  return TWL_RESULT_OK;
}
