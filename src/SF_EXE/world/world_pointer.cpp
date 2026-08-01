#include "world_pointer.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>

namespace osf {
namespace {

std::int32_t priorityIndex(std::int32_t retail_type) {
    // FUN_004165d0 maps the five scenario object types onto the five
    // user-configurable click-priority slots in this order.
    constexpr std::array<std::int32_t, 5> indices{{
        1, 3, 0, 2, 4,
    }};
    return retail_type >= 0 &&
                   static_cast<std::size_t>(retail_type) <
                       indices.size()
               ? indices[static_cast<std::size_t>(
                     retail_type)]
               : -1;
}

}  // namespace

std::int32_t worldPointerHalfSize(
    const WorldPointerConfiguration& configuration) {
    constexpr std::array<std::int32_t, 5> half_sizes{{
        0, 12, 16, 24, 48,
    }};
    if (!configuration.range_enabled ||
        configuration.range < 0 ||
        static_cast<std::size_t>(configuration.range) >=
            half_sizes.size()) {
        return 0;
    }
    return half_sizes[static_cast<std::size_t>(
        configuration.range)];
}

void WorldPointer::configure(
    const WorldPointerConfiguration& configuration) {
    configuration_ = configuration;
    configuration_.range =
        std::clamp<std::int32_t>(configuration_.range, 0, 4);
}

void WorldPointer::reset() {
    target_ = {};
    screen_x_ = 0;
    screen_y_ = 0;
    active_ = false;
}

void WorldPointer::clearSelection() {
    target_ = {};
}

void WorldPointer::update(
    std::int32_t screen_x,
    std::int32_t screen_y,
    std::vector<WorldPointerCandidate> candidates) {
    screen_x_ = screen_x;
    screen_y_ = screen_y;
    active_ = true;
    target_ = {};
    if (candidates.empty()) {
        return;
    }

    std::vector<DisplayOrderEntry> order;
    order.reserve(candidates.size());
    for (std::size_t index = 0;
         index < candidates.size();
         ++index) {
        candidates[index].display.source_index = index;
        order.push_back(candidates[index].display);
    }
    sortDisplayObjects(order);

    std::int32_t best_priority = -1;
    bool best_exact_hit = false;
    std::int64_t best_distance =
        std::numeric_limits<std::int64_t>::max();
    bool has_target = false;
    for (const DisplayOrderEntry& entry : order) {
        const WorldPointerCandidate& candidate =
            candidates[entry.source_index];
        const std::int32_t index =
            priorityIndex(candidate.retail_type);
        if (index < 0 ||
            static_cast<std::size_t>(index) >=
                configuration_.click_priority.size()) {
            continue;
        }
        const std::int32_t priority =
            configuration_.click_priority[
                static_cast<std::size_t>(index)];
        const bool better =
            !has_target ||
            priority > best_priority ||
            (priority == best_priority &&
             (candidate.exact_hit != best_exact_hit
                  ? candidate.exact_hit
                  : candidate.pointer_distance_squared <=
                        best_distance));
        if (!better) {
            continue;
        }
        // Within a retail priority group, a sprite directly below the
        // cursor wins. Otherwise the nearest actor or item in the
        // configured range wins. A later draw settles exact ties.
        has_target = true;
        best_priority = priority;
        best_exact_hit = candidate.exact_hit;
        best_distance =
            candidate.pointer_distance_squared;
        target_ = candidate.target;
    }
}

std::int32_t WorldPointer::screenX() const {
    return screen_x_;
}

std::int32_t WorldPointer::screenY() const {
    return screen_y_;
}

bool WorldPointer::active() const {
    return active_;
}

const WorldPointerTarget& WorldPointer::target() const {
    return target_;
}

const WorldPointerConfiguration&
WorldPointer::configuration() const {
    return configuration_;
}

}  // namespace osf
