#include "menu_states.hpp"

#include "core/retail_random.hpp"

#include <iomanip>
#include <sstream>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kMenuMusicSlot = 500;
constexpr std::int32_t kFullBrightness = 1000;

struct MenuRectangle {
    std::int32_t left;
    std::int32_t right;
    std::int32_t top;
    std::int32_t bottom;
};

constexpr std::array<MenuRectangle, 3> kTitleMenuRectangles{{
    {0xd6, 0x1a3, 0x16d, 0x184},
    {0xd7, 0x1a3, 0x186, 0x19c},
    {0x10e, 0x172, 0x19b, 0x1b9},
}};

constexpr std::array<MenuRectangle, 6> kSavedGameRectangles{{
    {32, 319, 188, 264},
    {336, 623, 188, 264},
    {32, 319, 276, 352},
    {336, 623, 276, 352},
    {32, 319, 364, 440},
    {336, 623, 364, 440},
}};

constexpr MenuRectangle kSavedBackRectangle{
    0x188, 0x1d2, 0x1c2, 0x1cf};
constexpr MenuRectangle kSavedExitRectangle{
    0x23f, 0x279, 7, 0x14};
constexpr MenuRectangle kSavedContinueRectangle{
    0x236, 0x25c, 0x1c2, 0x1cf};
constexpr MenuRectangle kSavedDeleteRectangle{
    0x3d, 0xa9, 0x1c2, 0x1cf};

constexpr std::array<std::int32_t, 57> kTitleInputBindings{{
    1, 2, 16, 17, 38, 40, 37, 39, 9, 27, 13,
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
    76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
    87, 88, 89, 90, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 96, 97, 98, 99, 100, 101, 102,
    103, 104, 105,
}};

constexpr std::array<std::int32_t, 58> kCharacterSelectInputBindings{{
    1, 2, 16, 17, 38, 40, 37, 39, 9, 27, 13, 46,
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
    76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
    87, 88, 89, 90, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 96, 97, 98, 99, 100, 101, 102,
    103, 104, 105,
}};

bool loadPattern(
    const std::function<bool(std::int32_t, std::string_view)>& callback,
    std::int32_t id,
    std::string_view path) {
    return !callback || callback(id, path);
}

bool loadAnimation(
    const std::function<bool(
        std::size_t, std::int32_t, std::string_view)>& callback,
    std::size_t index,
    std::int32_t pattern_id,
    std::string_view path) {
    return !callback || callback(index, pattern_id, path);
}

std::string numberedTitlePath(std::string_view extension, int index) {
    std::ostringstream path;
    path << "System\\Title\\Pattern\\Smoke"
         << std::setfill('0') << std::setw(2) << index
         << extension;
    return path.str();
}

bool isInside(
    const MenuRectangle& rectangle,
    std::int32_t x,
    std::int32_t y) {
    // Retail uses strict comparisons at every edge.
    return x > rectangle.left && x < rectangle.right &&
           y > rectangle.top && y < rectangle.bottom;
}

std::int32_t titleTransitionBrightness(std::int32_t timer) {
    const std::int32_t phase = timer % 1000;
    if (phase <= 5) {
        return kFullBrightness;
    }
    return kFullBrightness + ((5 - phase) * 1000) / 15;
}

std::int32_t modeTransitionBrightness(std::int32_t timer) {
    const std::int32_t brightness =
        kFullBrightness - (timer % 1000) * 100;
    return brightness < 0 ? 0 : brightness;
}

void beginSavedGameChoice(
    CharacterSelectStateData& data,
    CharacterSelectFrameResult& result) {
    data.selected_saved_game = data.saved_game_selection;
    data.screen = 10;
    data.input_latch = 1;
    data.brightness_increasing = 0;
    data.dialog_selection = 0;
    data.dialog_input_armed = 1;
    result.mode_action = CharacterSelectModeAction::choose_saved_game;
    ++result.play_selection_sound_count;
}

