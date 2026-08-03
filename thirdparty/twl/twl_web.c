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

#include "twl_internal.h"

#include <emscripten.h>
#include <emscripten/html5.h>

typedef struct {
  Twl *twl;
  const char *target;
  int presenter;
} TwlWeb;

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

EM_JS(void, twl_web_poll_gamepads, (Twl *twl, int capacity), {
  var pads = navigator.getGamepads ? navigator.getGamepads() : [];
  var buttonMap = [0, 1, 2, 3, 4, 5, -1, -1, 6, 7, 9, 10, 11, 12, 13, 14, 8];
  for (var index = 0; index < capacity; ++index) {
    var pad = pads[index];
    _twl_web_controller_connected(twl, index, pad ? 1 : 0);
    if (!pad) continue;
    for (var nativeButton = 0; nativeButton < buttonMap.length; ++nativeButton) {
      var mapped = buttonMap[nativeButton];
      if (mapped >= 0 && nativeButton < pad.buttons.length)
        _twl_web_controller_button(
          twl, index, mapped, pad.buttons[nativeButton].pressed ? 1 : 0);
    }
    for (var axis = 0; axis < 4; ++axis) {
      var value = axis < pad.axes.length ? pad.axes[axis] : 0;
      _twl_web_controller_axis(twl, index, axis,
        Math.max(-32767, Math.min(32767, Math.round(value * 32767))));
    }
    var leftTrigger = pad.buttons.length > 6 ? pad.buttons[6].value : 0;
    var rightTrigger = pad.buttons.length > 7 ? pad.buttons[7].value : 0;
    _twl_web_controller_axis(twl, index, 4, Math.round(leftTrigger * 32767));
    _twl_web_controller_axis(twl, index, 5, Math.round(rightTrigger * 32767));
  }
})

EMSCRIPTEN_KEEPALIVE
void twl_web_controller_connected(Twl *twl, int index, int connected) {
  if (index >= 0)
    twl_internal_set_controller_connected(
      twl, (uint32_t) index, connected != 0);
}

EMSCRIPTEN_KEEPALIVE
void twl_web_controller_button(Twl *twl, int index, int button, int pressed) {
  if (index >= 0 && button >= 0)
    twl_internal_set_controller_button(
      twl, (uint32_t) index, (TwlControllerButton) button, pressed != 0);
}

EMSCRIPTEN_KEEPALIVE
void twl_web_controller_axis(Twl *twl, int index, int axis, int value) {
  if (index >= 0 && axis >= 0)
    twl_internal_set_controller_axis(
      twl, (uint32_t) index, (TwlControllerAxis) axis, (int16_t) value);
}

static TwlKey twl_web_key(unsigned long key_code) {
  if (key_code >= 48u && key_code <= 57u) {
    return (TwlKey) (TWL_KEY_0 + key_code - 48u);
  }
  if (key_code >= 65u && key_code <= 90u) {
    return (TwlKey) (TWL_KEY_A + key_code - 65u);
  }
  switch (key_code) {
    case 8u: return TWL_KEY_BACKSPACE;
    case 9u: return TWL_KEY_TAB;
    case 13u: return TWL_KEY_RETURN;
    case 27u: return TWL_KEY_ESCAPE;
    case 32u: return TWL_KEY_SPACE;
    case 33u: return TWL_KEY_PAGE_UP;
    case 34u: return TWL_KEY_PAGE_DOWN;
    case 35u: return TWL_KEY_END;
    case 36u: return TWL_KEY_HOME;
    case 37u: return TWL_KEY_LEFT;
    case 38u: return TWL_KEY_UP;
    case 39u: return TWL_KEY_RIGHT;
    case 40u: return TWL_KEY_DOWN;
    case 45u: return TWL_KEY_INSERT;
    case 46u: return TWL_KEY_DELETE;
    case 112u: return TWL_KEY_F1;
    case 113u: return TWL_KEY_F2;
    case 114u: return TWL_KEY_F3;
    case 115u: return TWL_KEY_F4;
    case 116u: return TWL_KEY_F5;
    case 117u: return TWL_KEY_F6;
    case 118u: return TWL_KEY_F7;
    case 119u: return TWL_KEY_F8;
    case 120u: return TWL_KEY_F9;
    case 121u: return TWL_KEY_F10;
    case 122u: return TWL_KEY_F11;
    case 123u: return TWL_KEY_F12;
    default: return TWL_KEY_UNKNOWN;
  }
}

