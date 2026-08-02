#include "core/retail_random.hpp"
#include "render/character_select_renderer.hpp"
#include "render/gameplay_debug_renderer.hpp"
#include "render/gameplay_help_renderer.hpp"
#include "render/gameplay_options_renderer.hpp"
#include "resources/save_catalog.hpp"
#include "states/character_select_state.hpp"
#include "states/gameplay_debug_menu.hpp"
#include "states/gameplay_options_menu.hpp"
#include "states/save_slot.hpp"
#include "states/title_state.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct PatternCall {
    std::size_t index = 0;
    osf::gapi::PatternDraw draw;
};

struct TextCall {
    std::string text;
    osf::gapi::TextDraw draw;
};

class RecordingBackend final : public osf::gapi::Backend {
public:
    void beginFrame(osf::gapi::Color) override {}

    bool drawPattern(
        const osf::gapi::NjpImage&,
        std::size_t pattern_index,
        const osf::gapi::PatternDraw& draw) override {
        patterns.push_back({pattern_index, draw});
        return true;
    }

    bool drawBitmap(
        const osf::gapi::BitmapImage&,
        const osf::gapi::BitmapDraw&) override {
        return true;
    }

    bool drawText(
        const osf::gapi::NjpImage&,
        std::string_view text,
        const osf::gapi::TextDraw& draw) override {
        ++text_draws;
        texts.push_back({std::string(text), draw});
        return true;
    }

    bool drawRectangle(
        const osf::gapi::RectangleDraw& draw) override {
        rectangles.push_back(draw);
        return true;
    }

    void endFrame() override {}

    std::vector<PatternCall> patterns;
    std::vector<TextCall> texts;
    std::vector<osf::gapi::RectangleDraw> rectangles;
    std::int32_t text_draws = 0;
};

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

