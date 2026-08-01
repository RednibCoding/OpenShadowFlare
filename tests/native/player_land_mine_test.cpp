#include "core/retail_random.hpp"
#include "items/item_database.hpp"
#include "items/player_inventory.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/player_item_controller.hpp"
#include "world/player_land_mine.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool containsSample(
    const osf::PlayerLandMineUpdate& update,
    std::int32_t sample) {
    return std::find_if(
               update.audio.begin(),
               update.audio.end(),
               [sample](
                   const osf::RuntimeEffectAudioRequest& request) {
                   return request.sound.bank == 0 &&
                          request.sound.sample == sample;
               }) != update.audio.end();
}

bool findGroundItemPointer(
    osf::WorldScene& world,
    std::int32_t item_id,
    osf::ScreenPosition& point) {
    const auto found = std::find_if(
        world.groundItems().begin(),
        world.groundItems().end(),
        [item_id](const osf::GroundItem& item) {
            return item.id == item_id;
        });
    if (found == world.groundItems().end()) {
        return false;
    }
    const osf::ScreenPosition anchor =
        osf::calculateRealPosition(found->position);
    for (std::int32_t y = -96; y <= 64; ++y) {
        for (std::int32_t x = -96; x <= 96; ++x) {
            point = {
                anchor.x - world.cameraScreenX() + x,
                anchor.y - world.cameraScreenY() + y,
            };
            if (point.x < 0 || point.x >= 640 ||
                point.y < 0 || point.y >= 480) {
                continue;
            }
            world.updatePointerHover(point.x, point.y);
            if (world.hoveredGroundItemId() == item_id) {
                return true;
            }
        }
    }
    return false;
}

bool dropAndSelectMine(
    osf::WorldScene& world,
    const osf::ItemDefinition& mine,
    std::int32_t& item_id) {
    if (!world.dropInventoryItem(
            osf::makeInventoryItem(mine), 320, 240)) {
        return false;
    }
    item_id = world.groundItems().back().id;
    for (std::int32_t update = 0; update < 100; ++update) {
        world.update();
        world.takeAudioSamples();
        const auto found = std::find_if(
            world.groundItems().begin(),
            world.groundItems().end(),
            [item_id](const osf::GroundItem& item) {
                return item.id == item_id;
            });
        if (found != world.groundItems().end() &&
            found->bounce_state == 2) {
            break;
        }
    }
    osf::ScreenPosition point;
    return findGroundItemPointer(world, item_id, point) &&
           world.commandWorldInteraction(point.x, point.y);
}

bool testWorldMinePickup(
    const std::filesystem::path& root) {
    osf::WorldScene world;
    osf::PlayerLoadRequest player;
    player.name = "Mine Pickup";
    std::string error;
    if (!check(
            world.loadInitialScenario(root, player, &error),
            "Remote Town could not prepare the mine-pickup fixture.")) {
        std::cerr << error << '\n';
        return false;
    }
    const osf::ItemDefinition* mine =
        world.itemDatabase().find(4, 1);
    if (!check(
            mine && mine->name == "Mine" && mine->weight == 1 &&
                world.playerMineCount() == 5 &&
                world.playerMaximumMineCount() == 10,
            "The retail mine definition or starter capacity differs.")) {
        return false;
    }
    const std::size_t backpack_size =
        world.playerInventory().items().size();
    for (std::int32_t pickup = 0; pickup < 5; ++pickup) {
        std::int32_t item_id = -1;
        if (!check(
                dropAndSelectMine(world, *mine, item_id),
                "A mine below capacity could not be selected.")) {
            return false;
        }
        for (std::int32_t update = 0; update < 2000; ++update) {
            world.update();
            world.takeAudioSamples();
            const auto remaining = std::find_if(
                world.groundItems().begin(),
                world.groundItems().end(),
                [item_id](const osf::GroundItem& item) {
                    return item.id == item_id;
                });
            if (remaining == world.groundItems().end()) {
                break;
            }
        }
        if (!check(
                world.playerMineCount() == 6 + pickup &&
                    world.playerInventory().items().size() ==
                        backpack_size,
                "A mine pickup entered the backpack instead of its "
                "separate counter.")) {
            return false;
        }
    }

    std::int32_t rejected_id = -1;
    if (!check(
            dropAndSelectMine(world, *mine, rejected_id),
            "The full-capacity mine fixture could not be selected.")) {
        return false;
    }
    bool restarted = false;
    for (std::int32_t update = 0; update < 2000; ++update) {
        world.update();
        world.takeAudioSamples();
        const auto remaining = std::find_if(
            world.groundItems().begin(),
            world.groundItems().end(),
            [rejected_id](const osf::GroundItem& item) {
                return item.id == rejected_id;
            });
        if (remaining != world.groundItems().end() &&
            remaining->bounce_state == 0) {
            restarted = true;
            break;
        }
    }
    return check(
        restarted && world.playerMineCount() == 10 &&
            world.playerInventory().items().size() == backpack_size,
        "A mine above capacity did not remain a world drop or leaked "
        "into the backpack.");
}

}  // namespace

