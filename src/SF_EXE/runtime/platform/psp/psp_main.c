#include <pspkernel.h>
#include <pspthreadman.h>

PSP_MODULE_INFO("OpenShadowFlare", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
// Claim (almost) all available memory for the malloc heap. A negative value
// leaves that many KB free for the partition (utility threads, the on-screen
// keyboard).
PSP_HEAP_SIZE_KB(-2048);

static int osf_psp_exit_callback(int arg1, int arg2, void *common) {
  (void) arg1;
  (void) arg2;
  (void) common;
  sceKernelExitGame();
  return 0;
}

static int osf_psp_callback_thread(SceSize args, void *argp) {
  (void) args;
  (void) argp;
  const int callback_id = sceKernelCreateCallback(
      "OpenShadowFlare Exit", osf_psp_exit_callback, NULL);
  sceKernelRegisterExitCallback(callback_id);
  sceKernelSleepThreadCB();
  return 0;
}

void osf_psp_setup_callbacks(void) {
  const int thread_id = sceKernelCreateThread(
      "osf_callbacks", osf_psp_callback_thread, 0x11, 0xFA0,
      THREAD_ATTR_USER, NULL);
  if (thread_id >= 0) {
    sceKernelStartThread(thread_id, 0, NULL);
  }
}