bool testRetailSaveCatalogFields() {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_save_catalog_test";
    std::error_code filesystem_error;
    std::filesystem::remove_all(root, filesystem_error);
    std::filesystem::create_directories(
        root / "Save", filesystem_error);
    if (!check(
            !filesystem_error,
            "The save-catalog fixture directory could not be created.")) {
        return false;
    }

    std::array<std::uint8_t, 16 + 0x160> bytes{};
    const std::string name = "asof";
    std::copy(name.begin(), name.end(), bytes.begin() + 16);
    const auto writeI32 = [&bytes](
                              std::size_t record_offset,
                              std::int32_t value) {
        const std::uint32_t raw =
            static_cast<std::uint32_t>(value);
        const std::size_t offset = 16 + record_offset;
        bytes[offset] = static_cast<std::uint8_t>(raw);
        bytes[offset + 1] =
            static_cast<std::uint8_t>(raw >> 8u);
        bytes[offset + 2] =
            static_cast<std::uint8_t>(raw >> 16u);
        bytes[offset + 3] =
            static_cast<std::uint8_t>(raw >> 24u);
    };
    writeI32(0x18, 0);
    writeI32(0x1c, 16);
    writeI32(0x24, 1);
    writeI32(0x34, 260);
    writeI32(0x3c, 160);
    writeI32(0xd8, 18);

    const std::filesystem::path save_path =
        root / "Save" / "0000.Ssv";
    std::ofstream stream(save_path, std::ios::binary);
    stream.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    stream.close();

    const std::vector<osf::RetailSaveSummary> catalog =
        osf::loadRetailSaveCatalog(root);
    const bool passed = check(
        catalog.size() == 1 &&
            catalog[0].slot == 0 &&
            catalog[0].name == "asof" &&
            catalog[0].gender == 0 &&
            catalog[0].job == 16 &&
            catalog[0].level == 1 &&
            catalog[0].life == 260 &&
            catalog[0].mana == 160 &&
            catalog[0].experience == 18,
        "The save catalog did not decode the retail row fields.");
    std::filesystem::remove_all(root, filesystem_error);
    return passed;
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
                newCharacter.selection_result == -1 &&
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

bool testSavedGameSelectionFrames() {
    osf::CharacterSelectState state;
    osf::CharacterSelectFrameInput input;
    input.saved_game_count = 6;

    const auto makeReady = [&state] {
        state.enter(1);
        state.data().fade_value = 1000;
        state.data().fade_target = 1000;
        state.data().fade_steps_remaining = 0;
        state.data().brightness_increasing = 1;
        state.data().screen = 0;
        state.data().launch_counter = 0;
        state.data().save_hover_animation = 64;
        state.data().saved_game_selection = 0;
        state.data().input_latch = 0;
    };

    makeReady();
    input.down_pressed = true;
    osf::CharacterSelectFrameResult result = state.update(input);
    input.down_pressed = false;
    if (!check(
            state.data().saved_game_selection == 2 &&
                result.play_move_sound_count == 1,
            "Saved-game Down navigation does not use the retail two-column stride.")) {
        return false;
    }

    input.right_pressed = true;
    result = state.update(input);
    input.right_pressed = false;
    if (!check(
            state.data().saved_game_selection == 3 &&
                result.play_move_sound_count == 1,
            "Saved-game Right navigation differs from retail.")) {
        return false;
    }

    input.up_pressed = true;
    result = state.update(input);
    input.up_pressed = false;
    if (!check(
            state.data().saved_game_selection == 1 &&
                result.play_move_sound_count == 1,
            "Saved-game Up navigation differs from retail.")) {
        return false;
    }

    input.left_pressed = true;
    result = state.update(input);
    input.left_pressed = false;
    if (!check(
            state.data().saved_game_selection == 0 &&
                result.play_move_sound_count == 1,
            "Saved-game Left navigation differs from retail.")) {
        return false;
    }

    input.left_pressed = true;
    result = state.update(input);
    input.left_pressed = false;
    if (!check(
            state.data().saved_game_selection == 0 &&
                result.play_move_sound_count == 0,
            "Saved-game navigation played a sound at a clamped boundary.")) {
        return false;
    }

    input.pointer_x = 400;
    input.pointer_y = 300;
    input.pointer_primary_pressed = true;
    result = state.update(input);
    if (!check(
            state.data().saved_game_selection == 3 &&
                state.data().pointer_click_cooldown == 10 &&
                result.mode_action ==
                    osf::CharacterSelectModeAction::none &&
                result.play_selection_sound_count == 1,
            "A single click did not select the retail save-list cell.")) {
        return false;
    }

    result = state.update(input);
    input.pointer_primary_pressed = false;
    if (!check(
            result.pointer_double_click &&
                result.mode_action ==
                    osf::CharacterSelectModeAction::choose_saved_game &&
                result.screen_update ==
                    osf::CharacterSelectScreenUpdate::screen_10 &&
                result.play_selection_sound_count == 2 &&
                state.data().screen == 10 &&
                state.data().selected_saved_game == 3 &&
                state.data().brightness_increasing == 0,
            "Saved-game double-click timing or selection differs from retail.")) {
        return false;
    }

    makeReady();
    input.pointer_x = 0;
    input.pointer_y = 0;
    input.confirm_pressed = true;
    result = state.update(input);
    input.confirm_pressed = false;
    if (!check(
            result.mode_action ==
                osf::CharacterSelectModeAction::choose_saved_game &&
                state.data().screen == 10 &&
                state.data().selected_saved_game == 0,
            "Enter did not open the saved-game confirmation screen.")) {
        return false;
    }

    input.back_pressed = true;
    result = state.update(input);
    input.back_pressed = false;
    if (!check(
            state.data().screen == 0 &&
                state.data().brightness_increasing == 1,
            "Saved-game popup Back did not restore the list brightness.")) {
        return false;
    }
    input.right_pressed = true;
    result = state.update(input);
    input.right_pressed = false;
    if (!check(
            state.data().saved_game_selection == 1,
            "Saved-game input stayed locked after closing the popup.")) {
        return false;
    }

    makeReady();
    input.delete_pressed = true;
    result = state.update(input);
    input.delete_pressed = false;
    if (!check(
            result.mode_action ==
                osf::CharacterSelectModeAction::
                    open_delete_saved_game_dialog &&
                state.data().screen == 1 &&
                state.data().dialog_selection == 1 &&
                state.data().dialog_input_armed == 1,
            "Delete did not open the retail saved-game deletion screen.")) {
        return false;
    }

    makeReady();
    input.back_pressed = true;
    result = state.update(input);
    input.back_pressed = false;
    if (!check(
            result.mode_action ==
                osf::CharacterSelectModeAction::start_back_transition &&
                state.data().launch_counter == 1000,
            "Saved-game Back did not start retail transition 1000.")) {
        return false;
    }

    makeReady();
    input.pointer_x = 600;
    input.pointer_y = 10;
    input.pointer_primary_pressed = true;
    result = state.update(input);
    input.pointer_primary_pressed = false;
    if (!check(
            result.mode_action ==
                osf::CharacterSelectModeAction::start_exit_transition &&
                state.data().launch_counter == 2000,
            "The saved-game Exit button did not start retail transition 2000.")) {
        return false;
    }

    makeReady();
    state.data().save_hover_animation = 27;
    input.pointer_x = 100;
    input.pointer_y = 200;
    result = state.update(input);
    if (!check(
            state.data().save_hover_animation == 64,
            "The saved-game hover animation did not enter its retail idle frame.")) {
        return false;
    }
    result = state.update(input);
    if (!check(
            state.data().save_hover_animation == 65,
            "The saved-game hover animation did not advance inside a save cell.")) {
        return false;
    }

    makeReady();
    state.data().launch_counter = 1000;
    result = state.update(input);
    return check(
        state.data().launch_counter == 1001 &&
            result.mode_brightness == 900,
        "The saved-game transition brightness differs from retail.");
}

bool testSavedGameDeleteDialog() {
    std::vector<std::int32_t> deletedCharacters;
    osf::CharacterSelectStateHooks hooks;
    hooks.delete_saved_character =
        [&deletedCharacters](std::int32_t index) {
            deletedCharacters.push_back(index);
        };
    osf::CharacterSelectState state(std::move(hooks));
    osf::CharacterSelectFrameInput input;
    input.saved_game_count = 4;

    const auto makeReady =
        [&state](std::int32_t selected = 2) {
            state.enter(1);
            state.data().fade_value = 1000;
            state.data().fade_target = 1000;
            state.data().fade_steps_remaining = 0;
            state.data().brightness_increasing = 0;
            state.data().launch_counter = 0;
            state.data().screen = 1;
            state.data().saved_game_selection = selected;
            state.data().dialog_selection = 1;
            state.data().dialog_input_armed = 1;
            state.data().dialog_previous_pointer_x = 0;
            state.data().dialog_previous_pointer_y = 0;
            state.data().input_latch = 0;
        };

    makeReady();
    input.up_pressed = true;
    osf::CharacterSelectFrameResult result = state.update(input);
    input.up_pressed = false;
    if (!check(
            state.data().dialog_selection == 0 &&
                state.data().dialog_input_armed == 1 &&
                state.data().screen == 1 &&
                result.play_move_sound_count == 1,
            "Delete confirmation Up did not select the retail Yes choice.")) {
        return false;
    }

    input.right_pressed = true;
    result = state.update(input);
    input.right_pressed = false;
    if (!check(
            state.data().dialog_selection == 1 &&
                result.play_move_sound_count == 1,
            "Delete confirmation Right did not select the retail No choice.")) {
        return false;
    }

    input.confirm_pressed = true;
    result = state.update(input);
    input.confirm_pressed = false;
    if (!check(
            result.mode_action ==
                osf::CharacterSelectModeAction::
                    cancel_saved_game_delete &&
                result.play_selection_sound_count == 1 &&
                state.data().screen == 0 &&
                state.data().brightness_increasing == 1 &&
                deletedCharacters.empty(),
            "Confirming No did not close the deletion dialog without deleting.")) {
        return false;
    }

    makeReady(3);
    input.left_pressed = true;
    input.confirm_pressed = true;
    result = state.update(input);
    input.left_pressed = false;
    input.confirm_pressed = false;
    if (!check(
            result.mode_action ==
                osf::CharacterSelectModeAction::
                    confirm_saved_game_delete &&
                result.play_move_sound_count == 1 &&
                result.play_selection_sound_count == 1 &&
                state.data().saved_game_selection == 0 &&
                state.data().screen == 0 &&
                deletedCharacters ==
                    std::vector<std::int32_t>{3},
            "Confirming Yes did not delete and reset the retail selection.")) {
        return false;
    }

    makeReady();
    input.back_pressed = true;
    result = state.update(input);
    input.back_pressed = false;
    if (!check(
            result.mode_action ==
                osf::CharacterSelectModeAction::
                    cancel_saved_game_delete &&
                state.data().screen == 0 &&
                deletedCharacters.size() == 1,
            "Back did not cancel the saved-game deletion dialog.")) {
        return false;
    }

    makeReady(1);
    state.data().dialog_previous_pointer_x = 260;
    state.data().dialog_previous_pointer_y = 280;
    input.pointer_x = 260;
    input.pointer_y = 280;
    input.pointer_primary_pressed = true;
    result = state.update(input);
    input.pointer_primary_pressed = false;
    if (!check(
            state.data().screen == 1 &&
                state.data().dialog_input_armed == 1 &&
                deletedCharacters.size() == 1,
            "An unmoved pointer bypassed the retail dialog input arm.")) {
        return false;
    }

    input.pointer_x = 261;
    result = state.update(input);
    if (!check(
            state.data().dialog_input_armed == 0 &&
                state.data().dialog_selection == 0,
            "Pointer motion over Yes did not arm its retail hover choice.")) {
        return false;
    }

    input.pointer_primary_pressed = true;
    result = state.update(input);
    input.pointer_primary_pressed = false;
    return check(
        result.mode_action ==
            osf::CharacterSelectModeAction::
                confirm_saved_game_delete &&
            deletedCharacters ==
                std::vector<std::int32_t>({3, 1}),
        "Clicking an armed Yes choice did not delete the selected save.");
}

bool testNewCharacterCreationAndModeScreens() {
    osf::CharacterSelectStateHooks hooks;
    hooks.read_clipboard = [] {
        return std::string("127.0.0.1");
    };
    osf::CharacterSelectState state(std::move(hooks));
    state.enter(0);
    state.data().fade_value = 1000;
    state.data().fade_target = 1000;
    state.data().fade_steps_remaining = 0;
    state.data().brightness_increasing = 1;
    state.data().screen = 0;
    state.data().launch_counter = 0;
    state.data().input_latch = 0;

    osf::CharacterSelectFrameInput input;
    input.right_pressed = true;
    osf::CharacterSelectFrameResult result = state.update(input);
    input.right_pressed = false;
    if (!check(
            state.data().dialog_selection == 1 &&
                result.play_move_sound_count == 1,
            "New-character Right did not select the female character.")) {
        return false;
    }

    input.confirm_pressed = true;
    result = state.update(input);
    input.confirm_pressed = false;
    if (!check(
                state.data().screen == 1 &&
                state.data().character_gender == 0 &&
                state.data().name_entry_active &&
                state.data().character_transition_counter == 1000 &&
                result.mode_action ==
                    osf::CharacterSelectModeAction::begin_name_entry,
            "Confirm did not begin retail new-character name entry.")) {
        return false;
    }

    result = state.update(input);
    if (!check(
            result.character_transition_counter == 1000 &&
                state.data().character_transition_counter == 1001,
            "The portrait slide did not begin at retail phase 1000.")) {
        return false;
    }
    for (std::int32_t phase = 1001; phase < 1020; ++phase) {
        result = state.update(input);
        if (!check(
                result.character_transition_counter == phase,
                "The portrait slide skipped a retail animation phase.")) {
            return false;
        }
    }
    result = state.update(input);
    if (!check(
            result.character_transition_counter == 1020 &&
                state.data().character_transition_counter == 0,
            "The portrait slide did not finish at the centered position.")) {
        return false;
    }

    input.back_pressed = true;
    result = state.update(input);
    input.back_pressed = false;
    if (!check(
            state.data().screen == 0 &&
                result.character_transition_counter == 2000 &&
                state.data().character_transition_counter == 2000,
            "Cancel did not begin the retail reverse portrait slide.")) {
        return false;
    }
    result = state.update(input);
    if (!check(
            result.character_transition_counter == 2000 &&
                state.data().character_transition_counter == 2001,
            "The reverse portrait slide did not begin at phase 2000.")) {
        return false;
    }
    for (std::int32_t phase = 2001; phase < 2020; ++phase) {
        result = state.update(input);
        if (!check(
                result.character_transition_counter == phase,
                "The reverse portrait slide skipped a retail phase.")) {
            return false;
        }
    }
    result = state.update(input);
    if (!check(
            result.character_transition_counter == 2020 &&
                state.data().character_transition_counter == 0,
            "The reverse portrait slide did not restore both portraits.")) {
        return false;
    }

    input.confirm_pressed = true;
    result = state.update(input);
    input.confirm_pressed = false;
    if (!check(
            state.data().screen == 1 &&
                result.character_transition_counter == 1000,
            "Character selection did not restart after name-entry cancel.")) {
        return false;
    }

    input.text_input = "Mina";
    input.backspace_pressed = true;
    result = state.update(input);
    input.text_input.clear();
    input.backspace_pressed = false;
    if (!check(
            state.data().character_name == "Mina",
            "Portable character name input did not preserve frame order.")) {
        return false;
    }

    input.backspace_pressed = true;
    result = state.update(input);
    input.backspace_pressed = false;
    if (!check(
            state.data().character_name == "Min",
            "Backspace did not remove one UTF-8 character.")) {
        return false;
    }

    input.text_input = "a";
    input.confirm_pressed = true;
    result = state.update(input);
    input.text_input.clear();
    input.confirm_pressed = false;
    if (!check(
            state.data().character_name == "Mina" &&
                state.data().screen == 10 &&
                !state.data().name_entry_active &&
                result.mode_action ==
                    osf::CharacterSelectModeAction::
                        accept_character_name &&
                result.screen_update ==
                    osf::CharacterSelectScreenUpdate::screen_10,
            "A valid character name did not open the mode menu.")) {
        return false;
    }

    input.confirm_pressed = true;
    result = state.update(input);
    input.confirm_pressed = false;
    if (!check(
            state.data().screen == 11 &&
                result.screen_update ==
                    osf::CharacterSelectScreenUpdate::screen_10,
            "Online Mode did not open the online-game menu.")) {
        return false;
    }

    input.down_pressed = true;
    result = state.update(input);
    input.down_pressed = false;
    if (!check(
            state.data().dialog_selection == 1,
            "Online-game Down did not select Join Game.")) {
        return false;
    }

    input.confirm_pressed = true;
    result = state.update(input);
    input.confirm_pressed = false;
    if (!check(
            state.data().screen == 12 &&
                state.data().host_entry_active &&
                state.data().selection_result == 2,
            "Join Game did not open host-address entry.")) {
        return false;
    }

    input.pointer_x = 380;
    input.pointer_y = 235;
    input.pointer_primary_pressed = true;
    result = state.update(input);
    input.pointer_primary_pressed = false;
    input.pointer_x = 0;
    input.pointer_y = 0;
    if (!check(
            state.data().host_address == "127.0.0.1",
            "The host-address Paste button did not use the clipboard hook.")) {
        return false;
    }

    input.confirm_pressed = true;
    result = state.update(input);
    input.confirm_pressed = false;
    if (!check(
            state.data().host_address == "127.0.0.1" &&
                state.data().screen == 20 &&
                !state.data().host_entry_active,
            "A host address did not begin the gameplay transition.")) {
        return false;
    }

    state.enter(0);
    state.data().fade_value = 1000;
    state.data().fade_target = 1000;
    state.data().fade_steps_remaining = 0;
    state.data().screen = 10;
    state.data().dialog_selection = 1;
    state.data().input_latch = 0;
    input.confirm_pressed = true;
    result = state.update(input);
    input.confirm_pressed = false;
    return check(
        state.data().screen == 20 &&
            state.data().selection_result == 0,
        "Single Mode did not begin the offline gameplay transition.");
}

bool testNewCharacterRetailDrawing() {
    osf::gapi::NjpImage select;
    osf::gapi::NjpImage font;
    osf::CharacterSelectStateData data;
    data.mode = osf::CharacterSelectMode::new_character;
    data.screen = 1;
    data.character_gender = 1;
    data.name_entry_active = true;

    osf::CharacterSelectFrameResult frame;
    frame.background_brightness = 1000;
    frame.mode_brightness = 1000;
    frame.character_transition_counter = 1010;
    osf::CharacterSelectFrameInput input;

    RecordingBackend backend;
    osf::renderCharacterSelect(
        backend,
        select,
        &font,
        data,
        frame,
        {},
        {});
    if (!check(
            backend.patterns.size() == 9 &&
                backend.patterns[4].index == 9 &&
                backend.patterns[4].draw.x == 0 &&
                backend.patterns[4].draw.brightness == 500 &&
                backend.patterns[6].index == 8 &&
                backend.patterns[6].draw.x == 48 &&
                backend.patterns[6].draw.brightness == 1000 &&
                backend.patterns[8].index == 35,
            "The half-way portrait slide differs from retail packets.")) {
        return false;
    }
    if (!check(
            backend.rectangles.size() == 2 &&
                backend.rectangles[0].x == 190 &&
                backend.rectangles[0].y == 407 &&
                backend.rectangles[0].width == 130 &&
                backend.rectangles[0].height == 20 &&
                backend.rectangles[0].color.red == 64 &&
                backend.rectangles[1].x == 194 &&
                backend.rectangles[1].y == 411 &&
                backend.rectangles[1].width == 6 &&
                backend.rectangles[1].height == 12 &&
                backend.rectangles[1].color.red == 128,
            "The retail name field or block caret geometry differs.")) {
        return false;
    }

    data.character_name = "Mina";
    frame.character_transition_counter = 1020;
    backend = {};
    osf::renderCharacterSelect(
        backend,
        select,
        &font,
        data,
        frame,
        {},
        {});
    if (!check(
        backend.patterns.size() == 10 &&
            backend.patterns[6].index == 8 &&
            backend.patterns[6].draw.x == 97 &&
            backend.patterns[9].index == 4 &&
            backend.rectangles[1].x == 218 &&
            backend.text_draws == 1,
        "The centered portrait, normal OK button, or caret position differs.")) {
        return false;
    }

    data.mode = osf::CharacterSelectMode::saved_game;
    data.screen = 10;
    data.dialog_selection = 0;
    frame.mode_brightness = 500;
    backend = {};
    osf::renderCharacterSelect(
        backend,
        select,
        &font,
        data,
        frame,
        {},
        {});
    const std::size_t popupStart = backend.patterns.size() - 4;
    if (!check(
        backend.patterns.front().index == 41 &&
            backend.patterns.front().draw.brightness == 500 &&
            backend.patterns[4].index == 42 &&
            backend.patterns[6].index == 49 &&
            backend.patterns[popupStart].index == 38 &&
            backend.patterns[popupStart].draw.brightness == 1000 &&
            backend.patterns.back().draw.brightness == 1000,
        "The saved-game static numbers or popup layering differ.")) {
        return false;
    }

    data.screen = 0;
    data.brightness_increasing = 1;
    data.save_hover_animation = 65;
    input.pointer_x = 100;
    input.pointer_y = 200;
    frame.save_slot_hovered[0] = true;
    osf::RetailSaveSummary save;
    save.name = "asof";
    save.gender = 0;
    save.job = 16;
    save.level = 1;
    save.life = 260;
    save.mana = 160;
    save.experience = 18;
    backend = {};
    osf::renderCharacterSelect(
        backend,
        select,
        &font,
        data,
        frame,
        {save},
        {});
    if (!check(
            backend.patterns[4].index == 45 &&
                backend.patterns[6].index == 49,
            "The retail hover pulse affected more than one save number.")) {
        return false;
    }

    const auto hasText = [&backend](
                             std::string_view text,
                             std::int32_t x,
                             std::int32_t y,
                             osf::gapi::Color color) {
        return std::any_of(
            backend.texts.begin(),
            backend.texts.end(),
            [=](const TextCall& call) {
                return call.text == text &&
                       call.draw.x == x &&
                       call.draw.y == y &&
                       call.draw.color.red == color.red &&
                       call.draw.color.green == color.green &&
                       call.draw.color.blue == color.blue &&
                       call.draw.color.alpha == color.alpha;
            });
    };
    constexpr osf::gapi::Color labelColor{
        224, 192, 128, 255};
    constexpr osf::gapi::Color valueColor{
        224, 224, 224, 255};
    return check(
        hasText("Level.", 71, 200, labelColor) &&
            hasText("       1", 71, 200, valueColor) &&
            hasText("Job.", 136, 200, labelColor) &&
            hasText("     Mercenary", 136, 200, valueColor) &&
            hasText("Sex.", 232, 200, labelColor) &&
            hasText("     Female", 232, 200, valueColor) &&
            hasText("Name.", 71, 220, labelColor) &&
            hasText("      asof", 71, 220, valueColor) &&
            hasText("HP.", 71, 240, labelColor) &&
            hasText("    260", 71, 240, valueColor) &&
            hasText("MP.", 132, 240, labelColor) &&
            hasText("    160", 132, 240, valueColor) &&
            hasText("EXP.", 192, 240, labelColor) &&
            hasText("     18", 192, 240, valueColor) &&
            hasText(
                "No Data",
                375,
                200,
                {112, 112, 112, 255}),
        "The saved-character row fields, colors, or retail positions "
        "differ.");
}

bool testGameplayOptionsDrawing() {
    osf::GameplayOptionsMenu menu;
    osf::GameConfig config;
    menu.update({true, false, false, 0, 0}, config);

    osf::gapi::NjpImage status;
    osf::gapi::NjpImage font;
    RecordingBackend backend;
    osf::renderGameplayOptions(
        backend, status, font, menu, config);
    const bool has_screen_mode =
        std::any_of(
            backend.texts.begin(),
            backend.texts.end(),
            [](const TextCall& call) {
                return call.text ==
                    "Screen Mode at Start";
            });
    const bool has_first_live_row =
        std::any_of(
            backend.texts.begin(),
            backend.texts.end(),
            [](const TextCall& call) {
                return call.text ==
                           "Semi-transparent Objects" &&
                       call.draw.x == 184 &&
                       call.draw.y == 102;
            });
    const std::array<std::string, 5> expected_priority{{
        "ENEM", "ITEM", "OBJ.", "PEOP", "COMP",
    }};
    std::array<std::string, 5> priority{};
    for (const TextCall& call : backend.texts) {
        if (call.draw.y != 198 ||
            call.draw.x < 316 ||
            call.draw.x > 436 ||
            (call.draw.x - 316) % 30 != 0) {
            continue;
        }
        priority[static_cast<std::size_t>(
            (call.draw.x - 316) / 30)] = call.text;
    }
    if (!check(
        backend.patterns.size() == 6 &&
            backend.patterns[0].index == 59 &&
            backend.patterns[0].draw.opacity == 500 &&
            backend.patterns[1].index == 58 &&
            backend.patterns[2].index == 120 &&
            backend.patterns[2].draw.x == 246 &&
            backend.patterns[2].draw.y == 223 &&
            backend.patterns[4].index == 68 &&
            backend.patterns[4].draw.x == 446 &&
            !has_screen_mode &&
            has_first_live_row &&
            priority == expected_priority,
        "The gameplay options panel differs from retail layout "
        "or ordering.")) {
        return false;
    }

    menu.update(
        {false, true, true, 300, 302}, config);
    backend = {};
    osf::renderGameplayOptions(
        backend, status, font, menu, config);
    const auto prompt = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text ==
                       "Return to the Title Screen?         " &&
                   call.draw.x == 212 &&
                   call.draw.y == 170;
        });
    const auto yes = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "YES" &&
                   call.draw.x == 336 &&
                   call.draw.y == 202;
        });
    const bool has_settings_text =
        std::any_of(
            backend.texts.begin(),
            backend.texts.end(),
            [](const TextCall& call) {
                return call.text ==
                    "Semi-transparent Objects";
            });
    if (!check(
        backend.patterns.size() == 2 &&
            prompt != backend.texts.end() &&
            yes != backend.texts.end() &&
            !has_settings_text,
        "The Save and Return confirmation differs from retail "
        "layout or retained settings text.")) {
        return false;
    }

    menu.update(
        {false, true, true, 340, 202}, config);
    backend = {};
    osf::renderGameplayOptions(
        backend, status, font, menu, config);
    const auto saving = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "Now saving the data " &&
                   call.draw.x == 260 &&
                   call.draw.y == 170 &&
                   call.draw.color.red == 128 &&
                   call.draw.color.green == 128 &&
                   call.draw.color.blue == 224;
        });
    return check(
        backend.patterns.size() == 2 &&
            saving != backend.texts.end(),
        "The retail saving stage text differs in position or "
        "color.");
}

