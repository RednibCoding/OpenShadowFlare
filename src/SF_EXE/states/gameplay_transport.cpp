#include "gameplay_transport.hpp"

#include <algorithm>
#include <cstddef>

namespace osf {
namespace {

bool inside(
    std::int32_t x,
    std::int32_t y,
    std::int32_t left,
    std::int32_t top,
    std::int32_t right,
    std::int32_t bottom) {
    return x >= left && x < right &&
           y >= top && y < bottom;
}

}  // namespace

void GameplayTransport::open() {
    active_ = true;
    page_ = 0;
    hovered_destination_ = -1;
}

void GameplayTransport::close() {
    active_ = false;
    page_ = 0;
    hovered_destination_ = -1;
}

GameplayTransportResult GameplayTransport::update(
    const GameplayTransportInput& input,
    const std::vector<std::int32_t>& enabled_destinations) {
    GameplayTransportResult result;
    if (!active_) {
        return result;
    }
    result.pointer_consumed =
        input.pointer_primary_pressed &&
        input.pointer_x >= 0 &&
        input.pointer_x < 320 &&
        input.pointer_y >= 0 &&
        input.pointer_y < 412;
    updateHover(
        input.pointer_x,
        input.pointer_y,
        enabled_destinations);
    if (input.close_pressed) {
        close();
        return result;
    }
    if (!input.pointer_primary_pressed) {
        return result;
    }

    const std::int32_t pages =
        pageCount(enabled_destinations.size());
    if (page_ > 0 &&
        inside(
            input.pointer_x,
            input.pointer_y,
            28,
            368,
            93,
            387)) {
        --page_;
        hovered_destination_ = -1;
        result.play_move_sound = true;
        return result;
    }
    if (page_ + 1 < pages &&
        inside(
            input.pointer_x,
            input.pointer_y,
            224,
            368,
            290,
            387)) {
        ++page_;
        hovered_destination_ = -1;
        result.play_move_sound = true;
        return result;
    }

    result.selected_destination =
        destinationAt(
            input.pointer_x,
            input.pointer_y,
            enabled_destinations);
    if (result.selected_destination >= 0) {
        result.play_move_sound = true;
        close();
    }
    return result;
}

bool GameplayTransport::active() const {
    return active_;
}

std::int32_t GameplayTransport::page() const {
    return page_;
}

std::int32_t GameplayTransport::pageCount(
    std::size_t enabled_destination_count) const {
    return std::max<std::int32_t>(
        1,
        (static_cast<std::int32_t>(
             enabled_destination_count) +
         entries_per_page - 1) /
            entries_per_page);
}

std::int32_t
GameplayTransport::hoveredDestination() const {
    return hovered_destination_;
}

std::vector<std::int32_t>
GameplayTransport::visibleDestinations(
    const std::vector<std::int32_t>& enabled_destinations) const {
    const std::size_t begin = std::min(
        enabled_destinations.size(),
        static_cast<std::size_t>(page_) *
            static_cast<std::size_t>(entries_per_page));
    const std::size_t end = std::min(
        enabled_destinations.size(),
        begin + static_cast<std::size_t>(entries_per_page));
    return {
        enabled_destinations.begin() +
            static_cast<std::ptrdiff_t>(begin),
        enabled_destinations.begin() +
            static_cast<std::ptrdiff_t>(end),
    };
}

std::int32_t GameplayTransport::destinationAt(
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    const std::vector<std::int32_t>& enabled_destinations) const {
    if (pointer_x < 32 || pointer_x >= 289) {
        return -1;
    }
    const std::vector<std::int32_t> visible =
        visibleDestinations(enabled_destinations);
    for (std::size_t index = 0;
         index < visible.size();
         ++index) {
        const std::int32_t top =
            63 + static_cast<std::int32_t>(index) * 30;
        if (pointer_y >= top && pointer_y < top + 23) {
            return visible[index];
        }
    }
    return -1;
}

void GameplayTransport::updateHover(
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    const std::vector<std::int32_t>& enabled_destinations) {
    hovered_destination_ =
        destinationAt(
            pointer_x,
            pointer_y,
            enabled_destinations);
}

}  // namespace osf
