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

typedef struct {
  BITMAPINFOHEADER header;
  DWORD masks[3];
} TwlWin32Bitmap;

TwlResult twl_backend_present(Twl *twl, const TwlSurface *surface) {
  TwlWin32 *win32 = twl ? (TwlWin32 *) twl->backend : NULL;
  TwlWin32Bitmap bitmap;
  size_t expected_stride;
  int result;
  if (!win32 || !win32->device_context || !surface) {
    return TWL_RESULT_INVALID_ARGUMENT;
  }
  expected_stride = (size_t) surface->width *
    (surface->format == TWL_PIXEL_XRGB8888 ? 4u : 2u);
  expected_stride = (expected_stride + 3u) & ~(size_t) 3u;
  if (surface->stride_bytes != expected_stride)
    return TWL_RESULT_INVALID_ARGUMENT;
  twl_internal_zero(&bitmap, sizeof(bitmap));
  bitmap.header.biSize = sizeof(bitmap.header);
  bitmap.header.biWidth = (LONG) surface->width;
  bitmap.header.biHeight = -(LONG) surface->height;
  bitmap.header.biPlanes = 1;
  bitmap.header.biBitCount =
    surface->format == TWL_PIXEL_XRGB8888 ? 32u : 16u;
  if (surface->format == TWL_PIXEL_XRGB8888) {
    bitmap.header.biCompression = BI_RGB;
  } else {
    bitmap.header.biCompression = BI_BITFIELDS;
    bitmap.masks[0] = 0x001fu;
    bitmap.masks[1] = surface->format == TWL_PIXEL_RGB565
      ? 0x07e0u : 0x03e0u;
    bitmap.masks[2] = surface->format == TWL_PIXEL_RGB565
      ? 0xf800u : 0x7c00u;
  }
  result = StretchDIBits(
    win32->device_context,
    0, 0, (int) twl->display_width, (int) twl->display_height,
    0, 0, (int) surface->width, (int) surface->height,
    surface->pixels, (const BITMAPINFO *) &bitmap,
    DIB_RGB_COLORS, SRCCOPY);
  return result == 0 || result == (int) GDI_ERROR
    ? TWL_RESULT_BACKEND_FAILURE : TWL_RESULT_OK;
}
