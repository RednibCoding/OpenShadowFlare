#ifndef OPENSHADOWFLARE_GAMEPLAY_MAP_HPP
#define OPENSHADOWFLARE_GAMEPLAY_MAP_HPP

#include <cstdint>

namespace osf {

struct GameplayMapInput {
    bool toggle_pressed = false;
    bool close_pressed = false;
    bool scroll_left = false;
    bool scroll_up = false;
    bool scroll_right = false;
    bool scroll_down = false;
    bool recenter = false;
};

class GameplayMap {
public:
    void open();
    void close();
    void update(const GameplayMapInput& input);

    bool active() const;
    std::int32_t scrollX() const;
    std::int32_t scrollY() const;
    std::int32_t frameCounter() const;
    bool markerVisible() const;

private:
    bool active_ = false;
    std::int32_t scroll_x_ = 0;
    std::int32_t scroll_y_ = 0;
    std::int32_t frame_counter_ = 0;
    std::int32_t marker_counter_ = 0;
};

}  // namespace osf

#endif
