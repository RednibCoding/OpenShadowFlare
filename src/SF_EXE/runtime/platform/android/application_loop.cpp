#include "runtime/application_loop.hpp"

#include "runtime/game_runtime.hpp"

#include "lwl_android.h"

#include <android_native_app_glue.h>

#include <memory>

extern "C" void android_main(android_app* application) {
    lwl_android_set_application(application);
    if (lwl_android_wait_for_window()) {
        osf::runtime::runMain(0, nullptr);
    }
    lwl_android_set_application(nullptr);
}

namespace osf::runtime {

int runApplicationLoop(std::unique_ptr<FrameApplication> application) {
    while (application->frame()) {
    }
    return 0;
}

}  // namespace osf::runtime
