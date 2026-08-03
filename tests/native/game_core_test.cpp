#include "core/command_line.hpp"
#include "core/game_config.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "states/game_state.hpp"
#include "states/gameplay_debug_menu.hpp"
#include "states/gameplay_options_menu.hpp"
#include "states/gameplay_state.hpp"
#include "ui/companion_hud_input.hpp"
#include "ui/gameplay_hud_input.hpp"
#include "ui/pointer_input_guard.hpp"

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
                config.attack_while_moving &&
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
    config.attack_while_moving = false;
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
                config.attack_while_moving &&
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

bool testDisplayObjectOrdering() {
    std::vector<osf::DisplayOrderEntry> entries{
        {
            0,
            {0, 0},
            {0, 0, 1000, 100},
            0,
        },
        {
            1,
            {580, -20},
            {-80, -80, 79, 79},
            0,
        },
        {
            2,
            {},
            {},
            0x20,
        },
    };
    osf::sortDisplayObjects(entries);
    if (!check(
            entries.size() == 3 &&
                entries[0].source_index == 2 &&
                entries[1].source_index == 1 &&
                entries[2].source_index == 0,
            "Full judgement rectangles did not place an actor behind "
            "large scenery.")) {
        return false;
    }

    std::vector<osf::DisplayOrderEntry> touching{
        {0, {}, {0, 0, 100, 100}, 0},
        {1, {}, {100, 0, 200, 100}, 0},
    };
    osf::sortDisplayObjects(touching);
    return check(
        touching[0].source_index == 0 &&
            touching[1].source_index == 1 &&
            osf::displayClassForStatus(0x100) == 1 &&
            osf::displayClassForStatus(0x80) == 2 &&
            osf::displayClassForStatus(0x20) == 3,
        "Display ordering changed the retail strict-edge or status rules.");
}