void beginSavedGameDelete(
    CharacterSelectStateData& data,
    CharacterSelectFrameResult& result) {
    data.screen = 1;
    data.input_latch = 1;
    data.brightness_increasing = 0;
    data.dialog_selection = 1;
    data.dialog_input_armed = 1;
    result.mode_action = CharacterSelectModeAction::delete_saved_game;
    ++result.play_selection_sound_count;
}

void updateSavedGameMode(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result) {
    if (data.fade_target > data.fade_value) {
        data.fade_target = data.fade_value;
    }

    if (data.fade_steps_remaining == 0) {
        result.mode_brightness =
            modeTransitionBrightness(data.launch_counter);
        const std::int32_t phase = data.launch_counter % 1000;
        if (phase > 5) {
            data.fade_value =
                kFullBrightness + ((5 - phase) * 1000) / 15;
            if (data.fade_value < 0) {
                data.fade_value = 0;
            }
        }
        if (data.fade_target > data.fade_value) {
            data.fade_target = data.fade_value;
        }
        if (result.mode_brightness > data.fade_target) {
            result.mode_brightness = data.fade_target;
        }
    } else {
        data.pointer_click_cooldown = 0;
        result.mode_brightness = data.fade_target;
    }

    const std::int32_t save_count =
        input.saved_game_count < 0
            ? 0
            : (input.saved_game_count > 6
                   ? 6
                   : input.saved_game_count);
    if (data.screen == 0) {
        const std::int32_t previous_selection =
            data.saved_game_selection;
        if (data.input_latch == 0 && save_count != 0) {
            if (input.up_pressed) {
                data.saved_game_selection -= 2;
                if (data.saved_game_selection < 0) {
                    data.saved_game_selection = 0;
                }
            }
            if (input.down_pressed) {
                data.saved_game_selection += 2;
                if (data.saved_game_selection > save_count - 1) {
                    data.saved_game_selection = save_count - 1;
                }
            }
            if (input.left_pressed) {
                --data.saved_game_selection;
                if (data.saved_game_selection < 0) {
                    data.saved_game_selection = 0;
                }
            }
            if (input.right_pressed) {
                ++data.saved_game_selection;
                if (data.saved_game_selection > save_count - 1) {
                    data.saved_game_selection = save_count - 1;
                }
            }
        }
        if (data.saved_game_selection != previous_selection) {
            ++result.play_selection_sound_count;
        }

        if (data.pointer_click_cooldown != 0) {
            --data.pointer_click_cooldown;
        }
        if (input.pointer_primary_pressed && data.input_latch == 0) {
            if (data.pointer_click_cooldown != 0 &&
                data.previous_pointer_x == input.pointer_x &&
                data.previous_pointer_y == input.pointer_y) {
                result.pointer_double_click = true;
                data.pointer_click_cooldown = 0;
                data.previous_pointer_x = -1;
            } else {
                data.pointer_click_cooldown = 10;
                data.previous_pointer_x = input.pointer_x;
                data.previous_pointer_y = input.pointer_y;
            }
        }
    }

    bool save_hovered = false;
    for (std::int32_t index = 0; index < 6; ++index) {
        if (data.brightness_increasing == 0 ||
            data.launch_counter != 0 ||
            data.save_hover_animation < 28 ||
            !isInside(
                kSavedGameRectangles[static_cast<std::size_t>(index)],
                input.pointer_x,
                input.pointer_y)) {
            continue;
        }

        ++data.save_hover_animation;
        save_hovered = true;
        if (data.screen == 0 &&
            input.pointer_primary_pressed &&
            index < save_count &&
            data.input_latch == 0) {
            data.saved_game_selection = index;
            ++result.play_selection_sound_count;
            if (result.pointer_double_click) {
                beginSavedGameChoice(data, result);
            }
        }
    }

    if (data.fade_steps_remaining == 0 &&
        data.save_hover_animation < 28) {
        ++data.save_hover_animation;
    }
    if (data.save_hover_animation >= 28 && !save_hovered) {
        data.save_hover_animation = 64;
    }

    if (data.brightness_increasing != 0 &&
        data.launch_counter == 0) {
        const bool back_highlighted =
            isInside(
                kSavedBackRectangle,
                input.pointer_x,
                input.pointer_y) ||
            (input.back_pressed && data.input_latch == 0);
        if (back_highlighted &&
            data.screen == 0 &&
            (input.pointer_primary_pressed || input.back_pressed) &&
            data.input_latch == 0) {
            data.launch_counter = 1000;
            result.mode_action =
                CharacterSelectModeAction::start_back_transition;
            ++result.play_selection_sound_count;
        }

        const bool exit_highlighted =
            isInside(
                kSavedExitRectangle,
                input.pointer_x,
                input.pointer_y);
        if (data.launch_counter == 0 &&
            exit_highlighted &&
            data.screen == 0 &&
            input.pointer_primary_pressed &&
            data.input_latch == 0) {
            data.launch_counter = 2000;
            result.mode_action =
                CharacterSelectModeAction::start_exit_transition;
            ++result.play_selection_sound_count;
        }
    }

    const bool selected_save_exists =
        data.saved_game_selection >= 0 &&
        data.saved_game_selection < save_count;
    if (!selected_save_exists) {
        return;
    }

    if (data.brightness_increasing != 0 &&
        data.launch_counter == 0) {
        const bool continue_highlighted =
            isInside(
                kSavedContinueRectangle,
                input.pointer_x,
                input.pointer_y) ||
            (input.confirm_pressed && data.input_latch == 0);
        if (continue_highlighted &&
            data.screen == 0 &&
            (input.pointer_primary_pressed || input.confirm_pressed)) {
            beginSavedGameChoice(data, result);
        }

        const bool delete_highlighted =
            isInside(
                kSavedDeleteRectangle,
                input.pointer_x,
                input.pointer_y) ||
            (input.delete_pressed && data.input_latch == 0);
        if (delete_highlighted &&
            data.screen == 0 &&
            (input.pointer_primary_pressed || input.delete_pressed)) {
            beginSavedGameDelete(data, result);
        }
    }
}

}  // namespace

