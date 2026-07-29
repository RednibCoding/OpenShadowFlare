#include "title_state.hpp"

#include "core/retail_random.hpp"
#include "save_catalog.hpp"

#include <algorithm>
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

constexpr std::array<std::int32_t, 57> kTitleInputBindings{{
    1, 2, 16, 17, 38, 40, 37, 39, 9, 27, 13,
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

const std::array<std::int32_t, 57>& retailTitleInputBindings() {
    return kTitleInputBindings;
}

}  // namespace osf

