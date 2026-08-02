#include "runtime/application_loop.hpp"

#include <switch.h>

namespace osf::runtime {

int runApplicationLoop(std::unique_ptr<FrameApplication> application) {
    while (appletMainLoop() && application->frame()) {
    }
    return 0;
}

}
