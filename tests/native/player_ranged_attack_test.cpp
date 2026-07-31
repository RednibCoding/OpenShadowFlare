#include "core/retail_random.hpp"
#include "items/item_condition.hpp"
#include "items/item_database.hpp"
#include "items/player_equipment.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "world/actor_direction.hpp"
#include "world/generic_effect_actor.hpp"
#include "world/player_ranged_attack.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
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

osf::PlayerAttackImpactStats attackStats() {
    osf::PlayerAttackImpactStats stats;
    stats.source_character_number = 0;
    stats.level = 1;
    stats.physical_attack = 100;
    stats.physical_defense = 20;
    stats.hit_rate = 120;
    stats.reflection_chance = 50;
    stats.reflection_percent = 25;
    stats.weapon_identifier = 1000000;
    stats.weapon_subtype = 5;
    return stats;
}

bool near(double first, double second) {
    return std::abs(first - second) < 0.000001;
}

const osf::EnemyActor* findEnemy(
    const osf::WorldScene& world,
    std::int32_t enemy_id) {
    const auto found = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [enemy_id](const osf::EnemyActor& enemy) {
            return enemy.id() == enemy_id;
        });
    return found == world.enemies().end()
        ? nullptr
        : &*found;
}

bool findEnemyPointer(
    osf::WorldScene& world,
    std::int32_t enemy_id,
    osf::ScreenPosition& point) {
    const osf::EnemyActor* enemy =
        findEnemy(world, enemy_id);
    if (!enemy || enemy->currentLife() < 1 ||
        !enemy->visible() || !enemy->pointerEnabled()) {
        return false;
    }
    const osf::ScreenPosition anchor =
        osf::calculateRealPosition(enemy->position());
    for (std::int32_t y = -enemy->labelHeight();
         y <= 24;
         ++y) {
        for (std::int32_t x = -64; x <= 64; ++x) {
            point = {
                anchor.x - world.cameraScreenX() + x,
                anchor.y - world.cameraScreenY() + y,
            };
            if (point.x < 0 || point.x >= 640 ||
                point.y < 0 || point.y >= 480) {
                continue;
            }
            world.updatePointerHover(point.x, point.y);
            if (world.hoveredEnemyId() == enemy_id) {
                return true;
            }
        }
    }
    return false;
}

bool containsSample(
    const std::vector<std::int32_t>& samples,
    std::int32_t sample) {
    return std::find(
               samples.begin(), samples.end(), sample) !=
           samples.end();
}

bool testScaling() {
    return check(
        osf::retailRangedPhysicalAttack(100, 5, 0) == 100 &&
            osf::retailRangedPhysicalAttack(100, 16, 0) == 40 &&
            osf::retailRangedPhysicalAttack(100, 16, 15) == 65 &&
            osf::retailRangedPhysicalAttack(100, 16, 30) == 90 &&
            osf::retailRangedPhysicalAttack(0, 16, 0) == 1,
        "The retail ranged-job damage scale differs.");
}

