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

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <mach/mach_time.h>
#include <objc/message.h>
#include <objc/runtime.h>

extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *pool);

typedef double TwlCGFloat;
typedef struct { TwlCGFloat x; TwlCGFloat y; } TwlPoint;
typedef struct { TwlCGFloat width; TwlCGFloat height; } TwlSize;
typedef struct { TwlPoint origin; TwlSize size; } TwlRect;
typedef unsigned long TwlNSUInteger;
typedef long TwlNSInteger;

enum {
  TWL_NS_ACTIVATION_POLICY_REGULAR = 0,
  TWL_NS_BACKING_STORE_BUFFERED = 2,
  TWL_NS_STYLE_TITLED = 1u << 0u,
  TWL_NS_STYLE_CLOSABLE = 1u << 1u,
  TWL_NS_STYLE_MINIATURIZABLE = 1u << 2u,
  TWL_NS_STYLE_RESIZABLE = 1u << 3u,
  TWL_NS_EVENT_LEFT_DOWN = 1,
  TWL_NS_EVENT_LEFT_UP = 2,
  TWL_NS_EVENT_RIGHT_DOWN = 3,
  TWL_NS_EVENT_RIGHT_UP = 4,
  TWL_NS_EVENT_MOUSE_MOVED = 5,
  TWL_NS_EVENT_LEFT_DRAGGED = 6,
  TWL_NS_EVENT_RIGHT_DRAGGED = 7,
  TWL_NS_EVENT_KEY_DOWN = 10,
  TWL_NS_EVENT_KEY_UP = 11,
  TWL_NS_EVENT_SCROLL = 22,
  TWL_NS_EVENT_OTHER_DOWN = 25,
  TWL_NS_EVENT_OTHER_UP = 26,
  TWL_NS_EVENT_OTHER_DRAGGED = 27,
  TWL_NS_OPENGL_DOUBLE_BUFFER = 5,
  TWL_NS_OPENGL_COLOR_SIZE = 8,
  TWL_NS_OPENGL_ACCELERATED = 73
};

#define TWL_NS_EVENT_MASK_ANY (~(TwlNSUInteger) 0)

typedef struct {
  Twl *twl;
  id window;
  id view;
  id delegate;
  id context;
  id run_loop_mode;
  GLuint program;
  GLuint texture;
  GLint texture_uniform;
  GLint format_uniform;
  uint32_t texture_width;
  uint32_t texture_height;
  TwlPixelFormat texture_format;
  int32_t mouse_x;
  int32_t mouse_y;
  double backing_scale;
  id *controllers;
  uint32_t controller_count;
} TwlMacos;

static Class twl_macos_delegate_class;
static mach_timebase_info_data_t twl_macos_timebase;

static TwlRect twl_rect(
    TwlCGFloat x, TwlCGFloat y, TwlCGFloat width, TwlCGFloat height) {
  TwlRect result = {{x, y}, {width, height}};
  return result;
}

