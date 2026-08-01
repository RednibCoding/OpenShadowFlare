#ifndef OPENSHADOWFLARE_GAMEPLAY_STATUS_HPP
#define OPENSHADOWFLARE_GAMEPLAY_STATUS_HPP

#include <cstdint>

namespace osf {

struct GameplayStatusInput {
    bool toggle_pressed = false;
    bool close_pressed = false;
    bool pointer_primary_pressed = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

struct GameplayStatusResult {
    bool pointer_consumed = false;
    bool switch_to_magic = false;
    bool play_move_sound = false;
};

class GameplayStatus {
public:
    void open();
    void close();
    GameplayStatusResult update(
        const GameplayStatusInput& input);

    bool active() const;

private:
    bool active_ = false;
};

}  // namespace osf

#endif