bool testGameplayLoadingTransition() {
    std::int32_t interfacePrepares = 0;
    std::int32_t interfaceReleases = 0;
    std::int32_t loadingArtworkReleases = 0;
    std::int32_t prepares = 0;
    std::int32_t releases = 0;
    std::int32_t musicStarts = 0;
    std::int32_t musicStops = 0;
    std::int32_t movementCommands = 0;
    std::int32_t movementCancels = 0;
    std::int32_t interactionCommands = 0;
    std::int32_t magicCommands = 0;
    std::int32_t pointerUpdates = 0;
    std::int32_t pointerClears = 0;
    std::int32_t conversationAdvances = 0;
    std::int32_t conversationChoices = 0;
    std::int32_t worldUpdates = 0;
    std::int32_t runToggles = 0;
    std::int32_t companionToggles = 0;
    std::int32_t movementX = 0;
    std::int32_t movementY = 0;
    bool conversationActive = false;
    bool conversationSelectionRequired = false;
    osf::GameplayStateHooks hooks;
    hooks.prepare_interface = [&interfacePrepares] {
            ++interfacePrepares;
            return true;
        };
    hooks.release_interface =
        [&interfaceReleases] {
            ++interfaceReleases;
        };
    hooks.release_loading_artwork =
        [&loadingArtworkReleases] {
            ++loadingArtworkReleases;
        };
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
    hooks.clear_pointer_hover =
        [&pointerClears] {
            ++pointerClears;
        };
    hooks.command_world_interaction =
        [&](std::int32_t x, std::int32_t y) {
            ++interactionCommands;
            conversationActive = x == 300 && y == 220;
            return conversationActive;
        };
    hooks.command_player_magic =
        [&](std::int32_t x, std::int32_t y) {
            ++magicCommands;
            return x == 250 && y == 200;
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
    hooks.toggle_companion_activity =
        [&companionToggles] { ++companionToggles; };
    hooks.update_world =
        [&worldUpdates] { ++worldUpdates; };
    osf::GameplayState state(std::move(hooks));
    state.enter();
    osf::GameplayFrameResult frame = state.update();
    if (!check(
            prepares == 1 &&
                interfacePrepares == 1 &&
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
    state.update({false, true, 200, 450});
    osf::GameplayFrameInput magic_cast;
    magic_cast.pointer_x = 250;
    magic_cast.pointer_y = 200;
    magic_cast.pointer_secondary_pressed = true;
    state.update(magic_cast);
    state.update({false, true, 200, 210});
    state.update({false, true, -1, 210});
    osf::GameplayFrameInput mode_toggles;
    mode_toggles.run_toggle_pressed = true;
    mode_toggles.companion_toggle_pressed = true;
    state.update(mode_toggles);
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
            interfaceReleases == 1 &&
            loadingArtworkReleases == 1 &&
            musicStarts == 1 &&
            musicStops == 1 &&
            movementCommands == 4 &&
            movementCancels == 0 &&
            movementX == 240 &&
            movementY == 250 &&
            interactionCommands == 4 &&
            magicCommands == 1 &&
            pointerUpdates == 13 &&
            pointerClears == 2 &&
            conversationAdvances == 1 &&
            conversationChoices == 1 &&
            worldUpdates == 15 &&
            runToggles == 1 &&
            companionToggles == 1,
        "Gameplay did not hand loading off or lock conversation input cleanly.");
}

bool testCompanionHudInput() {
    if (!check(
        osf::companionHudToggleAtPointer(true, 0, 393) &&
            osf::companionHudToggleAtPointer(true, 111, 408) &&
            !osf::companionHudToggleAtPointer(false, 50, 400) &&
            !osf::companionHudToggleAtPointer(true, -1, 400) &&
            !osf::companionHudToggleAtPointer(true, 112, 400) &&
            !osf::companionHudToggleAtPointer(true, 50, 392) &&
            !osf::companionHudToggleAtPointer(true, 50, 409),
        "The companion HUD toggle rectangle differs from "
        "FUN_00445bd0.")) {
        return false;
    }

    std::int32_t toggles = 0;
    std::int32_t movements = 0;
    std::int32_t interactions = 0;
    osf::GameplayStateHooks hooks;
    hooks.prepare_world = [] { return true; };
    hooks.toggle_companion_activity = [&] { ++toggles; };
    hooks.command_player_movement =
        [&](std::int32_t, std::int32_t) { ++movements; };
    hooks.command_world_interaction =
        [&](std::int32_t, std::int32_t) {
            ++interactions;
            return false;
        };
    osf::GameplayState state(std::move(hooks));
    state.enter();
    state.update();
    state.update({false, true, 600, 460});
    osf::GameplayFrameInput hud_click;
    hud_click.pointer_primary_pressed = true;
    hud_click.pointer_primary_down = true;
    hud_click.pointer_x = 50;
    hud_click.pointer_y = 395;
    hud_click.companion_hud_pressed = true;
    state.update(hud_click);
    return check(
        toggles == 1 && movements == 0 && interactions == 0,
        "A companion HUD click leaked into world interaction or "
        "movement.");
}

bool testGameplayHudInput() {
    using osf::GameplayHudButton;
    if (!check(
            osf::gameplayHudButtonAtPointer(true, 589, 402) ==
                    GameplayHudButton::options &&
                osf::gameplayHudButtonAtPointer(true, 639, 413) ==
                    GameplayHudButton::options &&
                osf::gameplayHudButtonAtPointer(true, 537, 420) ==
                    GameplayHudButton::status &&
                osf::gameplayHudButtonAtPointer(true, 577, 437) ==
                    GameplayHudButton::status &&
                osf::gameplayHudButtonAtPointer(true, 583, 429) ==
                    GameplayHudButton::inventory &&
                osf::gameplayHudButtonAtPointer(true, 636, 448) ==
                    GameplayHudButton::inventory &&
                osf::gameplayHudButtonAtPointer(false, 600, 405) ==
                    GameplayHudButton::none &&
                osf::gameplayHudButtonAtPointer(true, 588, 402) ==
                    GameplayHudButton::none &&
                osf::gameplayHudButtonAtPointer(true, 578, 430) ==
                    GameplayHudButton::none &&
                osf::gameplayHudButtonAtPointer(true, 637, 440) ==
                    GameplayHudButton::none,
            "The lower-right HUD hitboxes differ from FUN_00445bd0.")) {
        return false;
    }

    osf::PointerInputGuard guard;
    guard.consumeUntilRelease(true);
    if (!check(
            guard.update(true) &&
                guard.update(false) &&
                !guard.update(false),
            "A UI-owned pointer press leaked before its release.")) {
        return false;
    }
    guard.consumeUntilRelease(false);
    return check(
        !guard.update(false),
        "An already released pointer was incorrectly retained by UI.");
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
    osf::GameplayFrameInput covered_panel_click;
    covered_panel_click.pointer_primary_pressed = true;
    covered_panel_click.pointer_x = 200;
    covered_panel_click.pointer_y = 200;
    covered_panel_click.world_view_left = 320;
    state.update(covered_panel_click);
    return check(
        movement_commands == 15 &&
            movement_cancels == 1,
        "Held movement or split-view input clipping diverged.");
}

bool testScenarioVisualInputLock() {
    bool visual_active = true;
    std::int32_t advances = 0;
    std::int32_t updates = 0;
    std::int32_t hover_clears = 0;
    std::int32_t movements = 0;
    std::int32_t interactions = 0;
    std::int32_t magic = 0;
    osf::GameplayStateHooks hooks;
    hooks.prepare_world = [] { return true; };
    hooks.scenario_visual_active = [&] {
        return visual_active;
    };
    hooks.advance_scenario_visual = [&] { ++advances; };
    hooks.update_world = [&] { ++updates; };
    hooks.clear_pointer_hover = [&] { ++hover_clears; };
    hooks.command_player_movement =
        [&](std::int32_t, std::int32_t) { ++movements; };
    hooks.command_world_interaction =
        [&](std::int32_t, std::int32_t) {
            ++interactions;
            return true;
        };
    hooks.command_player_magic =
        [&](std::int32_t, std::int32_t) {
            ++magic;
            return true;
        };

    osf::GameplayState state(std::move(hooks));
    state.enter();
    state.update();
    state.update({false, true, 600, 460});
    osf::GameplayFrameInput secondary;
    secondary.pointer_secondary_pressed = true;
    secondary.pointer_x = 200;
    secondary.pointer_y = 200;
    state.update(secondary);

    osf::GameplayFrameInput confirm;
    confirm.confirm_pressed = true;
    state.update(confirm);

    osf::GameplayFrameInput primary;
    primary.pointer_primary_pressed = true;
    primary.pointer_primary_down = true;
    primary.pointer_x = 200;
    primary.pointer_y = 200;
    state.update(primary);

    osf::GameplayFrameInput cancel;
    cancel.cancel_pressed = true;
    state.update(cancel);
    visual_active = false;
    return check(
        advances == 3 && updates == 4 && hover_clears == 4 &&
            movements == 0 && interactions == 0 && magic == 0,
        "A scenario visual did not accept only retail Left, Enter, and "
        "Escape input while locking the world.");
}

bool testGameplayOptionsMenu() {
    osf::GameConfig config;
    osf::GameplayOptionsMenu menu;
    menu.update({true}, config);
    if (!check(
            menu.active(),
            "Escape did not open the gameplay options menu.")) {
        return false;
    }

    const bool original_window_mode =
        config.windowed_at_start;
    osf::GameplayOptionsResult result =
        menu.update({false, true, true, 430, 86}, config);
    if (!check(
            !result.config_changed &&
                !result.play_click_sound &&
                config.windowed_at_start ==
                    original_window_mode,
            "The intentionally hidden screen-mode row remained "
            "interactive.")) {
        return false;
    }

    result = menu.update(
        {false, true, true, 430, 102}, config);
    if (!check(
            result.config_changed &&
                result.play_click_sound &&
                !config.semi_transparent_objects,
            "The retail object-transparency OFF cell was not "
            "applied.")) {
        return false;
    }
    menu.update({false, true, true, 430, 166}, config);
    menu.update({false, true, true, 440, 182}, config);
    if (!check(
            !config.click_range_enabled &&
                config.click_range == 4,
            "The two retail click-range rows used the wrong hit "
            "boxes.")) {
        return false;
    }

    menu.update({false, true, true, 320, 198}, config);
    if (!check(
            config.click_priority ==
                std::array<std::int32_t, 5>{{
                    0, 3, 4, 2, 1,
                }},
            "Clicking a priority label did not move that class "
            "to the retail end position.")) {
        return false;
    }

    menu.update({false, false, true, 252, 220}, config);
    if (!check(
            config.effect_volume == -10000,
            "The left edge of the retail effect slider was not "
            "mute.")) {
        return false;
    }
    menu.update({false, false, true, 253, 220}, config);
    if (!check(
            config.effect_volume == -2985,
            "The retail effect-volume slider scale differs.")) {
        return false;
    }
    menu.update({false, false, true, 452, 240}, config);
    if (!check(
            config.bgm_volume == 0 &&
                osf::gameplayOptionsVolumeSliderOffset(
                    config.bgm_volume) == 200,
            "The retail BGM slider maximum differs.")) {
        return false;
    }

    result = menu.update(
        {false, true, true, 300, 286}, config);
    if (!check(
            menu.active() &&
                menu.page() ==
                    osf::GameplayOptionsPage::help &&
                result.play_confirm_sound,
            "The HELP row did not open the gameplay help screen.")) {
        return false;
    }
    menu.update({true}, config);
    if (!check(
            !menu.active() &&
                menu.page() ==
                    osf::GameplayOptionsPage::settings,
            "Escape did not close the gameplay help screen.")) {
        return false;
    }
    menu.update({true}, config);
    menu.update(
        {false, true, true, 300, 286}, config);
    menu.update(
        {false, true, true, 500, 100}, config);
    if (!check(
            !menu.active(),
            "A retail help-screen click did not dismiss the page.")) {
        return false;
    }
    menu.update({true}, config);
    menu.update(
        {false, false, false, 0, 0, true}, config);
    menu.update(
        {false, false, false, 0, 0, true}, config);
    if (!check(
            !menu.active(),
            "The retail H shortcut did not toggle the help screen.")) {
        return false;
    }
    menu.update({true}, config);

    result = menu.update(
        {false, true, true, 300, 302}, config);
    if (!check(
            menu.page() ==
                    osf::GameplayOptionsPage::
                        return_to_title_confirmation &&
                result.play_confirm_sound &&
                result.action ==
                    osf::GameplayOptionsAction::none,
            "Save and Return did not open the retail confirmation "
            "state.")) {
        return false;
    }
    result = menu.update(
        {false, true, true, 390, 202}, config);
    if (!check(
            menu.page() ==
                    osf::GameplayOptionsPage::settings &&
                result.play_click_sound,
            "NO did not return from the confirmation state.")) {
        return false;
    }
    result = menu.update(
        {false, true, true, 300, 318}, config);
    result = menu.update(
        {false, true, true, 340, 202}, config);
    if (!check(
            result.play_confirm_sound &&
                result.action ==
                    osf::GameplayOptionsAction::save_and_exit &&
                menu.page() ==
                    osf::GameplayOptionsPage::saving,
            "YES did not dispatch the Save and Exit action.")) {
        return false;
    }

    menu.restoreConfirmation(result.action);
    if (!check(
            menu.page() ==
                osf::GameplayOptionsPage::
                    exit_game_confirmation,
            "A failed save did not restore its confirmation state.")) {
        return false;
    }

    menu.update({true}, config);
    return check(
        !menu.active(),
        "Escape did not close the gameplay options menu.");
}

#if OSF_ENABLE_DEBUG_TOOLS
bool testGameplayDebugMenu() {
    osf::GameplayDebugMenu menu;
    osf::GameplayDebugResult result =
        menu.update({true});
    if (!check(
            menu.active() && result.play_confirm_sound,
            "F12 did not open the gameplay debug menu.")) {
        return false;
    }
    result = menu.update(
        {false, false, true, 400, 118});
    if (!check(
            menu.fpsCounterEnabled() &&
                result.settings_changed &&
                result.play_click_sound,
            "The debug FPS toggle did not use its displayed hit box.")) {
        return false;
    }
    result = menu.update(
        {false, false, true, 400, 134});
    if (!check(
            menu.profilingEnabled() &&
                result.settings_changed &&
                result.play_click_sound,
            "The debug profiling toggle did not use its displayed hit "
            "box.")) {
        return false;
    }
    result = menu.update(
        {false, false, true, 400, 150});
    if (!check(
            menu.allSpellsEnabled() &&
                result.settings_changed &&
                result.play_click_sound,
            "The debug all-spells toggle did not use its displayed hit "
            "box.")) {
        return false;
    }
    result = menu.update(
        {false, false, true, 400, 166});
    if (!check(
            menu.infiniteLifeEnabled() &&
                result.settings_changed &&
                result.play_click_sound,
            "The debug infinite-HP toggle did not use its displayed hit "
            "box.")) {
        return false;
    }
    result = menu.update(
        {false, false, true, 400, 182});
    if (!check(
            menu.infiniteManaEnabled() &&
                result.settings_changed &&
                result.play_click_sound,
            "The debug infinite-MP toggle did not use its displayed hit "
            "box.")) {
        return false;
    }
    result = menu.update(
        {false, false, true, 300, 214});
    if (!check(
            !menu.active() &&
                menu.fpsCounterEnabled() &&
                menu.profilingEnabled() &&
                menu.allSpellsEnabled() &&
                menu.infiniteLifeEnabled() &&
                menu.infiniteManaEnabled() &&
                result.play_confirm_sound,
            "The debug CLOSE row changed settings or failed to close.")) {
        return false;
    }
    menu.update({true});
    result = menu.update({false, true});
    return check(
        !menu.active() &&
            menu.fpsCounterEnabled() &&
            menu.profilingEnabled() &&
            menu.allSpellsEnabled() &&
            menu.infiniteLifeEnabled() &&
            menu.infiniteManaEnabled() &&
            result.play_confirm_sound,
        "Escape did not close the debug menu while retaining its runtime "
        "settings.");
}
#endif

}  // namespace

int main() {
#if OSF_ENABLE_DEBUG_TOOLS
    const bool debug_tests_passed = testGameplayDebugMenu();
#else
    constexpr bool debug_tests_passed = true;
#endif
    if (!testRetailDefaultsAndFixture() ||
        !testConfigValidationAndWriting() ||
        !testConfigFailureSideEffects() ||
        !testCommandLine() ||
        !testStateDispatcher() ||
        !testGroundMapDecode() ||
        !testObjectMapDecode() ||
        !testDisplayObjectOrdering() ||
        !testGameplayLoadingTransition() ||
        !testCompanionHudInput() ||
        !testGameplayHudInput() ||
        !testGameplayClickAndHoldMovement() ||
        !testScenarioVisualInputLock() ||
        !testGameplayOptionsMenu() ||
        !debug_tests_passed) {
        return 1;
    }
    return 0;
}
