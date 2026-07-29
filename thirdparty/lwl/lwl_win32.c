/*
 * Copyright (C) 2026 Michael Binder
 *
 * This file is part of LWL.
 *
 * LWL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * LWL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along with
 * LWL. If not, see <https://www.gnu.org/licenses/>.
 */

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include "lwl.h"
#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LWL_QUEUE_MAX 64

struct LwlWindow {
  HWND hwnd;
  HDC hdc;
  BITMAPINFO bmi;
  LwlColor *pixels;
  LwlColor *native_pixels;
  int width, height;
  int mouse_x, mouse_y;
  bool shown;
  bool owns_window;
  bool cursor_visible;
  LPCTSTR cursor_id;
  HCURSOR custom_cursor;
  bool use_custom_cursor;
  LwlNativeMessageHandler native_message_handler;
  void *native_message_user_data;
  LwlEvent queue[LWL_QUEUE_MAX];
  int queue_head, queue_tail;
};

struct LwlGlContext {
  LwlWindow *window;
  HGLRC handle;
  bool double_buffer;
};

static HINSTANCE g_instance;
static LARGE_INTEGER g_freq;
static const char *g_class_name = "LwlWindow";
static HMODULE g_opengl_module;

static char* lwl_strdup(const char *s) {
  size_t n = strlen(s) + 1;
  char *res = (char*) malloc(n);
  if (res) { memcpy(res, s, n); }
  return res;
}

static bool queue_push(LwlWindow *window, const LwlEvent *event) {
  int next = (window->queue_tail + 1) % LWL_QUEUE_MAX;
  if (next == window->queue_head) { return false; }
  window->queue[window->queue_tail] = *event;
  window->queue_tail = next;
  return true;
}

static bool queue_pop(LwlWindow *window, LwlEvent *event) {
  if (window->queue_head == window->queue_tail) { return false; }
  *event = window->queue[window->queue_head];
  window->queue_head = (window->queue_head + 1) % LWL_QUEUE_MAX;
  return true;
}

static void set_key_name(WPARAM vk, LPARAM lp, char *dst, int size) {
  switch (vk) {
    case VK_RETURN: snprintf(dst, size, (lp & (1 << 24)) ? "keypad enter" : "return"); return;
    case VK_ESCAPE: snprintf(dst, size, "escape"); return;
    case VK_BACK: snprintf(dst, size, "backspace"); return;
    case VK_TAB: snprintf(dst, size, "tab"); return;
    case VK_DELETE: snprintf(dst, size, "delete"); return;
    case VK_LEFT: snprintf(dst, size, "left"); return;
    case VK_RIGHT: snprintf(dst, size, "right"); return;
    case VK_UP: snprintf(dst, size, "up"); return;
    case VK_DOWN: snprintf(dst, size, "down"); return;
    case VK_HOME: snprintf(dst, size, "home"); return;
    case VK_END: snprintf(dst, size, "end"); return;
    case VK_PRIOR: snprintf(dst, size, "pageup"); return;
    case VK_NEXT: snprintf(dst, size, "pagedown"); return;
    case VK_LCONTROL: snprintf(dst, size, "left ctrl"); return;
    case VK_RCONTROL: snprintf(dst, size, "right ctrl"); return;
    case VK_LSHIFT: snprintf(dst, size, "left shift"); return;
    case VK_RSHIFT: snprintf(dst, size, "right shift"); return;
    case VK_LMENU: snprintf(dst, size, "left alt"); return;
    case VK_RMENU: snprintf(dst, size, "right alt"); return;
    case VK_SPACE: snprintf(dst, size, "space"); return;
  }
  if (vk >= VK_F1 && vk <= VK_F24) {
    snprintf(dst, size, "f%d", (int) (vk - VK_F1 + 1));
    return;
  }
  UINT scan = (lp >> 16) & 0xff;
  char name[32] = {0};
  if (GetKeyNameTextA((LONG) lp, name, sizeof(name))) {
    CharLowerBuffA(name, (DWORD) strlen(name));
    if (strcmp(name, "enter") == 0) { snprintf(name, sizeof(name), "return"); }
    snprintf(dst, size, "%s", name);
  } else {
    snprintf(dst, size, "%c", (int) vk >= 32 && (int) vk < 127 ? (char) tolower((int) vk) : '?');
  }
  (void) scan;
}