TitleState::TitleState(RetailRandom& random, TitleStateHooks hooks)
    : random_(random), hooks_(std::move(hooks)) {}

bool TitleState::enter() {
    if (hooks_.begin_scene) {
        hooks_.begin_scene();
    }

    // Retail does not check the title background result, but each smoke
    // pattern and animation is required.
    loadPattern(
        hooks_.load_pattern, 4,
        "System\\Title\\Pattern\\Title.njp");
    for (int index = 0; index < 10; ++index) {
        const std::int32_t pattern_id = 5 + index * 2;
        const std::string pattern =
            numberedTitlePath(".njp", index);
        if (!loadPattern(hooks_.load_pattern, pattern_id, pattern)) {
            data_.active = false;
            return false;
        }

        const std::string animation =
            numberedTitlePath(".Caf", index);
        if (!loadAnimation(
                hooks_.load_animation,
                static_cast<std::size_t>(index),
                pattern_id,
                animation)) {
            data_.active = false;
            return false;
        }
    }

    if (hooks_.configure_input) {
        hooks_.configure_input(
            kTitleInputBindings.data(),
            kTitleInputBindings.size());
    }

    data_.fade_steps_remaining = 0x14;
    data_.transition_timer = 0;
    data_.saved_game_exists =
        hooks_.files_exist && hooks_.files_exist("Save\\*.Ssv");

    if (hooks_.set_cursor_state) {
        hooks_.set_cursor_state(-1);
    }

    const RetailSavePath next_save =
        findNextRetailSavePath(hooks_.file_exists);
    data_.next_save_path = next_save.path;
    data_.next_save_path_available = next_save.available;
    data_.animation_frame = 0;
    data_.network_error_kind = 0;
    for (std::int32_t& delay : data_.smoke_delays) {
        delay = random_.next() % 0x5a;
    }

    if (hooks_.load_voice) {
        hooks_.load_voice(
            "System\\Title\\Music\\BGM00.Voc", kMenuMusicSlot);
    }

    data_.sound_started = 0;
    data_.music_started = 0;
    data_.music_delay_frames = 0;
    data_.menu_selection = 0;
    data_.selection_armed = 1;
    data_.transition_started = 0;
    data_.active = true;
    return true;
}

