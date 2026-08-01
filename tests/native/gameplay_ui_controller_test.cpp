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

#include <algorithm>
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

bool testHudButtonsOpenRetailPanels() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "00000000")) {
        return true;
    }

    osf::PlayerLoadRequest player;
    player.name = "HUD Buttons";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(data_root, player, &error),
            "The HUD-button fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }

    Fixture fixture;
    fixture.click(557, 428, world, player);
    if (!check(
            fixture.controller.status().active(),
            "The STATUS HUD button did not open the Status panel.")) {
        return false;
    }
    fixture.click(557, 428, world, player);
    fixture.click(600, 438, world, player);
    if (!check(
            !fixture.controller.status().active() &&
                fixture.controller.inventory().active(),
            "The ITEM HUD button did not open the Inventory panel.")) {
        return false;
    }
    fixture.click(610, 407, world, player);
    return check(
        fixture.controller.options().active() &&
            !fixture.controller.inventory().active(),
        "The MENU HUD button did not own input and open Settings.");
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
    if (!check(
            world.placePlayerLandMine() &&
                world.playerMineCount() == 4,
            "The save fixture could not spend one mine before saving.")) {
        return false;
    }
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
    osf::WorldScene restored_world;
    if (!check(
            restored_world.loadInitialScenario(
                data_root, player, &error) &&
                restored_world.playerMineCount() == 4,
            "Save and Return did not persist the live mine count.")) {
        std::cerr << error << '\n';
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

bool testLandMineKeyEdge() {
    osf::runtime::InputAdapter input{640, 480};
    LwlEvent event{};
    event.type = LWL_EVENT_KEY_DOWN;
    std::strncpy(event.key, "b", sizeof(event.key) - 1u);
    input.handleEvent(
        nullptr, event, osf::GameState::gameplay);
    if (!check(
            input.landMinePressed(),
            "B did not publish the retail Land Mine input edge.")) {
        return false;
    }
    input.clearTransientInput();
    input.handleEvent(
        nullptr, event, osf::GameState::gameplay);
    if (!check(
            !input.landMinePressed(),
            "A held B key repeated the Land Mine input edge.")) {
        return false;
    }
    event.type = LWL_EVENT_KEY_UP;
    input.handleEvent(
        nullptr, event, osf::GameState::gameplay);
    event.type = LWL_EVENT_KEY_DOWN;
    input.handleEvent(
        nullptr, event, osf::GameState::gameplay);
    return check(
        input.landMinePressed(),
        "B did not re-arm after its key-up event.");
}

bool testLandMineHudClick() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "00000000")) {
        return true;
    }
    osf::PlayerLoadRequest player;
    player.name = "Mine HUD";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(data_root, player, &error),
            "The Land Mine HUD fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    Fixture fixture;
    const std::int32_t before = world.playerMineCount();
    fixture.click(500, 430, world, player);
    return check(
        before == 5 && world.playerMineCount() == 4 &&
            world.playerLandMineVisuals().size() == 1,
        "The retail mine HUD rectangle did not place and consume a mine.");
#else
    return true;
#endif
}

bool testScriptTransportClosesOutsidePoint() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "00000000")) {
        return true;
    }
    osf::PlayerLoadRequest player;
    player.name = "Transport Close";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(data_root, player, &error) &&
                world.activateTransportDestination(0) ==
                    osf::ScenarioTravelResult::relocated,
            "The transport-close fixture could not enter Remote Town's "
            "transport point.")) {
        std::cerr << error << '\n';
        return false;
    }
    world.update();
    const auto object = std::find_if(
        world.scenarioObjects().begin(),
        world.scenarioObjects().end(),
        [](const osf::ScenarioObjectActor& candidate) {
            return candidate.id() == 200;
        });
    if (!check(
            object != world.scenarioObjects().end(),
            "Remote Town's transport object is missing.")) {
        return false;
    }
    const osf::ScreenPosition anchor =
        osf::calculateRealPosition(object->position());
    osf::ScreenPosition pointer;
    bool found_pointer = false;
    for (std::int32_t y = -object->labelHeight();
         y <= 24 && !found_pointer;
         ++y) {
        for (std::int32_t x = -48; x <= 48; ++x) {
            pointer = {
                anchor.x - world.cameraScreenX() + x,
                anchor.y - world.cameraScreenY() + y,
            };
            world.updatePointerHover(pointer.x, pointer.y);
            if (world.hoveredScenarioObjectId() == 200) {
                found_pointer = true;
                break;
            }
        }
    }
    if (!check(
            found_pointer &&
                world.commandWorldInteraction(pointer.x, pointer.y),
            "Remote Town's transport object could not be clicked.")) {
        return false;
    }
    Fixture fixture;
    fixture.pressKey("i", world, player);
    if (!check(
            fixture.controller.inventory().active(),
            "The transport fixture could not open its right-side inventory.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 2000 && world.interactionPending();
         ++update) {
        world.update();
    }
    fixture.update(world, player);
    if (!check(
            fixture.controller.transport().active() &&
                fixture.controller.inventory().active(),
            "Opcode 37 did not open the transport panel through the UI "
            "controller beside the existing inventory.")) {
        return false;
    }
    if (!check(
            world.transitionScenario({0, 0, 0}) ==
                    osf::ScenarioTravelResult::relocated,
            "The transport-close fixture could not leave the point.")) {
        return false;
    }
    world.update();
    fixture.update(world, player);
    const osf::ScreenPosition player_screen =
        osf::calculateRealPosition(world.playerRenderPosition(1.0));
    return check(
        !fixture.controller.transport().active() &&
            fixture.controller.inventory().active() &&
            world.cameraScreenX() == player_screen.x - 160,
        "Opcode 38 did not close only the transport panel and preserve "
        "the right-side inventory camera anchor.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testEscapeClosesPanelsBeforeSettings() &&
                   testHudButtonsOpenRetailPanels() &&
                   testSaveTransitionsOwnModalInput() &&
                   testIncreasedPowerKeyEdge() &&
                   testLandMineKeyEdge() &&
                   testLandMineHudClick() &&
                   testScriptTransportClosesOutsidePoint()
               ? 0
               : 1;
}
