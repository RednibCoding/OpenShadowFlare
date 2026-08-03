#include "gapi.hpp"

#include <algorithm>
#include <cstdlib>

namespace osf::gapi {

bool Backend::drawBitMask(
    const BitMaskImage&,
    const BitMaskDraw&) {
    return false;
}

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
    result.width = std::max(result.width, std::int32_t{1});
    result.height = std::max(result.height, std::int32_t{1});
    result.x = (target_width - result.width) / 2;
    result.y = (target_height - result.height) / 2;
    return result;
}

bool Backend::drawLine(const LineDraw& draw) {
    const bool clipped =
        draw.clip.width > 0 && draw.clip.height > 0;
    const auto inside_clip = [&draw, clipped](
                                 std::int64_t x,
                                 std::int64_t y) {
        return !clipped ||
               (x >= draw.clip.x && y >= draw.clip.y &&
                x < static_cast<std::int64_t>(draw.clip.x) +
                        draw.clip.width &&
                y < static_cast<std::int64_t>(draw.clip.y) +
                        draw.clip.height);
    };

    std::int64_t x = draw.start_x;
    std::int64_t y = draw.start_y;
    const std::int64_t end_x = draw.end_x;
    const std::int64_t end_y = draw.end_y;
    const std::int64_t delta_x = std::abs(end_x - x);
    const std::int64_t delta_y = -std::abs(end_y - y);
    const std::int64_t step_x = x < end_x ? 1 : -1;
    const std::int64_t step_y = y < end_y ? 1 : -1;
    std::int64_t error = delta_x + delta_y;
    bool result = true;
    for (;;) {
        if (inside_clip(x, y)) {
            result = drawRectangle({
                         static_cast<std::int32_t>(x),
                         static_cast<std::int32_t>(y),
                         1,
                         1,
                         draw.color,
                         draw.brightness,
                         draw.opacity,
                     }) &&
                     result;
        }
        if (x == end_x && y == end_y) {
            break;
        }
        const std::int64_t doubled_error = error * 2;
        if (doubled_error >= delta_y) {
            error += delta_y;
            x += step_x;
        }
        if (doubled_error <= delta_x) {
            error += delta_x;
            y += step_y;
        }
    }
    return result;
}

}  // namespace osf::gapi
