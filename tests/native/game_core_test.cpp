#include "core/command_line.hpp"
#include "core/game_config.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "states/game_state.hpp"
#include "states/gameplay_state.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using osf::GameConfig;

std::string encode(const std::array<std::int32_t, 16>& values) {
    std::string bytes;
    bytes.reserve(osf::kGameConfigByteSize);
    for (const std::int32_t value : values) {
        const auto raw = static_cast<std::uint32_t>(value);
        bytes.push_back(static_cast<char>(raw & 0xffU));
        bytes.push_back(static_cast<char>((raw >> 8U) & 0xffU));
        bytes.push_back(static_cast<char>((raw >> 16U) & 0xffU));
        bytes.push_back(static_cast<char>((raw >> 24U) & 0xffU));
    }
    return bytes;
}

void appendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value) {
    const std::uint32_t raw = static_cast<std::uint32_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(raw));
    bytes.push_back(static_cast<std::uint8_t>(raw >> 8u));
    bytes.push_back(static_cast<std::uint8_t>(raw >> 16u));
    bytes.push_back(static_cast<std::uint8_t>(raw >> 24u));
}

void appendI16(
    std::vector<std::uint8_t>& bytes,
    std::int16_t value) {
    const std::uint16_t raw = static_cast<std::uint16_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(raw));
    bytes.push_back(static_cast<std::uint8_t>(raw >> 8u));
}

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool testRetailDefaultsAndFixture() {
    GameConfig config;
    if (!check(
            !config.windowed_at_start &&
                config.semi_transparent_shadow &&
                config.semi_transparent_objects &&
                config.display_darkness &&
                config.unknown_48d528 &&
                config.unknown_48d540 &&
                config.save_image_at_game_end &&
                config.click_range == 2 &&
                config.click_range_enabled &&
                config.click_priority ==
                    std::array<std::int32_t, 5>{{4, 2, 3, 1, 0}} &&
                config.effect_volume == 0 &&
                config.bgm_volume == 0,
            "Retail constructor defaults do not match 0x00401000.")) {
        return false;
    }

    const std::array<std::int32_t, 16> shippedValues{{
        1, 1, 1, 1, 1, 1, 1, 2, 1, 4, 2, 3, 1, 0, 0, 0,
    }};
    std::istringstream input(encode(shippedValues), std::ios::binary);
    if (!check(
            osf::loadGameConfig(input, config),
            "The shipped SFlare.Cfg values were rejected.")) {
        return false;
    }
    return check(
        config.windowed_at_start &&
            config.click_priority ==
                std::array<std::int32_t, 5>{{4, 2, 3, 1, 0}},
        "The shipped SFlare.Cfg values were decoded incorrectly.");
}

bool testConfigValidationAndWriting() {
    std::array<std::int32_t, 16> values{{
        -9, -5, 0, 2, 1, 0, 9, 8, -2, 0, 1, 2, 3, 4, -12000, 100,
    }};
    GameConfig config;
    config.unknown_48d540 = false;
    std::istringstream input(encode(values), std::ios::binary);
    if (!check(
            osf::loadGameConfig(input, config),
            "Validatable config values unexpectedly failed to load.")) {
        return false;
    }
    if (!check(
            config.windowed_at_start &&
                config.semi_transparent_shadow &&
                !config.semi_transparent_objects &&
                config.display_darkness &&
                config.unknown_48d540 &&
                config.save_image_at_game_end &&
                config.click_range == 2 &&
                config.click_range_enabled &&
                config.effect_volume == -10000 &&
                config.bgm_volume == 0,
            "Retail config normalization differs from 0x00401eb0.")) {
        return false;
    }

    std::ostringstream output(std::ios::binary);
    if (!check(
            osf::saveGameConfig(output, config),
            "Could not write a game config.")) {
        return false;
    }
    return check(
        output.str().size() == osf::kGameConfigByteSize,
        "SFlare.Cfg output is not exactly 64 bytes.");
}