static bool resize_framebuffer(LwlWindow *window, int width, int height) {
  size_t pixel_count;
  LwlColor *pixels;
  LwlColor *native_pixels;
  if (width < 1) { width = 1; }
  if (height < 1) { height = 1; }
  if (width == window->width && height == window->height && window->pixels) {
    return true;
  }
  pixel_count = (size_t) width * (size_t) height;
  pixels = (LwlColor*) calloc(pixel_count, sizeof(*pixels));
  native_pixels = (LwlColor*) calloc(pixel_count, sizeof(*native_pixels));
  if (!pixels || !native_pixels) {
    free(pixels);
    free(native_pixels);
    return false;
  }
  free(window->pixels);
  free(window->native_pixels);
  window->pixels = pixels;
  window->native_pixels = native_pixels;
  memset(&window->bmi, 0, sizeof(window->bmi));
  window->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  window->bmi.bmiHeader.biWidth = width;
  window->bmi.bmiHeader.biHeight = -height;
  window->bmi.bmiHeader.biPlanes = 1;
  window->bmi.bmiHeader.biBitCount = 32;
  window->bmi.bmiHeader.biCompression = BI_RGB;
  window->width = width;
  window->height = height;
  return true;
}

static HCURSOR selected_cursor(const LwlWindow *window) {
  if (!window->cursor_visible) { return NULL; }
  if (window->use_custom_cursor) { return window->custom_cursor; }
  return LoadCursor(NULL, window->cursor_id);
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  LwlWindow *window = (LwlWindow*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
  LwlEvent event;
  LRESULT native_result = 0;
  bool native_handler_called = false;
  memset(&event, 0, sizeof(event));

  if (msg == WM_NCCREATE) {
    window = (LwlWindow*) ((CREATESTRUCT*) lp)->lpCreateParams;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) window);
  }
  if (window && window->native_message_handler) {
    native_result = (LRESULT) window->native_message_handler(
      (void*) hwnd, (uint32_t) msg, (uintptr_t) wp, (intptr_t) lp,
      window->native_message_user_data);
    native_handler_called = true;
  }

  switch (msg) {
    case WM_NCCREATE:
      return native_handler_called ? native_result :
        DefWindowProc(hwnd, msg, wp, lp);

    case WM_CLOSE:
      event.type = LWL_EVENT_QUIT;
      queue_push(window, &event);
      return native_handler_called ? native_result : 0;

    case WM_SIZE:
      if (window) {
        resize_framebuffer(window, LOWORD(lp), HIWORD(lp));
        event.type = LWL_EVENT_RESIZED;
        event.x = window->width;
        event.y = window->height;
        queue_push(window, &event);
      }
      return native_handler_called ? native_result : 0;

    case WM_PAINT:
      if (window) {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        event.type = LWL_EVENT_EXPOSED;
        queue_push(window, &event);
      }
      return native_handler_called ? native_result : 0;

    case WM_SETCURSOR:
      if (window && LOWORD(lp) == HTCLIENT) {
        SetCursor(selected_cursor(window));
        return TRUE;
      }
      break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      event.type = LWL_EVENT_KEY_DOWN;
      set_key_name(wp, lp, event.key, sizeof(event.key));
      queue_push(window, &event);
      return native_handler_called ? native_result : 0;

    case WM_KEYUP:
    case WM_SYSKEYUP:
      event.type = LWL_EVENT_KEY_UP;
      set_key_name(wp, lp, event.key, sizeof(event.key));
      queue_push(window, &event);
      return native_handler_called ? native_result : 0;

    case WM_CHAR:
      if (wp >= 32 && wp != 127) {
        wchar_t wbuf[2] = { (wchar_t) wp, 0 };
        event.type = LWL_EVENT_TEXT_INPUT;
        WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, event.text, sizeof(event.text), NULL, NULL);
        queue_push(window, &event);
      }
      return native_handler_called ? native_result : 0;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
      SetCapture(hwnd);
      event.type = LWL_EVENT_MOUSE_DOWN;
      event.x = GET_X_LPARAM(lp);
      event.y = GET_Y_LPARAM(lp);
      event.button = msg == WM_LBUTTONDOWN ? 1 : msg == WM_MBUTTONDOWN ? 2 : 3;
      event.clicks = 1;
      queue_push(window, &event);
      return native_handler_called ? native_result : 0;

    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
      ReleaseCapture();
      event.type = LWL_EVENT_MOUSE_UP;
      event.x = GET_X_LPARAM(lp);
      event.y = GET_Y_LPARAM(lp);
      event.button = msg == WM_LBUTTONUP ? 1 : msg == WM_MBUTTONUP ? 2 : 3;
      queue_push(window, &event);
      return native_handler_called ? native_result : 0;

    case WM_MOUSEMOVE:
      event.type = LWL_EVENT_MOUSE_MOVE;
      event.x = GET_X_LPARAM(lp);
      event.y = GET_Y_LPARAM(lp);
      if (window) {
        event.dx = event.x - window->mouse_x;
        event.dy = event.y - window->mouse_y;
        window->mouse_x = event.x;
        window->mouse_y = event.y;
        queue_push(window, &event);
      }
      return native_handler_called ? native_result : 0;

    case WM_MOUSEWHEEL:
      event.type = LWL_EVENT_MOUSE_WHEEL;
      {
        POINT point = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd, &point);
        event.x = point.x;
        event.y = point.y;
        if (window) {
          window->mouse_x = event.x;
          window->mouse_y = event.y;
        }
      }
      event.dy = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
      queue_push(window, &event);
      return native_handler_called ? native_result : 0;
  }

  if (native_handler_called) { return native_result; }
  return DefWindowProc(hwnd, msg, wp, lp);
}