static EM_BOOL twl_web_mouse(
    int event_type, const EmscriptenMouseEvent *mouse, void *user_data) {
  TwlWeb *web = (TwlWeb *) user_data;
  TwlEvent event = {0};
  if (!web || !web->twl) {
    return EM_FALSE;
  }
  event.timestamp_us = twl_backend_time_microseconds(web->twl);
  event.x = mouse->targetX;
  event.y = mouse->targetY;
  event.dx = mouse->movementX;
  event.dy = mouse->movementY;
  event.button = (uint8_t) (mouse->button + 1u);
  if (event_type == EMSCRIPTEN_EVENT_MOUSEDOWN) {
    event.type = TWL_EVENT_POINTER_DOWN;
  } else if (event_type == EMSCRIPTEN_EVENT_MOUSEUP) {
    event.type = TWL_EVENT_POINTER_UP;
  } else {
    event.type = TWL_EVENT_POINTER_MOVE;
  }
  twl_internal_push_event(web->twl, &event);
  return EM_TRUE;
}

static EM_BOOL twl_web_wheel(
    int event_type, const EmscriptenWheelEvent *wheel, void *user_data) {
  TwlWeb *web = (TwlWeb *) user_data;
  TwlEvent event = {0};
  (void) event_type;
  if (!web || !web->twl) {
    return EM_FALSE;
  }
  event.type = TWL_EVENT_POINTER_WHEEL;
  event.timestamp_us = twl_backend_time_microseconds(web->twl);
  event.x = wheel->mouse.targetX;
  event.y = wheel->mouse.targetY;
  event.dy = wheel->deltaY < 0.0 ? 1 : (wheel->deltaY > 0.0 ? -1 : 0);
  twl_internal_push_event(web->twl, &event);
  return EM_TRUE;
}

static EM_BOOL twl_web_keyboard(
    int event_type, const EmscriptenKeyboardEvent *key, void *user_data) {
  TwlWeb *web = (TwlWeb *) user_data;
  TwlEvent event = {0};
  if (!web || !web->twl) {
    return EM_FALSE;
  }
  event.type = event_type == EMSCRIPTEN_EVENT_KEYDOWN
    ? TWL_EVENT_KEY_DOWN : TWL_EVENT_KEY_UP;
  event.timestamp_us = twl_backend_time_microseconds(web->twl);
  event.key = twl_web_key(key->keyCode);
  event.repeat = key->repeat;
  twl_internal_push_event(web->twl, &event);
  if (event_type == EMSCRIPTEN_EVENT_KEYDOWN &&
      key->key[0] >= 0x20 && key->key[0] <= 0x7e && key->key[1] == '\0') {
    event.type = TWL_EVENT_TEXT;
    event.codepoint = (uint8_t) key->key[0];
    twl_internal_push_event(web->twl, &event);
  }
  return EM_TRUE;
}

size_t twl_backend_memory_alignment(void) {
  return _Alignof(TwlWeb);
}

size_t twl_backend_memory_required(const TwlConfig *config) {
  (void) config;
  return sizeof(TwlWeb);
}

TwlResult twl_backend_init(
    Twl *twl, void *memory, size_t memory_size, const TwlConfig *config) {
  TwlWeb *web;
  if (!twl || !memory || memory_size < sizeof(TwlWeb) ||
      !config->display_target) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  web = (TwlWeb *) memory;
  web->twl = twl;
  web->target = config->display_target;
  web->presenter = twl_web_prepare_canvas(
    web->target, config->title,
    (int) config->width, (int) config->height);
  if (web->presenter == 0) {
    return TWL_RESULT_BACKEND_UNAVAILABLE;
  }
  emscripten_set_mousedown_callback(web->target, web, false, twl_web_mouse);
  emscripten_set_mouseup_callback(web->target, web, false, twl_web_mouse);
  emscripten_set_mousemove_callback(web->target, web, false, twl_web_mouse);
  emscripten_set_wheel_callback(web->target, web, false, twl_web_wheel);
  emscripten_set_keydown_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, web, false, twl_web_keyboard);
  emscripten_set_keyup_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, web, false, twl_web_keyboard);
  twl_internal_set_display_size(twl, config->width, config->height);
  return TWL_RESULT_OK;
}

void twl_backend_shutdown(Twl *twl) {
  TwlWeb *web = twl ? (TwlWeb *) twl->backend : NULL;
  if (!web) {
    return;
  }
  emscripten_set_mousedown_callback(web->target, NULL, false, NULL);
  emscripten_set_mouseup_callback(web->target, NULL, false, NULL);
  emscripten_set_mousemove_callback(web->target, NULL, false, NULL);
  emscripten_set_wheel_callback(web->target, NULL, false, NULL);
  emscripten_set_keydown_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, NULL);
  emscripten_set_keyup_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, NULL);
  twl_web_release_canvas(web->presenter);
  web->presenter = 0;
  web->twl = NULL;
}

void twl_backend_pump_events(Twl *twl) {
  if (twl)
    twl_web_poll_gamepads(twl, (int) twl->config.controller_capacity);
}

TwlResult twl_backend_present(Twl *twl, const TwlSurface *surface) {
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

uint64_t twl_backend_time_microseconds(const Twl *twl) {
  (void) twl;
  return (uint64_t) (emscripten_get_now() * 1000.0);
}