int main() {
    osf::PlayerItemController items;
    items.initializeNew();
    if (!check(
            items.mineCount() == 5 &&
                items.consumeMine() &&
                items.mineCount() == 4 &&
                items.collectMine(10) &&
                items.mineCount() == 5,
            "The retail five-mine starter count did not consume and collect.")) {
        return 1;
    }
    items.restoreMineCount(10);
    if (!check(
            !items.collectMine(10) && items.mineCount() == 10,
            "A full mine counter accepted another mine.")) {
        return 1;
    }

    osf::TableDatabase tables;
    std::string error;
    const std::filesystem::path root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!testWorldMinePickup(root)) {
        return 1;
    }
    if (!check(
            tables.load(
                root / "System" / "Game" / "Parameter" /
                    "Table.Tbd",
                &error),
            "Table.Tbd could not be loaded for the mine fixture.")) {
        std::cerr << error << '\n';
        return 1;
    }

    osf::PlayerLandMineSystem mines;
    osf::RetailRandom random(1);
    const osf::WorldPosition origin{1000, 2000};
    if (!check(
            mines.place(origin, 1, 0) &&
                !mines.place(origin, 1, 0) &&
                mines.cooldown() == 10 &&
                mines.visuals().size() == 1 &&
                mines.visuals().front().static_pattern,
            "Mine placement did not create the retail static actor and lockout.")) {
        return 1;
    }

    osf::RuntimeEffectTargetSnapshot enemy;
    enemy.kind = osf::RuntimeEffectTargetKind::enemy;
    enemy.character_number = 200;
    enemy.identifier = 200;
    enemy.position = origin;
    enemy.judgement = {-20, -20, 20, 20};
    enemy.current_life = 100;
    const std::vector<osf::RuntimeEffectTargetSnapshot> targets{enemy};
    const auto animationLength = [](std::int32_t) { return 20; };
    constexpr std::int32_t damage_bonus = 7;

    osf::PlayerLandMineUpdate update;
    for (std::int32_t tick = 1; tick < 40; ++tick) {
        update = mines.update(
            targets, tables, damage_bonus, random, animationLength);
        if (!check(
                update.dispatches.empty(),
                "The mine triggered before retail arms it at counter 40.")) {
            return 1;
        }
    }
    update = mines.update(
        targets, tables, damage_bonus, random, animationLength);
    const osf::TableData* mine_damage = tables.find(23);
    if (!check(
            update.dispatches.size() == 1 &&
                containsSample(update, 54) &&
                containsSample(update, 29) &&
                update.dispatches.front().contact.identifier == 200 &&
                update.dispatches.front().packet[4] ==
                    mine_damage->value(0, 0) + damage_bonus &&
                update.dispatches.front().packet[36] == 99999 &&
                mines.visuals().size() == 1 &&
                mines.visuals().front().resource_id == 1001,
            "The armed mine did not beep, explode, or dispatch Table 23 damage.")) {
        return 1;
    }

    for (std::int32_t tick = 0; tick < 12; ++tick) {
        mines.update({}, tables, 0, random, animationLength);
    }
    bool has_ring = false;
    bool has_bounce = false;
    for (const osf::PlayerLandMineVisual& visual : mines.visuals()) {
        has_ring = has_ring ||
            (visual.resource_id >= 1002 &&
             visual.resource_id <= 1004);
        has_bounce = has_bounce ||
            (visual.resource_id >= 1005 &&
             visual.resource_id <= 1008 &&
             visual.vertical_acceleration == -100);
    }
    if (!check(
            has_ring && has_bounce,
            "The mine explosion omitted its retail ring or bouncing debris.")) {
        return 1;
    }

    for (std::int32_t tick = 0; tick < 90; ++tick) {
        mines.update({}, tables, 0, random, animationLength);
    }
    if (!check(
            mines.activeMineCount() == 0,
            "The mine controller did not finish at its retail counter.")) {
        return 1;
    }

    osf::RuntimeEffectTargetSnapshot object;
    object.kind = osf::RuntimeEffectTargetKind::scenario_object;
    object.character_number = 10000001;
    object.identifier = 10000001;
    object.position = origin;
    object.judgement = {-20, -20, 20, 20};
    object.displayed = true;
    object.runtime_state = 0;
    osf::PlayerLandMineSystem object_mine;
    object_mine.place(origin, 1, 0);
    for (std::int32_t tick = 0; tick < 40; ++tick) {
        update = object_mine.update(
            {object}, tables, 0, random, animationLength);
    }
    if (!check(
            containsSample(update, 29) &&
                update.dispatches.empty() &&
                !object_mine.visuals().empty() &&
                object_mine.visuals().front().resource_id == 1001,
            "An active scenario object did not trigger the mine actor.")) {
        return 1;
    }

    osf::PlayerLandMineSystem timed_mine;
    timed_mine.place(origin, 1, 0);
    for (std::int32_t tick = 0; tick < 300; ++tick) {
        update = timed_mine.update(
            {}, tables, 0, random, animationLength);
    }
    return check(
               containsSample(update, 29),
               "An untriggered mine did not explode at lifetime 300.")
        ? 0
        : 1;
}
