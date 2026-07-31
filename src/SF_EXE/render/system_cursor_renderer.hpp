#ifndef OPENSHADOWFLARE_SYSTEM_CURSOR_RENDERER_HPP
#define OPENSHADOWFLARE_SYSTEM_CURSOR_RENDERER_HPP

#include "gapi/gapi.hpp"

#include <cstdint>

namespace osf {

void renderSystemCursor(
    gapi::Backend& renderer,
    const gapi::NjpImage& patterns,
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    bool identification_active);

}  // namespace osf

#endif
