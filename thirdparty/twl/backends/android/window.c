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
#include "twl_android.h"

#include <android/log.h>

#include <time.h>

#define TWL_ANDROID_LOG(...) \
  __android_log_print(ANDROID_LOG_INFO, "OpenShadowFlare", __VA_ARGS__)

static const GLfloat twl_android_quad[] = {
  -1.0f, -1.0f, 0.0f, 1.0f,
   1.0f, -1.0f, 1.0f, 1.0f,
  -1.0f,  1.0f, 0.0f, 0.0f,
   1.0f,  1.0f, 1.0f, 0.0f,
};

static const char *const twl_android_vertex_source =
  "attribute vec2 a_pos;\n"
  "attribute vec2 a_uv;\n"
  "varying vec2 v_uv;\n"
  "void main() {\n"
  "  v_uv = a_uv;\n"
  "  gl_Position = vec4(a_pos, 0.0, 1.0);\n"
  "}\n";

static const char *const twl_android_fragment_source =
  "precision mediump float;\n"
  "varying vec2 v_uv;\n"
  "uniform sampler2D u_tex;\n"
  "void main() {\n"
  "  gl_FragColor = texture2D(u_tex, v_uv).bgra;\n"
  "}\n";

static struct android_app *twl_android_pending_app;

void twl_android_set_app(struct android_app *app) {
  twl_android_pending_app = app;
}

static void twl_android_fit_viewport(TwlAndroid *android) {
  const int64_t content_w = (int64_t) android->frame_width;
  const int64_t content_h = (int64_t) android->frame_height;
  const int32_t surface_w = android->surface_width;
  const int32_t surface_h = android->surface_height;
  if (surface_w <= 0 || surface_h <= 0) {
    return;
  }
  if (content_w * surface_h >= content_h * surface_w) {
    android->view_w = surface_w;
    android->view_h = (int32_t) (content_h * surface_w / content_w);
  } else {
    android->view_h = surface_h;
    android->view_w = (int32_t) (content_w * surface_h / content_h);
  }
  android->view_x = (surface_w - android->view_w) / 2;
  android->view_y = (surface_h - android->view_h) / 2;
}

static GLuint twl_android_compile(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  GLint compiled = 0;
  if (!shader) {
    return 0u;
  }
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    char log[256];
    glGetShaderInfoLog(shader, (GLsizei) sizeof(log), NULL, log);
    TWL_ANDROID_LOG("shader compile failed: %s", log);
    glDeleteShader(shader);
    return 0u;
  }
  return shader;
}

static bool twl_android_build_gl(TwlAndroid *android) {
  GLuint vertex = twl_android_compile(GL_VERTEX_SHADER, twl_android_vertex_source);
  GLuint fragment =
    twl_android_compile(GL_FRAGMENT_SHADER, twl_android_fragment_source);
  GLint linked = 0;
  if (!vertex || !fragment) {
    if (vertex) glDeleteShader(vertex);
    if (fragment) glDeleteShader(fragment);
    return false;
  }
  android->program = glCreateProgram();
  glAttachShader(android->program, vertex);
  glAttachShader(android->program, fragment);
  glLinkProgram(android->program);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  glGetProgramiv(android->program, GL_LINK_STATUS, &linked);
  if (!linked) {
    char log[256];
    glGetProgramInfoLog(android->program, (GLsizei) sizeof(log), NULL, log);
    TWL_ANDROID_LOG("program link failed: %s", log);
    return false;
  }
  android->attr_pos = glGetAttribLocation(android->program, "a_pos");
  android->attr_uv = glGetAttribLocation(android->program, "a_uv");
  android->uniform_tex = glGetUniformLocation(android->program, "u_tex");

  glGenBuffers(1, &android->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, android->vbo);
  glBufferData(
    GL_ARRAY_BUFFER, sizeof(twl_android_quad), twl_android_quad,
    GL_STATIC_DRAW);

  glGenTextures(1, &android->texture);
  glBindTexture(GL_TEXTURE_2D, android->texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei) android->frame_width,
    (GLsizei) android->frame_height, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1,
    NULL);

  android->gl_ready = true;
  return true;
}

static void twl_android_query_size(TwlAndroid *android) {
  EGLint width = 0;
  EGLint height = 0;
  eglQuerySurface(android->display, android->surface, EGL_WIDTH, &width);
  eglQuerySurface(android->display, android->surface, EGL_HEIGHT, &height);
  android->surface_width = width;
  android->surface_height = height;
  twl_android_fit_viewport(android);
}

static void twl_android_create_surface(TwlAndroid *android) {
  if (android->surface != EGL_NO_SURFACE || !android->app->window) {
    return;
  }
  android->surface = eglCreateWindowSurface(
    android->display, android->config, android->app->window, NULL);
  if (android->surface == EGL_NO_SURFACE) {
    TWL_ANDROID_LOG("eglCreateWindowSurface failed: 0x%x", eglGetError());
    return;
  }
  if (!eglMakeCurrent(
        android->display, android->surface, android->surface,
        android->context)) {
    TWL_ANDROID_LOG("eglMakeCurrent failed: 0x%x", eglGetError());
    eglDestroySurface(android->display, android->surface);
    android->surface = EGL_NO_SURFACE;
    return;
  }
  eglSwapInterval(android->display, 1);
  if (!android->gl_ready && !twl_android_build_gl(android)) {
    return;
  }
  twl_android_query_size(android);
  android->window_ready = true;
}

