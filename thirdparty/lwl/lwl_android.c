#include "lwl.h"
#include "lwl_android.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/input.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

enum { LWL_EVENT_CAPACITY = 64 };

struct LwlWindow { int width, height; bool focused; };
struct LwlGlContext { EGLDisplay display; EGLContext context; EGLSurface surface; };

static struct android_app *g_application;
static struct LwlWindow g_window;
static struct LwlGlContext *g_context;
static LwlEvent g_events[LWL_EVENT_CAPACITY];
static int g_event_head, g_event_tail;
static int g_pointer_x, g_pointer_y;

static void push_event(const LwlEvent *event) {
  int next = (g_event_tail + 1) % LWL_EVENT_CAPACITY;
  if (next == g_event_head) g_event_head = (g_event_head + 1) % LWL_EVENT_CAPACITY;
  g_events[g_event_tail] = *event;
  g_event_tail = next;
}

static bool pop_event(LwlEvent *event) {
  if (g_event_head == g_event_tail) return false;
  *event = g_events[g_event_head];
  g_event_head = (g_event_head + 1) % LWL_EVENT_CAPACITY;
  return true;
}

static void resize_window(void) {
  if (!g_application || !g_application->window) return;
  g_window.width = ANativeWindow_getWidth(g_application->window);
  g_window.height = ANativeWindow_getHeight(g_application->window);
  LwlEvent event = {0};
  event.type = LWL_EVENT_RESIZED;
  event.x = g_window.width;
  event.y = g_window.height;
  push_event(&event);
}

static void on_command(struct android_app *app, int32_t command) {
  (void) app;
  switch (command) {
    case APP_CMD_INIT_WINDOW:
    case APP_CMD_WINDOW_RESIZED:
      resize_window();
      break;
    case APP_CMD_GAINED_FOCUS: g_window.focused = true; break;
    case APP_CMD_LOST_FOCUS: g_window.focused = false; break;
    case APP_CMD_DESTROY: {
      LwlEvent event = {0}; event.type = LWL_EVENT_QUIT; push_event(&event); break;
    }
  }
}

static void key_name(int32_t keycode, char *out, size_t size) {
  const char *key = "";
  switch (keycode) {
    case AKEYCODE_DPAD_UP: key = "up"; break;
    case AKEYCODE_DPAD_DOWN: key = "down"; break;
    case AKEYCODE_DPAD_LEFT: key = "left"; break;
    case AKEYCODE_DPAD_RIGHT: key = "right"; break;
    case AKEYCODE_ENTER: case AKEYCODE_DPAD_CENTER: key = "return"; break;
    case AKEYCODE_BACK: key = "escape"; break;
    case AKEYCODE_DEL: key = "backspace"; break;
    case AKEYCODE_FORWARD_DEL: key = "delete"; break;
    case AKEYCODE_TAB: key = "tab"; break;
    case AKEYCODE_SPACE: key = "space"; break;
    default:
      if (keycode >= AKEYCODE_A && keycode <= AKEYCODE_Z) {
        out[0] = (char) ('a' + keycode - AKEYCODE_A); out[1] = '\0'; return;
      }
      if (keycode >= AKEYCODE_0 && keycode <= AKEYCODE_9) {
        out[0] = (char) ('0' + keycode - AKEYCODE_0); out[1] = '\0'; return;
      }
      break;
  }
  strncpy(out, key, size - 1); out[size - 1] = '\0';
}

