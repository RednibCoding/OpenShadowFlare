#ifndef OPENSHADOWFLARE_RUNTIME_INPUT_ADAPTER_HPP
#define OPENSHADOWFLARE_RUNTIME_INPUT_ADAPTER_HPP

#include "states/character_select_state.hpp"
#include "states/game_state.hpp"
#include "states/title_state.hpp"

#include "lwl.h"

#include <cstdint>

namespace osf::runtime {

class InputAdapter {
public:
    InputAdapter(
        std::int32_t virtual_width,
        std::int32_t virtual_height);

    bool handleEvent(
        LwlWindow* window,
        const LwlEvent& event,
        GameState current_state);
    void clearTransientInput();

    MenuFrameInput& menu();
    const MenuFrameInput& menu() const;
    CharacterSelectFrameInput& characterSelect();
    const CharacterSelectFrameInput& characterSelect() const;
    bool pointerPrimaryDown() const;
    bool pointerSecondaryPressed() const;
    bool runTogglePressed() const;
    bool gameplayOptionsPressed() const;
    bool gameplayHelpPressed() const;
    bool gameplayMissionListPressed() const;
    bool gameplayMapPressed() const;
    bool gameplayInventoryPressed() const;
    bool upHeld() const;
    bool downHeld() const;
    bool leftHeld() const;
    bool rightHeld() const;

private:
    void setPointerPosition(
        LwlWindow* window,
        std::int32_t x,
        std::int32_t y);

    std::int32_t virtual_width_ = 640;
    std::int32_t virtual_height_ = 480;
    MenuFrameInput menu_;
    CharacterSelectFrameInput character_select_;
    bool up_held_ = false;
    bool down_held_ = false;
    bool left_held_ = false;
    bool right_held_ = false;
    bool confirm_held_ = false;
    bool back_held_ = false;
    bool delete_held_ = false;
    bool backspace_held_ = false;
    bool pointer_primary_down_ = false;
    bool pointer_secondary_pressed_ = false;
    bool run_held_ = false;
    bool help_held_ = false;
    bool mission_list_held_ = false;
    bool map_held_ = false;
    bool inventory_held_ = false;
    bool run_toggle_pressed_ = false;
    bool gameplay_options_pressed_ = false;
    bool gameplay_help_pressed_ = false;
    bool gameplay_mission_list_pressed_ = false;
    bool gameplay_map_pressed_ = false;
    bool gameplay_inventory_pressed_ = false;
};

}  // namespace osf::runtime

#endif