bool testConfigFailureSideEffects() {
    GameConfig truncated;
    truncated.windowed_at_start = false;
    const std::string onlyScreen = encode(
        std::array<std::int32_t, 16>{{
            7, 1, 1, 1, 1, 1, 1, 2, 1, 4, 2, 3, 1, 0, 0, 0,
        }}).substr(0, 4);
    std::istringstream shortInput(onlyScreen, std::ios::binary);
    if (!check(
            !osf::loadGameConfig(shortInput, truncated) &&
                truncated.windowed_at_start,
            "A short config did not retain the retail partial mutation.")) {
        return false;
    }

    GameConfig invalidPriority;
    const auto originalPriority = invalidPriority.click_priority;
    std::array<std::int32_t, 16> values{{
        1, 1, 1, 1, 1, 1, 1, 2, 1, 0, 0, 2, 3, 4, -10, -20,
    }};
    std::istringstream invalidInput(encode(values), std::ios::binary);
    return check(
        !osf::loadGameConfig(invalidInput, invalidPriority) &&
            invalidPriority.click_priority == originalPriority,
        "An invalid click-priority permutation was accepted or copied.");
}

bool testCommandLine() {
    GameConfig config;
    config.windowed_at_start = false;
    osf::applyRetailCommandLine("/W anything /f /w", config);
    if (!check(
            config.windowed_at_start,
            "Later /w and case-insensitive parsing do not match retail.")) {
        return false;
    }

    config.windowed_at_start = false;
    std::string shiftJis;
    shiftJis.push_back(static_cast<char>(0x81));
    shiftJis += "/w";
    osf::applyRetailCommandLine(shiftJis, config);
    if (!check(
            !config.windowed_at_start,
            "A Shift-JIS trail byte was incorrectly parsed as a switch.")) {
        return false;
    }

    constexpr char withEmbeddedNul[] = "ignored\0/w";
    osf::applyRetailCommandLine(
        std::string_view(withEmbeddedNul, sizeof(withEmbeddedNul) - 1),
        config);
    return check(
        !config.windowed_at_start,
        "Command-line parsing continued beyond the first NUL.");
}

bool testStateDispatcher() {
    std::vector<std::string> calls;
    osf::GameStateDispatcherCallbacks callbacks;
    callbacks.wait_until_renderer_idle =
        [&calls] { calls.emplace_back("wait"); };
    callbacks.title.enter =
        [&calls](std::int32_t value) {
            calls.emplace_back("title enter " + std::to_string(value));
        };
    callbacks.title.leave =
        [&calls] { calls.emplace_back("title leave"); };
    callbacks.character_select.enter =
        [&calls](std::int32_t value) {
            calls.emplace_back(
                "character select enter " + std::to_string(value));
        };
    callbacks.character_select.leave =
        [&calls] { calls.emplace_back("character select leave"); };
    callbacks.gameplay.enter =
        [&calls](std::int32_t value) {
            calls.emplace_back("gameplay enter " + std::to_string(value));
        };
    callbacks.gameplay.leave =
        [&calls] { calls.emplace_back("gameplay leave"); };

    osf::GameStateDispatcher dispatcher(std::move(callbacks));
    if (!check(
            dispatcher.currentRetailState() == -1,
            "The retail state dispatcher did not start at -1.")) {
        return false;
    }
    dispatcher.transition(osf::GameState::title, 99);
    dispatcher.transition(osf::GameState::character_select, 77);
    dispatcher.transition(osf::GameState::character_select, 8);
    dispatcher.transition(9, 123);
    dispatcher.transition(osf::GameState::gameplay, 42);

    const std::vector<std::string> expected{
        "wait",
        "title enter 0",
        "wait",
        "title leave",
        "character select enter 77",
        "wait",
        "character select leave",
        "character select enter 8",
        "wait",
        "character select leave",
        "wait",
        "gameplay enter 0",
    };
    return check(
        calls == expected &&
            dispatcher.currentState() ==
                osf::GameState::gameplay,
        "State transition call order differs from 0x004023d0.");
}

