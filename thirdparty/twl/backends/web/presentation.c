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

EM_JS(int, twl_web_prepare_canvas,
      (const char *target, const char *title, int width, int height), {
  var selector = UTF8ToString(target);
  var canvas = document.querySelector(selector);
  if (!canvas) return 0;
  canvas.width = width;
  canvas.height = height;
  canvas.oncontextmenu = function(event) { event.preventDefault(); };
  if (title) document.title = UTF8ToString(title);
  var gl = canvas.getContext('webgl2', {
    alpha: false, antialias: false, depth: false, stencil: false
  });
  if (!gl) return 0;
  var compile = function(type, source) {
    var shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) return null;
    return shader;
  };
  var vertex = compile(gl.VERTEX_SHADER,
    '#version 300 es\n' +
    'out vec2 texture_position;\n' +
    'void main() {\n' +
    '  vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));\n' +
    '  texture_position = vec2(p.x * 0.5, 1.0 - p.y * 0.5);\n' +
    '  gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);\n' +
    '}\n');
  var fragment = compile(gl.FRAGMENT_SHADER,
    '#version 300 es\n' +
    'precision highp float;\n' +
    'precision highp usampler2D;\n' +
    'uniform usampler2D framebuffer_texture;\n' +
    'uniform int framebuffer_format;\n' +
    'in vec2 texture_position;\n' +
    'out vec4 output_color;\n' +
    'void main() {\n' +
    '  uint p = texture(framebuffer_texture, texture_position).r;\n' +
    '  if (framebuffer_format == 2) {\n' +
    '    output_color = vec4(float((p >> 16u) & 255u) / 255.0, ' +
    '      float((p >> 8u) & 255u) / 255.0, float(p & 255u) / 255.0, 1.0);\n' +
    '  } else {\n' +
    '    uint green_mask = framebuffer_format == 1 ? 63u : 31u;\n' +
    '    uint blue_shift = framebuffer_format == 1 ? 11u : 10u;\n' +
    '    output_color = vec4(float(p & 31u) / 31.0, ' +
    '      float((p >> 5u) & green_mask) / float(green_mask), ' +
    '      float((p >> blue_shift) & 31u) / 31.0, 1.0);\n' +
    '  }\n' +
    '}\n');
  if (!vertex || !fragment) {
    if (vertex) gl.deleteShader(vertex);
    if (fragment) gl.deleteShader(fragment);
    return 0;
  }
  var program = gl.createProgram();
  gl.attachShader(program, vertex);
  gl.attachShader(program, fragment);
  gl.linkProgram(program);
  gl.deleteShader(vertex);
  gl.deleteShader(fragment);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
    gl.deleteProgram(program);
    return 0;
  }
  var texture = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, texture);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  var presenter = {
    gl: gl, program: program, texture: texture,
    textureUniform: gl.getUniformLocation(program, 'framebuffer_texture'),
    formatUniform: gl.getUniformLocation(program, 'framebuffer_format'),
    format: -1, width: 0, height: 0
  };
  canvas.twlPresenter = presenter;
  if (!Module.twlPresenters) Module.twlPresenters = [];
  var slot = Module.twlPresenters.indexOf(null);
  if (slot < 0) slot = Module.twlPresenters.length;
  Module.twlPresenters[slot] = presenter;
  return slot + 1;
})

EM_JS(int, twl_web_present_pixels,
      (int presenter_handle, const void *pixels, int width, int height,
       int stride, int format), {
  var presenters = Module.twlPresenters;
  var presenter = presenters ? presenters[presenter_handle - 1] : null;
  if (!presenter) return 0;
  var gl = presenter.gl;
  var canvas = gl.canvas;
  var pixelSize = format === 2 ? 4 : 2;
  var internalFormat = format === 2 ? gl.R32UI : gl.R16UI;
  var type = format === 2 ? gl.UNSIGNED_INT : gl.UNSIGNED_SHORT;
  var data = format === 2 ? HEAPU32 : HEAPU16;
  var sourceOffset = pixels / pixelSize;
  gl.bindTexture(gl.TEXTURE_2D, presenter.texture);
  gl.pixelStorei(gl.UNPACK_ALIGNMENT, pixelSize);
  gl.pixelStorei(gl.UNPACK_ROW_LENGTH, stride / pixelSize);
  if (presenter.format !== format || presenter.width !== width ||
      presenter.height !== height) {
    gl.texImage2D(gl.TEXTURE_2D, 0, internalFormat, width, height, 0,
      gl.RED_INTEGER, type, data, sourceOffset);
    presenter.format = format;
    presenter.width = width;
    presenter.height = height;
  } else {
    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, width, height,
      gl.RED_INTEGER, type, data, sourceOffset);
  }
  gl.pixelStorei(gl.UNPACK_ROW_LENGTH, 0);
  gl.viewport(0, 0, canvas.width, canvas.height);
  gl.useProgram(presenter.program);
  gl.uniform1i(presenter.textureUniform, 0);
  gl.uniform1i(presenter.formatUniform, format);
  gl.drawArrays(gl.TRIANGLES, 0, 3);
  return 1;
})

EM_JS(void, twl_web_release_canvas, (int presenter_handle), {
  var presenters = Module.twlPresenters;
  var presenter = presenters ? presenters[presenter_handle - 1] : null;
  if (!presenter) return;
  presenter.gl.deleteTexture(presenter.texture);
  presenter.gl.deleteProgram(presenter.program);
  if (presenter.gl.canvas.twlPresenter === presenter)
    presenter.gl.canvas.twlPresenter = null;
  presenters[presenter_handle - 1] = null;
})

TwlResult twl_backend_prepare_frame(Twl *twl, const TwlSurface *surface) {
  TwlWeb *web = twl ? (TwlWeb *) twl->backend : NULL;
  if (!web || !surface || surface->width != twl->config.width ||
      surface->height != twl->config.height ||
      surface->stride_bytes > (size_t) INT32_MAX) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  return twl_web_present_pixels(
           web->presenter, surface->pixels,
           (int) surface->width, (int) surface->height,
           (int) surface->stride_bytes, (int) surface->format)
           ? TWL_RESULT_OK
           : TWL_RESULT_BACKEND_FAILURE;
}

TwlResult twl_backend_display_frame(Twl *twl) {
  TwlWeb *web = twl ? (TwlWeb *) twl->backend : NULL;
  return web && web->presenter != 0
    ? TWL_RESULT_OK : TWL_RESULT_INVALID_ARGUMENT;
}
