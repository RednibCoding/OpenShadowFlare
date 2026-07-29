#include "world_pointer.hpp"

#include <algorithm>
#include <cstddef>
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

void WorldPointer::configure(
    const WorldPointerConfiguration& configuration) {
    configuration_ = configuration;
    configuration_.range =
        std::clamp(configuration_.range, 0, 4);
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
        // Retail inserts lower priority groups first, then scans the
        // resulting display list backwards. A later draw wins ties.
        if (priority >= best_priority) {
            best_priority = priority;
            target_ = candidate.target;
        }
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