bool lwl_init(void) {
  g_instance = GetModuleHandle(NULL);
  QueryPerformanceFrequency(&g_freq);
  SetProcessDPIAware();

  WNDCLASSA wc;
  memset(&wc, 0, sizeof(wc));
  wc.lpfnWndProc = wndproc;
  wc.hInstance = g_instance;
  wc.lpszClassName = g_class_name;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hIcon = LoadIcon(g_instance, MAKEINTRESOURCE(101));
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  RegisterClassA(&wc);
  return true;
}

void lwl_shutdown(void) {
  if (g_opengl_module) {
    FreeLibrary(g_opengl_module);
    g_opengl_module = NULL;
  }
}

LwlWindow* lwl_window_create(const char *title, int width, int height) {
  return lwl_window_create_with_native_message_handler(
    title, width, height, NULL, NULL);
}

LwlWindow* lwl_window_create_with_native_message_handler(
    const char *title, int width, int height,
    LwlNativeMessageHandler handler, void *user_data) {
  POINT cursor = { 0, 0 };
  GetCursorPos(&cursor);
  HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);
  MONITORINFO info;
  memset(&info, 0, sizeof(info));
  info.cbSize = sizeof(info);
  if (!GetMonitorInfoA(monitor, &info)) {
    info.rcWork.left = 0;
    info.rcWork.top = 0;
    info.rcWork.right = GetSystemMetrics(SM_CXSCREEN);
    info.rcWork.bottom = GetSystemMetrics(SM_CYSCREEN);
  }
  int area_w = info.rcWork.right - info.rcWork.left;
  int area_h = info.rcWork.bottom - info.rcWork.top;
  if (width <= 0) { width = area_w * 8 / 10; }
  if (height <= 0) { height = area_h * 8 / 10; }

  LwlWindow *window = (LwlWindow*) calloc(1, sizeof(*window));
  if (!window) { return NULL; }
  window->cursor_visible = true;
  window->cursor_id = IDC_ARROW;
  window->owns_window = true;
  window->native_message_handler = handler;
  window->native_message_user_data = user_data;
  if (!resize_framebuffer(window, width, height)) {
    free(window);
    return NULL;
  }

  RECT rect = { 0, 0, width, height };
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
  int outer_w = rect.right - rect.left;
  int outer_h = rect.bottom - rect.top;
  int x = info.rcWork.left + (area_w - outer_w) / 2;
  int y = info.rcWork.top + (area_h - outer_h) / 2;
  window->hwnd = CreateWindowExA(0, g_class_name, title, WS_OVERLAPPEDWINDOW,
    x, y, outer_w, outer_h,
    NULL, NULL, g_instance, window);
  if (!window->hwnd) {
    lwl_window_destroy(window);
    return NULL;
  }
  window->hdc = GetDC(window->hwnd);
  return window;
}

