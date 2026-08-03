#include "player_resource_rate.hpp"

#include "core/retail_integer.hpp"

#include <algorithm>
#include <cstdint>

namespace osf {
namespace {

std::int32_t negativeMagnitudeRemainder(
    std::int32_t value) {
    const std::uint32_t magnitude =
        0u - static_cast<std::uint32_t>(value);
    return static_cast<std::int32_t>(magnitude % 100u);
}

}  // namespace

PlayerResourceRateUpdate PlayerResourceRateController::update(
    std::int32_t current_value,
    std::int32_t maximum_value,
    std::int32_t percentage_rate,
    std::int32_t minimum_value,
    bool resource_active) {
    PlayerResourceRateUpdate result;
    result.value = current_value;
    const bool update_due = update_counter_ % 3 == 0;
    update_counter_ = retailAdd(update_counter_, 1);
    if (!update_due || !resource_active) {
        return result;
    }
    if (percentage_rate == 0) {
        remainder_ = 0;
        return result;
    }

    std::int32_t scaled =
        retailMultiply(maximum_value, percentage_rate) /
        100;
    if (scaled < 0) {
        remainder_ = retailSubtract(
            remainder_,
            negativeMagnitudeRemainder(scaled));
    } else {
        remainder_ = retailAdd(
            remainder_, scaled % 100);
    }
    if (remainder_ <= -100) {
        scaled = retailSubtract(scaled, 100);
        remainder_ = retailAdd(remainder_, 100);
    } else if (remainder_ >= 100) {
        scaled = retailAdd(scaled, 100);
        remainder_ = retailSubtract(remainder_, 100);
    }
    result.value = std::clamp<std::int32_t>(
        retailAdd(current_value, scaled / 100),
        std::min(minimum_value, maximum_value),
        std::max<std::int32_t>(minimum_value, maximum_value));
    result.changed = result.value != current_value;
    return result;
}

void PlayerResourceRateController::clear() {
    *this = {};
}

std::int32_t PlayerResourceRateController::remainder() const {
    return remainder_;
}

std::int32_t PlayerResourceRateController::updateCounter() const {
    return update_counter_;
}

}  // namespace osf
