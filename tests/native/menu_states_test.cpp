#include "core/retail_random.hpp"
#include "states/menu_states.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool testRetailRandom() {
    osf::RetailRandom random;
    const std::array<std::int32_t, 10> expected{{
        41, 18467, 6334, 26500, 19169,
        15724, 11478, 29358, 26962, 24464,
    }};
    for (const std::int32_t value : expected) {
        if (!check(
                random.next() == value,
                "The Visual C++ retail random sequence differs.")) {
            return false;
        }
    }
    return true;
}

bool testSaveNameSearch() {
    int probes = 0;
    const osf::RetailSavePath next = osf::findNextRetailSavePath(
        [&probes](std::string_view path) {
            ++probes;
            return path == "Save\\0000.Ssv" ||
                   path == "Save\\0001.Ssv";
        });
    if (!check(
            next.available &&
                next.path == "Save\\0002.Ssv" &&
                probes == 3,
            "The next retail save-name search differs from 0x004021b0.")) {
        return false;
    }

    probes = 0;
    const osf::RetailSavePath full = osf::findNextRetailSavePath(
        [&probes](std::string_view) {
            ++probes;
            return true;
        });
    return check(
        !full.available &&
            full.path == "Save\\0005.Ssv" &&
            probes == 6,
        "The six-save retail limit behavior is incorrect.");
}

bool testTitleLifecycle() {
    std::vector<std::string> calls;
    osf::TitleStateHooks hooks;
    hooks.begin_scene = [&calls] { calls.emplace_back("begin"); };
    hooks.load_pattern =
        [&calls](std::int32_t id, std::string_view path) {
            calls.emplace_back(
                "pattern " + std::to_string(id) + " " +
                std::string(path));
            return true;
        };
    hooks.load_animation =
        [&calls](
            std::size_t index,
            std::int32_t id,
            std::string_view path) {
            calls.emplace_back(
                "animation " + std::to_string(index) + " " +
                std::to_string(id) + " " + std::string(path));
            return true;
        };
    hooks.configure_input =
        [&calls](const std::int32_t* bindings, std::size_t count) {
            calls.emplace_back("input " + std::to_string(count));
            if (count != 57 || bindings[11] != 65) {
                calls.emplace_back("bad title bindings");
            }
        };
    hooks.files_exist =
        [&calls](std::string_view path) {
            calls.emplace_back("files " + std::string(path));
            return true;
        };
    hooks.file_exists = [](std::string_view path) {
        return path == "Save\\0000.Ssv" ||
               path == "Save\\0001.Ssv";
    };
    hooks.set_cursor_state =
        [&calls](std::int32_t state) {
            calls.emplace_back("cursor " + std::to_string(state));
        };
    hooks.load_voice =
        [&calls](std::string_view path, std::int32_t slot) {
            calls.emplace_back(
                "voice " + std::to_string(slot) + " " +
                std::string(path));
        };
    hooks.clear_scene = [&calls] { calls.emplace_back("clear"); };
    hooks.release_pattern =
        [&calls](std::int32_t id) {
            calls.emplace_back("release pattern " + std::to_string(id));
        };
    hooks.release_animation =
        [&calls](std::size_t index) {
            calls.emplace_back(
                "release animation " + std::to_string(index));
        };

    osf::RetailRandom random;
    osf::TitleState title(random, std::move(hooks));
    if (!check(title.enter(), "Title-state entry unexpectedly failed.")) {
        return false;
    }

    const auto& data = title.data();
    const std::array<std::int32_t, 10> expectedDelays{{
        41, 17, 34, 40, 89, 64, 48, 18, 52, 74,
    }};
    if (!check(
            data.active &&
                data.fade_steps_remaining == 20 &&
                data.transition_timer == 0 &&
                data.saved_game_exists &&
                data.next_save_path_available &&
                data.next_save_path == "Save\\0002.Ssv" &&
                data.animation_frame == 0 &&
                data.smoke_delays == expectedDelays &&
                data.menu_selection == 0 &&
                data.selection_armed == 1 &&
                data.version.major == 2 &&
                data.version.minor == 4 &&
                data.version.patch == 0,
            "Title-state fields differ from 0x00420c10/0x00420c40.")) {
        return false;
    }
    if (!check(
            calls.size() == 26 &&
                calls.front() == "begin" &&
                calls[1] ==
                    "pattern 4 System\\Title\\Pattern\\Title.njp" &&
                calls[22] == "input 57" &&
                calls[23] == "files Save\\*.Ssv" &&
                calls[24] == "cursor -1" &&
                calls[25] ==
                    "voice 500 System\\Title\\Music\\BGM00.Voc",
            "Title-state entry service order differs from retail.")) {
        return false;
    }

    title.leave();
    return check(
        !title.data().active &&
            calls.size() == 48 &&
            calls[26] == "clear" &&
            calls[27] == "release pattern 4" &&
            calls.back() == "release animation 9",
        "Title-state resource release order differs from 0x00420df0.");
}