LwlWindow* lwl_window_attach_native(void *native_window, int width, int height) {
  HWND hwnd = (HWND) native_window;
  if (!hwnd || !IsWindow(hwnd)) { return NULL; }

  LwlWindow *window = (LwlWindow*) calloc(1, sizeof(*window));
  if (!window) { return NULL; }
  window->hwnd = hwnd;
  window->cursor_visible = true;
  window->cursor_id = IDC_ARROW;
  if (!resize_framebuffer(window, width, height)) {
    free(window);
    return NULL;
  }
  window->hdc = GetDC(hwnd);
  if (!window->hdc) {
    free(window->pixels);
    free(window->native_pixels);
    free(window);
    return NULL;
  }
  return window;
}

void* lwl_window_get_native_handle(LwlWindow *window) {
  return window ? (void*) window->hwnd : NULL;
}

void lwl_window_destroy(LwlWindow *window) {
  if (!window) { return; }
  if (window->custom_cursor) { DestroyCursor(window->custom_cursor); }
  if (window->hwnd && window->hdc) { ReleaseDC(window->hwnd, window->hdc); }
  if (window->hwnd && window->owns_window) { DestroyWindow(window->hwnd); }
  free(window->pixels);
  free(window->native_pixels);
  free(window);
}

void lwl_window_show(LwlWindow *window) {
  ShowWindow(window->hwnd, SW_SHOW);
  UpdateWindow(window->hwnd);
  window->shown = true;
}

void lwl_window_set_title(LwlWindow *window, const char *title) {
  SetWindowTextA(window->hwnd, title);
}

void lwl_window_set_mode(LwlWindow *window, LwlWindowMode mode) {
  ShowWindow(window->hwnd,
    mode == LWL_WINDOW_MAXIMIZED ? SW_MAXIMIZE :
    mode == LWL_WINDOW_NORMAL ? SW_RESTORE : SW_SHOWMAXIMIZED);
}

bool lwl_window_has_focus(LwlWindow *window) {
  return GetFocus() == window->hwnd;
}

void lwl_window_set_cursor(LwlWindow *window, LwlCursor cursor) {
  LPCTSTR id = IDC_ARROW;
  if (cursor == LWL_CURSOR_IBEAM) { id = IDC_IBEAM; }
  if (cursor == LWL_CURSOR_SIZEH) { id = IDC_SIZEWE; }
  if (cursor == LWL_CURSOR_SIZEV) { id = IDC_SIZENS; }
  if (cursor == LWL_CURSOR_HAND) { id = IDC_HAND; }
  window->cursor_id = id;
  window->use_custom_cursor = false;
  SetCursor(selected_cursor(window));
}

