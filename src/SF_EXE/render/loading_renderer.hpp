#ifndef OPENSHADOWFLARE_LOADING_RENDERER_HPP
#define OPENSHADOWFLARE_LOADING_RENDERER_HPP

#include <cstdint>

namespace osf {

namespace gapi {
class Backend;
class NjpImage;
}

void renderInitialLoadingScreen(
    gapi::Backend& renderer,
    const gapi::NjpImage& waiting,
    std::int32_t counter,
    bool ready_to_continue);

void renderScenarioLoadingScreen(
    gapi::Backend& renderer,
    const gapi::NjpImage& waiting,
    const gapi::NjpImage& wait_icon,
    std::int32_t counter);

}  // namespace osf

#endif
