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
                data.elapsed_frames == 0 &&
                data.saved_game_exists &&
                data.next_save_path_available &&
                data.next_save_path == "Save\\0002.Ssv" &&
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
    hooks.load_save_catalog =
        [&calls](std::string_view path) {
            calls.emplace_back("load saves " + std::string(path));
        };
    hooks.release_save_catalog =
        [&calls] { calls.emplace_back("release saves"); };
    hooks.reset_new_character_data =
        [&calls] { calls.emplace_back("reset new character"); };
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
    const auto& saved = state.data();
    if (!check(
            saved.active &&
                saved.mode == osf::CharacterSelectMode::saved_game &&
                saved.fade_steps_remaining == 20 &&
                saved.launch_counter == 0 &&
                saved.first_frame == 1 &&
                saved.selected_save == -1 &&
                saved.screen == 0 &&
                saved.input_accepted == 1 &&
                saved.save_catalog_loaded &&
                saved.next_save_path == "Save\\0001.Ssv",
            "Saved-game character-select entry fields differ.")) {
        return false;
    }
    if (!check(
            calls.size() == 6 &&
                calls[0] == "begin" &&
                calls[1] ==
                    "pattern 4 System\\Select\\Pattern\\Select.njp" &&
                calls[2] == "input 58" &&
                calls[3] == "load saves Save\\0001.Ssv" &&
                calls[4] == "cursor -1" &&
                calls[5] == "play 500 loop",
            "Saved-game character-select service order differs.")) {
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
                osf::CharacterSelectMode::new_character &&
            !state.data().save_catalog_loaded &&
            state.data().next_save_path.empty() &&
            calls[calls.size() - 3] == "release saves" &&
            calls[calls.size() - 2] == "reset new character" &&
            calls.back() == "cursor -1",
        "New-character entry did not replace the saved-game catalog.");
}

}  // namespace

int main() {
    if (!testRetailRandom() ||
        !testSaveNameSearch() ||
        !testTitleLifecycle() ||
        !testTitleLoadFailure() ||
        !testCharacterSelectLifecycle()) {
        return 1;
    }
    return 0;
}
