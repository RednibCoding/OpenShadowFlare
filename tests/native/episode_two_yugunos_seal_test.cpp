#include "episode_one_test_support.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_progress.hpp"
#include "world/retail_save_world_state.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using osf::test::check;
using osf::test::containsSample;
using osf::test::loadSavedFixture;
using osf::test::raiseToLevel;

osf::RetailSaveProgress deeperMineProgress(const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed :
         {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 13, 14}) {
        progress.quest_flags[
            static_cast<std::size_t>(completed)] = 2;
    }
    progress.quest_flags[12] = 1;
    progress.quest_flags[15] = 2;
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    progress.script_state_flags[11] = 2;
    progress.script_state_flags[15] = 1;
    progress.script_state_flags[23] = 1;
    progress.script_state_flags[24] = 1;
    progress.script_state_flags[38] = 1;
    progress.script_state_flags[40] = 1;
    progress.script_state_flags[41] = 1;
    progress.script_state_flags[45] = 1;
    progress.script_state_flags[71] = 1;
    return progress;
}

bool writeFixture(
    const std::filesystem::path& save_path,
    const osf::WorldScene& world,
    const osf::PlayerData& player,
    const osf::RetailSaveProgress& progress,
    const osf::RetailSaveWorldState& world_state,
    std::string& error) {
    return osf::writeRetailSave(
        save_path,
        player,
        world.itemDatabase(),
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        progress,
        world.playerMagic(),
        world.playerMineCount(),
        world_state,
        world.playerGiantWarehouse(),
        world.playerAutomaticItems(),
        0x54,
        &error);
}

const osf::ScenarioObjectActor* findObject(
    const osf::WorldScene& world,
    std::int32_t object_id) {
    const auto found = std::find_if(
        world.scenarioObjects().begin(),
        world.scenarioObjects().end(),
        [object_id](const osf::ScenarioObjectActor& object) {
            return object.id() == object_id;
        });
    return found == world.scenarioObjects().end() ? nullptr : &*found;
}

bool findPointerPoint(
    osf::WorldScene& world,
    std::int32_t object_id,
    osf::ScreenPosition& point) {
    const osf::ScenarioObjectActor* object = findObject(world, object_id);
    if (!object) {
        return false;
    }
    const osf::ScreenPosition anchor =
        osf::calculateRealPosition(object->position());
    for (std::int32_t y = -240; y <= 160; ++y) {
        for (std::int32_t x = -240; x <= 240; ++x) {
            point = {
                anchor.x - world.cameraScreenX() + x,
                anchor.y - world.cameraScreenY() + y,
            };
            world.updatePointerHover(point.x, point.y);
            if (world.hoveredScenarioObjectId() == object_id) {
                return true;
            }
        }
    }
    return false;
}

bool interactSwitch(
    osf::WorldScene& world,
    std::int32_t active_id,
    std::int32_t replacement_id,
    std::vector<std::int32_t>& audio,
    std::int32_t maximum_updates = 5000) {
    const osf::ScenarioObjectActor* active =
        findObject(world, active_id);
    if (!active) {
        return false;
    }
    const osf::WorldPosition target = active->position();
    bool interaction_issued = false;
    for (std::int32_t update = 0;
         update < maximum_updates;
         ++update) {
        const osf::ScenarioObjectActor* replacement =
            findObject(world, replacement_id);
        active = findObject(world, active_id);
        if (active && replacement &&
            !active->pointerEnabled() &&
            replacement->pointerEnabled()) {
            return true;
        }
        if (!interaction_issued && update % 30 == 0) {
            const osf::ScreenPosition screen =
                osf::calculateRealPosition(target);
            const std::int32_t view_x =
                screen.x - world.cameraScreenX();
            const std::int32_t view_y =
                screen.y - world.cameraScreenY();
            if (view_x >= -240 && view_x < 880 &&
                view_y >= -240 && view_y < 640) {
                osf::ScreenPosition pointer;
                if (findPointerPoint(world, active_id, pointer) &&
                    world.commandWorldInteraction(
                        pointer.x, pointer.y)) {
                    interaction_issued = true;
                }
            }
            if (!interaction_issued) {
                world.commandPlayerMovement(view_x, view_y);
            }
        }
        world.update();
        std::vector<std::int32_t> samples =
            world.takeAudioSamples();
        audio.insert(audio.end(), samples.begin(), samples.end());
    }
    return false;
}

