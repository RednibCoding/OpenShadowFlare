#include "loading_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include <algorithm>
#include <cstdint>

namespace osf {

void renderInitialLoadingScreen(
    gapi::Backend& renderer,
    const gapi::NjpImage& waiting,
    std::int32_t counter,
    bool ready_to_continue) {
    renderer.drawPattern(waiting, 0, {0, 0});
    if (!ready_to_continue) {
        renderer.drawPattern(waiting, 3, {572, 443});
        return;
    }

    const std::int32_t arrow_offset =
        std::max(counter, 0) % 16;
    renderer.drawPattern(
        waiting, 2, {592 + arrow_offset, 450});
}

}  // namespace osf