void TitleState::leave() {
    if (hooks_.clear_scene) {
        hooks_.clear_scene();
    }
    if (hooks_.release_pattern) {
        hooks_.release_pattern(4);
    }
    for (int index = 0; index < 10; ++index) {
        if (hooks_.release_pattern) {
            hooks_.release_pattern(5 + index * 2);
        }
        if (hooks_.release_animation) {
            hooks_.release_animation(static_cast<std::size_t>(index));
        }
    }
    data_.active = false;
}

TitleFrameResult TitleState::update(const MenuFrameInput& input) {
    TitleFrameResult result;
    result.menu_visible[0] = data_.next_save_path_available;
    result.menu_visible[1] = data_.saved_game_exists;

    if (input.input_suspended) {
        result.processed = false;
        return result;
    }

    if (data_.network_error_kind != 0) {
        result.network_error_visible = true;
        result.network_error_kind = data_.network_error_kind;
        --data_.network_error_frames;
        if (data_.network_error_frames == 0) {
            data_.network_error_kind = 0;
        }
        return result;
    }

    if (data_.transition_timer == 1020) {
        result.action = TitleAction::open_character_select;
        result.character_select_argument = 0;
        return result;
    }
    if (data_.transition_timer == 2020) {
        result.action = TitleAction::open_character_select;
        result.character_select_argument = 1;
        return result;
    }
    if (data_.transition_timer == 3020) {
        // Retail posts its close request and still completes this frame.
        result.action = TitleAction::exit_game;
    }
    if (data_.transition_timer > 0) {
        ++data_.transition_timer;
    }

    if (data_.fade_steps_remaining > 0) {
        result.scene_brightness =
            (20 - data_.fade_steps_remaining) * 50;
        --data_.fade_steps_remaining;
    } else {
        result.scene_brightness =
            titleTransitionBrightness(data_.transition_timer);

        if (data_.sound_started == 0) {
            result.play_title_sound = true;
            data_.sound_started = 1;
        }
        if (data_.sound_started == 1) {
            ++data_.music_delay_frames;
            if (data_.music_delay_frames == 60 &&
                data_.music_started == 0) {
                result.start_menu_music = true;
                data_.music_started = 1;
            }
        }
    }

    if (data_.transition_started == 0) {
        if (input.up_pressed && data_.menu_selection != 0) {
            --data_.menu_selection;
            ++result.play_move_sound_count;
            data_.selection_armed = 1;
            if (!data_.saved_game_exists &&
                data_.menu_selection == 1) {
                data_.menu_selection = 0;
            }
        }

        if (input.down_pressed && data_.menu_selection != 2) {
            ++data_.menu_selection;
            ++result.play_move_sound_count;
            data_.selection_armed = 1;
            if (!data_.saved_game_exists &&
                data_.menu_selection == 1) {
                data_.menu_selection = 2;
            }
        }

        if (!data_.next_save_path_available &&
            data_.menu_selection == 0) {
            data_.menu_selection = 1;
        }
        if (!data_.saved_game_exists &&
            data_.menu_selection == 1) {
            data_.menu_selection = 2;
        }
    }

    const bool pointer_moved =
        input.pointer_x != data_.previous_pointer_x ||
        input.pointer_y != data_.previous_pointer_y;

    for (std::int32_t item = 0; item < 3; ++item) {
        if (!result.menu_visible[static_cast<std::size_t>(item)]) {
            continue;
        }

        const MenuRectangle& rectangle =
            kTitleMenuRectangles[static_cast<std::size_t>(item)];
        const bool pointer_inside =
            isInside(rectangle, input.pointer_x, input.pointer_y);
        if (pointer_moved &&
            data_.transition_timer == 0 &&
            pointer_inside) {
            data_.selection_armed = 0;
        }

        const std::int32_t transition_start = (item + 1) * 1000;
        const bool transition_highlighted =
            data_.transition_timer >= transition_start &&
            data_.transition_timer < transition_start + 1000;
        const bool pointer_highlighted =
            data_.transition_timer == 0 &&
            pointer_inside &&
            data_.selection_armed == 0;
        if (!transition_highlighted && !pointer_highlighted) {
            continue;
        }

        result.menu_brightness[static_cast<std::size_t>(item)] =
            kFullBrightness;
        data_.menu_selection = item;
        if (data_.transition_started == 0 &&
            data_.transition_timer == 0 &&
            input.pointer_primary_pressed) {
            data_.transition_started = 1;
            data_.transition_timer = transition_start;
            result.play_confirm_sound = true;
        }
    }

    const bool any_item_highlighted =
        result.menu_brightness[0] == kFullBrightness ||
        result.menu_brightness[1] == kFullBrightness ||
        result.menu_brightness[2] == kFullBrightness;
    if (!any_item_highlighted &&
        data_.selection_armed == 1 &&
        data_.menu_selection >= 0 &&
        data_.menu_selection < 3) {
        result.menu_brightness[
            static_cast<std::size_t>(data_.menu_selection)] =
            kFullBrightness;
    }

    if (data_.transition_started == 0 && input.confirm_pressed) {
        data_.transition_started = 1;
        data_.transition_timer =
            (data_.menu_selection + 1) * 1000;
        result.play_confirm_sound = true;
    }

    for (std::size_t index = 0;
         index < data_.smoke_delays.size();
         ++index) {
        const std::int32_t frame_count =
            input.smoke_frame_counts[index];
        if (frame_count <= 0) {
            continue;
        }

        const std::int32_t first_frame = data_.smoke_delays[index];
        const std::int32_t end_frame = first_frame + frame_count;
        if (data_.animation_frame >= first_frame &&
            data_.animation_frame < end_frame) {
            result.smoke_frames[index] =
                data_.animation_frame - first_frame;
        }
        if (data_.animation_frame == end_frame - 1) {
            data_.smoke_delays[index] =
                data_.animation_frame + frame_count + 30 +
                random_.next() % 100;
        }
    }

    ++data_.animation_frame;
    data_.previous_pointer_x = input.pointer_x;
    data_.previous_pointer_y = input.pointer_y;
    return result;
}

