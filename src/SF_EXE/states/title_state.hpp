#ifndef OPENSHADOWFLARE_TITLE_STATE_HPP
#define OPENSHADOWFLARE_TITLE_STATE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace osf {

class RetailRandom;

struct GameVersion {
    std::int32_t major = 2;
    std::int32_t minor = 4;
    std::int32_t patch = 0;
};

struct TitleStateData {
    std::int32_t fade_steps_remaining = 0;
    std::int32_t transition_timer = 0;
    bool saved_game_exists = false;
    bool next_save_path_available = false;
    std::string next_save_path;
    std::int32_t animation_frame = 0;
    std::array<std::int32_t, 10> smoke_delays{};
    std::int32_t sound_started = 0;
    std::int32_t music_started = 0;
    std::int32_t music_delay_frames = 0;
    std::int32_t network_error_kind = 0;
    std::int32_t network_error_frames = 0;
    std::int32_t menu_selection = 0;
    std::int32_t selection_armed = 1;
    std::int32_t previous_pointer_x = 0;
    std::int32_t previous_pointer_y = 0;
    std::int32_t transition_started = 0;
    GameVersion version;
    bool active = false;
};

struct MenuFrameInput {
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

const std::array<std::int32_t, 57>& retailTitleInputBindings();

}  // namespace osf

#endif
