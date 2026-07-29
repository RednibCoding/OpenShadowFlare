#ifndef OPENSHADOWFLARE_LIBS_RKC_RPGSCRN_DISPLAY_HIT_TEST_HPP
#define OPENSHADOWFLARE_LIBS_RKC_RPGSCRN_DISPLAY_HIT_TEST_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace osf {

using DisplayPartEnabled =
    std::function<bool(std::size_t)>;

struct DisplayHitRectangle {
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t right = 0;
    std::int32_t bottom = 0;
};

bool displayPatternIntersectsRectangle(
    const gapi::NjpImage& image,
    std::size_t pattern_index,
    ScreenPosition anchor,
    DisplayHitRectangle rectangle,
    std::int32_t height = 0);

bool displayPatternContainsPoint(
    const gapi::NjpImage& image,
    std::size_t pattern_index,
    ScreenPosition anchor,
    ScreenPosition point,
    std::int32_t height = 0);

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
    std::int32_t height = 0);

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
    std::int32_t height = 0);

}  // namespace osf

#endif
