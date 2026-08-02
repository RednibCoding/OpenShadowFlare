#include "runtime/application_loop.hpp"

#include <whb/proc.h>

namespace osf::runtime {

int runApplicationLoop(std::unique_ptr<FrameApplication> application) {
    while (WHBProcIsRunning() && application->frame()) {
    }
    return 0;
}

}  // namespace osf::runtime