bool testTitleLoadFailure() {
    osf::TitleStateHooks hooks;
    hooks.load_pattern = [](
        std::int32_t id, std::string_view) {
        return id != 9;
    };

    osf::RetailRandom random;
    osf::TitleState title(random, std::move(hooks));
    return check(
        !title.enter() && !title.data().active &&
            random.state() == 1,
        "Title entry did not stop at a failed required smoke pattern.");
}

bool testTitleFrames() {
    osf::RetailRandom random;
    osf::TitleState title(random);
    if (!check(title.enter(), "Title-state entry unexpectedly failed.")) {
        return false;
    }

    osf::MenuFrameInput input;
    for (int frame = 0; frame < 20; ++frame) {
        const osf::TitleFrameResult result = title.update(input);
        if (!check(
                result.scene_brightness == frame * 50 &&
                    !result.play_title_sound,
                "The title fade-in timing differs from retail.")) {
            return false;
        }
    }

    osf::TitleFrameResult result = title.update(input);
    if (!check(
            result.scene_brightness == 1000 &&
                result.play_title_sound &&
                !result.start_menu_music &&
                title.data().music_delay_frames == 1,
            "The title sound did not start on the first full-brightness frame.")) {
        return false;
    }
    for (int frame = 1; frame < 60; ++frame) {
        result = title.update(input);
    }
    if (!check(
            result.start_menu_music &&
                title.data().music_started == 1 &&
                title.data().music_delay_frames == 60,
            "The menu music did not start after the retail 60-frame delay.")) {
        return false;
    }

    title.data().saved_game_exists = false;
    title.data().next_save_path_available = true;
    title.data().menu_selection = 0;
    input.down_pressed = true;
    result = title.update(input);
    input.down_pressed = false;
    if (!check(
            title.data().menu_selection == 2 &&
                result.play_move_sound_count == 1,
            "Title navigation did not skip an unavailable Continue item.")) {
        return false;
    }

    input.up_pressed = true;
    result = title.update(input);
    input.up_pressed = false;
    if (!check(
            title.data().menu_selection == 0 &&
                result.play_move_sound_count == 1,
            "Title navigation did not skip Continue while moving up.")) {
        return false;
    }

    title.data().next_save_path_available = false;
    title.data().menu_selection = 0;
    result = title.update(input);
    if (!check(
            title.data().menu_selection == 2,
            "The title did not settle on Exit when both game choices were unavailable.")) {
        return false;
    }

    title.data().next_save_path_available = true;
    title.data().saved_game_exists = true;
    title.data().menu_selection = 2;
    input.pointer_x = 300;
    input.pointer_y = 375;
    input.pointer_primary_pressed = true;
    result = title.update(input);
    input.pointer_primary_pressed = false;
    if (!check(
            title.data().menu_selection == 0 &&
                title.data().selection_armed == 0 &&
                title.data().transition_started == 1 &&
                title.data().transition_timer == 1000 &&
                result.play_confirm_sound,
            "The first title-menu hover/click rectangle differs from retail.")) {
        return false;
    }

    for (int frame = 0; frame < 20; ++frame) {
        result = title.update(input);
        if (!check(
                result.action == osf::TitleAction::none,
                "The title changed state before its 20-frame transition finished.")) {
            return false;
        }
    }
    result = title.update(input);
    if (!check(
            result.action == osf::TitleAction::open_character_select &&
                result.character_select_argument == 0 &&
                title.data().transition_timer == 1020,
            "New Game did not dispatch retail state 1 argument 0 at frame 1020.")) {
        return false;
    }

    title.data().network_error_kind = 2;
    title.data().network_error_frames = 2;
    const std::int32_t animationFrame = title.data().animation_frame;
    result = title.update(input);
    if (!check(
            result.network_error_visible &&
                result.network_error_kind == 2 &&
                title.data().network_error_frames == 1 &&
                title.data().animation_frame == animationFrame,
            "The title network-error overlay advanced hidden state.")) {
        return false;
    }
    result = title.update(input);
    return check(
        result.network_error_visible &&
            title.data().network_error_kind == 0 &&
            title.data().animation_frame == animationFrame,
        "The title network-error overlay duration differs from retail.");
}

