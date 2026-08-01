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
        std::max<std::int32_t>(counter, 0) % 16;
    renderer.drawPattern(
        waiting, 2, {592 + arrow_offset, 450});
}

void renderScenarioLoadingScreen(
    gapi::Backend& renderer,
    const gapi::NjpImage& waiting,
    const gapi::NjpImage& wait_icon,
    std::int32_t counter) {
    const std::int32_t strength = std::clamp<std::int32_t>(
        std::max<std::int32_t>(counter, 0) * 1000 / 120,
        0,
        1000);
    renderer.drawPattern(
        waiting,
        4,
        {
            0,
            0,
            1000,
            1000,
            1000,
            1000,
            strength,
            strength,
            strength,
        });

    const std::int32_t phase =
        std::max<std::int32_t>(counter, 0) % 15;
    const std::int32_t offset =
        phase < 5 ? 0 : phase < 10 ? 8 : 16;
    renderer.drawPattern(
        wait_icon,
        0,
        {
            590 + offset,
            440,
            1000,
            1000,
            1000,
            1000,
            strength,
            strength,
            strength,
        });
}

}  // namespace osf