static int32_t on_input(struct android_app *app, AInputEvent *input) {
  (void) app;
  if (AInputEvent_getType(input) == AINPUT_EVENT_TYPE_MOTION) {
    int32_t action = AMotionEvent_getAction(input) & AMOTION_EVENT_ACTION_MASK;
    LwlEvent event = {0};
    g_pointer_x = (int) AMotionEvent_getX(input, 0);
    g_pointer_y = (int) AMotionEvent_getY(input, 0);
    event.x = g_pointer_x; event.y = g_pointer_y; event.button = 1;
    if (action == AMOTION_EVENT_ACTION_DOWN) { event.type = LWL_EVENT_MOUSE_DOWN; event.clicks = 1; }
    else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) { event.type = LWL_EVENT_MOUSE_UP; event.clicks = 1; }
    else if (action == AMOTION_EVENT_ACTION_MOVE) event.type = LWL_EVENT_MOUSE_MOVE;
    else return 0;
    push_event(&event); return 1;
  }
  if (AInputEvent_getType(input) == AINPUT_EVENT_TYPE_KEY) {
    int32_t action = AKeyEvent_getAction(input);
    if (action != AKEY_EVENT_ACTION_DOWN && action != AKEY_EVENT_ACTION_UP) return 0;
    LwlEvent event = {0};
    event.type = action == AKEY_EVENT_ACTION_DOWN ? LWL_EVENT_KEY_DOWN : LWL_EVENT_KEY_UP;
    key_name(AKeyEvent_getKeyCode(input), event.key, sizeof(event.key));
    if (event.key[0]) { push_event(&event); return 1; }
  }
  return 0;
}

static void pump_events(int timeout) {
  int events; struct android_poll_source *source;
  while (ALooper_pollOnce(timeout, NULL, &events, (void **) &source) >= 0) {
    if (source) source->process(g_application, source);
    if (!g_application || g_application->destroyRequested) break;
    timeout = 0;
  }
}

void lwl_android_set_application(struct android_app *application) {
  g_application = application;
  if (!application) return;
  application->onAppCmd = on_command;
  application->onInputEvent = on_input;
  if (application->activity->internalDataPath) {
    chdir(application->activity->internalDataPath);
  }
}

bool lwl_android_wait_for_window(void) {
  while (g_application && !g_application->destroyRequested && !g_application->window) pump_events(-1);
  return g_application && !g_application->destroyRequested;
}