const TitleStateData& TitleState::data() const {
    return data_;
}

TitleStateData& TitleState::data() {
    return data_;
}

CharacterSelectState::CharacterSelectState(
    CharacterSelectStateHooks hooks)
    : hooks_(std::move(hooks)) {}

void CharacterSelectState::enter(std::int32_t retail_argument) {
    if (hooks_.begin_scene) {
        hooks_.begin_scene();
    }
    // Like retail function 0x00421920, failure is not checked here.
    loadPattern(
        hooks_.load_pattern, 4,
        "System\\Select\\Pattern\\Select.njp");
    if (hooks_.configure_input) {
        hooks_.configure_input(
            kCharacterSelectInputBindings.data(),
            kCharacterSelectInputBindings.size());
    }

    if (data_.mode == CharacterSelectMode::new_character &&
        data_.new_character_data_loaded) {
        if (hooks_.release_new_character) {
            hooks_.release_new_character();
        }
        data_.new_character_data_loaded = false;
    }

    data_.screen = 0;
    data_.input_latch = 1;
    if (retail_argument == 0) {
        data_.mode = CharacterSelectMode::new_character;
        data_.new_character_data_loaded = true;
        data_.next_save_path =
            findNextRetailSavePath(hooks_.file_exists).path;
        if (hooks_.prepare_new_character) {
            hooks_.prepare_new_character(data_.next_save_path);
        }
        data_.raw_30 = 0;
        data_.raw_34 = 0;
    } else {
        data_.mode = CharacterSelectMode::saved_game;
        data_.next_save_path.clear();
        if (hooks_.load_saved_characters) {
            hooks_.load_saved_characters();
        }
        data_.save_hover_animation = 0;
        data_.saved_game_selection = 0;
    }

    data_.fade_steps_remaining = 0x14;
    data_.launch_counter = 0;
    data_.brightness_increasing = 1;
    data_.selection_result = -1;
    data_.selected_saved_game = -1;
    if (hooks_.set_cursor_state) {
        hooks_.set_cursor_state(-1);
    }
    if ((!hooks_.voice_is_playing ||
         !hooks_.voice_is_playing(kMenuMusicSlot)) &&
        hooks_.play_voice) {
        hooks_.play_voice(kMenuMusicSlot, true);
    }
    data_.input_latch = 1;
    data_.active = true;
}

