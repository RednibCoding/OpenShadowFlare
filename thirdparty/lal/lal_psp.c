#include "lal_internal.h"

#include <stdbool.h>

bool lal_platform_init(void) {
  lal_set_error("The PSP audio backend is not implemented yet.");
  return false;
}

void lal_platform_shutdown(void) {
}

void lal_platform_lock(void) {
}

void lal_platform_unlock(void) {
}
