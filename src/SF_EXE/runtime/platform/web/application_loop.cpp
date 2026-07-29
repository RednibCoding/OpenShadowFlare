#include "runtime/application_loop.hpp"

#include <emscripten.h>

namespace osf::runtime {
namespace {

void runFrame(void* user_data) {
    auto* application =
        static_cast<FrameApplication*>(user_data);
    if (application->frame()) {
        return;
    }

    delete application;
    emscripten_cancel_main_loop();
}

}  // namespace

int runApplicationLoop(std::unique_ptr<FrameApplication> application) {
    FrameApplication* persistentApplication = application.release();
    emscripten_set_main_loop_arg(
        runFrame, persistentApplication, 0, 1);
    return 0;
}

}  // namespace osf::runtime