#if OSF_ENABLE_DEBUG_TOOLS
bool testGameplayDebugDrawing() {
    osf::GameplayDebugMenu menu;
    menu.update({true});
    menu.update({false, false, true, 400, 118});
    menu.update({false, false, true, 400, 134});
    menu.update({false, false, true, 400, 150});
    menu.update({false, false, true, 400, 166});
    menu.update({false, false, true, 400, 182});

    osf::gapi::NjpImage status;
    osf::gapi::NjpImage font;
    RecordingBackend backend;
    osf::renderGameplayDebugMenu(
        backend, status, font, menu);
    const auto title = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "DEBUG MENU" &&
                   call.draw.x == 290 &&
                   call.draw.y == 86;
        });
    const auto spells = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "Profiling" &&
                   call.draw.x == 184 &&
                   call.draw.y == 134;
        });
    const auto infinite_mana = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "Infinite MP" &&
                   call.draw.x == 184 &&
                   call.draw.y == 182;
        });
    if (!check(
            backend.patterns.size() == 2 &&
                backend.patterns[0].index == 59 &&
                backend.patterns[0].draw.opacity == 500 &&
                backend.patterns[1].index == 58 &&
                title != backend.texts.end() &&
                spells != backend.texts.end() &&
                infinite_mana != backend.texts.end(),
            "The F12 debug menu does not use the gameplay menu frame or "
            "its declared rows.")) {
        return false;
    }

    backend = {};
    osf::renderGameplayDebugFps(
        backend, font, 60);
    const auto fps = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "FPS 60" &&
                   call.draw.x == 600 &&
                   call.draw.y == 4 &&
                   call.draw.color.red == 255;
        });
    if (!check(
            backend.texts.size() == 2 &&
            fps != backend.texts.end(),
        "The debug FPS counter is not anchored to the top-right with a "
        "shadowed readable draw.")) {
        return false;
    }

    backend = {};
    osf::debug::ProfilingMetrics metrics;
    metrics.ram_bytes = 32ULL * 1024ULL * 1024ULL;
    metrics.video_memory_bytes = 4ULL * 1024ULL * 1024ULL;
    metrics.average_framebuffer_fill_ms = 2.5;
    metrics.average_present_ms = 1.25;
    osf::renderGameplayProfiling(
        backend, font, metrics, true);
    const auto ram = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "RAM 32.00 MiB" &&
                   call.draw.y == 16;
        });
    const auto vram = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "VRAM 4.00 MiB" &&
                   call.draw.y == 28;
        });
    const auto fill = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "FILL 2.50 ms" &&
                   call.draw.y == 40;
        });
    const auto present = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "PRESENT 1.25 ms" &&
                   call.draw.y == 52;
        });
    return check(
        backend.texts.size() == 8 &&
            ram != backend.texts.end() &&
            vram != backend.texts.end() &&
            fill != backend.texts.end() &&
            present != backend.texts.end(),
        "The profiling metrics are not stacked below FPS with stable "
        "units and right alignment.");
}
#endif

