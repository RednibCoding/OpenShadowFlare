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

}  // namespace osf

#endif