void CharacterSelectState::leave() {
    data_.temporary_buffer.clear();
    if (hooks_.release_voice) {
        hooks_.release_voice(kMenuMusicSlot);
    }
    if (hooks_.clear_scene) {
        hooks_.clear_scene();
    }
    if (hooks_.release_pattern) {
        hooks_.release_pattern(4);
    }
    data_.active = false;
}

CharacterSelectFrameResult CharacterSelectState::update(
    const CharacterSelectFrameInput& input) {
    CharacterSelectFrameResult result;
    result.mode = data_.mode;
    if (input.input_suspended) {
        result.processed = false;
        return result;
    }

    if (data_.fade_steps_remaining > 0) {
        const std::int32_t brightness =
            (21 - data_.fade_steps_remaining) * 50;
        data_.fade_value = brightness;
        data_.fade_target = brightness;
        --data_.fade_steps_remaining;
    }
    result.background_brightness = data_.fade_value;

    if (data_.brightness_increasing == 0) {
        if (data_.fade_target > 500) {
            data_.fade_target -= 80;
        }
    } else if (
        data_.fade_steps_remaining == 0 &&
        data_.fade_target < 1000) {
        data_.fade_target += 80;
    }

    const bool valid_mode =
        data_.mode == CharacterSelectMode::new_character ||
        data_.mode == CharacterSelectMode::saved_game;
    if (valid_mode) {
        data_.rendered_mode = data_.mode;

        // Both retail mode renderers contain this same transition-counter
        // prelude before their mode-specific interaction and drawing.
        bool mode_returned_early = false;
        if (data_.launch_counter == 1022) {
            result.action = CharacterSelectAction::return_to_title;
            mode_returned_early = true;
        } else if (data_.launch_counter == 2022) {
            result.action = CharacterSelectAction::exit_game;
            mode_returned_early = true;
        } else if (data_.launch_counter > 0) {
            ++data_.launch_counter;
        }

        if (!mode_returned_early &&
            data_.mode == CharacterSelectMode::saved_game) {
            updateSavedGameMode(data_, input, result);
        }

        switch (data_.screen) {
        case 10:
            result.screen_update =
                CharacterSelectScreenUpdate::screen_10;
            break;
        case 11:
            result.screen_update =
                CharacterSelectScreenUpdate::screen_11;
            break;
        case 12:
            result.screen_update =
                CharacterSelectScreenUpdate::screen_12;
            break;
        case 20:
            if (data_.launch_counter == 0) {
                data_.launch_counter = 5010;
            }
            if (data_.launch_counter == 5024) {
                result.action =
                    CharacterSelectAction::enter_gameplay;
                return result;
            }
            break;
        default:
            break;
        }
    }

    if (data_.input_latch == 1) {
        data_.input_latch = 0;
    }
    return result;
}

const CharacterSelectStateData& CharacterSelectState::data() const {
    return data_;
}

CharacterSelectStateData& CharacterSelectState::data() {
    return data_;
}

RetailSavePath findNextRetailSavePath(
    const std::function<bool(std::string_view)>& file_exists) {
    RetailSavePath result;
    for (int index = 0; index < 6; ++index) {
        std::ostringstream path;
        path << "Save\\" << std::setfill('0') << std::setw(4)
             << index << ".Ssv";
        result.path = path.str();
        if (!file_exists || !file_exists(result.path)) {
            result.available = true;
            return result;
        }
    }
    return result;
}

const std::array<std::int32_t, 57>& retailTitleInputBindings() {
    return kTitleInputBindings;
}

const std::array<std::int32_t, 58>&
retailCharacterSelectInputBindings() {
    return kCharacterSelectInputBindings;
}

}  // namespace osf
