#include "core/game_config.hpp"
#include "core/retail_random.hpp"
#include "runtime/audio_system.hpp"
#include "runtime/gameplay_ui_controller.hpp"
#include "runtime/input_adapter.hpp"
#include "states/game_state.hpp"
#include "states/gameplay_state.hpp"
#include "world/player_data.hpp"
#include "world/retail_save_preview.hpp"
#include "world/world_scene.hpp"

#include "lwl.h"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

struct Fixture {
    osf::runtime::GameplayUiController controller;
    osf::runtime::InputAdapter input{640, 480};
    osf::runtime::AudioSystem audio;
    osf::GameConfig config;
    osf::RetailRandom random;
    osf::RetailSavePreview preview;
    osf::GameStateDispatcher game_state;
    osf::GameplayFrameResult frame;
    bool config_dirty = false;
    bool running = true;
    std::int32_t shadow_opacity = 500;

    Fixture() {
        config.save_image_at_game_end = false;
        frame.phase = osf::GameplayPhase::world;
        game_state.transition(osf::GameState::gameplay);
    }

    bool update(
        osf::WorldScene& world,
        osf::PlayerLoadRequest& player) {
        return controller.update(
            frame,
            input,
            world,
            audio,
            config,
            config_dirty,
            random,
            player,
            preview,
            game_state,
            running,
            shadow_opacity);
    }

    void releaseKey(const char* key) {
        LwlEvent event{};
        event.type = LWL_EVENT_KEY_UP;
        std::strncpy(event.key, key, sizeof(event.key) - 1u);
        input.handleEvent(
            nullptr,
            event,
            osf::GameState::gameplay);
    }

    void pressKey(
        const char* key,
        osf::WorldScene& world,
        osf::PlayerLoadRequest& player) {
        LwlEvent event{};
        event.type = LWL_EVENT_KEY_DOWN;
        std::strncpy(event.key, key, sizeof(event.key) - 1u);
        input.handleEvent(
            nullptr,
            event,
            osf::GameState::gameplay);
        update(world, player);
        input.clearTransientInput();
        releaseKey(key);
    }

    void click(
        std::int32_t x,
        std::int32_t y,
        osf::WorldScene& world,
        osf::PlayerLoadRequest& player) {
        input.menu().pointer_x = x;
        input.menu().pointer_y = y;
        input.menu().pointer_primary_pressed = true;
        update(world, player);
        input.clearTransientInput();
    }
};

bool openSaveConfirmation(
    Fixture& fixture,
    osf::WorldScene& world,
    osf::PlayerLoadRequest& player,
    bool exit_game) {
    fixture.pressKey("escape", world, player);
    fixture.click(
        300,
        exit_game ? 322 : 306,
        world,
        player);
    return check(
        !fixture.controller.inventory().active() &&
            fixture.controller.options().page() ==
                (exit_game
                     ? osf::GameplayOptionsPage::
                           exit_game_confirmation
                     : osf::GameplayOptionsPage::
                           return_to_title_confirmation),
        "The save confirmation did not open from Settings.");
}

bool testEscapeClosesPanelsBeforeSettings() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "00000000")) {
        return true;
    }

    osf::PlayerLoadRequest player;
    player.name = "Escape Panels";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                data_root, player, &error),
            "The Escape-panel fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }

    Fixture fixture;
    fixture.pressKey("n", world, player);
    fixture.pressKey("i", world, player);
    if (!check(
            fixture.controller.map().active() &&
                fixture.controller.inventory().active() &&
                !fixture.controller.options().active(),
            "The independent Map and Inventory panels did not open.")) {
        return false;
    }

    fixture.pressKey("escape", world, player);
    if (!check(
            !fixture.controller.map().active() &&
                !fixture.controller.inventory().active() &&
                !fixture.controller.magic().active() &&
                !fixture.controller.status().active() &&
                !fixture.controller.missionList().active() &&
                !fixture.controller.transport().active() &&
                !fixture.controller.options().active(),
            "Escape opened Settings instead of closing the visible "
            "gameplay panels.")) {
        return false;
    }

    fixture.pressKey("escape", world, player);
    if (!check(
            fixture.controller.options().active(),
            "Escape did not open Settings after all gameplay panels "
            "were closed.")) {
        return false;
    }
    fixture.pressKey("escape", world, player);
    return check(
        !fixture.controller.options().active(),
        "Escape did not close Settings once it was the active menu.");
#else
    return true;
#endif
}

bool testSaveTransitionsOwnModalInput() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "00000000")) {
        return true;
    }

    const std::filesystem::path save_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_gameplay_ui_save_test";
    std::error_code filesystem_error;
    std::filesystem::remove_all(save_root, filesystem_error);

    osf::PlayerLoadRequest player;
    player.name = "Modal Save";
    player.save_path = save_root / "Save" / "0000.Ssv";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                data_root, player, &error),
            "The save-transition fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }

    Fixture fixture;
    if (!openSaveConfirmation(
            fixture, world, player, false)) {
        return false;
    }
    fixture.click(340, 206, world, player);
    fixture.update(world, player);
    if (!check(
            std::filesystem::is_regular_file(player.save_path) &&
                fixture.game_state.currentState() ==
                    osf::GameState::title,
            "Save and Return did not save and enter the title state.")) {
        return false;
    }

    fixture.controller.reset();
    fixture.game_state.transition(osf::GameState::gameplay);
    fixture.running = true;
    player.save_path = save_root / "Save" / "0001.Ssv";
    if (!openSaveConfirmation(
            fixture, world, player, true)) {
        return false;
    }
    fixture.click(340, 206, world, player);
    fixture.update(world, player);
    const bool passed = check(
        std::filesystem::is_regular_file(player.save_path) &&
            !fixture.running,
        "Save and Exit did not save and stop the application.");
    std::filesystem::remove_all(save_root, filesystem_error);
    return passed;
#else
    return true;
#endif
}

bool testIncreasedPowerKeyEdge() {
    osf::runtime::InputAdapter input{640, 480};
    LwlEvent event{};
    event.type = LWL_EVENT_KEY_DOWN;
    std::strncpy(event.key, "p", sizeof(event.key) - 1u);
    input.handleEvent(
        nullptr, event, osf::GameState::gameplay);
    if (!check(
            input.increasedPowerPressed(),
            "P did not publish the Increased Power input edge.")) {
        return false;
    }
    input.clearTransientInput();
    input.handleEvent(
        nullptr, event, osf::GameState::gameplay);
    if (!check(
            !input.increasedPowerPressed(),
            "A held P key repeated the Increased Power edge.")) {
        return false;
    }
    event.type = LWL_EVENT_KEY_UP;
    input.handleEvent(
        nullptr, event, osf::GameState::gameplay);
    event.type = LWL_EVENT_KEY_DOWN;
    input.handleEvent(
        nullptr, event, osf::GameState::gameplay);
    return check(
        input.increasedPowerPressed(),
        "P did not re-arm after its key-up event.");
}

}  // namespace

int main() {
    return testEscapeClosesPanelsBeforeSettings() &&
                   testSaveTransitionsOwnModalInput() &&
                   testIncreasedPowerKeyEdge()
               ? 0
               : 1;
}