bool lwl_window_set_cursor_image(
    LwlWindow *window, const LwlColor *pixels,
    int width, int height, int hotspot_x, int hotspot_y) {
  if (!window || !pixels || width < 1 || height < 1) { return false; }

  BITMAPV5HEADER header;
  memset(&header, 0, sizeof(header));
  header.bV5Size = sizeof(header);
  header.bV5Width = width;
  header.bV5Height = -height;
  header.bV5Planes = 1;
  header.bV5BitCount = 32;
  header.bV5Compression = BI_BITFIELDS;
  header.bV5RedMask = 0x00ff0000;
  header.bV5GreenMask = 0x0000ff00;
  header.bV5BlueMask = 0x000000ff;
  header.bV5AlphaMask = 0xff000000;

  void *bits = NULL;
  HDC screen = GetDC(NULL);
  HBITMAP color = CreateDIBSection(
    screen, (const BITMAPINFO*) &header, DIB_RGB_COLORS,
    &bits, NULL, 0);
  ReleaseDC(NULL, screen);
  if (!color || !bits) {
    if (color) { DeleteObject(color); }
    return false;
  }
  {
    size_t pixel_count = (size_t) width * (size_t) height;
    LwlColor *native_pixels = (LwlColor*) bits;
    size_t index;
    for (index = 0; index < pixel_count; ++index) {
      native_pixels[index].r = pixels[index].b;
      native_pixels[index].g = pixels[index].g;
      native_pixels[index].b = pixels[index].r;
      native_pixels[index].a = pixels[index].a;
    }
  }
  HBITMAP mask = CreateBitmap(width, height, 1, 1, NULL);
  if (!mask) {
    DeleteObject(color);
    return false;
  }

  ICONINFO info;
  memset(&info, 0, sizeof(info));
  info.fIcon = FALSE;
  info.xHotspot = (DWORD) (hotspot_x < 0 ? 0 :
    hotspot_x >= width ? width - 1 : hotspot_x);
  info.yHotspot = (DWORD) (hotspot_y < 0 ? 0 :
    hotspot_y >= height ? height - 1 : hotspot_y);
  info.hbmMask = mask;
  info.hbmColor = color;
  const HCURSOR cursor = CreateIconIndirect(&info);
  DeleteObject(mask);
  DeleteObject(color);
  if (!cursor) { return false; }

  if (window->custom_cursor) { DestroyCursor(window->custom_cursor); }
  window->custom_cursor = cursor;
  window->use_custom_cursor = true;
  SetCursor(selected_cursor(window));
  return true;
}

void lwl_window_set_cursor_visible(LwlWindow *window, bool visible) {
  if (!window || window->cursor_visible == visible) { return; }
  window->cursor_visible = visible;
  SetCursor(selected_cursor(window));
}

bool lwl_window_set_size(LwlWindow *window, int width, int height) {
  if (!window || width < 1 || height < 1) { return false; }
  DWORD style = (DWORD) GetWindowLongPtr(window->hwnd, GWL_STYLE);
  DWORD ex_style = (DWORD) GetWindowLongPtr(window->hwnd, GWL_EXSTYLE);
  RECT rect = { 0, 0, width, height };
  if (!AdjustWindowRectEx(&rect, style, FALSE, ex_style)) { return false; }
  if (!SetWindowPos(window->hwnd, NULL, 0, 0,
      rect.right - rect.left, rect.bottom - rect.top,
      SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)) {
    return false;
  }
  return resize_framebuffer(window, width, height);
}

void lwl_window_get_size(LwlWindow *window, int *width, int *height) {
  *width = window->width;
  *height = window->height;
}

LwlColor* lwl_window_get_framebuffer(LwlWindow *window, int *width, int *height) {
  if (width) { *width = window->width; }
  if (height) { *height = window->height; }
  return window->pixels;
}

bool lwl_window_resize_framebuffer(LwlWindow *window, int width, int height) {
  return window != NULL && resize_framebuffer(window, width, height);
}