bool lwl_init(void) { return g_application != NULL; }
void lwl_shutdown(void) { g_event_head = g_event_tail = 0; }
LwlWindow *lwl_window_create(const char *title, int width, int height) {
  (void) title; g_window.width = width; g_window.height = height; g_window.focused = true; resize_window(); return &g_window;
}
LwlWindow *lwl_window_create_with_native_message_handler(const char *t, int w, int h, LwlNativeMessageHandler handler, void *data) { (void) handler; (void) data; return lwl_window_create(t,w,h); }
LwlWindow *lwl_window_attach_native(void *native_window, int width, int height) { (void) native_window; return lwl_window_create(NULL,width,height); }
void *lwl_window_get_native_handle(LwlWindow *window) { (void) window; return g_application ? g_application->window : NULL; }
void lwl_window_destroy(LwlWindow *window) { (void) window; }
void lwl_window_show(LwlWindow *window) { (void) window; }
void lwl_window_set_title(LwlWindow *window, const char *title) { (void) window; (void) title; }
void lwl_window_set_mode(LwlWindow *window, LwlWindowMode mode) { (void) window; (void) mode; }
bool lwl_window_has_focus(LwlWindow *window) { return window && window->focused; }
void lwl_window_set_cursor(LwlWindow *window, LwlCursor cursor) { (void) window; (void) cursor; }
bool lwl_window_set_cursor_image(LwlWindow *w,const LwlColor *p,int a,int b,int c,int d) { (void)w;(void)p;(void)a;(void)b;(void)c;(void)d; return false; }
void lwl_window_set_cursor_visible(LwlWindow *window, bool visible) { (void) window; (void) visible; }
bool lwl_window_set_size(LwlWindow *window, int width, int height) { if (!window) return false; window->width=width; window->height=height; return true; }
void lwl_window_get_size(LwlWindow *window, int *width, int *height) { if (width) *width=window?window->width:0; if (height) *height=window?window->height:0; }
LwlColor *lwl_window_get_framebuffer(LwlWindow *w,int *x,int *y) { (void)w; if(x)*x=0;if(y)*y=0;return NULL; }
bool lwl_window_resize_framebuffer(LwlWindow *w,int x,int y) { (void)w;(void)x;(void)y;return false; }
void lwl_window_update_rects(LwlWindow *w,const LwlRect *r,int c) { (void)w;(void)r;(void)c; }
bool lwl_poll_event(LwlWindow *window, LwlEvent *event) { (void) window; if (!event) return false; if (pop_event(event)) return true; pump_events(0); return pop_event(event); }
bool lwl_wait_event(LwlWindow *window, double timeout) { (void)window; if(g_event_head!=g_event_tail)return true; pump_events((int)(timeout*1000.0)); return g_event_head!=g_event_tail; }
char *lwl_clipboard_get(LwlWindow *w) { (void)w;return NULL; } void lwl_clipboard_set(LwlWindow *w,const char *t) { (void)w;(void)t; } char *lwl_select_folder(LwlWindow *w,const char *t) { (void)w;(void)t;return NULL; } void lwl_free(void *p) { free(p); }
double lwl_time_seconds(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); return ts.tv_sec + ts.tv_nsec/1000000000.0; }
void lwl_sleep_seconds(double s) { if(s>0) usleep((useconds_t)(s*1000000.0)); } void lwl_sleep_until_seconds(double t) { double now=lwl_time_seconds(); if(t>now)lwl_sleep_seconds(t-now); }
const char *lwl_platform_name(void) { return "android"; } double lwl_display_scale(void) { return 1.0; } bool lwl_exe_path(char *b,int s) { (void)b;(void)s;return false; }
bool lwl_data_path(char *buffer, int size) {
  if (!buffer || size <= 0 || !g_application ||
      !g_application->activity ||
      !g_application->activity->internalDataPath) return false;
  int length = snprintf(
      buffer, (size_t) size, "%s",
      g_application->activity->internalDataPath);
  return length >= 0 && length < size;
}
LwlGlConfig lwl_gl_config_default(void) { LwlGlConfig c={0};c.api=LWL_GL_API_ES;c.major_version=3;c.depth_bits=24;c.double_buffer=true;return c; }
LwlGlContext *lwl_gl_context_create(LwlWindow *window,const LwlGlConfig *requested) {
  const EGLint attrs[]={EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE}; EGLConfig config; EGLint count; LwlGlConfig wanted=requested?*requested:lwl_gl_config_default();
  if(!window||!g_application||!g_application->window||wanted.api!=LWL_GL_API_ES||wanted.major_version!=3)return NULL;
  LwlGlContext *c=calloc(1,sizeof(*c)); if(!c)return NULL; c->display=eglGetDisplay(EGL_DEFAULT_DISPLAY); if(c->display==EGL_NO_DISPLAY||!eglInitialize(c->display,NULL,NULL)||!eglChooseConfig(c->display,attrs,&config,1,&count)||count<1)goto fail;
  const EGLint context_attrs[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE}; c->context=eglCreateContext(c->display,config,EGL_NO_CONTEXT,context_attrs); c->surface=eglCreateWindowSurface(c->display,config,g_application->window,NULL); if(c->context==EGL_NO_CONTEXT||c->surface==EGL_NO_SURFACE||!eglMakeCurrent(c->display,c->surface,c->surface,c->context))goto fail; g_context=c; return c;
fail: if(c->display!=EGL_NO_DISPLAY)eglTerminate(c->display); free(c); return NULL;
}
void lwl_gl_context_destroy(LwlGlContext *c) { if(!c)return; if(c->display!=EGL_NO_DISPLAY){eglMakeCurrent(c->display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(c->surface!=EGL_NO_SURFACE)eglDestroySurface(c->display,c->surface);if(c->context!=EGL_NO_CONTEXT)eglDestroyContext(c->display,c->context);eglTerminate(c->display);} if(g_context==c)g_context=NULL;free(c); }
bool lwl_gl_context_make_current(LwlGlContext *c) { return c&&eglMakeCurrent(c->display,c->surface,c->surface,c->context)==EGL_TRUE; }
void lwl_gl_context_swap_buffers(LwlGlContext *c) { if(c)eglSwapBuffers(c->display,c->surface); }
bool lwl_gl_context_set_swap_interval(LwlGlContext *c,int interval) { return c&&eglSwapInterval(c->display,interval)==EGL_TRUE; }
void *lwl_gl_get_proc_address(const char *name) { return (void *)eglGetProcAddress(name); }