bool testTitleSmokeSchedule() {
    osf::RetailRandom random;
    osf::TitleState title(random);
    if (!check(title.enter(), "Title-state entry unexpectedly failed.")) {
        return false;
    }

    title.data().animation_frame = 4;
    title.data().smoke_delays.fill(1000);
    title.data().smoke_delays[0] = 2;
    osf::MenuFrameInput input;
    input.smoke_frame_counts[0] = 3;
    const osf::TitleFrameResult result = title.update(input);

    // enter() consumed ten rand() results. The next result is 5705.
    return check(
        result.smoke_frames[0] == 2 &&
            title.data().smoke_delays[0] == 4 + 3 + 30 + 5 &&
            title.data().animation_frame == 5,
        "The title smoke animation reschedule differs from 0x00420e60.");
}

bool testCharacterSelectLifecycle() {
    std::vector<std::string> calls;
    bool voicePlaying = false;
    osf::CharacterSelectStateHooks hooks;
    hooks.begin_scene = [&calls] { calls.emplace_back("begin"); };
    hooks.load_pattern =
        [&calls](std::int32_t id, std::string_view path) {
            calls.emplace_back(
                "pattern " + std::to_string(id) + " " +
                std::string(path));
            return false;
        };
    hooks.configure_input =
        [&calls](const std::int32_t* bindings, std::size_t count) {
            calls.emplace_back("input " + std::to_string(count));
            if (count != 58 || bindings[11] != 46) {
                calls.emplace_back("bad select bindings");
            }
        };
    hooks.file_exists = [](std::string_view path) {
        return path == "Save\\0000.Ssv";
    };
    hooks.prepare_new_character =
        [&calls](std::string_view path) {
            calls.emplace_back("prepare new " + std::string(path));
        };
    hooks.release_new_character =
        [&calls] { calls.emplace_back("release new"); };
    hooks.load_saved_characters =
        [&calls] { calls.emplace_back("load saved"); };
    hooks.set_cursor_state =
        [&calls](std::int32_t state) {
            calls.emplace_back("cursor " + std::to_string(state));
        };
    hooks.voice_is_playing =
        [&voicePlaying](std::int32_t) { return voicePlaying; };
    hooks.play_voice =
        [&calls, &voicePlaying](std::int32_t slot, bool loop) {
            calls.emplace_back(
                "play " + std::to_string(slot) +
                (loop ? " loop" : " once"));
            voicePlaying = true;
        };
    hooks.release_voice =
        [&calls](std::int32_t slot) {
            calls.emplace_back("release voice " + std::to_string(slot));
        };
    hooks.clear_scene = [&calls] { calls.emplace_back("clear"); };
    hooks.release_pattern =
        [&calls](std::int32_t id) {
            calls.emplace_back("release pattern " + std::to_string(id));
        };

    osf::CharacterSelectState state(std::move(hooks));
    state.enter(0);
    const auto& newCharacter = state.data();
    if (!check(
            newCharacter.active &&
                newCharacter.mode ==
                    osf::CharacterSelectMode::new_character &&
                newCharacter.fade_steps_remaining == 20 &&
                newCharacter.launch_counter == 0 &&
                newCharacter.brightness_increasing == 1 &&
                newCharacter.selected_save == -1 &&
                newCharacter.screen == 0 &&
                newCharacter.input_latch == 1 &&
                newCharacter.new_character_data_loaded &&
                newCharacter.next_save_path == "Save\\0001.Ssv",
            "New-character selection entry fields differ.")) {
        return false;
    }
    if (!check(
            calls.size() == 6 &&
                calls[0] == "begin" &&
                calls[1] ==
                    "pattern 4 System\\Select\\Pattern\\Select.njp" &&
                calls[2] == "input 58" &&
                calls[3] == "prepare new Save\\0001.Ssv" &&
                calls[4] == "cursor -1" &&
                calls[5] == "play 500 loop",
            "New-character selection service order differs.")) {
        return false;
    }

    state.data().temporary_buffer = {1, 2, 3};
    state.leave();
    if (!check(
            state.data().temporary_buffer.empty() &&
                !state.data().active &&
                calls[calls.size() - 3] == "release voice 500" &&
                calls[calls.size() - 2] == "clear" &&
                calls.back() == "release pattern 4",
            "Character-select leave behavior differs from 0x00421bf0.")) {
        return false;
    }

    state.enter(5);
    return check(
        state.data().active &&
            state.data().mode ==
                osf::CharacterSelectMode::saved_game &&
            !state.data().new_character_data_loaded &&
            state.data().next_save_path.empty() &&
            calls[calls.size() - 3] == "release new" &&
            calls[calls.size() - 2] == "load saved" &&
            calls.back() == "cursor -1",
        "Saved-game entry did not replace pending new-character data.");
}