bool walkNear(
    osf::WorldScene& world,
    osf::WorldPosition target,
    std::int32_t distance,
    std::int32_t maximum_updates = 2000) {
    for (std::int32_t update = 0;
         update < maximum_updates;
         ++update) {
        const std::int64_t delta_x =
            static_cast<std::int64_t>(world.playerWorldX()) - target.x;
        const std::int64_t delta_y =
            static_cast<std::int64_t>(world.playerWorldY()) - target.y;
        if (delta_x * delta_x + delta_y * delta_y <=
            static_cast<std::int64_t>(distance) * distance) {
            return true;
        }
        if (update % 30 == 0) {
            const osf::ScreenPosition screen =
                osf::calculateRealPosition(target);
            world.commandPlayerMovement(
                screen.x - world.cameraScreenX(),
                screen.y - world.cameraScreenY());
        }
        world.update();
        world.takeAudioSamples();
    }
    return false;
}

std::int32_t floorDivide(
    std::int32_t value,
    std::int32_t divisor) {
    return value >= 0
        ? value / divisor
        : -((-value + divisor - 1) / divisor);
}

bool walkStaticRoute(
    osf::WorldScene& world,
    osf::WorldPosition target,
    std::int32_t target_distance,
    std::int32_t margin = 2400,
    std::int32_t allowed_trigger_id = -1,
    std::int32_t step = 20,
    bool exclude_special_objects = false,
    const osf::ScenarioObjectActor* contact_target = nullptr) {
    if (step <= 0) {
        return false;
    }
    const osf::WorldPosition start{
        world.playerWorldX(), world.playerWorldY()};
    const std::int32_t origin_x =
        world.ground().judgeOffsetX() *
        world.ground().baseMagnificationX();
    const std::int32_t origin_y =
        world.ground().judgeOffsetY() *
        world.ground().baseMagnificationY();
    const std::int32_t minimum_x = floorDivide(
        std::min(start.x, target.x) - margin - origin_x, step);
    const std::int32_t maximum_x = floorDivide(
        std::max(start.x, target.x) + margin - origin_x, step);
    const std::int32_t minimum_y = floorDivide(
        std::min(start.y, target.y) - margin - origin_y, step);
    const std::int32_t maximum_y = floorDivide(
        std::max(start.y, target.y) + margin - origin_y, step);
    const std::int32_t width = maximum_x - minimum_x + 1;
    const std::int32_t height = maximum_y - minimum_y + 1;
    if (width <= 0 || height <= 0) {
        return false;
    }
    const std::size_t count =
        static_cast<std::size_t>(width) * height;
    if (count > 20000000u) {
        return false;
    }
    const auto indexFor = [&](std::int32_t x, std::int32_t y) {
        return static_cast<std::size_t>(
            (y - minimum_y) * width + (x - minimum_x));
    };
    const auto positionFor = [&](std::int32_t x, std::int32_t y) {
        return osf::WorldPosition{
            origin_x + x * step,
            origin_y + y * step,
        };
    };
    const auto positionIsAvailable = [&](osf::WorldPosition position) {
        if (!osf::positionIsWalkable(
                world.ground(),
                world.objectMap(),
                position,
                world.playerJudgement(),
                exclude_special_objects)) {
            return false;
        }
        for (const osf::ScenarioObjectActor& object :
             world.scenarioObjects()) {
            if (allowed_trigger_id == -2) {
                break;
            }
            if (object.id() < 0 || object.id() > 9 ||
                object.id() == allowed_trigger_id ||
                !object.judgementEnabled()) {
                continue;
            }
            const osf::ObjectBounds& player = world.playerJudgement();
            const osf::ObjectBounds& trigger = object.judgement();
            if (position.x + player.left <=
                    object.position().x + trigger.right &&
                object.position().x + trigger.left <=
                    position.x + player.right &&
                position.y + player.top <=
                    object.position().y + trigger.bottom &&
                object.position().y + trigger.top <=
                    position.y + player.bottom) {
                return false;
            }
        }
        return true;
    };
    std::vector<std::int32_t> previous(count, -2);
    std::vector<std::size_t> queue;
    queue.reserve(count / 4u);
    const std::int32_t nearest_x = static_cast<std::int32_t>(
        std::lround(
            static_cast<double>(start.x - origin_x) / step));
    const std::int32_t nearest_y = static_cast<std::int32_t>(
        std::lround(
            static_cast<double>(start.y - origin_y) / step));
    std::int32_t start_x = nearest_x;
    std::int32_t start_y = nearest_y;
    std::int64_t best_distance = std::numeric_limits<std::int64_t>::max();
    for (std::int32_t y = nearest_y - 4; y <= nearest_y + 4; ++y) {
        for (std::int32_t x = nearest_x - 4; x <= nearest_x + 4; ++x) {
            if (x < minimum_x || x > maximum_x ||
                y < minimum_y || y > maximum_y) {
                continue;
            }
            const osf::WorldPosition position = positionFor(x, y);
            if (!positionIsAvailable(position) ||
                osf::advanceLinearMovement(
                    world.ground(),
                    world.objectMap(),
                    world.playerJudgement(),
                    start,
                    position,
                    exclude_special_objects).collided) {
                continue;
            }
            const std::int64_t delta_x =
                static_cast<std::int64_t>(position.x) - start.x;
            const std::int64_t delta_y =
                static_cast<std::int64_t>(position.y) - start.y;
            const std::int64_t distance =
                delta_x * delta_x + delta_y * delta_y;
            if (distance < best_distance) {
                best_distance = distance;
                start_x = x;
                start_y = y;
            }
        }
    }
    if (best_distance == std::numeric_limits<std::int64_t>::max()) {
        return false;
    }
    const std::size_t start_index = indexFor(start_x, start_y);
    previous[start_index] = -1;
    queue.push_back(start_index);
    std::size_t goal_index = std::numeric_limits<std::size_t>::max();
    for (std::size_t cursor = 0; cursor < queue.size(); ++cursor) {
        const std::size_t current_index = queue[cursor];
        const std::int32_t x =
            static_cast<std::int32_t>(current_index % width) + minimum_x;
        const std::int32_t y =
            static_cast<std::int32_t>(current_index / width) + minimum_y;
        const osf::WorldPosition position = positionFor(x, y);
        const std::int64_t delta_x =
            static_cast<std::int64_t>(position.x) - target.x;
        const std::int64_t delta_y =
            static_cast<std::int64_t>(position.y) - target.y;
        const bool reached = contact_target
            ? osf::distanceBetweenBounds(
                  position,
                  world.playerJudgement(),
                  contact_target->position(),
                  contact_target->judgement()) <= target_distance
            : delta_x * delta_x + delta_y * delta_y <=
                  static_cast<std::int64_t>(target_distance) *
                      target_distance;
        if (reached) {
            goal_index = current_index;
            break;
        }
        for (const osf::WorldPosition direction : {
                 osf::WorldPosition{-1, 0},
                 osf::WorldPosition{1, 0},
                 osf::WorldPosition{0, -1},
                 osf::WorldPosition{0, 1}}) {
            const std::int32_t next_x = x + direction.x;
            const std::int32_t next_y = y + direction.y;
            if (next_x < minimum_x || next_x > maximum_x ||
                next_y < minimum_y || next_y > maximum_y) {
                continue;
            }
            const std::size_t next_index = indexFor(next_x, next_y);
            if (previous[next_index] != -2) {
                continue;
            }
            if (!positionIsAvailable(positionFor(next_x, next_y))) {
                continue;
            }
            previous[next_index] =
                static_cast<std::int32_t>(current_index);
            queue.push_back(next_index);
        }
    }
    if (goal_index == std::numeric_limits<std::size_t>::max()) {
        std::cerr << "route failed " << start.x << ',' << start.y << " -> "
                  << target.x << ',' << target.y << " visited="
                  << queue.size() << '\n';
        return false;
    }
    std::vector<osf::WorldPosition> path;
    for (std::int32_t current = static_cast<std::int32_t>(goal_index);
         current >= 0;
         current = previous[static_cast<std::size_t>(current)]) {
        const std::size_t index = static_cast<std::size_t>(current);
        path.push_back(positionFor(
            static_cast<std::int32_t>(index % width) + minimum_x,
            static_cast<std::int32_t>(index / width) + minimum_y));
    }
    std::reverse(path.begin(), path.end());
    if (!path.empty() &&
        !walkNear(world, path.front(), std::max(step / 2, 10), 300)) {
        return false;
    }
    for (std::size_t index = 4; index < path.size(); index += 4) {
        if (!walkNear(world, path[index], 30, 300)) {
            return false;
        }
    }
    return path.empty() || walkNear(world, path.back(), 30, 300);
}

