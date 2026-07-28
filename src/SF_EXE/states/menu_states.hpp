#ifndef OPENSHADOWFLARE_MENU_STATES_HPP
#define OPENSHADOWFLARE_MENU_STATES_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace osf {

class RetailRandom;

struct GameVersion {
    std::int32_t major = 2;
    std::int32_t minor = 4;
    std::int32_t patch = 0;
};

struct TitleStateData {
    std::int32_t fade_steps_remaining = 0;       // retail +0x00
    std::int32_t transition_timer = 0;            // retail +0x04
    bool saved_game_exists = false;              // retail +0x08
    bool next_save_path_available = false;       // retail +0x0c
    std::string next_save_path;
    std::int32_t animation_frame = 0;             // retail +0x10
    std::array<std::int32_t, 10> smoke_delays{};
    std::int32_t sound_started = 0;               // retail +0x3c
    std::int32_t music_started = 0;               // retail +0x40
    std::int32_t music_delay_frames = 0;          // retail +0x44
    std::int32_t network_error_kind = 0;          // retail +0x48
    std::int32_t network_error_frames = 0;        // retail +0x4c
    std::int32_t menu_selection = 0;              // retail +0x50
    std::int32_t selection_armed = 1;              // retail +0x54
    std::int32_t previous_pointer_x = 0;          // retail +0x58
    std::int32_t previous_pointer_y = 0;          // retail +0x5c
    std::int32_t transition_started = 0;          // retail +0x60
    GameVersion version;
    bool active = false;
};

struct MenuFrameInput {
    // The platform layer maps any text-entry/IME ownership to this neutral
    // pause flag. Retail returns before advancing any menu state while it is
    // set.
    bool input_suspended = false;
    bool pointer_primary_pressed = false;
    bool confirm_pressed = false;
    bool up_pressed = false;
    bool down_pressed = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
    std::array<std::int32_t, 10> smoke_frame_counts{};
};

enum class TitleAction {
    none,
    open_character_select,
    exit_game,
};

struct TitleFrameResult {
    bool processed = true;
    TitleAction action = TitleAction::none;
    std::int32_t character_select_argument = 0;
    std::int32_t scene_brightness = 1000;
    std::array<std::int32_t, 3> menu_brightness{{500, 500, 500}};
    std::array<bool, 3> menu_visible{{true, true, true}};
    std::array<std::int32_t, 10> smoke_frames{{
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    }};
    bool network_error_visible = false;
    std::int32_t network_error_kind = 0;
    bool play_title_sound = false;
    std::int32_t play_move_sound_count = 0;
    bool play_confirm_sound = false;
    bool start_menu_music = false;
};

struct TitleStateHooks {
    std::function<void()> begin_scene;
    std::function<bool(std::int32_t, std::string_view)> load_pattern;
    std::function<bool(
        std::size_t, std::int32_t, std::string_view)> load_animation;
    std::function<void()> clear_scene;
    std::function<void(std::int32_t)> release_pattern;
    std::function<void(std::size_t)> release_animation;
    std::function<void(const std::int32_t*, std::size_t)> configure_input;
    std::function<bool(std::string_view)> files_exist;
    std::function<bool(std::string_view)> file_exists;
    std::function<void(std::int32_t)> set_cursor_state;
    std::function<void(std::string_view, std::int32_t)> load_voice;
};

class TitleState {
public:
    explicit TitleState(
        RetailRandom& random,
        TitleStateHooks hooks = {});

    // Returns false only when one of the ten required smoke resources fails,
    // matching the early return in retail function 0x00420c40.
    bool enter();
    void leave();
    TitleFrameResult update(const MenuFrameInput& input = {});

    const TitleStateData& data() const;
    TitleStateData& data();

private:
    RetailRandom& random_;
    TitleStateHooks hooks_;
    TitleStateData data_;
};

enum class CharacterSelectMode : std::int32_t {
    new_character = 0,
    saved_game = 1,
};

struct CharacterSelectStateData {
    std::int32_t fade_value = 0;                  // retail +0x00
    std::int32_t fade_target = 0;                 // retail +0x04
    std::int32_t fade_steps_remaining = 0;        // retail +0x08
    std::int32_t launch_counter = 0;              // retail +0x0c
    std::int32_t raw_14 = 0;
    std::int32_t brightness_increasing = 0;       // retail +0x18
    std::int32_t raw_1c = 0;
    std::int32_t selected_save = -1;              // retail +0x20
    CharacterSelectMode mode = CharacterSelectMode::new_character;
    std::int32_t screen = 0;                      // retail +0x28
    std::int32_t raw_30 = 0;
    std::int32_t raw_34 = 0;
    CharacterSelectMode rendered_mode =
        CharacterSelectMode::new_character;       // retail +0x40
    std::int32_t input_latch = 1;                 // retail +0x60
    bool new_character_data_loaded = false;
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

struct CharacterSelectFrameInput {
    bool input_suspended = false;
};

struct CharacterSelectFrameResult {
    bool processed = true;
    std::int32_t background_brightness = 0;
    CharacterSelectMode mode = CharacterSelectMode::new_character;
    CharacterSelectScreenUpdate screen_update =
        CharacterSelectScreenUpdate::none;
    CharacterSelectAction action = CharacterSelectAction::none;
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

struct RetailSavePath {
    std::string path;
    bool available = false;
};

// Reconstructs retail function 0x004021b0. It checks the six slots from
// Save\0000.Ssv through Save\0005.Ssv. When all are occupied, path contains
// Save\0005.Ssv but available is false, matching the ignored output buffer.
RetailSavePath findNextRetailSavePath(
    const std::function<bool(std::string_view)>& file_exists);

const std::array<std::int32_t, 57>& retailTitleInputBindings();
const std::array<std::int32_t, 58>& retailCharacterSelectInputBindings();

}  // namespace osf

#endif