bool testCharacterSelectFrames() {
    osf::CharacterSelectState state;
    state.enter(0);

    osf::CharacterSelectFrameResult result = state.update();
    if (!check(
            result.processed &&
                result.background_brightness == 50 &&
                result.mode == osf::CharacterSelectMode::new_character &&
                state.data().fade_steps_remaining == 19 &&
                state.data().input_latch == 0,
            "The first character-selection frame differs from retail.")) {
        return false;
    }

    osf::CharacterSelectFrameInput suspended;
    suspended.input_suspended = true;
    const std::int32_t remaining = state.data().fade_steps_remaining;
    result = state.update(suspended);
    if (!check(
            !result.processed &&
                state.data().fade_steps_remaining == remaining,
            "Suspended character-selection input advanced state.")) {
        return false;
    }

    state.data().screen = 20;
    state.data().launch_counter = 0;
    result = state.update();
    if (!check(
            state.data().launch_counter == 5010 &&
                result.action == osf::CharacterSelectAction::none,
            "The gameplay launch delay did not begin at retail counter 5010.")) {
        return false;
    }
    for (int frame = 0; frame < 13; ++frame) {
        result = state.update();
        if (!check(
                result.action == osf::CharacterSelectAction::none,
                "Character selection entered gameplay too early.")) {
            return false;
        }
    }
    result = state.update();
    if (!check(
            state.data().launch_counter == 5024 &&
                result.action ==
                    osf::CharacterSelectAction::enter_gameplay,
            "Character selection did not enter gameplay at counter 5024.")) {
        return false;
    }

    state.data().screen = 10;
    state.data().launch_counter = 1022;
    result = state.update();
    if (!check(
            result.action ==
                osf::CharacterSelectAction::return_to_title &&
                result.screen_update ==
                    osf::CharacterSelectScreenUpdate::screen_10,
            "The character-selection return-to-title timer differs.")) {
        return false;
    }

    state.data().launch_counter = 2022;
    result = state.update();
    return check(
        result.action == osf::CharacterSelectAction::exit_game,
        "The character-selection exit timer differs.");
}

}  // namespace

int main() {
    if (!testRetailRandom() ||
        !testSaveNameSearch() ||
        !testTitleLifecycle() ||
        !testTitleLoadFailure() ||
        !testTitleFrames() ||
        !testTitleSmokeSchedule() ||
        !testCharacterSelectLifecycle() ||
        !testCharacterSelectFrames()) {
        return 1;
    }
    return 0;
}
