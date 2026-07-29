#include "runtime/application_loop.hpp"

namespace osf::runtime {

int runApplicationLoop(std::unique_ptr<FrameApplication> application) {
    while (application->frame()) {
    }
    return 0;
}

}  // namespace osf::runtime