bool testGameplayHelpDrawing() {
    osf::gapi::NjpImage status;
    osf::gapi::NjpImage font;
    osf::WorldScene world;
    RecordingBackend backend;
    osf::renderGameplayHelp(
        backend, status, font, world, 17, true, 24);

    const auto heading = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text ==
                       "SHADOW FLARE  \" MOUSE ACTION HELP \"" &&
                   call.draw.x == 42 &&
                   call.draw.y == 48 &&
                   call.draw.color.red == 224 &&
                   call.draw.color.green == 224 &&
                   call.draw.color.blue == 64;
        });
    const auto mouse_action = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text ==
                       "Companions's Attack" &&
                   call.draw.x == 310 &&
                   call.draw.y == 148 &&
                   call.draw.color.red == 139;
        });
    const auto escape_action = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text ==
                       "Open the Settings Menu" &&
                   call.draw.x == 406 &&
                   call.draw.y == 358 &&
                   call.draw.color.red == 192;
        });
    return check(
        backend.patterns.size() == 3 &&
            backend.patterns[0].index == 10 &&
            backend.patterns[1].index == 66 &&
            backend.patterns[1].draw.x == 64 &&
            backend.patterns[1].draw.y == 70 &&
            backend.patterns[2].index == 28 &&
            backend.patterns[2].draw.x == 301 &&
            backend.patterns[2].draw.y == 393 &&
            backend.rectangles.size() == 4 &&
            backend.rectangles[0].x == 63 &&
            backend.rectangles[0].y == 69 &&
            backend.rectangles[0].width == 232 &&
            backend.rectangles[0].opacity == 500 &&
            heading != backend.texts.end() &&
            mouse_action != backend.texts.end() &&
            escape_action != backend.texts.end(),
        "The gameplay help frame, preview, or retail text "
        "layout differs.");
}

}  // namespace

int main() {
#if OSF_ENABLE_DEBUG_TOOLS
    const bool debug_tests_passed = testGameplayDebugDrawing();
#else
    constexpr bool debug_tests_passed = true;
#endif
    if (!testRetailRandom() ||
        !testSaveNameSearch() ||
        !testRetailSaveCatalogFields() ||
        !testTitleLifecycle() ||
        !testTitleLoadFailure() ||
        !testTitleFrames() ||
        !testTitleSmokeSchedule() ||
        !testCharacterSelectLifecycle() ||
        !testCharacterSelectFrames() ||
        !testSavedGameSelectionFrames() ||
        !testSavedGameDeleteDialog() ||
        !testNewCharacterCreationAndModeScreens() ||
        !testNewCharacterRetailDrawing() ||
        !testGameplayOptionsDrawing() ||
        !debug_tests_passed ||
        !testGameplayHelpDrawing()) {
        return 1;
    }
    return 0;
}