bool testGroundMapDecode() {
    std::vector<std::uint8_t> bytes;
    const char header[16] = "RPGSCRN_GNDv000";
    bytes.insert(bytes.end(), header, header + sizeof(header));
    appendI32(bytes, 2);
    appendI32(bytes, 1);
    appendI32(bytes, 64);
    appendI32(bytes, 48);
    appendI32(bytes, 160);
    appendI32(bytes, 160);
    bytes.push_back(0);
    appendI16(bytes, 1);
    appendI16(bytes, 2);
    appendI16(bytes, 0);
    appendI16(bytes, 3);
    appendI16(bytes, 4);
    appendI16(bytes, 5);
    bytes.push_back(0);
    for (std::int32_t index = 0; index < 36; ++index) {
        appendI16(bytes, index == 25 ? 1 : 0);
    }

    osf::GroundMap map;
    if (!check(
            map.decode(bytes) &&
                map.width() == 2 &&
                map.height() == 1 &&
                map.chipWidth() == 64 &&
                map.chipHeight() == 48 &&
                map.baseMagnificationX() == 160 &&
                map.baseMagnificationY() == 160 &&
                map.judgeWidth() == 6 &&
                map.judgeHeight() == 6 &&
                map.judgeOffsetX() == -1 &&
                map.judgeOffsetY() == -4,
            "A valid retail GND fixture was rejected.")) {
        return false;
    }
    const osf::GroundCell* first = map.cell(0, 0);
    const osf::GroundCell* second = map.cell(1, 0);
    return check(
        first && second &&
            first->status == 1 &&
            first->pattern_set == 0 &&
            first->pattern == 4 &&
            second->status == 2 &&
            second->pattern_set == 3 &&
            second->pattern == 5 &&
            map.judge(0, 0) &&
            *map.judge(0, 0) == 1 &&
            map.cell(2, 0) == nullptr,
        "The retail GND cell or judgement planes were decoded incorrectly.");
}

bool testObjectMapDecode() {
    std::vector<std::uint8_t> bytes;
    const char header[] = "RPGSCRN_OBJv001\x1a";
    bytes.insert(bytes.end(), header, header + 16);
    appendI32(bytes, 1);
    appendI32(bytes, 100);
    appendI32(bytes, 200);
    appendI16(bytes, 2);
    appendI16(bytes, 3);
    appendI16(bytes, -1);
    appendI16(bytes, 750);
    appendI16(bytes, 8);
    appendI16(bytes, 10);
    appendI16(bytes, 900);
    appendI16(bytes, 800);
    appendI16(bytes, 700);
    appendI32(bytes, -10);
    appendI32(bytes, -20);
    appendI32(bytes, 30);
    appendI32(bytes, 40);

    osf::ObjectMap map;
    if (!check(
            map.decode(bytes) &&
                map.version() == 1 &&
                map.objects().size() == 1,
            "A valid retail OBL fixture was rejected.")) {
        return false;
    }
    const osf::MapObject& object = map.objects().front();
    if (!check(
            object.world_x == 100 &&
                object.world_y == 200 &&
                object.pattern_set == 2 &&
                object.pattern == 3 &&
                object.palette == -1 &&
                object.opacity == 750 &&
                object.status == 8 &&
                object.height == 10 &&
                object.red_strength == 900 &&
                object.green_strength == 800 &&
                object.blue_strength == 700 &&
                object.judgement.left == -10 &&
                object.judgement.top == -20 &&
                object.judgement.right == 30 &&
                object.judgement.bottom == 40,
            "The retail OBL record fields were decoded incorrectly.")) {
        return false;
    }

    bytes.pop_back();
    return check(
        !map.decode(bytes),
        "A truncated retail OBL record was accepted.");
}