static id twl_msg_id(id object, const char *selector) {
  return ((id (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector));
}

static id twl_msg_id_id(id object, const char *selector, id argument) {
  return ((id (*)(id, SEL, id)) objc_msgSend)(
    object, sel_registerName(selector), argument);
}

static void twl_msg_void(id object, const char *selector) {
  ((void (*)(id, SEL)) objc_msgSend)(object, sel_registerName(selector));
}

static void twl_msg_void_id(id object, const char *selector, id argument) {
  ((void (*)(id, SEL, id)) objc_msgSend)(
    object, sel_registerName(selector), argument);
}

static void twl_msg_void_bool(id object, const char *selector, bool value) {
  ((void (*)(id, SEL, BOOL)) objc_msgSend)(
    object, sel_registerName(selector), value ? YES : NO);
}

static void twl_msg_void_integer(
    id object, const char *selector, TwlNSInteger value) {
  ((void (*)(id, SEL, TwlNSInteger)) objc_msgSend)(
    object, sel_registerName(selector), value);
}

static bool twl_msg_bool(id object, const char *selector) {
  return object && ((BOOL (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector)) != NO;
}

static TwlNSInteger twl_msg_integer(id object, const char *selector) {
  return ((TwlNSInteger (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector));
}

static TwlNSUInteger twl_msg_count(id object) {
  return object ? ((TwlNSUInteger (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName("count")) : 0u;
}

static id twl_msg_object_at(id object, TwlNSUInteger index) {
  return ((id (*)(id, SEL, TwlNSUInteger)) objc_msgSend)(
    object, sel_registerName("objectAtIndex:"), index);
}

static double twl_msg_double(id object, const char *selector) {
  return object ? ((double (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector)) : 0.0;
}

static float twl_msg_float(id object, const char *selector) {
  return object ? ((float (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector)) : 0.0f;
}

static TwlPoint twl_msg_point(id object, const char *selector) {
  return ((TwlPoint (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector));
}

static TwlRect twl_msg_rect(id object, const char *selector) {
#if defined(__x86_64__)
  TwlRect result;
  ((void (*)(TwlRect *, id, SEL)) objc_msgSend_stret)(
    &result, object, sel_registerName(selector));
  return result;
#else
  return ((TwlRect (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector));
#endif
}

static id twl_ns_string(const char *text) {
  return ((id (*)(id, SEL, const char *)) objc_msgSend)(
    (id) objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"),
    text ? text : "");
}

static const char *twl_ns_utf8(id string) {
  return string ? ((const char *(*)(id, SEL)) objc_msgSend)(
    string, sel_registerName("UTF8String")) : "";
}

static id twl_optional_id(id object, const char *selector) {
  SEL selected = sel_registerName(selector);
  if (!object || !((BOOL (*)(id, SEL, SEL)) objc_msgSend)(
        object, sel_registerName("respondsToSelector:"), selected)) {
    return nil;
  }
  return ((id (*)(id, SEL)) objc_msgSend)(object, selected);
}

static TwlMacos *twl_macos_from_delegate(id delegate) {
  TwlMacos *macos = NULL;
  object_getInstanceVariable(delegate, "_twlMacos", (void **) &macos);
  return macos;
}

static BOOL twl_macos_window_should_close(
    id delegate, SEL selector, id sender) {
  TwlMacos *macos = twl_macos_from_delegate(delegate);
  TwlEvent event;
  (void) selector;
  (void) sender;
  if (macos && macos->twl) {
    twl_internal_zero(&event, sizeof(event));
    event.type = TWL_EVENT_QUIT;
    event.timestamp_us = twl_backend_time_microseconds(macos->twl);
    twl_internal_push_event(macos->twl, &event);
  }
  return NO;
}

static bool twl_macos_prepare_delegate(void) {
  twl_macos_delegate_class = (Class) objc_getClass("TwlWindowDelegate");
  if (twl_macos_delegate_class) return true;
  twl_macos_delegate_class = objc_allocateClassPair(
    (Class) objc_getClass("NSObject"), "TwlWindowDelegate", 0u);
  if (!twl_macos_delegate_class) return false;
  if (!class_addIvar(
        twl_macos_delegate_class, "_twlMacos", sizeof(void *), 3u, "^v") ||
      !class_addMethod(
        twl_macos_delegate_class, sel_registerName("windowShouldClose:"),
        (IMP) twl_macos_window_should_close, "c@:@")) {
    return false;
  }
  objc_registerClassPair(twl_macos_delegate_class);
  return true;
}

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

static GLuint twl_macos_create_program(void) {
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

static id twl_macos_create_context(id view) {
  const uint32_t attributes[] = {
    TWL_NS_OPENGL_ACCELERATED,
    TWL_NS_OPENGL_DOUBLE_BUFFER,
    TWL_NS_OPENGL_COLOR_SIZE, 24u,
    0u
  };
  id pixel_format = twl_msg_id(
    (id) objc_getClass("NSOpenGLPixelFormat"), "alloc");
  id context;
  pixel_format = ((id (*)(id, SEL, const uint32_t *)) objc_msgSend)(
    pixel_format, sel_registerName("initWithAttributes:"), attributes);
  if (!pixel_format) return nil;
  context = twl_msg_id((id) objc_getClass("NSOpenGLContext"), "alloc");
  context = ((id (*)(id, SEL, id, id)) objc_msgSend)(
    context, sel_registerName("initWithFormat:shareContext:"),
    pixel_format, nil);
  twl_msg_void(pixel_format, "release");
  if (context) {
    int interval = 1;
    twl_msg_void_id(context, "setView:", view);
    ((void (*)(id, SEL, const int *, TwlNSInteger)) objc_msgSend)(
      context, sel_registerName("setValues:forParameter:"),
      &interval, 222);
    twl_msg_void(context, "makeCurrentContext");
  }
  return context;
}

static size_t twl_macos_controller_offset(void) {
  return twl_internal_align_up(sizeof(TwlMacos), _Alignof(id));
}

size_t twl_backend_memory_alignment(void) {
  return _Alignof(TwlMacos);
}

size_t twl_backend_memory_required(const TwlConfig *config) {
  const size_t offset = twl_macos_controller_offset();
  if (!config || config->controller_capacity >
      (SIZE_MAX - offset) / sizeof(id)) return 0u;
  return offset + (size_t) config->controller_capacity * sizeof(id);
}

TwlResult twl_backend_init(
    Twl *twl, void *memory, size_t memory_size, const TwlConfig *config) {
  TwlMacos *macos;
  void *pool;
  id application;
  TwlNSUInteger style;
  TwlRect rectangle;
  if (!twl || !memory || !config ||
      memory_size < twl_backend_memory_required(config))
    return TWL_RESULT_INVALID_ARGUMENT;
  if (!twl_macos_prepare_delegate()) return TWL_RESULT_BACKEND_FAILURE;
  pool = objc_autoreleasePoolPush();
  macos = (TwlMacos *) memory;
  macos->twl = twl;
  macos->controllers = (id *)
    ((uint8_t *) memory + twl_macos_controller_offset());
  macos->controller_count = config->controller_capacity;
  application = twl_msg_id(
    (id) objc_getClass("NSApplication"), "sharedApplication");
  twl_msg_void_integer(
    application, "setActivationPolicy:", TWL_NS_ACTIVATION_POLICY_REGULAR);
  twl_msg_void(application, "finishLaunching");
  rectangle = twl_rect(100.0, 100.0, config->width, config->height);
  style = TWL_NS_STYLE_TITLED | TWL_NS_STYLE_CLOSABLE |
    TWL_NS_STYLE_MINIATURIZABLE;
  if (config->resizable) style |= TWL_NS_STYLE_RESIZABLE;
  macos->window = twl_msg_id((id) objc_getClass("NSWindow"), "alloc");
  macos->window = ((id (*)(
    id, SEL, TwlRect, TwlNSUInteger, TwlNSUInteger, BOOL)) objc_msgSend)(
      macos->window,
      sel_registerName("initWithContentRect:styleMask:backing:defer:"),
      rectangle, style, TWL_NS_BACKING_STORE_BUFFERED, NO);
  macos->view = twl_msg_id((id) objc_getClass("NSView"), "alloc");
  macos->view = ((id (*)(id, SEL, TwlRect)) objc_msgSend)(
    macos->view, sel_registerName("initWithFrame:"),
    twl_rect(0.0, 0.0, config->width, config->height));
  macos->delegate = twl_msg_id((id) twl_macos_delegate_class, "alloc");
  macos->delegate = twl_msg_id(macos->delegate, "init");
  if (!macos->window || !macos->view || !macos->delegate) {
    objc_autoreleasePoolPop(pool);
    twl_backend_shutdown(twl);
    return TWL_RESULT_BACKEND_FAILURE;
  }
  object_setInstanceVariable(macos->delegate, "_twlMacos", macos);
  twl_msg_void_id(macos->window, "setTitle:", twl_ns_string(config->title));
  twl_msg_void_id(macos->window, "setContentView:", macos->view);
  twl_msg_void_id(macos->window, "setDelegate:", macos->delegate);
  twl_msg_void_bool(macos->window, "setAcceptsMouseMovedEvents:", true);
  twl_msg_void_bool(macos->window, "setReleasedWhenClosed:", false);
  twl_msg_void_id(macos->window, "makeFirstResponder:", macos->view);
  macos->run_loop_mode = twl_msg_id(
    twl_ns_string("kCFRunLoopDefaultMode"), "retain");
  macos->context = twl_macos_create_context(macos->view);
  if (!macos->context) {
    objc_autoreleasePoolPop(pool);
    twl_backend_shutdown(twl);
    return TWL_RESULT_BACKEND_FAILURE;
  }
  macos->program = twl_macos_create_program();
  if (macos->program == 0u) {
    objc_autoreleasePoolPop(pool);
    twl_backend_shutdown(twl);
    return TWL_RESULT_BACKEND_FAILURE;
  }
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
  macos->backing_scale = twl_msg_double(macos->window, "backingScaleFactor");
  if (macos->backing_scale <= 0.0) macos->backing_scale = 1.0;
  twl_msg_void_id(macos->window, "makeKeyAndOrderFront:", nil);
  twl_msg_void_bool(application, "activateIgnoringOtherApps:", true);
  twl_internal_set_display_size(twl, config->width, config->height);
  objc_autoreleasePoolPop(pool);
  return TWL_RESULT_OK;
}

void twl_backend_shutdown(Twl *twl) {
  TwlMacos *macos = twl ? (TwlMacos *) twl->backend : NULL;
  void *pool;
  if (!macos) return;
  pool = objc_autoreleasePoolPush();
  if (macos->context) twl_msg_void(macos->context, "makeCurrentContext");
  if (macos->texture != 0u) glDeleteTextures(1, &macos->texture);
  if (macos->program != 0u) glDeleteProgram(macos->program);
  if (macos->window) twl_msg_void_id(macos->window, "setDelegate:", nil);
  if (macos->context) {
    twl_msg_void(macos->context, "clearDrawable");
    twl_msg_void(macos->context, "release");
  }
  if (macos->window) {
    twl_msg_void(macos->window, "close");
    twl_msg_void(macos->window, "release");
  }
  if (macos->view) twl_msg_void(macos->view, "release");
  if (macos->delegate) twl_msg_void(macos->delegate, "release");
  if (macos->run_loop_mode)
    twl_msg_void(macos->run_loop_mode, "release");
  macos->context = nil;
  macos->window = nil;
  macos->view = nil;
  macos->delegate = nil;
  macos->run_loop_mode = nil;
  macos->twl = NULL;
  objc_autoreleasePoolPop(pool);
}

static uint32_t twl_macos_utf8_codepoint(const char *text) {
  const uint8_t *bytes = (const uint8_t *) text;
  if (!bytes || bytes[0] == 0u) return 0u;
  if (bytes[0] < 0x80u) return bytes[0];
  if ((bytes[0] & 0xe0u) == 0xc0u)
    return ((uint32_t) (bytes[0] & 0x1fu) << 6u) |
      (uint32_t) (bytes[1] & 0x3fu);
  if ((bytes[0] & 0xf0u) == 0xe0u)
    return ((uint32_t) (bytes[0] & 0x0fu) << 12u) |
      ((uint32_t) (bytes[1] & 0x3fu) << 6u) |
      (uint32_t) (bytes[2] & 0x3fu);
  if ((bytes[0] & 0xf8u) == 0xf0u)
    return ((uint32_t) (bytes[0] & 0x07u) << 18u) |
      ((uint32_t) (bytes[1] & 0x3fu) << 12u) |
      ((uint32_t) (bytes[2] & 0x3fu) << 6u) |
      (uint32_t) (bytes[3] & 0x3fu);
  return 0u;
}

static TwlKey twl_macos_key(id event) {
  const unsigned short code = ((unsigned short (*)(id, SEL)) objc_msgSend)(
    event, sel_registerName("keyCode"));
  const char *characters;
  switch (code) {
    case 51u: return TWL_KEY_BACKSPACE;
    case 48u: return TWL_KEY_TAB;
    case 36u: return TWL_KEY_RETURN;
    case 53u: return TWL_KEY_ESCAPE;
    case 49u: return TWL_KEY_SPACE;
    case 117u: return TWL_KEY_DELETE;
    case 123u: return TWL_KEY_LEFT;
    case 124u: return TWL_KEY_RIGHT;
    case 126u: return TWL_KEY_UP;
    case 125u: return TWL_KEY_DOWN;
    case 115u: return TWL_KEY_HOME;
    case 119u: return TWL_KEY_END;
    case 116u: return TWL_KEY_PAGE_UP;
    case 121u: return TWL_KEY_PAGE_DOWN;
    case 114u: return TWL_KEY_INSERT;
    case 122u: return TWL_KEY_F1;
    case 120u: return TWL_KEY_F2;
    case 99u: return TWL_KEY_F3;
    case 118u: return TWL_KEY_F4;
    case 96u: return TWL_KEY_F5;
    case 97u: return TWL_KEY_F6;
    case 98u: return TWL_KEY_F7;
    case 100u: return TWL_KEY_F8;
    case 101u: return TWL_KEY_F9;
    case 109u: return TWL_KEY_F10;
    case 103u: return TWL_KEY_F11;
    case 111u: return TWL_KEY_F12;
    default: break;
  }
  characters = twl_ns_utf8(
    twl_msg_id(event, "charactersIgnoringModifiers"));
  if (characters[0] >= 'a' && characters[0] <= 'z')
    return (TwlKey) (TWL_KEY_A + characters[0] - 'a');
  if (characters[0] >= 'A' && characters[0] <= 'Z')
    return (TwlKey) (TWL_KEY_A + characters[0] - 'A');
  if (characters[0] >= '0' && characters[0] <= '9')
    return (TwlKey) (TWL_KEY_0 + characters[0] - '0');
  return TWL_KEY_UNKNOWN;
}

static uint8_t twl_macos_mouse_button(TwlNSInteger native_button) {
  if (native_button == 0) return 1u;
  if (native_button == 1) return 3u;
  if (native_button == 2) return 2u;
  return native_button < 255 ? (uint8_t) (native_button + 1) : 0u;
}

static void twl_macos_push_event(TwlMacos *macos, id native_event) {
  const TwlNSInteger type = twl_msg_integer(native_event, "type");
  TwlEvent event;
  TwlPoint position;
  twl_internal_zero(&event, sizeof(event));
  event.timestamp_us = twl_backend_time_microseconds(macos->twl);
  if (type == TWL_NS_EVENT_KEY_DOWN || type == TWL_NS_EVENT_KEY_UP) {
    event.type = type == TWL_NS_EVENT_KEY_DOWN
      ? TWL_EVENT_KEY_DOWN : TWL_EVENT_KEY_UP;
    event.key = twl_macos_key(native_event);
    event.repeat = twl_msg_bool(native_event, "isARepeat");
    twl_internal_push_event(macos->twl, &event);
    if (type == TWL_NS_EVENT_KEY_DOWN) {
      const uint32_t codepoint = twl_macos_utf8_codepoint(
        twl_ns_utf8(twl_msg_id(native_event, "characters")));
      if (codepoint >= 0x20u && codepoint != 0x7fu) {
        event.type = TWL_EVENT_TEXT;
        event.codepoint = codepoint;
        twl_internal_push_event(macos->twl, &event);
      }
    }
    return;
  }
  position = twl_msg_point(native_event, "locationInWindow");
  event.x = (int32_t) position.x;
  event.y = (int32_t) macos->twl->display_height - (int32_t) position.y;
  event.dx = event.x - macos->mouse_x;
  event.dy = event.y - macos->mouse_y;
  macos->mouse_x = event.x;
  macos->mouse_y = event.y;
  switch (type) {
    case TWL_NS_EVENT_LEFT_DOWN:
    case TWL_NS_EVENT_RIGHT_DOWN:
    case TWL_NS_EVENT_OTHER_DOWN:
      event.type = TWL_EVENT_POINTER_DOWN;
      event.button = twl_macos_mouse_button(
        twl_msg_integer(native_event, "buttonNumber"));
      break;
    case TWL_NS_EVENT_LEFT_UP:
    case TWL_NS_EVENT_RIGHT_UP:
    case TWL_NS_EVENT_OTHER_UP:
      event.type = TWL_EVENT_POINTER_UP;
      event.button = twl_macos_mouse_button(
        twl_msg_integer(native_event, "buttonNumber"));
      break;
    case TWL_NS_EVENT_MOUSE_MOVED:
    case TWL_NS_EVENT_LEFT_DRAGGED:
    case TWL_NS_EVENT_RIGHT_DRAGGED:
    case TWL_NS_EVENT_OTHER_DRAGGED:
      event.type = TWL_EVENT_POINTER_MOVE;
      break;
    case TWL_NS_EVENT_SCROLL:
      event.type = TWL_EVENT_POINTER_WHEEL;
      event.dx = (int32_t) twl_msg_double(native_event, "scrollingDeltaX");
      event.dy = (int32_t) twl_msg_double(native_event, "scrollingDeltaY");
      break;
    default:
      return;
  }
  twl_internal_push_event(macos->twl, &event);
}

static int16_t twl_macos_axis(float value) {
  if (value > 1.0f) value = 1.0f;
  if (value < -1.0f) value = -1.0f;
  return (int16_t) (value * 32767.0f);
}

static void twl_macos_controller_button(
    Twl *twl, uint32_t index, id gamepad, const char *selector,
    TwlControllerButton button) {
  id input = twl_optional_id(gamepad, selector);
  twl_internal_set_controller_button(
    twl, index, button, twl_msg_bool(input, "isPressed"));
}

static bool twl_macos_controller_in_array(id controllers, id controller) {
  TwlNSUInteger index;
  const TwlNSUInteger count = twl_msg_count(controllers);
  for (index = 0u; index < count; ++index) {
    if (twl_msg_object_at(controllers, index) == controller) return true;
  }
  return false;
}

static bool twl_macos_controller_assigned(
    const TwlMacos *macos, id controller) {
  uint32_t index;
  for (index = 0u; index < macos->controller_count; ++index) {
    if (macos->controllers[index] == controller) return true;
  }
  return false;
}

static void twl_macos_assign_controllers(
    TwlMacos *macos, id controllers) {
  const TwlNSUInteger connected_count = twl_msg_count(controllers);
  uint32_t slot;
  TwlNSUInteger connected_index;
  for (slot = 0u; slot < macos->controller_count; ++slot) {
    if (macos->controllers[slot] &&
        !twl_macos_controller_in_array(
          controllers, macos->controllers[slot])) {
      macos->controllers[slot] = nil;
    }
  }
  for (connected_index = 0u;
       connected_index < connected_count; ++connected_index) {
    id controller = twl_msg_object_at(controllers, connected_index);
    if (twl_macos_controller_assigned(macos, controller)) continue;
    for (slot = 0u; slot < macos->controller_count; ++slot) {
      if (!macos->controllers[slot]) {
        macos->controllers[slot] = controller;
        break;
      }
    }
  }
}

static void twl_macos_pump_controllers(Twl *twl, TwlMacos *macos) {
  id controllers = twl_msg_id((id) objc_getClass("GCController"), "controllers");
  uint32_t index;
  twl_macos_assign_controllers(macos, controllers);
  for (index = 0u; index < twl->config.controller_capacity; ++index) {
    id controller = macos->controllers[index];
    id gamepad = twl_optional_id(controller, "extendedGamepad");
    id stick;
    id axis;
    twl_internal_set_controller_connected(twl, index, gamepad != nil);
    if (!gamepad) continue;
    twl_macos_controller_button(twl, index, gamepad, "buttonA",
      TWL_CONTROLLER_BUTTON_SOUTH);
    twl_macos_controller_button(twl, index, gamepad, "buttonB",
      TWL_CONTROLLER_BUTTON_EAST);
    twl_macos_controller_button(twl, index, gamepad, "buttonX",
      TWL_CONTROLLER_BUTTON_WEST);
    twl_macos_controller_button(twl, index, gamepad, "buttonY",
      TWL_CONTROLLER_BUTTON_NORTH);
    twl_macos_controller_button(twl, index, gamepad, "leftShoulder",
      TWL_CONTROLLER_BUTTON_LEFT_SHOULDER);
    twl_macos_controller_button(twl, index, gamepad, "rightShoulder",
      TWL_CONTROLLER_BUTTON_RIGHT_SHOULDER);
    twl_macos_controller_button(twl, index, gamepad, "buttonOptions",
      TWL_CONTROLLER_BUTTON_BACK);
    twl_macos_controller_button(twl, index, gamepad, "buttonMenu",
      TWL_CONTROLLER_BUTTON_START);
    twl_macos_controller_button(twl, index, gamepad, "buttonHome",
      TWL_CONTROLLER_BUTTON_GUIDE);
    twl_macos_controller_button(twl, index, gamepad, "leftThumbstickButton",
      TWL_CONTROLLER_BUTTON_LEFT_STICK);
    twl_macos_controller_button(twl, index, gamepad, "rightThumbstickButton",
      TWL_CONTROLLER_BUTTON_RIGHT_STICK);
    stick = twl_optional_id(gamepad, "dpad");
    axis = twl_optional_id(stick, "xAxis");
    twl_internal_set_controller_button(twl, index,
      TWL_CONTROLLER_BUTTON_DPAD_LEFT, twl_msg_float(axis, "value") < -0.5f);
    twl_internal_set_controller_button(twl, index,
      TWL_CONTROLLER_BUTTON_DPAD_RIGHT, twl_msg_float(axis, "value") > 0.5f);
    axis = twl_optional_id(stick, "yAxis");
    twl_internal_set_controller_button(twl, index,
      TWL_CONTROLLER_BUTTON_DPAD_UP, twl_msg_float(axis, "value") > 0.5f);
    twl_internal_set_controller_button(twl, index,
      TWL_CONTROLLER_BUTTON_DPAD_DOWN, twl_msg_float(axis, "value") < -0.5f);
    stick = twl_optional_id(gamepad, "leftThumbstick");
    axis = twl_optional_id(stick, "xAxis");
    twl_internal_set_controller_axis(twl, index,
      TWL_CONTROLLER_AXIS_LEFT_X, twl_macos_axis(twl_msg_float(axis, "value")));
    axis = twl_optional_id(stick, "yAxis");
    twl_internal_set_controller_axis(twl, index,
      TWL_CONTROLLER_AXIS_LEFT_Y, twl_macos_axis(-twl_msg_float(axis, "value")));
    stick = twl_optional_id(gamepad, "rightThumbstick");
    axis = twl_optional_id(stick, "xAxis");
    twl_internal_set_controller_axis(twl, index,
      TWL_CONTROLLER_AXIS_RIGHT_X, twl_macos_axis(twl_msg_float(axis, "value")));
    axis = twl_optional_id(stick, "yAxis");
    twl_internal_set_controller_axis(twl, index,
      TWL_CONTROLLER_AXIS_RIGHT_Y, twl_macos_axis(-twl_msg_float(axis, "value")));
    axis = twl_optional_id(gamepad, "leftTrigger");
    twl_internal_set_controller_axis(twl, index,
      TWL_CONTROLLER_AXIS_LEFT_TRIGGER,
      twl_macos_axis(twl_msg_float(axis, "value")));
    axis = twl_optional_id(gamepad, "rightTrigger");
    twl_internal_set_controller_axis(twl, index,
      TWL_CONTROLLER_AXIS_RIGHT_TRIGGER,
      twl_macos_axis(twl_msg_float(axis, "value")));
  }
}

void twl_backend_pump_events(Twl *twl) {
  TwlMacos *macos = twl ? (TwlMacos *) twl->backend : NULL;
  void *pool;
  id application;
  id distant_past;
  id mode;
  id native_event;
  TwlRect bounds;
  uint32_t width;
  uint32_t height;
  if (!macos || !macos->window) return;
  pool = objc_autoreleasePoolPush();
  twl_macos_pump_controllers(twl, macos);
  application = twl_msg_id(
    (id) objc_getClass("NSApplication"), "sharedApplication");
  distant_past = twl_msg_id((id) objc_getClass("NSDate"), "distantPast");
  mode = macos->run_loop_mode;
  for (;;) {
    native_event = ((id (*)(
      id, SEL, TwlNSUInteger, id, id, BOOL)) objc_msgSend)(
        application,
        sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"),
        TWL_NS_EVENT_MASK_ANY, distant_past, mode, YES);
    if (!native_event) break;
    if (twl_msg_id(native_event, "window") == macos->window)
      twl_macos_push_event(macos, native_event);
    twl_msg_void_id(application, "sendEvent:", native_event);
  }
  bounds = twl_msg_rect(macos->view, "bounds");
  width = bounds.size.width > 0.0 ? (uint32_t) bounds.size.width : 1u;
  height = bounds.size.height > 0.0 ? (uint32_t) bounds.size.height : 1u;
  if (width != twl->display_width || height != twl->display_height) {
    TwlEvent event;
    twl_internal_set_display_size(twl, width, height);
    twl_msg_void(macos->context, "update");
    macos->backing_scale = twl_msg_double(macos->window, "backingScaleFactor");
    if (macos->backing_scale <= 0.0) macos->backing_scale = 1.0;
    twl_internal_zero(&event, sizeof(event));
    event.type = TWL_EVENT_RESIZED;
    event.timestamp_us = twl_backend_time_microseconds(twl);
    event.width = (int32_t) width;
    event.height = (int32_t) height;
    twl_internal_push_event(twl, &event);
  }
  objc_autoreleasePoolPop(pool);
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

uint64_t twl_backend_time_microseconds(const Twl *twl) {
  const uint64_t ticks = mach_absolute_time();
  (void) twl;
  if (twl_macos_timebase.denom == 0u)
    mach_timebase_info(&twl_macos_timebase);
  return ticks * twl_macos_timebase.numer /
    twl_macos_timebase.denom / UINT64_C(1000);
}