void lwl_window_update_rects(LwlWindow *window, const LwlRect *rects, int count) {
  for (int i = 0; i < count; i++) {
    LwlRect r = rects[i];
    int y;
    if (r.x < 0) { r.width += r.x; r.x = 0; }
    if (r.y < 0) { r.height += r.y; r.y = 0; }
    if (r.x + r.width > window->width) {
      r.width = window->width - r.x;
    }
    if (r.y + r.height > window->height) {
      r.height = window->height - r.y;
    }
    if (r.width <= 0 || r.height <= 0) { continue; }

    for (y = r.y; y < r.y + r.height; ++y) {
      int x;
      for (x = r.x; x < r.x + r.width; ++x) {
        size_t index;
        LwlColor source;
        index = (size_t) y * (size_t) window->width + (size_t) x;
        source = window->pixels[index];
        window->native_pixels[index].r = source.b;
        window->native_pixels[index].g = source.g;
        window->native_pixels[index].b = source.r;
        window->native_pixels[index].a = source.a;
      }
    }
    SetDIBitsToDevice(window->hdc, r.x, r.y, r.width, r.height,
      r.x, window->height - r.y - r.height, 0, window->height,
      window->native_pixels, &window->bmi, DIB_RGB_COLORS);
  }
}

bool lwl_poll_event(LwlWindow *window, LwlEvent *event) {
  if (queue_pop(window, event)) { return true; }
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    if (queue_pop(window, event)) { return true; }
  }
  return false;
}

bool lwl_wait_event(LwlWindow *window, double timeout_seconds) {
  if (window->queue_head != window->queue_tail) { return true; }
  DWORD ms = timeout_seconds < 0 ? INFINITE : (DWORD) (timeout_seconds * 1000.0);
  return MsgWaitForMultipleObjects(0, NULL, FALSE, ms, QS_ALLINPUT) == WAIT_OBJECT_0;
}

char* lwl_clipboard_get(LwlWindow *window) {
  if (!OpenClipboard(window->hwnd)) { return lwl_strdup(""); }
  HANDLE h = GetClipboardData(CF_UNICODETEXT);
  if (!h) {
    CloseClipboard();
    return lwl_strdup("");
  }
  wchar_t *wtext = (wchar_t*) GlobalLock(h);
  int len = WideCharToMultiByte(CP_UTF8, 0, wtext, -1, NULL, 0, NULL, NULL);
  char *text = (char*) malloc(len > 0 ? len : 1);
  if (text && len > 0) {
    WideCharToMultiByte(CP_UTF8, 0, wtext, -1, text, len, NULL, NULL);
  } else if (text) {
    text[0] = '\0';
  }
  GlobalUnlock(h);
  CloseClipboard();
  return text;
}

void lwl_clipboard_set(LwlWindow *window, const char *text) {
  int wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
  HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(wchar_t));
  if (!h) { return; }
  wchar_t *wtext = (wchar_t*) GlobalLock(h);
  MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, wlen);
  GlobalUnlock(h);
  if (OpenClipboard(window->hwnd)) {
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, h);
    CloseClipboard();
  } else {
    GlobalFree(h);
  }
}

char* lwl_select_folder(LwlWindow *window, const char *title) {
  HRESULT hr = OleInitialize(NULL);
  bool ole_initialized = SUCCEEDED(hr);

  BROWSEINFOA info;
  memset(&info, 0, sizeof(info));
  info.hwndOwner = window ? window->hwnd : NULL;
  info.lpszTitle = title ? title : "Select Folder";
  info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

  PIDLIST_ABSOLUTE id = SHBrowseForFolderA(&info);
  if (!id) {
    if (ole_initialized) { OleUninitialize(); }
    return NULL;
  }

  char path[MAX_PATH];
  bool ok = SHGetPathFromIDListA(id, path) != 0;
  CoTaskMemFree(id);
  if (ole_initialized) { OleUninitialize(); }
  return ok ? lwl_strdup(path) : NULL;
}

void lwl_free(void *ptr) {
  free(ptr);
}

double lwl_time_seconds(void) {
  LARGE_INTEGER now;
  QueryPerformanceCounter(&now);
  return now.QuadPart / (double) g_freq.QuadPart;
}

void lwl_sleep_seconds(double seconds) {
  if (seconds > 0) { Sleep((DWORD) (seconds * 1000.0)); }
}

