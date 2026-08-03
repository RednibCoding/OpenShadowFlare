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

static GLuint twl_macos_compile_shader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  GLint compiled = GL_FALSE;
  if (shader == 0u) return 0u;
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled != GL_TRUE) {
    glDeleteShader(shader);
    return 0u;
  }
  return shader;
}

GLuint twl_macos_create_program(void) {
  static const char vertex_source[] =
    "#version 120\n"
    "varying vec2 texture_position;\n"
    "void main() {\n"
    "  gl_Position = gl_Vertex;\n"
    "  texture_position = gl_MultiTexCoord0.xy;\n"
    "}\n";
  static const char fragment_source[] =
    "#version 120\n"
    "uniform sampler2D framebuffer_texture;\n"
    "uniform float framebuffer_format;\n"
    "varying vec2 texture_position;\n"
    "void main() {\n"
    "  vec4 texel = texture2D(framebuffer_texture, texture_position);\n"
    "  if (framebuffer_format > 1.5) {\n"
    "    gl_FragColor = vec4(texel.rgb, 1.0); return;\n"
    "  }\n"
    "  float p = floor(texel.r * 65535.0 + 0.5);\n"
    "  float g_bits = framebuffer_format > 0.5 ? 64.0 : 32.0;\n"
    "  float b_div = framebuffer_format > 0.5 ? 2048.0 : 1024.0;\n"
    "  gl_FragColor = vec4(mod(p, 32.0) / 31.0,\n"
    "    mod(floor(p / 32.0), g_bits) / (g_bits - 1.0),\n"
    "    mod(floor(p / b_div), 32.0) / 31.0, 1.0);\n"
    "}\n";
  GLuint vertex = twl_macos_compile_shader(GL_VERTEX_SHADER, vertex_source);
  GLuint fragment = twl_macos_compile_shader(
    GL_FRAGMENT_SHADER, fragment_source);
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

TwlResult twl_macos_presentation_init(TwlMacos *macos) {
  macos->program = twl_macos_create_program();
  if (macos->program == 0u) return TWL_RESULT_BACKEND_FAILURE;
  macos->texture_uniform = glGetUniformLocation(
    macos->program, "framebuffer_texture");
  macos->format_uniform = glGetUniformLocation(
    macos->program, "framebuffer_format");
  glGenTextures(1, &macos->texture);
  glBindTexture(GL_TEXTURE_2D, macos->texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  return TWL_RESULT_OK;
}

void twl_macos_presentation_shutdown(TwlMacos *macos) {
  if (!macos || !macos->context) return;
  twl_msg_void(macos->context, "makeCurrentContext");
  if (macos->texture != 0u) glDeleteTextures(1, &macos->texture);
  if (macos->program != 0u) glDeleteProgram(macos->program);
  macos->texture = 0u;
  macos->program = 0u;
}

TwlResult twl_backend_present(Twl *twl, const TwlSurface *surface) {
  TwlMacos *macos = twl ? (TwlMacos *) twl->backend : NULL;
  GLenum internal_format;
  GLenum source_format;
  GLenum source_type;
  size_t pixel_size;
  if (!macos || !macos->context || !surface ||
      surface->stride_bytes > (size_t) INT32_MAX) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  pixel_size = surface->format == TWL_PIXEL_XRGB8888 ? 4u : 2u;
  if (surface->stride_bytes % pixel_size != 0u)
    return TWL_RESULT_INVALID_ARGUMENT;
  twl_msg_void(macos->context, "makeCurrentContext");
  if (surface->format == TWL_PIXEL_XRGB8888) {
    internal_format = GL_RGB8;
    source_format = GL_BGRA;
    source_type = GL_UNSIGNED_INT_8_8_8_8_REV;
  } else {
    internal_format = GL_LUMINANCE16;
    source_format = GL_LUMINANCE;
    source_type = GL_UNSIGNED_SHORT;
  }
  glBindTexture(GL_TEXTURE_2D, macos->texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) pixel_size);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,
    (GLint) (surface->stride_bytes / pixel_size));
  if (macos->texture_width != surface->width ||
      macos->texture_height != surface->height ||
      macos->texture_format != surface->format) {
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint) internal_format,
      (GLsizei) surface->width, (GLsizei) surface->height, 0,
      source_format, source_type, surface->pixels);
    macos->texture_width = surface->width;
    macos->texture_height = surface->height;
    macos->texture_format = surface->format;
  } else {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
      (GLsizei) surface->width, (GLsizei) surface->height,
      source_format, source_type, surface->pixels);
  }
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glViewport(0, 0,
    (GLsizei) ((double) twl->display_width * macos->backing_scale),
    (GLsizei) ((double) twl->display_height * macos->backing_scale));
  glUseProgram(macos->program);
  glUniform1i(macos->texture_uniform, 0);
  glUniform1f(macos->format_uniform, (GLfloat) surface->format);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, -1.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 1.0f);
  glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, 1.0f);
  glEnd();
  twl_msg_void(macos->context, "flushBuffer");
  return TWL_RESULT_OK;
}
