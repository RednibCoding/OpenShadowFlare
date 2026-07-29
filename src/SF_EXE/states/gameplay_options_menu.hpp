#ifndef OPENSHADOWFLARE_GAMEPLAY_OPTIONS_MENU_HPP
#define OPENSHADOWFLARE_GAMEPLAY_OPTIONS_MENU_HPP

#include "core/game_config.hpp"

#include <cstdint>

namespace osf {

enum class GameplayOptionsPage {
    settings,
    help,
    return_to_title_confirmation,
    exit_game_confirmation,
    saving,
};

enum class GameplayOptionsAction {
    none,
    save_and_return_to_title,
    save_and_exit,
};

struct GameplayOptionsInput {
    bool toggle_pressed = false;
    bool pointer_primary_pressed = false;
    bool pointer_primary_down = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
    bool help_pressed = false;
};

struct GameplayOptionsResult {
    bool config_changed = false;
    bool play_click_sound = false;
    bool play_confirm_sound = false;
    GameplayOptionsAction action = GameplayOptionsAction::none;
};

class GameplayOptionsMenu {
public:
    GameplayOptionsResult update(
        const GameplayOptionsInput& input,
        GameConfig& config);
    void restoreConfirmation(
        GameplayOptionsAction action);
    void close();

    bool active() const;
    GameplayOptionsPage page() const;
    std::int32_t pointerX() const;
    std::int32_t pointerY() const;
    std::int32_t animationCounter() const;
    bool helpCloseVisible() const;
    std::int32_t helpCloseAnimationCounter() const;

private:
    bool active_ = false;
    GameplayOptionsPage page_ = GameplayOptionsPage::settings;
    std::int32_t pointer_x_ = 0;
    std::int32_t pointer_y_ = 0;
    std::int32_t animation_counter_ = 0;
    bool help_close_visible_ = false;
    std::int32_t help_close_animation_counter_ = 0;
};

std::int32_t gameplayOptionsVolumeFromPointerX(
    std::int32_t pointer_x);
std::int32_t gameplayOptionsVolumeSliderOffset(
    std::int32_t volume);

}  // namespace osf

#endif