void lwl_sleep_until_seconds(double time_seconds) {
  double now = lwl_time_seconds();
  if (time_seconds > now) {
    lwl_sleep_seconds(time_seconds - now);
  }
}

const char* lwl_platform_name(void) {
  return "Windows";
}

double lwl_display_scale(void) {
  HDC dc = GetDC(NULL);
  int dpi = GetDeviceCaps(dc, LOGPIXELSX);
  ReleaseDC(NULL, dc);
  return dpi / 96.0;
}

bool lwl_exe_path(char *buf, int size) {
  DWORD len = GetModuleFileNameA(NULL, buf, size);
  if (len == 0 || len >= (DWORD) size) { return false; }
  return true;
}

LwlGlConfig lwl_gl_config_default(void) {
  LwlGlConfig config;
  config.api = LWL_GL_API_DESKTOP;
  config.major_version = 3;
  config.minor_version = 3;
  config.depth_bits = 24;
  config.stencil_bits = 8;
  config.core_profile = true;
  config.debug = false;
  config.double_buffer = true;
  return config;
}

void* lwl_gl_get_proc_address(const char *name) {
  PROC address;
  void *result;
  if (!name || !name[0]) { return NULL; }

  address = wglGetProcAddress(name);
  if (address &&
      address != (PROC) (intptr_t) 1 &&
      address != (PROC) (intptr_t) 2 &&
      address != (PROC) (intptr_t) 3 &&
      address != (PROC) (intptr_t) -1) {
    result = NULL;
    if (sizeof(result) == sizeof(address)) {
      memcpy(&result, &address, sizeof(result));
    }
    return result;
  }

  if (!g_opengl_module) {
    g_opengl_module = LoadLibraryA("opengl32.dll");
  }
  address = g_opengl_module
    ? GetProcAddress(g_opengl_module, name)
    : NULL;
  result = NULL;
  if (address && sizeof(result) == sizeof(address)) {
    memcpy(&result, &address, sizeof(result));
  }
  return result;
}