static void twl_android_destroy_surface(TwlAndroid *android) {
  android->window_ready = false;
  if (android->display == EGL_NO_DISPLAY) {
    return;
  }
  eglMakeCurrent(
    android->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  if (android->surface != EGL_NO_SURFACE) {
    eglDestroySurface(android->display, android->surface);
    android->surface = EGL_NO_SURFACE;
  }
}

void twl_android_on_command(struct android_app *app, int32_t command) {
  Twl *twl = (Twl *) app->userData;
  TwlAndroid *android = twl ? (TwlAndroid *) twl->backend : NULL;
  if (!android) {
    return;
  }
  switch (command) {
    case APP_CMD_INIT_WINDOW:
      twl_android_create_surface(android);
      break;
    case APP_CMD_TERM_WINDOW:
      twl_android_destroy_surface(android);
      break;
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_CONFIG_CHANGED:
      if (android->window_ready) {
        twl_android_query_size(android);
      }
      break;
    default:
      break;
  }
}

size_t twl_backend_memory_alignment(void) {
  return _Alignof(TwlAndroid);
}

size_t twl_backend_memory_required(const TwlConfig *config) {
  const uint32_t width = (config && config->width) ? config->width : 640u;
  const uint32_t height = (config && config->height) ? config->height : 480u;
  return sizeof(TwlAndroid) +
    (size_t) width * (size_t) height * sizeof(uint16_t);
}

TwlResult twl_backend_init(
    Twl *twl, void *memory, size_t memory_size, const TwlConfig *config) {
  TwlAndroid *android;
  size_t required;
  const EGLint config_attributes[] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_NONE
  };
  const EGLint context_attributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
  };
  EGLint config_count = 0;
  if (!twl || !memory || !config) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  if (!twl_android_pending_app) {
    return TWL_RESULT_BACKEND_UNAVAILABLE;
  }
  android = (TwlAndroid *) memory;
  android->frame_width = config->width ? config->width : 640u;
  android->frame_height = config->height ? config->height : 480u;
  required = sizeof(TwlAndroid) +
    (size_t) android->frame_width * android->frame_height * sizeof(uint16_t);
  if (memory_size < required) {
    return TWL_RESULT_INSUFFICIENT_MEMORY;
  }
  android->staging = (uint16_t *) ((uint8_t *) memory + sizeof(TwlAndroid));
  android->app = twl_android_pending_app;
  android->display = EGL_NO_DISPLAY;
  android->surface = EGL_NO_SURFACE;
  android->context = EGL_NO_CONTEXT;
  android->pointer_x = (int32_t) (android->frame_width / 2u);
  android->pointer_y = (int32_t) (android->frame_height / 2u);

  android->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (android->display == EGL_NO_DISPLAY ||
      !eglInitialize(android->display, NULL, NULL)) {
    return TWL_RESULT_BACKEND_FAILURE;
  }
  if (!eglChooseConfig(
        android->display, config_attributes, &android->config, 1,
        &config_count) ||
      config_count < 1) {
    return TWL_RESULT_BACKEND_FAILURE;
  }
  android->context = eglCreateContext(
    android->display, android->config, EGL_NO_CONTEXT, context_attributes);
  if (android->context == EGL_NO_CONTEXT) {
    return TWL_RESULT_BACKEND_FAILURE;
  }

  android->app->userData = twl;
  android->app->onAppCmd = twl_android_on_command;
  android->app->onInputEvent = twl_android_on_input;

  twl_internal_set_display_size(
    twl, android->frame_width, android->frame_height);

  while (!android->window_ready && !android->app->destroyRequested) {
    twl_android_pump_once(android, -1);
  }
  return TWL_RESULT_OK;
}

void twl_backend_shutdown(Twl *twl) {
  TwlAndroid *android = twl ? (TwlAndroid *) twl->backend : NULL;
  if (!android) {
    return;
  }
  twl_android_destroy_surface(android);
  if (android->display != EGL_NO_DISPLAY) {
    if (android->context != EGL_NO_CONTEXT) {
      eglDestroyContext(android->display, android->context);
      android->context = EGL_NO_CONTEXT;
    }
    eglTerminate(android->display);
    android->display = EGL_NO_DISPLAY;
  }
  if (android->app) {
    android->app->userData = NULL;
    android->app->onAppCmd = NULL;
    android->app->onInputEvent = NULL;
  }
}

uint64_t twl_backend_time_microseconds(const Twl *twl) {
  struct timespec now;
  (void) twl;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return (uint64_t) now.tv_sec * UINT64_C(1000000) +
    (uint64_t) now.tv_nsec / UINT64_C(1000);
}

void twl_backend_sleep_microseconds(Twl *twl, uint64_t duration) {
  struct timespec request;
  (void) twl;
  request.tv_sec = (time_t) (duration / UINT64_C(1000000));
  request.tv_nsec = (long) ((duration % UINT64_C(1000000)) * UINT64_C(1000));
  nanosleep(&request, NULL);
}
