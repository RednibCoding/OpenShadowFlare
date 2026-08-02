#include "runtime/application_loop.hpp"

#include <3ds.h>

namespace osf::runtime {

int runApplicationLoop(std::unique_ptr<FrameApplication> application) {
    bool is_new_3ds = false;
    if (R_SUCCEEDED(APT_CheckNew3DS(&is_new_3ds)) && is_new_3ds) {
        osSetSpeedupEnable(true);
    }

    while (aptMainLoop() && application->frame()) {
    }
    return 0;
}

}  // namespace osf::runtime
