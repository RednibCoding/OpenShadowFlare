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

#define _POSIX_C_SOURCE 200809L
#define GL_GLEXT_PROTOTYPES

#include "twl_internal.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

#include <errno.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <time.h>
#include <unistd.h>

typedef struct {
  int descriptor;
  uint64_t next_open_attempt_us;
} TwlX11Controller;

typedef struct {
  Display *display;
  Window window;
  Colormap colormap;
  GLXContext context;
  Atom wm_delete;
  GLuint program;
  GLuint texture;
  GLint texture_uniform;
  GLint format_uniform;
  uint32_t texture_width;
  uint32_t texture_height;
  TwlPixelFormat texture_format;
  TwlX11Controller *controllers;
  uint32_t controller_count;
} TwlX11;

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

static size_t twl_x11_controller_offset(void) {
  return twl_internal_align_up(
    sizeof(TwlX11), _Alignof(TwlX11Controller));
}

size_t twl_backend_memory_alignment(void) {
  return _Alignof(max_align_t);
}

size_t twl_backend_memory_required(const TwlConfig *config) {
  const size_t offset = twl_x11_controller_offset();
  if (!config || config->controller_capacity >
      (SIZE_MAX - offset) / sizeof(TwlX11Controller)) {
    return 0u;
  }
  return offset +
    (size_t) config->controller_capacity * sizeof(TwlX11Controller);
}

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

