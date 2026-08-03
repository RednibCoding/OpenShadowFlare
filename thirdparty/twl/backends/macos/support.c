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

TwlRect twl_rect(
    TwlCGFloat x, TwlCGFloat y, TwlCGFloat width, TwlCGFloat height) {
  TwlRect result = {{x, y}, {width, height}};
  return result;
}

id twl_msg_id(id object, const char *selector) {
  return ((id (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector));
}

id twl_msg_id_id(id object, const char *selector, id argument) {
  return ((id (*)(id, SEL, id)) objc_msgSend)(
    object, sel_registerName(selector), argument);
}

void twl_msg_void(id object, const char *selector) {
  ((void (*)(id, SEL)) objc_msgSend)(object, sel_registerName(selector));
}

void twl_msg_void_id(id object, const char *selector, id argument) {
  ((void (*)(id, SEL, id)) objc_msgSend)(
    object, sel_registerName(selector), argument);
}

void twl_msg_void_bool(id object, const char *selector, bool value) {
  ((void (*)(id, SEL, BOOL)) objc_msgSend)(
    object, sel_registerName(selector), value ? YES : NO);
}

void twl_msg_void_integer(
    id object, const char *selector, TwlNSInteger value) {
  ((void (*)(id, SEL, TwlNSInteger)) objc_msgSend)(
    object, sel_registerName(selector), value);
}

bool twl_msg_bool(id object, const char *selector) {
  return object && ((BOOL (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector)) != NO;
}

TwlNSInteger twl_msg_integer(id object, const char *selector) {
  return ((TwlNSInteger (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector));
}

TwlNSUInteger twl_msg_count(id object) {
  return object ? ((TwlNSUInteger (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName("count")) : 0u;
}

id twl_msg_object_at(id object, TwlNSUInteger index) {
  return ((id (*)(id, SEL, TwlNSUInteger)) objc_msgSend)(
    object, sel_registerName("objectAtIndex:"), index);
}

double twl_msg_double(id object, const char *selector) {
  return object ? ((double (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector)) : 0.0;
}

float twl_msg_float(id object, const char *selector) {
  return object ? ((float (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector)) : 0.0f;
}

TwlPoint twl_msg_point(id object, const char *selector) {
  return ((TwlPoint (*)(id, SEL)) objc_msgSend)(
    object, sel_registerName(selector));
}

TwlRect twl_msg_rect(id object, const char *selector) {
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

id twl_ns_string(const char *text) {
  return ((id (*)(id, SEL, const char *)) objc_msgSend)(
    (id) objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"),
    text ? text : "");
}

const char *twl_ns_utf8(id string) {
  return string ? ((const char *(*)(id, SEL)) objc_msgSend)(
    string, sel_registerName("UTF8String")) : "";
}

id twl_optional_id(id object, const char *selector) {
  SEL selected = sel_registerName(selector);
  if (!object || !((BOOL (*)(id, SEL, SEL)) objc_msgSend)(
        object, sel_registerName("respondsToSelector:"), selected)) {
    return nil;
  }
  return ((id (*)(id, SEL)) objc_msgSend)(object, selected);
}