bool walkUntilEntry(
    osf::WorldScene& world,
    std::int32_t object_id,
    std::int32_t expected_entry,
    std::int32_t maximum_updates = 5000) {
    const osf::ScenarioObjectActor* trigger =
        osf::test::findScenarioTrigger(world, object_id);
    if (!trigger) {
        return false;
    }
    const osf::WorldPosition target =
        osf::test::scenarioTriggerCenter(*trigger);
    for (std::int32_t update = 0;
         update < maximum_updates;
         ++update) {
        if (world.retailSaveWorldState().entry_value == expected_entry) {
            return true;
        }
        if (update % 30 == 0) {
            const osf::ScreenPosition screen =
                osf::calculateRealPosition(target);
            world.commandPlayerMovement(
                screen.x - world.cameraScreenX(),
                screen.y - world.cameraScreenY());
        }
        world.update();
        world.takeAudioSamples();
    }
    return false;
}

bool walkUntilFlag(
    osf::WorldScene& world,
    std::int32_t object_id,
    std::size_t flag_index,
    std::int32_t expected_value,
    std::int32_t maximum_updates = 5000) {
    const osf::ScenarioObjectActor* trigger =
        osf::test::findScenarioTrigger(world, object_id);
    if (!trigger) {
        return false;
    }
    const osf::WorldPosition target =
        osf::test::scenarioTriggerCenter(*trigger);
    for (std::int32_t update = 0;
         update < maximum_updates;
         ++update) {
        const osf::RetailSaveProgress& progress =
            world.retailSaveProgress();
        if (flag_index < progress.script_state_flags.size() &&
            progress.script_state_flags[flag_index] == expected_value) {
            return true;
        }
        if (update % 30 == 0) {
            const osf::ScreenPosition screen =
                osf::calculateRealPosition(target);
            world.commandPlayerMovement(
                screen.x - world.cameraScreenX(),
                screen.y - world.cameraScreenY());
        }
        world.update();
        world.takeAudioSamples();
    }
    return false;
}

