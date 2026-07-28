#include "menu_states.hpp"

#include "core/retail_random.hpp"

#include <iomanip>
#include <sstream>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kMenuMusicSlot = 500;

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
    data_.elapsed_frames = 0;
    data_.saved_game_exists =
        hooks_.files_exist && hooks_.files_exist("Save\\*.Ssv");

    if (hooks_.set_cursor_state) {
        hooks_.set_cursor_state(-1);
    }

    const RetailSavePath next_save =
        findNextRetailSavePath(hooks_.file_exists);
    data_.next_save_path = next_save.path;
    data_.next_save_path_available = next_save.available;
    data_.overlay_index = 0;
    for (std::int32_t& delay : data_.smoke_delays) {
        delay = random_.next() % 0x5a;
    }

    if (hooks_.load_voice) {
        hooks_.load_voice(
            "System\\Title\\Music\\BGM00.Voc", kMenuMusicSlot);
    }

    data_.sound_started = 0;
    data_.music_started = 0;
    data_.sound_delay = 0;
    data_.menu_selection = 0;
    data_.selection_armed = 1;
    data_.input_accepted = 0;
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

    if (data_.mode == CharacterSelectMode::saved_game &&
        data_.save_catalog_loaded) {
        if (hooks_.release_save_catalog) {
            hooks_.release_save_catalog();
        }
        data_.save_catalog_loaded = false;
    }

    data_.screen = 0;
    data_.input_accepted = 1;
    if (retail_argument == 0) {
        data_.mode = CharacterSelectMode::saved_game;
        data_.save_catalog_loaded = true;
        data_.next_save_path =
            findNextRetailSavePath(hooks_.file_exists).path;
        if (hooks_.load_save_catalog) {
            hooks_.load_save_catalog(data_.next_save_path);
        }
        data_.raw_30 = 0;
        data_.raw_34 = 0;
    } else {
        data_.mode = CharacterSelectMode::new_character;
        data_.next_save_path.clear();
        if (hooks_.reset_new_character_data) {
            hooks_.reset_new_character_data();
        }
        data_.raw_14 = 0;
        data_.raw_1c = 0;
    }

    data_.fade_steps_remaining = 0x14;
    data_.launch_counter = 0;
    data_.first_frame = 1;
    data_.selected_save = -1;
    if (hooks_.set_cursor_state) {
        hooks_.set_cursor_state(-1);
    }
    if ((!hooks_.voice_is_playing ||
         !hooks_.voice_is_playing(kMenuMusicSlot)) &&
        hooks_.play_voice) {
        hooks_.play_voice(kMenuMusicSlot, true);
    }
    data_.input_accepted = 1;
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
