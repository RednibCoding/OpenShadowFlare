#ifndef OPENSHADOWFLARE_GAMEPLAY_DEBUG_MENU_HPP
#define OPENSHADOWFLARE_GAMEPLAY_DEBUG_MENU_HPP

#include <cstdint>

namespace osf {

struct GameplayDebugInput {
    bool toggle_pressed = false;
    bool close_pressed = false;
    bool pointer_primary_pressed = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

struct GameplayDebugResult {
    bool settings_changed = false;
    bool play_click_sound = false;
    bool play_confirm_sound = false;
};

class GameplayDebugMenu {
public:
    GameplayDebugResult update(
        const GameplayDebugInput& input);
    void close();

    bool active() const;
    bool fpsCounterEnabled() const;
    bool allSpellsEnabled() const;
    bool infiniteLifeEnabled() const;
    bool infiniteManaEnabled() const;
    std::int32_t pointerX() const;
    std::int32_t pointerY() const;

private:
    bool active_ = false;
    bool fps_counter_enabled_ = false;
    bool all_spells_enabled_ = false;
    bool infinite_life_enabled_ = false;
    bool infinite_mana_enabled_ = false;
    std::int32_t pointer_x_ = 0;
    std::int32_t pointer_y_ = 0;
};

}  // namespace osf

#endif
