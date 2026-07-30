#ifndef OPENSHADOWFLARE_CHARACTER_SELECT_STATE_HPP
#define OPENSHADOWFLARE_CHARACTER_SELECT_STATE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace osf {

enum class CharacterSelectMode : std::int32_t {
    new_character = 0,
    saved_game = 1,
};

enum class CharacterSelectScreen : std::int32_t {
    character_or_save = 0,
    text_or_delete = 1,
    game_mode = 10,
    network_mode = 11,
    host_address = 12,
    launching = 20,
};

struct CharacterSelectStateData {
    std::int32_t fade_value = 0;
    std::int32_t fade_target = 0;
    std::int32_t fade_steps_remaining = 0;
    std::int32_t launch_counter = 0;
    std::int32_t character_transition_counter = 0;
    std::int32_t save_hover_animation = 0;
    std::int32_t brightness_increasing = 0;
    std::int32_t saved_game_selection = 0;
    std::int32_t selection_result = -1;
    CharacterSelectMode mode = CharacterSelectMode::new_character;
    // Retail uses the numeric screen values represented by
    // CharacterSelectScreen. Keep the stored value integral while the
    // reconstruction still compares it with traced integers.
    std::int32_t screen =
        static_cast<std::int32_t>(
            CharacterSelectScreen::character_or_save);
    CharacterSelectMode rendered_mode =
        CharacterSelectMode::new_character;
    std::int32_t pointer_click_cooldown = 0;
    std::int32_t dialog_selection = 0;
    std::int32_t dialog_input_armed = 0;
    std::int32_t dialog_previous_pointer_x = 0;
    std::int32_t dialog_previous_pointer_y = 0;
    std::int32_t previous_pointer_x = 0;
    std::int32_t previous_pointer_y = 0;
    std::int32_t input_latch = 1;
    bool new_character_data_loaded = false;
    std::int32_t character_gender = 1;
    std::string character_name;
    bool name_entry_active = false;
    std::string host_address;
    bool host_entry_active = false;
    std::int32_t selected_saved_game = -1;
    std::string next_save_path;
    std::vector<std::uint8_t> temporary_buffer;
    bool active = false;
};

enum class CharacterSelectAction {
    none,
    return_to_title,
    exit_game,
    enter_gameplay,
};

enum class CharacterSelectScreenUpdate {
    none,
    screen_10,
    screen_11,
    screen_12,
};

enum class CharacterSelectModeAction {
    none,
    start_back_transition,
    start_exit_transition,
    choose_saved_game,
    open_delete_saved_game_dialog,
    confirm_saved_game_delete,
    cancel_saved_game_delete,
    begin_name_entry,
    accept_character_name,
    cancel_name_entry,
    open_mode_menu,
};

struct CharacterSelectFrameInput {
    bool input_suspended = false;
    bool pointer_primary_pressed = false;
    bool confirm_pressed = false;
    bool back_pressed = false;
    bool delete_pressed = false;
    bool up_pressed = false;
    bool down_pressed = false;
    bool left_pressed = false;
    bool right_pressed = false;
    bool backspace_pressed = false;
    std::string text_input;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
    std::int32_t saved_game_count = 0;
};

struct CharacterSelectFrameResult {
    bool processed = true;
    std::int32_t background_brightness = 0;
    std::int32_t mode_brightness = 1000;
    std::int32_t character_transition_counter = 0;
    CharacterSelectMode mode = CharacterSelectMode::new_character;
    CharacterSelectScreenUpdate screen_update =
        CharacterSelectScreenUpdate::none;
    CharacterSelectModeAction mode_action =
        CharacterSelectModeAction::none;
    CharacterSelectAction action = CharacterSelectAction::none;
    std::int32_t play_move_sound_count = 0;
    std::int32_t play_selection_sound_count = 0;
    bool pointer_double_click = false;
    bool host_connect_hovered = false;
    bool host_back_hovered = false;
    bool host_paste_hovered = false;
    bool name_confirm_hovered = false;
    std::array<bool, 6> save_slot_hovered{};
};

struct CharacterSelectStateHooks {
    std::function<void()> begin_scene;
    std::function<bool(std::int32_t, std::string_view)> load_pattern;
    std::function<void()> clear_scene;
    std::function<void(std::int32_t)> release_pattern;
    std::function<void(const std::int32_t*, std::size_t)> configure_input;
    std::function<bool(std::string_view)> file_exists;
    std::function<void(std::string_view)> prepare_new_character;
    std::function<void()> release_new_character;
    std::function<void()> load_saved_characters;
    std::function<void(std::int32_t)> delete_saved_character;
    std::function<std::string()> read_clipboard;
    std::function<void(std::int32_t)> set_cursor_state;
    std::function<bool(std::int32_t)> voice_is_playing;
    std::function<void(std::int32_t, bool)> play_voice;
    std::function<void(std::int32_t)> release_voice;
};

class CharacterSelectState {
public:
    explicit CharacterSelectState(
        CharacterSelectStateHooks hooks = {});

    void enter(std::int32_t retail_argument);
    void leave();
    CharacterSelectFrameResult update(
        const CharacterSelectFrameInput& input = {});

    const CharacterSelectStateData& data() const;
    CharacterSelectStateData& data();

private:
    CharacterSelectStateHooks hooks_;
    CharacterSelectStateData data_;
};

const std::array<std::int32_t, 58>&
retailCharacterSelectInputBindings();

}  // namespace osf

#endif
