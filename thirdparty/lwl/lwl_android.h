#ifndef LWL_ANDROID_H
#define LWL_ANDROID_H

#include <stdbool.h>

struct android_app;

#ifdef __cplusplus
extern "C" {
#endif

void lwl_android_set_application(struct android_app *application);
bool lwl_android_wait_for_window(void);

#ifdef __cplusplus
}
#endif

#endif