bool testPatternMatrix(
    const osf::ItemDefinition& fixture) {
    constexpr std::array<std::int32_t, 9>
        counts{{1, 2, 1, 3, 3, 5, 5, 7, 7}};
    constexpr std::array<double, 7>
        wide{{
            0.0, -15.0, 15.0, -30.0,
            30.0, -45.0, 45.0,
        }};
    constexpr std::array<double, 7>
        medium{{
            0.0, -10.0, 10.0, -20.0,
            20.0, -30.0, 30.0,
        }};
    constexpr std::array<double, 7>
        narrow{{
            0.0, -8.0, 8.0, -16.0,
            16.0, -24.0, 24.0,
        }};

    for (std::int32_t pattern = 0;
         pattern <= 8;
         ++pattern) {
        osf::ItemDefinition weapon = fixture;
        weapon.ranged_pattern = pattern;
        weapon.ranged_pierces_targets = pattern == 8;
        osf::RetailRandom random(1);
        const osf::PlayerRangedAttackResult shot =
            osf::resolvePlayerRangedAttack(
                {
                    0,
                    {1000, 2000},
                    {2000, 2000},
                    77,
                    5,
                    0,
                    attackStats(),
                    &weapon,
                },
                random);
        if (!shot.valid ||
            shot.projectiles.size() !=
                static_cast<std::size_t>(
                    counts[
                        static_cast<std::size_t>(
                            pattern)])) {
            return check(
                false,
                "A retail bowgun spread produced the wrong "
                "projectile count.");
        }
        const bool homing =
            pattern == 2 || pattern == 4 ||
            pattern == 6 || pattern == 8;
        const std::array<double, 7>* offsets = &wide;
        if (pattern == 5 || pattern == 6) {
            offsets = &medium;
        } else if (
            pattern == 1 || pattern == 7 ||
            pattern == 8) {
            offsets = &narrow;
        }
        for (std::size_t index = 0;
             index < shot.projectiles.size();
             ++index) {
            const osf::CombatEffectSpawnRequest& request =
                shot.projectiles[index];
            const double expected_direction =
                pattern == 1
                    ? 0.0
                    : (*offsets)[index] *
                          osf::kRetailRadiansPerDegree;
            if (request.constructor_value_20 !=
                    (homing ? 1 : 0) ||
                request.packet_kind !=
                    ((homing || pattern == 0)
                         ? 8
                         : 1) ||
                request.owner_kind !=
                    (pattern == 1 ? 0 : 1) ||
                request.constructor_value_22 !=
                    (pattern == 8 ? 1 : 0) ||
                !near(
                    request.direction_radians,
                    expected_direction)) {
                return check(
                    false,
                    "A retail bowgun spread lost its homing, "
                    "direction, owner, or piercing descriptor.");
            }
        }
    }

    constexpr std::array<std::int32_t, 4>
        effect_numbers{{1, 0, 4, 5}};
    for (std::int32_t selector = 0;
         selector < 4;
         ++selector) {
        osf::ItemDefinition weapon = fixture;
        weapon.ranged_effect_selector = selector;
        weapon.ranged_pattern = 0;
        osf::RetailRandom random(1);
        const osf::PlayerRangedAttackResult shot =
            osf::resolvePlayerRangedAttack(
                {
                    0,
                    {1000, 2000},
                    {2000, 2000},
                    77,
                    5,
                    0,
                    attackStats(),
                    &weapon,
                },
                random);
        if (!shot.valid ||
            shot.projectiles.size() != 1 ||
            shot.projectiles[0].effect_number !=
                effect_numbers[
                    static_cast<std::size_t>(
                        selector)]) {
            return check(
                false,
                "A retail bowgun effect selector mapped to the "
                "wrong generic actor family.");
        }
    }
    return true;
}

