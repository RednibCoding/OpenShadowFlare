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

#include <whb/proc.h>

void twl_backend_pump_events(Twl *twl) {
  TwlWiiU *wiiu = twl ? (TwlWiiU *) twl->backend : NULL;
  if (!wiiu) {
    return;
  }
  if (!wiiu->quit_pushed && !WHBProcIsRunning()) {
    TwlEvent event;
    twl_internal_zero(&event, sizeof(event));
    event.type = TWL_EVENT_QUIT;
    event.timestamp_us = twl_backend_time_microseconds(twl);
    twl_internal_push_event(twl, &event);
    wiiu->quit_pushed = true;
  }
}
