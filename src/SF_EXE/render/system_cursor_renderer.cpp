#include "system_cursor_renderer.hpp"

#include <cstddef>

namespace osf {

void renderSystemCursor(
    gapi::Backend& renderer,
    const gapi::NjpImage& patterns,
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    bool identification_active) {
    renderer.drawPattern(
        patterns,
        identification_active ? std::size_t{1} : std::size_t{0},
        {pointer_x, pointer_y});
}

}  // namespace osf
