#include "gameplay_map.hpp"

namespace osf {

void GameplayMap::open() {
    active_ = true;
    scroll_x_ = 0;
    scroll_y_ = 0;
    frame_counter_ = 0;
    marker_counter_ = 0;
}

void GameplayMap::close() {
    active_ = false;
    scroll_x_ = 0;
    scroll_y_ = 0;
    frame_counter_ = 0;
    marker_counter_ = 0;
}

void GameplayMap::update(const GameplayMapInput& input) {
    if (input.toggle_pressed) {
        if (active_) {
            close();
        } else {
            open();
        }
        return;
    }
    if (!active_) {
        return;
    }
    if (input.close_pressed) {
        close();
        return;
    }

    if (input.scroll_left) {
        scroll_x_ -= 16;
    }
    if (input.scroll_up) {
        scroll_y_ -= 10;
    }
    if (input.scroll_right) {
        scroll_x_ += 16;
    }
    if (input.scroll_down) {
        scroll_y_ += 10;
    }
    if (input.recenter) {
        scroll_x_ = 0;
        scroll_y_ = 0;
    }

    frame_counter_ = (frame_counter_ + 1) % 60;
    marker_counter_ = (marker_counter_ + 1) % 20;
}

bool GameplayMap::active() const {
    return active_;
}

std::int32_t GameplayMap::scrollX() const {
    return scroll_x_;
}

std::int32_t GameplayMap::scrollY() const {
    return scroll_y_;
}

std::int32_t GameplayMap::frameCounter() const {
    return frame_counter_;
}

bool GameplayMap::markerVisible() const {
    return marker_counter_ < 15;
}

}  // namespace osf
