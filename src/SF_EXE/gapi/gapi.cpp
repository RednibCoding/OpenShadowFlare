#include "gapi.hpp"

#include <algorithm>

namespace osf::gapi {

Viewport fitViewport(
    std::int32_t source_width,
    std::int32_t source_height,
    std::int32_t target_width,
    std::int32_t target_height) {
    Viewport result;
    if (source_width <= 0 || source_height <= 0 ||
        target_width <= 0 || target_height <= 0) {
        return result;
    }

    result.width = target_width;
    result.height = static_cast<std::int32_t>(
        static_cast<std::int64_t>(target_width) *
        source_height / source_width);
    if (result.height > target_height) {
        result.height = target_height;
        result.width = static_cast<std::int32_t>(
            static_cast<std::int64_t>(target_height) *
            source_width / source_height);
    }
    result.width = std::max<std::int32_t>(result.width, 1);
    result.height = std::max<std::int32_t>(result.height, 1);
    result.x = (target_width - result.width) / 2;
    result.y = (target_height - result.height) / 2;
    return result;
}

}  // namespace osf::gapi