LwlGlContext* lwl_gl_context_create(
    LwlWindow *window, const LwlGlConfig *requested_config) {
  typedef HGLRC (WINAPI *LwlWglCreateContextAttribs)(
    HDC device, HGLRC shared, const int *attributes);
  enum {
    LWL_WGL_CONTEXT_MAJOR_VERSION = 0x2091,
    LWL_WGL_CONTEXT_MINOR_VERSION = 0x2092,
    LWL_WGL_CONTEXT_FLAGS = 0x2094,
    LWL_WGL_CONTEXT_PROFILE_MASK = 0x9126,
    LWL_WGL_CONTEXT_DEBUG_BIT = 0x0001,
    LWL_WGL_CONTEXT_CORE_PROFILE_BIT = 0x00000001,
    LWL_WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT = 0x00000002
  };
  LwlGlConfig config;
  PIXELFORMATDESCRIPTOR descriptor;
  HGLRC previous_context;
  HDC previous_device;
  HGLRC bootstrap;
  HGLRC handle;
  LwlWglCreateContextAttribs create_context;
  int format;
  int attributes[9];
  int attribute_count;
  LwlGlContext *context;

  if (!window || !window->hdc) { return NULL; }
  config = requested_config ? *requested_config : lwl_gl_config_default();
  if (config.api != LWL_GL_API_DESKTOP ||
      config.major_version < 1 || config.minor_version < 0 ||
      config.depth_bits < 0 || config.depth_bits > 255 ||
      config.stencil_bits < 0 || config.stencil_bits > 255 ||
      (config.core_profile &&
       (config.major_version < 3 ||
        (config.major_version == 3 && config.minor_version < 2)))) {
    return NULL;
  }

  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.nSize = sizeof(descriptor);
  descriptor.nVersion = 1;
  descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
  if (config.double_buffer) { descriptor.dwFlags |= PFD_DOUBLEBUFFER; }
  descriptor.iPixelType = PFD_TYPE_RGBA;
  descriptor.cColorBits = 32;
  descriptor.cAlphaBits = 8;
  descriptor.cDepthBits = (BYTE) config.depth_bits;
  descriptor.cStencilBits = (BYTE) config.stencil_bits;
  descriptor.iLayerType = PFD_MAIN_PLANE;

  if (GetPixelFormat(window->hdc) == 0) {
    format = ChoosePixelFormat(window->hdc, &descriptor);
    if (format == 0 ||
        !SetPixelFormat(window->hdc, format, &descriptor)) {
      return NULL;
    }
  }

  previous_context = wglGetCurrentContext();
  previous_device = wglGetCurrentDC();
  bootstrap = wglCreateContext(window->hdc);
  if (!bootstrap || !wglMakeCurrent(window->hdc, bootstrap)) {
    if (bootstrap) { wglDeleteContext(bootstrap); }
    return NULL;
  }

  {
    void *address =
      lwl_gl_get_proc_address("wglCreateContextAttribsARB");
    create_context = NULL;
    if (address && sizeof(address) == sizeof(create_context)) {
      memcpy(&create_context, &address, sizeof(create_context));
    }
  }
  if (!create_context) {
    wglMakeCurrent(previous_device, previous_context);
    wglDeleteContext(bootstrap);
    return NULL;
  }

  attribute_count = 0;
  attributes[attribute_count++] = LWL_WGL_CONTEXT_MAJOR_VERSION;
  attributes[attribute_count++] = config.major_version;
  attributes[attribute_count++] = LWL_WGL_CONTEXT_MINOR_VERSION;
  attributes[attribute_count++] = config.minor_version;
  if (config.major_version > 3 ||
      (config.major_version == 3 && config.minor_version >= 2)) {
    attributes[attribute_count++] = LWL_WGL_CONTEXT_PROFILE_MASK;
    attributes[attribute_count++] = config.core_profile
      ? LWL_WGL_CONTEXT_CORE_PROFILE_BIT
      : LWL_WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT;
  }
  if (config.debug) {
    attributes[attribute_count++] = LWL_WGL_CONTEXT_FLAGS;
    attributes[attribute_count++] = LWL_WGL_CONTEXT_DEBUG_BIT;
  }
  attributes[attribute_count] = 0;

  handle = create_context(window->hdc, NULL, attributes);
  wglMakeCurrent(previous_device, previous_context);
  wglDeleteContext(bootstrap);
  if (!handle) { return NULL; }

  context = (LwlGlContext*) calloc(1, sizeof(*context));
  if (!context) {
    wglDeleteContext(handle);
    return NULL;
  }
  context->window = window;
  context->handle = handle;
  context->double_buffer = config.double_buffer;
  return context;
}

void lwl_gl_context_destroy(LwlGlContext *context) {
  if (!context) { return; }
  if (wglGetCurrentContext() == context->handle) {
    wglMakeCurrent(NULL, NULL);
  }
  if (context->handle) { wglDeleteContext(context->handle); }
  free(context);
}

bool lwl_gl_context_make_current(LwlGlContext *context) {
  if (!context) { return wglMakeCurrent(NULL, NULL) != FALSE; }
  return wglMakeCurrent(
    context->window->hdc, context->handle) != FALSE;
}

void lwl_gl_context_swap_buffers(LwlGlContext *context) {
  if (!context || !context->double_buffer) { return; }
  SwapBuffers(context->window->hdc);
}

bool lwl_gl_context_set_swap_interval(
    LwlGlContext *context, int interval) {
  typedef BOOL (WINAPI *LwlWglSwapInterval)(int interval);
  LwlWglSwapInterval swap_interval;
  if (!context || !lwl_gl_context_make_current(context)) { return false; }
  {
    void *address = lwl_gl_get_proc_address("wglSwapIntervalEXT");
    swap_interval = NULL;
    if (address && sizeof(address) == sizeof(swap_interval)) {
      memcpy(&swap_interval, &address, sizeof(swap_interval));
    }
  }
  return swap_interval ? swap_interval(interval) != FALSE : false;
}

#else
typedef int lwl_win32_backend_not_selected;
#endif