static GLuint twl_x11_create_program(void) {
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

static TwlKey twl_x11_key(KeySym symbol) {
  if (symbol >= XK_a && symbol <= XK_z)
    return (TwlKey) (TWL_KEY_A + (symbol - XK_a));
  if (symbol >= XK_A && symbol <= XK_Z)
    return (TwlKey) (TWL_KEY_A + (symbol - XK_A));
  if (symbol >= XK_0 && symbol <= XK_9)
    return (TwlKey) (TWL_KEY_0 + (symbol - XK_0));
  switch (symbol) {
    case XK_BackSpace: return TWL_KEY_BACKSPACE;
    case XK_Tab: return TWL_KEY_TAB;
    case XK_Return: return TWL_KEY_RETURN;
    case XK_Escape: return TWL_KEY_ESCAPE;
    case XK_space: return TWL_KEY_SPACE;
    case XK_Delete: return TWL_KEY_DELETE;
    case XK_Left: return TWL_KEY_LEFT;
    case XK_Right: return TWL_KEY_RIGHT;
    case XK_Up: return TWL_KEY_UP;
    case XK_Down: return TWL_KEY_DOWN;
    case XK_Home: return TWL_KEY_HOME;
    case XK_End: return TWL_KEY_END;
    case XK_Page_Up: return TWL_KEY_PAGE_UP;
    case XK_Page_Down: return TWL_KEY_PAGE_DOWN;
    case XK_Insert: return TWL_KEY_INSERT;
    case XK_F1: return TWL_KEY_F1;
    case XK_F2: return TWL_KEY_F2;
    case XK_F3: return TWL_KEY_F3;
    case XK_F4: return TWL_KEY_F4;
    case XK_F5: return TWL_KEY_F5;
    case XK_F6: return TWL_KEY_F6;
    case XK_F7: return TWL_KEY_F7;
    case XK_F8: return TWL_KEY_F8;
    case XK_F9: return TWL_KEY_F9;
    case XK_F10: return TWL_KEY_F10;
    case XK_F11: return TWL_KEY_F11;
    case XK_F12: return TWL_KEY_F12;
    default: return TWL_KEY_UNKNOWN;
  }
}

TwlResult twl_backend_init(
    Twl *twl, void *memory, size_t memory_size, const TwlConfig *config) {
  static int visual_attributes[] = {
    GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None
  };
  TwlX11 *x11;
  XVisualInfo *visual;
  XSetWindowAttributes attributes;
  XSizeHints hints;
  int screen;
  uint32_t index;

  if (!twl || !memory ||
      memory_size < twl_backend_memory_required(config)) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  x11 = (TwlX11 *) memory;
  x11->controllers = (TwlX11Controller *)
    ((uint8_t *) memory + twl_x11_controller_offset());
  x11->controller_count = config->controller_capacity;
  for (index = 0u; index < x11->controller_count; ++index) {
    x11->controllers[index].descriptor = -1;
  }

  x11->display = XOpenDisplay(NULL);
  if (!x11->display) {
    return TWL_RESULT_BACKEND_UNAVAILABLE;
  }
  screen = DefaultScreen(x11->display);
  visual = glXChooseVisual(x11->display, screen, visual_attributes);
  if (!visual) {
    XCloseDisplay(x11->display);
    x11->display = NULL;
    return TWL_RESULT_BACKEND_FAILURE;
  }
  x11->colormap = XCreateColormap(
    x11->display, RootWindow(x11->display, screen), visual->visual, AllocNone);
  attributes.colormap = x11->colormap;
  attributes.event_mask =
    ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask |
    ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
  x11->window = XCreateWindow(
    x11->display, RootWindow(x11->display, screen), 0, 0,
    config->width, config->height, 0, visual->depth, InputOutput,
    visual->visual, CWColormap | CWEventMask, &attributes);
  if (!x11->window) {
    XFree(visual);
    XFreeColormap(x11->display, x11->colormap);
    XCloseDisplay(x11->display);
    x11->display = NULL;
    return TWL_RESULT_BACKEND_FAILURE;
  }
  if (config->title) XStoreName(x11->display, x11->window, config->title);
  x11->wm_delete = XInternAtom(x11->display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(x11->display, x11->window, &x11->wm_delete, 1);
  if (!config->resizable) {
    hints.flags = PMinSize | PMaxSize;
    hints.min_width = hints.max_width = (int) config->width;
    hints.min_height = hints.max_height = (int) config->height;
    XSetWMNormalHints(x11->display, x11->window, &hints);
  }
  x11->context = glXCreateContext(x11->display, visual, NULL, True);
  XFree(visual);
  if (!x11->context ||
      !glXMakeCurrent(x11->display, x11->window, x11->context)) {
    if (x11->context) glXDestroyContext(x11->display, x11->context);
    XDestroyWindow(x11->display, x11->window);
    XFreeColormap(x11->display, x11->colormap);
    XCloseDisplay(x11->display);
    x11->display = NULL;
    return TWL_RESULT_BACKEND_FAILURE;
  }
  x11->program = twl_x11_create_program();
  if (x11->program == 0u) {
    twl->backend = x11;
    twl_backend_shutdown(twl);
    return TWL_RESULT_BACKEND_FAILURE;
  }
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
  XMapWindow(x11->display, x11->window);
  XFlush(x11->display);
  twl_internal_set_display_size(twl, config->width, config->height);
  return TWL_RESULT_OK;
}

void twl_backend_shutdown(Twl *twl) {
  TwlX11 *x11 = twl ? (TwlX11 *) twl->backend : NULL;
  uint32_t index;
  if (!x11) return;
  for (index = 0u; index < x11->controller_count; ++index) {
    if (x11->controllers[index].descriptor >= 0) {
      close(x11->controllers[index].descriptor);
      x11->controllers[index].descriptor = -1;
    }
  }
  if (!x11->display) return;
  if (x11->context) {
    if (x11->texture != 0u) glDeleteTextures(1, &x11->texture);
    if (x11->program != 0u) glDeleteProgram(x11->program);
    glXMakeCurrent(x11->display, None, NULL);
    glXDestroyContext(x11->display, x11->context);
  }
  if (x11->window) XDestroyWindow(x11->display, x11->window);
  if (x11->colormap) XFreeColormap(x11->display, x11->colormap);
  XCloseDisplay(x11->display);
  x11->display = NULL;
}

static void twl_x11_push_key(Twl *twl, XKeyEvent *key, bool pressed) {
  TwlEvent event = {0};
  char text[8];
  KeySym symbol = NoSymbol;
  const int count = XLookupString(key, text, (int) sizeof(text), &symbol, NULL);
  event.type = pressed ? TWL_EVENT_KEY_DOWN : TWL_EVENT_KEY_UP;
  event.timestamp_us = twl_backend_time_microseconds(twl);
  event.key = twl_x11_key(symbol);
  twl_internal_push_event(twl, &event);
  if (pressed && count == 1 && (unsigned char) text[0] >= 0x20u) {
    event.type = TWL_EVENT_TEXT;
    event.codepoint = (unsigned char) text[0];
    twl_internal_push_event(twl, &event);
  }
}

static bool twl_x11_controller_path(
    char *path, size_t path_size, uint32_t index) {
  static const char prefix[] = "/dev/input/js";
  char digits[10];
  size_t prefix_length = sizeof(prefix) - 1u;
  size_t digit_count = 0u;
  size_t position;
  if (!path || path_size <= prefix_length + 1u) return false;
  do {
    digits[digit_count++] = (char) ('0' + index % 10u);
    index /= 10u;
  } while (index > 0u && digit_count < sizeof(digits));
  if (prefix_length + digit_count + 1u > path_size) return false;
  for (position = 0u; position < prefix_length; ++position)
    path[position] = prefix[position];
  for (position = 0u; position < digit_count; ++position)
    path[prefix_length + position] = digits[digit_count - position - 1u];
  path[prefix_length + digit_count] = '\0';
  return true;
}

static void twl_x11_controller_button(
    Twl *twl, uint32_t index, uint8_t native_button, bool pressed) {
  static const TwlControllerButton buttons[] = {
    TWL_CONTROLLER_BUTTON_SOUTH, TWL_CONTROLLER_BUTTON_EAST,
    TWL_CONTROLLER_BUTTON_WEST, TWL_CONTROLLER_BUTTON_NORTH,
    TWL_CONTROLLER_BUTTON_LEFT_SHOULDER,
    TWL_CONTROLLER_BUTTON_RIGHT_SHOULDER,
    TWL_CONTROLLER_BUTTON_BACK, TWL_CONTROLLER_BUTTON_START,
    TWL_CONTROLLER_BUTTON_GUIDE, TWL_CONTROLLER_BUTTON_LEFT_STICK,
    TWL_CONTROLLER_BUTTON_RIGHT_STICK
  };
  if (native_button < sizeof(buttons) / sizeof(buttons[0]))
    twl_internal_set_controller_button(
      twl, index, buttons[native_button], pressed);
}

static void twl_x11_controller_axis(
    Twl *twl, uint32_t index, uint8_t native_axis, int16_t value) {
  switch (native_axis) {
    case 0u:
      twl_internal_set_controller_axis(
        twl, index, TWL_CONTROLLER_AXIS_LEFT_X, value);
      break;
    case 1u:
      twl_internal_set_controller_axis(
        twl, index, TWL_CONTROLLER_AXIS_LEFT_Y, value);
      break;
    case 2u:
      twl_internal_set_controller_axis(
        twl, index, TWL_CONTROLLER_AXIS_LEFT_TRIGGER,
        (int16_t) (((int32_t) value + 32767) / 2));
      break;
    case 3u:
      twl_internal_set_controller_axis(
        twl, index, TWL_CONTROLLER_AXIS_RIGHT_X, value);
      break;
    case 4u:
      twl_internal_set_controller_axis(
        twl, index, TWL_CONTROLLER_AXIS_RIGHT_Y, value);
      break;
    case 5u:
      twl_internal_set_controller_axis(
        twl, index, TWL_CONTROLLER_AXIS_RIGHT_TRIGGER,
        (int16_t) (((int32_t) value + 32767) / 2));
      break;
    case 6u:
      twl_internal_set_controller_button(
        twl, index, TWL_CONTROLLER_BUTTON_DPAD_LEFT, value < -16384);
      twl_internal_set_controller_button(
        twl, index, TWL_CONTROLLER_BUTTON_DPAD_RIGHT, value > 16384);
      break;
    case 7u:
      twl_internal_set_controller_button(
        twl, index, TWL_CONTROLLER_BUTTON_DPAD_UP, value < -16384);
      twl_internal_set_controller_button(
        twl, index, TWL_CONTROLLER_BUTTON_DPAD_DOWN, value > 16384);
      break;
    default:
      break;
  }
}

static void twl_x11_pump_controllers(Twl *twl, TwlX11 *x11) {
  const uint64_t now = twl_backend_time_microseconds(twl);
  uint32_t index;
  for (index = 0u; index < x11->controller_count; ++index) {
    TwlX11Controller *controller = &x11->controllers[index];
    if (controller->descriptor < 0 && now >= controller->next_open_attempt_us) {
      char path[32];
      if (twl_x11_controller_path(path, sizeof(path), index))
        controller->descriptor = open(path, O_RDONLY | O_NONBLOCK);
      controller->next_open_attempt_us = now + UINT64_C(1000000);
      if (controller->descriptor >= 0)
        twl_internal_set_controller_connected(twl, index, true);
    }
    if (controller->descriptor >= 0) {
      struct js_event native_event;
      ssize_t read_size;
      while ((read_size = read(
                controller->descriptor, &native_event,
                sizeof(native_event))) == (ssize_t) sizeof(native_event)) {
        const uint8_t type = native_event.type & (uint8_t) ~JS_EVENT_INIT;
        if (type == JS_EVENT_BUTTON)
          twl_x11_controller_button(
            twl, index, native_event.number, native_event.value != 0);
        else if (type == JS_EVENT_AXIS)
          twl_x11_controller_axis(
            twl, index, native_event.number, native_event.value);
      }
      if (read_size < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        close(controller->descriptor);
        controller->descriptor = -1;
        controller->next_open_attempt_us = now + UINT64_C(1000000);
        twl_internal_set_controller_connected(twl, index, false);
      }
    }
  }
}

void twl_backend_pump_events(Twl *twl) {
  TwlX11 *x11 = twl ? (TwlX11 *) twl->backend : NULL;
  if (!x11 || !x11->display) return;
  twl_x11_pump_controllers(twl, x11);
  while (XPending(x11->display) > 0) {
    XEvent native_event;
    TwlEvent event = {0};
    XNextEvent(x11->display, &native_event);
    event.timestamp_us = twl_backend_time_microseconds(twl);
    switch (native_event.type) {
      case ClientMessage:
        if ((Atom) native_event.xclient.data.l[0] == x11->wm_delete) {
          event.type = TWL_EVENT_QUIT;
          twl_internal_push_event(twl, &event);
        }
        break;
      case ConfigureNotify:
        event.type = TWL_EVENT_RESIZED;
        event.width = native_event.xconfigure.width;
        event.height = native_event.xconfigure.height;
        twl_internal_set_display_size(
          twl, (uint32_t) event.width, (uint32_t) event.height);
        twl_internal_push_event(twl, &event);
        break;
      case KeyPress:
        twl_x11_push_key(twl, &native_event.xkey, true);
        break;
      case KeyRelease:
        twl_x11_push_key(twl, &native_event.xkey, false);
        break;
      case ButtonPress:
      case ButtonRelease:
        event.type = native_event.type == ButtonPress
          ? TWL_EVENT_POINTER_DOWN : TWL_EVENT_POINTER_UP;
        event.x = native_event.xbutton.x;
        event.y = native_event.xbutton.y;
        event.button = (uint8_t) native_event.xbutton.button;
        if (native_event.xbutton.button == Button4 ||
            native_event.xbutton.button == Button5) {
          if (native_event.type == ButtonPress) {
            event.type = TWL_EVENT_POINTER_WHEEL;
            event.dy = native_event.xbutton.button == Button4 ? 1 : -1;
            twl_internal_push_event(twl, &event);
          }
        } else {
          twl_internal_push_event(twl, &event);
        }
        break;
      case MotionNotify:
        event.type = TWL_EVENT_POINTER_MOVE;
        event.x = native_event.xmotion.x;
        event.y = native_event.xmotion.y;
        twl_internal_push_event(twl, &event);
        break;
      default:
        break;
    }
  }
}

TwlResult twl_backend_present(Twl *twl, const TwlSurface *surface) {
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
  glXSwapBuffers(x11->display, x11->window);
  return TWL_RESULT_OK;
}

uint64_t twl_backend_time_microseconds(const Twl *twl) {
  struct timespec now;
  (void) twl;
  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return 0u;
  return (uint64_t) now.tv_sec * UINT64_C(1000000) +
         (uint64_t) now.tv_nsec / UINT64_C(1000);
}