bool testRetailBowguns() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path item_path =
        std::filesystem::path(
            OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "System" / "Game" /
        "Parameter" / "Item.Ibn";
    if (!std::filesystem::is_regular_file(item_path)) {
        return true;
    }
    osf::ItemDatabase items;
    std::string error;
    if (!check(
            items.load(item_path, &error),
            "The ranged Item.Ibn fixture could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const osf::ItemDefinition* wood =
        items.find(0, 1000000);
    const osf::ItemDefinition* double_bowgun =
        items.find(0, 1000400);
    const osf::ItemDefinition* piercing =
        items.find(0, 80000022);
    if (!check(
            wood && wood->name == "Wood Bowgun" &&
                wood->subtype == 5 &&
                wood->ranged_effect_selector == 1 &&
                wood->ranged_pattern == 0 &&
                wood->ranged_travel_speed == 100 &&
                !wood->ranged_pierces_targets &&
                double_bowgun &&
                double_bowgun->ranged_pattern == 1 &&
                piercing &&
                piercing->ranged_effect_selector == 2 &&
                piercing->ranged_travel_speed == 120 &&
                piercing->ranged_pierces_targets,
            "Known retail bowgun projectile fields differ.")) {
        return false;
    }

    osf::RetailRandom random(1);
    osf::RetailRandom expected(1);
    const std::int32_t reflection_draw =
        expected.next();
    const std::int32_t effect_draw =
        expected.next();
    const osf::PlayerRangedAttackResult shot =
        osf::resolvePlayerRangedAttack(
            {
                0,
                {1000, 2000},
                {2000, 2000},
                77,
                16,
                0,
                attackStats(),
                wood,
            },
            random);
    if (!check(
            shot.valid &&
                shot.consume_durability &&
                shot.projectiles.size() == 1 &&
                random.state() == expected.state(),
            "The Wood Bowgun did not create its one retail projectile "
            "with the expected random-call order.")) {
        return false;
    }
    const osf::CombatEffectSpawnRequest& request =
        shot.projectiles.front();
    if (!check(
            request.effect_number == 0 &&
                request.owner_kind == 1 &&
                request.source_character_number == 0 &&
                request.target_kind == 20 &&
                request.target_identifier == 77 &&
                request.constructor_value_6 == 100 &&
                request.constructor_value_7 == 350 &&
                near(request.direction_radians, 0.0) &&
                request.packet_kind == 8 &&
                request.constructor_value_20 == 0 &&
                request.constructor_value_21 == 200 &&
                request.constructor_value_22 == 0 &&
                request.packet[4] == 40 &&
                request.packet[36] == 120 &&
                request.packet[39] ==
                    (reflection_draw % 100 < 50 ? 25 : 0) &&
                request.packet[34] ==
                    effect_draw % 3 + 21007,
            "The Wood Bowgun request or combat packet differs.")) {
        return false;
    }

    osf::RuntimeEffectActorSpawnRequest actor;
    if (!check(
            osf::buildGenericEffectActor(
                request, {1000, 2000}, actor) &&
                actor.resource_id == 0 &&
                actor.position.x == 1200 &&
                actor.position.y == 2000 &&
                actor.travel_speed == 100 &&
                actor.judgement.left == -30 &&
                actor.judgement.bottom == 30 &&
                actor.target_collision_start == 0 &&
                actor.expire_on_target &&
                actor.expire_on_environment_collision &&
                actor.animation_direction == 1 &&
                actor.has_packet,
            "The generic effect actor lost the retail Wood Bowgun "
            "descriptor.")) {
        return false;
    }

    osf::RetailRandom double_random(1);
    const osf::PlayerRangedAttackResult double_shot =
        osf::resolvePlayerRangedAttack(
            {
                0,
                {1000, 2000},
                {2000, 2000},
                77,
                16,
                0,
                attackStats(),
                double_bowgun,
            },
            double_random);
    if (!check(
        double_shot.valid &&
            double_shot.projectiles.size() == 2 &&
            double_shot.projectiles[0].owner_kind == 0 &&
            double_shot.projectiles[1].owner_kind == 0 &&
            double_shot.projectiles[0].has_explicit_origin &&
            double_shot.projectiles[1].has_explicit_origin &&
            near(
                double_shot.projectiles[0].direction_radians,
                0.0) &&
            near(
                double_shot.projectiles[1].direction_radians,
                0.0) &&
            double_shot.projectiles[0].origin.x == 1198 &&
            double_shot.projectiles[0].origin.y == 2027 &&
            double_shot.projectiles[1].origin.x == 1198 &&
            double_shot.projectiles[1].origin.y == 1973,
        "The Double Bowgun did not preserve its two explicit "
        "eight-degree launch points and parallel trajectories.")) {
        return false;
    }
    return testPatternMatrix(*wood);
#else
    return true;
#endif
}

bool testLiveWoodBowgun() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(
            OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "00000001")) {
        return true;
    }

    osf::WorldScene world;
    osf::PlayerLoadRequest player;
    player.name = "RangedTest";
    player.gender =
        osf::playerGenderValue(osf::PlayerGender::male);
    std::string error;
    if (!check(
            world.loadInitialScenario(
                data_root,
                player,
                {1, 0, 0},
                &error),
            "The live ranged fixture could not load the outdoor "
            "scenario.")) {
        std::cerr << error << '\n';
        return false;
    }

    constexpr std::int32_t kFirstGoblinId = 101;
    const osf::ItemDefinition* bowgun =
        world.itemDatabase().find(0, 1000000);
    const osf::EnemyActor* goblin =
        findEnemy(world, kFirstGoblinId);
    const bool bowgun_equipped =
        bowgun && world.playerEquipment()
                      .place(
                          osf::EquipmentSlot::main_hand,
                          osf::makeInventoryItem(*bowgun),
                          *bowgun,
                          std::max(
                              world.playerData().level(),
                              bowgun->required_level))
                      .accepted;
    if (!check(
            bowgun && goblin && bowgun_equipped,
            "The live ranged fixture is missing its Wood Bowgun "
            "or first Goblin.")) {
        return false;
    }
    world.refreshPlayerAppearance();

    osf::ScreenPosition pointer;
    for (std::int32_t update = 0;
         update < 1000 &&
         !findEnemyPointer(
             world, kFirstGoblinId, pointer);
         ++update) {
        goblin = findEnemy(world, kFirstGoblinId);
        if (!goblin) {
            return false;
        }
        if (update % 30 == 0) {
            const osf::ScreenPosition target =
                osf::calculateRealPosition(
                    goblin->position());
            world.commandPlayerMovement(
                target.x - world.cameraScreenX(),
                target.y - world.cameraScreenY());
        }
        world.update();
        world.takeAudioSamples();
    }
    world.cancelPlayerMovement();
    goblin = findEnemy(world, kFirstGoblinId);
    if (!goblin ||
        !findEnemyPointer(
            world, kFirstGoblinId, pointer)) {
        return check(
            false,
            "The live ranged target never entered the pointer "
            "viewport.");
    }

    const std::int32_t starting_distance =
        osf::distanceBetweenBounds(
            {world.playerWorldX(), world.playerWorldY()},
            world.playerJudgement(),
            goblin->position(),
            goblin->judgement());
    const osf::WorldPosition starting_player{
        world.playerWorldX(), world.playerWorldY()};
    const std::int32_t starting_life =
        goblin->currentLife();
    const osf::InventoryItem* equipped =
        world.playerEquipment().item(
            osf::EquipmentSlot::main_hand);
    const std::int32_t starting_durability =
        equipped
            ? osf::itemCurrentDurability(
                  *equipped, *bowgun)
            : -1;

    bool saw_projectile = false;
    bool heard_launch = false;
    bool heard_contact = false;
    for (std::int32_t shot = 0;
         shot < 12;
         ++shot) {
        goblin = findEnemy(world, kFirstGoblinId);
        if (!goblin ||
            goblin->currentLife() < starting_life) {
            break;
        }
        if (!findEnemyPointer(
                world, kFirstGoblinId, pointer) ||
            !world.commandWorldInteraction(
                pointer.x, pointer.y)) {
            continue;
        }
        for (std::int32_t update = 0;
             update < 240;
             ++update) {
            world.update();
            saw_projectile =
                saw_projectile ||
                !world.runtimeEffects().empty();
            const std::vector<std::int32_t> samples =
                world.takeAudioSamples();
            heard_launch =
                heard_launch ||
                containsSample(samples, 3);
            heard_contact =
                heard_contact ||
                containsSample(samples, 20);
            goblin = findEnemy(world, kFirstGoblinId);
            if (goblin &&
                goblin->currentLife() <
                    starting_life) {
                break;
            }
            if (world.playerMotion() ==
                    osf::PlayerMotion::idle &&
                world.runtimeEffects().empty() &&
                update > 30) {
                break;
            }
        }
    }

    goblin = findEnemy(world, kFirstGoblinId);
    equipped = world.playerEquipment().item(
        osf::EquipmentSlot::main_hand);
    const std::int32_t ending_durability =
        equipped
            ? osf::itemCurrentDurability(
                  *equipped, *bowgun)
            : -1;
    return check(
        starting_distance >
                osf::kRetailPlayerAttackRange &&
            world.playerWorldX() == starting_player.x &&
            world.playerWorldY() == starting_player.y &&
            goblin &&
            goblin->currentLife() < starting_life &&
            saw_projectile &&
            heard_launch &&
            heard_contact &&
            starting_durability > ending_durability,
        "The shipped Wood Bowgun did not attack at range through "
        "the live CAF, generic projectile, receiver, audio, and "
        "durability paths.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testScaling() &&
                   testRetailBowguns() &&
                   testLiveWoodBowgun()
        ? 0
        : 1;
}
