#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <algorithm>
#include <cstdint>

namespace osf {
namespace {

bool overlaps(
    std::int32_t first_left,
    std::int32_t first_top,
    std::int32_t first_right,
    std::int32_t first_bottom,
    std::int32_t second_left,
    std::int32_t second_top,
    std::int32_t second_right,
    std::int32_t second_bottom) {
    return first_left <= second_right &&
           second_left <= first_right &&
           first_top <= second_bottom &&
           second_top <= first_bottom;
}

bool groundIsWalkable(
    const GroundMap& ground,
    std::int32_t left,
    std::int32_t top,
    std::int32_t right,
    std::int32_t bottom) {
    if (ground.judgeWidth() <= 0 ||
        ground.judgeHeight() <= 0) {
        return true;
    }

    const std::int32_t cell_width =
        ground.baseMagnificationX();
    const std::int32_t cell_height =
        ground.baseMagnificationY();
    if (cell_width <= 0 || cell_height <= 0) {
        return false;
    }

    std::int32_t first_x = left / cell_width - 1;
    std::int32_t first_y = top / cell_height - 1;
    std::int32_t last_x = right / cell_width + 1;
    std::int32_t last_y = bottom / cell_height + 1;
    first_x = std::clamp(
        first_x,
        ground.judgeOffsetX(),
        ground.judgeOffsetX() + ground.judgeWidth() - 1);
    last_x = std::clamp(
        last_x,
        ground.judgeOffsetX(),
        ground.judgeOffsetX() + ground.judgeWidth() - 1);
    first_y = std::clamp(
        first_y,
        ground.judgeOffsetY(),
        ground.judgeOffsetY() + ground.judgeHeight() - 1);
    last_y = std::clamp(
        last_y,
        ground.judgeOffsetY(),
        ground.judgeOffsetY() + ground.judgeHeight() - 1);

    for (std::int32_t y = first_y; y <= last_y; ++y) {
        for (std::int32_t x = first_x; x <= last_x; ++x) {
            const std::int16_t* value = ground.judge(x, y);
            if (!value || (*value & 1) == 0) {
                continue;
            }
            if (overlaps(
                    left,
                    top,
                    right,
                    bottom,
                    x * cell_width,
                    y * cell_height,
                    (x + 1) * cell_width - 1,
                    (y + 1) * cell_height - 1)) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

bool positionIsWalkable(
    const GroundMap& ground,
    const ObjectMap& objects,
    WorldPosition position,
    const ObjectBounds& bounds,
    bool exclude_special_objects) {
    const std::int32_t left = position.x + bounds.left;
    const std::int32_t top = position.y + bounds.top;
    const std::int32_t right = position.x + bounds.right;
    const std::int32_t bottom = position.y + bounds.bottom;

    for (const MapObject& object : objects.objects()) {
        if ((object.status & 1) == 0 ||
            (exclude_special_objects &&
             (object.status & 3) == 3)) {
            continue;
        }
        if (overlaps(
                left,
                top,
                right,
                bottom,
                object.world_x + object.judgement.left,
                object.world_y + object.judgement.top,
                object.world_x + object.judgement.right,
                object.world_y + object.judgement.bottom)) {
            return false;
        }
    }
    return groundIsWalkable(
        ground, left, top, right, bottom);
}

}  // namespace osf