bool testGameplayLoadingTransition() {
    std::int32_t prepares = 0;
    std::int32_t releases = 0;
    std::int32_t musicStarts = 0;
    std::int32_t musicStops = 0;
    std::int32_t movementCommands = 0;
    std::int32_t movementCancels = 0;
    std::int32_t interactionCommands = 0;
    std::int32_t pointerUpdates = 0;
    std::int32_t conversationAdvances = 0;
    std::int32_t conversationChoices = 0;
    std::int32_t worldUpdates = 0;
    std::int32_t runToggles = 0;
    std::int32_t movementX = 0;
    std::int32_t movementY = 0;
    bool conversationActive = false;
    bool conversationSelectionRequired = false;
    osf::GameplayStateHooks hooks;
    hooks.prepare_world = [&prepares] {
            ++prepares;
            return true;
        };
    hooks.release_world = [&releases] { ++releases; };
    hooks.start_world_music =
        [&musicStarts] { ++musicStarts; };
    hooks.stop_world_music =
        [&musicStops] { ++musicStops; };
    hooks.command_player_movement =
        [&](std::int32_t x, std::int32_t y) {
            ++movementCommands;
            movementX = x;
            movementY = y;
        };
    hooks.cancel_player_movement =
        [&movementCancels] {
            ++movementCancels;
        };
    hooks.update_pointer_hover =
        [&](std::int32_t, std::int32_t) {
            ++pointerUpdates;
        };
    hooks.command_world_interaction =
        [&](std::int32_t x, std::int32_t y) {
            ++interactionCommands;
            conversationActive = x == 300 && y == 220;
            return conversationActive;
        };
    hooks.conversation_active =
        [&conversationActive] {
            return conversationActive;
        };
    hooks.conversation_requires_selection =
        [&conversationSelectionRequired] {
            return conversationSelectionRequired;
        };
    hooks.choose_conversation_option =
        [&](std::int32_t x, std::int32_t y) {
            if (x != 330 || y != 240) {
                return false;
            }
            ++conversationChoices;
            conversationActive = false;
            return true;
        };
    hooks.advance_conversation =
        [&] {
            ++conversationAdvances;
            conversationActive = false;
        };
    hooks.toggle_player_run =
        [&runToggles] { ++runToggles; };
    hooks.update_world =
        [&worldUpdates] { ++worldUpdates; };
    osf::GameplayState state(std::move(hooks));
    state.enter();
    osf::GameplayFrameResult frame = state.update();
    if (!check(
            prepares == 1 &&
                musicStarts == 1 &&
                frame.phase == osf::GameplayPhase::loading &&
                frame.loading_counter == 1 &&
                frame.ready_to_continue,
            "Gameplay did not expose the loaded world confirmation.")) {
        return false;
    }
    frame = state.update({false, true, 100, 100});
    if (!check(
            frame.phase == osf::GameplayPhase::loading,
            "Gameplay accepted a click outside the retail arrow region.")) {
        return false;
    }
    frame = state.update({false, true, 600, 460});
    state.update({false, true, 200, 210});
    state.update({false, true, -1, 210});
    state.update({false, false, 0, 0, false, true});
    state.update({false, true, 300, 220});
    state.update({false, false, 310, 220, true, true});
    state.update({true});
    state.update({false, true, 220, 230, true});
    state.update({false, false, 230, 240, true});
    state.update({false, false, 230, 240, false});
    conversationActive = true;
    conversationSelectionRequired = true;
    state.update({false, true, 330, 240, true});
    state.update({false, false, 330, 240, true});
    state.update({false, false, 330, 240, false});
    state.update({false, true, 240, 250});
    state.leave();
    return check(
            frame.phase == osf::GameplayPhase::world &&
            frame.world_ready &&
            releases == 1 &&
            musicStarts == 1 &&
            musicStops == 1 &&
            movementCommands == 4 &&
            movementCancels == 0 &&
            movementX == 240 &&
            movementY == 250 &&
            interactionCommands == 4 &&
            pointerUpdates == 13 &&
            conversationAdvances == 1 &&
            conversationChoices == 1 &&
            worldUpdates == 13 &&
            runToggles == 1,
        "Gameplay did not hand loading off or lock conversation input cleanly.");
}

bool testGameplayClickAndHoldMovement() {
    std::int32_t movement_commands = 0;
    std::int32_t movement_cancels = 0;
    osf::GameplayStateHooks hooks;
    hooks.prepare_world = [] { return true; };
    hooks.command_player_movement =
        [&](std::int32_t, std::int32_t) {
            ++movement_commands;
        };
    hooks.cancel_player_movement =
        [&] { ++movement_cancels; };

    osf::GameplayState state(std::move(hooks));
    state.enter();
    state.update();
    state.update({false, true, 600, 460});

    state.update({false, true, 200, 200, true});
    for (std::int32_t update = 0; update < 4; ++update) {
        state.update({false, false, 200, 200, true});
    }
    state.update({false, false, 200, 200, false});
    if (!check(
            movement_commands == 5 &&
                movement_cancels == 0,
            "A normal-duration click was mistaken for held movement.")) {
        return false;
    }

    state.update({false, true, 300, 200, true});
    for (std::int32_t update = 0; update < 9; ++update) {
        state.update({false, false, 300, 200, true});
    }
    state.update({false, false, 300, 200, false});
    return check(
        movement_commands == 15 &&
            movement_cancels == 1,
        "A retail-length held command did not stop on release.");
}

}  // namespace

int main() {
    if (!testRetailDefaultsAndFixture() ||
        !testConfigValidationAndWriting() ||
        !testConfigFailureSideEffects() ||
        !testCommandLine() ||
        !testStateDispatcher() ||
        !testGroundMapDecode() ||
        !testObjectMapDecode() ||
        !testGameplayLoadingTransition() ||
        !testGameplayClickAndHoldMovement()) {
        return 1;
    }
    return 0;
}
