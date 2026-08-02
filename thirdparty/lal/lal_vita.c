/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of LAL.
 *
 * LAL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "lal_internal.h"

#include <stdbool.h>

bool lal_platform_init(void) {
  return true;
}

void lal_platform_shutdown(void) {
}

void lal_platform_lock(void) {
}

void lal_platform_unlock(void) {
}
