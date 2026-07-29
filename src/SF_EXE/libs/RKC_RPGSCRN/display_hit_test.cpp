#include "rkc_rpgscrn.hpp"

#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

std::uint8_t pixelIndex(
    const gapi::NjpPart& part,
    std::int32_t x,
    std::int32_t y) {
    const std::int32_t source_y = part.height - y - 1;
    const std::size_t row =
        static_cast<std::size_t>(source_y) *
        static_cast<std::size_t>(part.stride);
    if (part.bits_per_pixel == 8) {
        return part.pixels[
            row + static_cast<std::size_t>(x)];
    }
    if (part.bits_per_pixel == 4) {
        const std::uint8_t packed =
            part.pixels[
                row + static_cast<std::size_t>(x / 2)];
        return static_cast<std::uint8_t>(
            (packed >> ((1 - (x & 1)) * 4)) & 0x0f);
    }
    const std::uint8_t packed =
        part.pixels[
            row + static_cast<std::size_t>(x / 8)];
    return static_cast<std::uint8_t>(
        (packed >> (7 - (x & 7))) & 1);
}

}  // namespace

bool displayPatternIntersectsRectangle(
    const gapi::NjpImage& image,
    std::size_t pattern_index,
    ScreenPosition anchor,
    DisplayHitRectangle rectangle,
    std::int32_t height) {
    if (pattern_index >= image.patterns().size() ||
        rectangle.left > rectangle.right ||
        rectangle.top > rectangle.bottom) {
        return false;
    }
    const gapi::NjpPattern& pattern =
        image.patterns()[pattern_index];
    for (const gapi::NjpPatternPart& pattern_part :
         pattern.parts) {
        if (pattern_part.part_index < 0 ||
            static_cast<std::size_t>(
                pattern_part.part_index) >=
                image.parts().size() ||
            pattern_part.scale_x <= 0 ||
            pattern_part.scale_y <= 0) {
            continue;
        }
        const gapi::NjpPart& part =
            image.parts()[static_cast<std::size_t>(
                pattern_part.part_index)];
        const std::int32_t width =
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(part.width) *
                pattern_part.scale_x / 1000);
        const std::int32_t part_height =
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(part.height) *
                pattern_part.scale_y / 1000);
        const std::int32_t left =
            anchor.x + pattern_part.x;
        const std::int32_t top =
            anchor.y + pattern_part.y - height;
        if (width <= 0 || part_height <= 0) {
            continue;
        }
        const std::int32_t overlap_left =
            std::max(left, rectangle.left);
        const std::int32_t overlap_top =
            std::max(top, rectangle.top);
        const std::int32_t overlap_right =
            std::min(left + width - 1, rectangle.right);
        const std::int32_t overlap_bottom =
            std::min(top + part_height - 1, rectangle.bottom);
        for (std::int32_t y = overlap_top;
             y <= overlap_bottom;
             ++y) {
            const std::int32_t source_y =
                static_cast<std::int32_t>(
                    static_cast<std::int64_t>(y - top) *
                    part.height / part_height);
            for (std::int32_t x = overlap_left;
                 x <= overlap_right;
                 ++x) {
                const std::int32_t source_x =
                    static_cast<std::int32_t>(
                        static_cast<std::int64_t>(
                            x - left) *
                        part.width / width);
                if (pixelIndex(
                        part, source_x, source_y) != 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool displayPatternContainsPoint(
    const gapi::NjpImage& image,
    std::size_t pattern_index,
    ScreenPosition anchor,
    ScreenPosition point,
    std::int32_t height) {
    return displayPatternIntersectsRectangle(
        image,
        pattern_index,
        anchor,
        {point.x, point.y, point.x, point.y},
        height);
}

bool displayAnimationIntersectsRectangle(
    const gapi::CafAnimation& animation,
    const gapi::NjpImage& patterns,
    WorldPosition position,
    std::int32_t chart_index,
    std::int32_t direction_index,
    std::int32_t animation_frame,
    const DisplayPartEnabled& part_enabled,
    std::int32_t camera_x,
    std::int32_t camera_y,
    DisplayHitRectangle rectangle,
    std::int32_t height) {
    if (animation.charts().empty()) {
        return false;
    }
    chart_index = std::clamp(
        chart_index,
        0,
        static_cast<std::int32_t>(
            animation.charts().size() - 1));
    const gapi::CafChart& chart =
        animation.charts()[static_cast<std::size_t>(
            chart_index)];
    if (direction_index < 0 ||
        static_cast<std::size_t>(direction_index) >=
            chart.directions.size()) {
        return false;
    }
    const gapi::CafDirection& direction =
        chart.directions[static_cast<std::size_t>(
            direction_index)];
    if (direction.frame_count <= 0) {
        return false;
    }
    if ((chart.status & 1) != 0) {
        animation_frame %= direction.frame_count;
    }
    if (animation_frame < 0 ||
        animation_frame >= direction.frame_count) {
        animation_frame = 0;
    }
    const ScreenPosition projected =
        calculateRealPosition(position);
    const ScreenPosition anchor{
        projected.x - camera_x,
        projected.y - camera_y,
    };
    for (std::size_t part_index = 0;
         part_index < direction.parts.size();
         ++part_index) {
        if (!part_enabled(part_index)) {
            continue;
        }
        const auto& part = direction.parts[part_index];
        if (static_cast<std::size_t>(animation_frame) >=
            part.size()) {
            continue;
        }
        const gapi::CafCell& cell =
            part[static_cast<std::size_t>(
                animation_frame)];
        if ((cell.status & 8) != 0 ||
            cell.pattern_index < 0) {
            continue;
        }
        if (displayPatternIntersectsRectangle(
                patterns,
                static_cast<std::size_t>(
                    cell.pattern_index),
                anchor,
                rectangle,
                height)) {
            return true;
        }
    }
    return false;
}

bool displayAnimationContainsPoint(
    const gapi::CafAnimation& animation,
    const gapi::NjpImage& patterns,
    WorldPosition position,
    std::int32_t chart_index,
    std::int32_t direction_index,
    std::int32_t animation_frame,
    const DisplayPartEnabled& part_enabled,
    std::int32_t camera_x,
    std::int32_t camera_y,
    ScreenPosition point,
    std::int32_t height) {
    return displayAnimationIntersectsRectangle(
        animation,
        patterns,
        position,
        chart_index,
        direction_index,
        animation_frame,
        part_enabled,
        camera_x,
        camera_y,
        {point.x, point.y, point.x, point.y},
        height);
}

}  // namespace osf
