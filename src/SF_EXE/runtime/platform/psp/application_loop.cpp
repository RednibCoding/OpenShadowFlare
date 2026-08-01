#include "runtime/application_loop.hpp"

extern "C" void osf_psp_setup_callbacks(void);

namespace osf::runtime {

int runApplicationLoop(std::unique_ptr<FrameApplication> application) {
    osf_psp_setup_callbacks();
    while (application->frame()) {
    }
    return 0;
}

}  // namespace osf::runtime
