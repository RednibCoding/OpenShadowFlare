#include "ground_item.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace osf {
namespace {

constexpr std::int32_t kGoldCategory = 4;
constexpr std::int32_t kGoldDefinition = 0;
constexpr std::int32_t kMaximumGoldStack = 10000;
constexpr double kGoldDropRadius = 200.0;
constexpr double kGoldDropAngleStep = 0.3141592;

}  // namespace

bool createGroundItems(
    std::vector<GroundItem>& items,
    RetailRandom& random,
    std::int32_t category,
    std::int32_t definition_id,
    WorldPosition position,
    std::int32_t minimum_quantity,
    std::int32_t maximum_quantity) {
    if (category != kGoldCategory ||
        definition_id != kGoldDefinition) {
        items.push_back({
            category,
            definition_id,
            1,
            position,
            -1,
            -1,
            0,
            1600,
            280,
            0,
        });
        return true;
    }

    if (minimum_quantity < 0 ||
        maximum_quantity < minimum_quantity) {
        return false;
    }
    const std::int64_t range =
        static_cast<std::int64_t>(maximum_quantity) -
        minimum_quantity + 1;
    if (range <= 0 ||
        range > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }

    std::int32_t remaining =
        minimum_quantity +
        random.next() % static_cast<std::int32_t>(range);
    double angle = 0.0;
    while (remaining > 0) {
        const std::int32_t quantity =
            std::min(remaining, kMaximumGoldStack);
        const WorldPosition drop_position{
            position.x + static_cast<std::int32_t>(
                             std::cos(angle) * kGoldDropRadius),
            position.y - static_cast<std::int32_t>(
                             std::sin(angle) * kGoldDropRadius),
        };
        items.push_back({
            category,
            definition_id,
            quantity,
            drop_position,
            -1,
            -1,
            0,
            1600,
            280,
            0,
        });
        remaining -= quantity;
        angle += kGoldDropAngleStep;
    }
    return true;
}

void updateGroundItem(GroundItem& item) {
    if (item.bounce_state >= 2) {
        return;
    }
    item.height += item.vertical_velocity / 10;
    item.vertical_velocity -= item.vertical_gravity;
    if (item.height > 0) {
        return;
    }

    item.height = 0;
    if (item.bounce_state == 0) {
        item.bounce_state = 1;
        item.vertical_velocity = 700;
    } else {
        item.bounce_state = 2;
    }
}

}  // namespace osf
