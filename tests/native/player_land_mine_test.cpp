#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/player_item_controller.hpp"
#include "world/player_land_mine.hpp"

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