bool testYugunosSeal(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "YugunosSeal";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The deeper Yugunos fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The deeper Yugunos fixture could not reach its area level.")) {
        return false;
    }
    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_yugunos_seal_test";
    const std::filesystem::path route_save =
        fixture_root / "route" / "Save" / "0000.Ssv";
    const std::filesystem::path report_save =
        fixture_root / "report" / "Save" / "0000.Ssv";
    const std::filesystem::path completed_save =
        fixture_root / "completed" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);
    if (!check(
            writeFixture(
                route_save,
                seed,
                level_sixty,
                deeperMineProgress(seed),
                {true, 2210002, 0},
                error),
            "The deeper Yugunos route fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene route;
    if (!check(
            loadSavedFixture(data_root, route_save, route, error) &&
                route.scenario().title() ==
                    "Mining Tunnel of Yugnos, B3F",
            "The deeper Yugunos route fixture could not be restored.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
            osf::test::markScenarioEnemiesDefeated(route, 0, 63),
            "The B3F route fixture could not isolate its switch path.")) {
        return false;
    }
    std::vector<std::int32_t> b3_audio;
    const osf::ScenarioObjectActor* b3_exit =
        osf::test::findScenarioTrigger(route, 1);
    const osf::ScenarioObjectActor* b3_stair =
        osf::test::findScenarioTrigger(route, 2);
    if (!check(
            b3_exit && b3_stair &&
                walkNear(route, {33700, 1800}, 500) &&
                walkNear(route, {32400, -600}, 500) &&
                walkNear(route, {33700, -2200}, 500) &&
                interactSwitch(route, 40000, 40001, b3_audio) &&
                walkNear(route, {32400, -600}, 500) &&
                walkNear(route, {33700, 1800}, 500) &&
                walkNear(route, {27000, 3000}, 500) &&
                walkNear(route, {26080, 2400}, 300) &&
                walkNear(route, {26080, 4800}, 300) &&
                walkNear(route, {28480, 4800}, 300) &&
                walkNear(route, {28480, 8320}, 300) &&
                interactSwitch(route, 40002, 40003, b3_audio) &&
                containsSample(b3_audio, 38) &&
                walkStaticRoute(
                    route,
                    osf::test::scenarioTriggerCenter(*b3_stair),
                    300,
                    2400,
                    2) &&
                walkUntilEntry(route, 2, 2, 2000) &&
                osf::test::markScenarioEnemiesDefeated(route, 0, 63) &&
                (b3_exit = osf::test::findScenarioTrigger(route, 1)) &&
                walkStaticRoute(
                    route,
                    osf::test::scenarioTriggerCenter(*b3_exit),
                    300,
                    2400,
                    1) &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 2210003) &&
                route.scenario().title() ==
                    "Mining Tunnel of Yugnos, B5F" &&
                osf::test::markScenarioEnemiesDefeated(
                    route, 0, 89),
            "The two B3F switches did not open the authored route to B5F.")) {
        std::cerr << "scenario=" << route.scenarioId()
                  << " player=" << route.playerWorldX() << ','
                  << route.playerWorldY() << " life="
                  << route.playerData().currentLife() << '\n';
        for (const osf::EnemyActor& enemy : route.enemies()) {
            const std::int64_t dx =
                static_cast<std::int64_t>(enemy.position().x) -
                route.playerWorldX();
            const std::int64_t dy =
                static_cast<std::int64_t>(enemy.position().y) -
                route.playerWorldY();
            if (enemy.currentLife() > 0 &&
                dx * dx + dy * dy < 4000000) {
                std::cerr << "near enemy=" << enemy.id() << ' '
                          << enemy.name() << ' ' << enemy.position().x
                          << ',' << enemy.position().y << " life="
                          << enemy.currentLife() << '\n';
            }
        }
        return false;
    }

    std::vector<std::int32_t> b5_audio;
    const osf::ScenarioObjectActor* lower_switch =
        findObject(route, 40002);
    const osf::ScenarioObjectActor* upper_switch =
        findObject(route, 40000);
    const osf::ScenarioObjectActor* seal =
        osf::test::findScenarioTrigger(route, 800);
    const bool lower_switch_approached =
        lower_switch && walkStaticRoute(
            route, lower_switch->position(), 500, 25000, -2, 20);
    const bool lower_switch_opened =
        lower_switch_approached &&
        interactSwitch(route, 40002, 40003, b5_audio);
    if (!check(
            lower_switch && upper_switch && seal &&
                lower_switch_opened &&
                walkStaticRoute(
                    route,
                    upper_switch->position(),
                    500,
                    25000,
                    -2,
                    40) &&
                interactSwitch(route, 40000, 40001, b5_audio) &&
                walkStaticRoute(
                    route,
                    seal->position(),
                    100,
                    25000,
                    -2,
                    40,
                    false,
                    seal) &&
                containsSample(b5_audio, 38) &&
                walkUntilFlag(route, 800, 39, 1, 8000),
            "The two B5F switches did not open the route to the deeper seal.")) {
        std::cerr << "scenario=" << route.scenarioId()
                  << " entry="
                  << route.retailSaveWorldState().entry_value
                  << " flag39="
                  << route.retailSaveProgress().script_state_flags[39]
                  << " player=" << route.playerWorldX() << ','
                  << route.playerWorldY() << " life="
                  << route.playerData().currentLife() << '\n';
        for (const osf::ScenarioObjectActor& object :
             route.scenarioObjects()) {
            const std::int64_t dx =
                static_cast<std::int64_t>(object.position().x) -
                route.playerWorldX();
            const std::int64_t dy =
                static_cast<std::int64_t>(object.position().y) -
                route.playerWorldY();
            if (object.judgementEnabled() &&
                dx * dx + dy * dy < 16000000) {
                std::cerr << " blocker " << object.id() << " at "
                          << object.position().x << ','
                          << object.position().y << " bounds "
                          << object.judgement().left << ','
                          << object.judgement().top << ','
                          << object.judgement().right << ','
                          << object.judgement().bottom << '\n';
            }
        }
        return false;
    }

    if (!check(
            writeFixture(
                report_save,
                route,
                route.playerData(),
                route.retailSaveProgress(),
                {true, 2200000, 0},
                error),
            "The Yugunos findings could not be saved for Kirarru.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene fanann;
    if (!check(
            loadSavedFixture(data_root, report_save, fanann, error) &&
                fanann.scenarioId() == 2200000 &&
                fanann.retailSaveProgress().script_state_flags[38] == 1 &&
                fanann.retailSaveProgress().script_state_flags[39] == 1 &&
                fanann.retailSaveProgress().script_state_flags[40] == 1 &&
                fanann.retailSaveProgress().script_state_flags[45] == 1 &&
                fanann.quests().state(12) == 1 &&
                fanann.quests().state(15) == 2 &&
                osf::test::openNpcConversation(fanann, 4) &&
                fanann.conversationMessageId() == 1000058,
            "Kirarru did not recognize the damaged seal facilities.")) {
        std::cerr << error << " message="
                  << fanann.conversationMessageId() << '\n';
        return false;
    }
    for (std::int32_t message = 1000059;
         message <= 1000068;
         ++message) {
        fanann.advanceConversation();
        if (!check(
                fanann.conversationMessageId() == message,
                "Kirarru's dragon warning skipped a message.")) {
            std::cerr << "message=" << fanann.conversationMessageId()
                      << " expected=" << message << '\n';
            return false;
        }
    }
    fanann.advanceConversation();
    if (!check(
            !fanann.conversationActive() &&
                fanann.retailSaveProgress().script_state_flags[39] == 2 &&
                fanann.quests().state(12) == 1 &&
                fanann.quests().state(15) == 2,
            "Kirarru did not save the completed dragon warning.")) {
        std::cerr << "active=" << fanann.conversationActive()
                  << " message=" << fanann.conversationMessageId()
                  << " flag39="
                  << fanann.retailSaveProgress().script_state_flags[39]
                  << " quest12=" << fanann.quests().state(12)
                  << " quest15=" << fanann.quests().state(15) << '\n';
        return false;
    }

    if (!check(
            writeFixture(
                completed_save,
                fanann,
                fanann.playerData(),
                fanann.retailSaveProgress(),
                fanann.retailSaveWorldState(),
                error),
            "Kirarru's dragon warning could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    if (!check(
            loadSavedFixture(
                data_root, completed_save, persisted, error) &&
                persisted.scenarioId() == 2200000 &&
                persisted.retailSaveProgress().script_state_flags[38] == 1 &&
                persisted.retailSaveProgress().script_state_flags[39] == 2 &&
                persisted.retailSaveProgress().script_state_flags[40] == 1 &&
                persisted.retailSaveProgress().script_state_flags[45] == 1 &&
                persisted.quests().state(12) == 1 &&
                persisted.quests().state(15) == 2,
            "Saving Kirarru's dragon warning lost its completed state.")) {
        std::cerr << error << '\n';
        return false;
    }

    std::filesystem::remove_all(fixture_root, cleanup_error);
    return true;
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "02210003")) {
        return 0;
    }
    return testYugunosSeal(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
