#include "core/retail_random.hpp"
#include "items/item_database.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/player_attack_impact.hpp"
#include "world/player_data.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstdint>
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

osf::PlayerAttackImpactStats stats() {
    osf::PlayerAttackImpactStats stats;
    stats.source_character_number = 0;
    stats.level = 7;
    stats.physical_attack = 100;
    stats.physical_defense = 40;
    stats.hit_rate = 120;
    for (std::size_t index = 0;
         index < stats.element_affinities.size();
         ++index) {
        stats.element_affinities[index] =
            static_cast<std::int32_t>(index) - 4;
    }
    for (std::size_t index = 0;
         index < stats.state_words.size();
         ++index) {
        stats.state_words[index] =
            static_cast<std::int32_t>(100 + index);
    }
    stats.reflection_chance = 50;
    stats.reflection_percent = 25;
    stats.reaction_motion = 1;
    stats.reaction_chance_modifier = 8;
    stats.reaction_duration_modifier = 9;
    stats.weapon_identifier = 1234;
    stats.weapon_subtype = 8;
    return stats;
}

bool testHitPacketAndRandomOrder() {
    osf::RetailRandom random(1);
    const osf::PlayerAttackImpactResult result =
        osf::resolvePlayerAttackImpact(
            {stats(), 12, 20},
            random);
    osf::RetailRandom expected(1);
    expected.next();
    expected.next();
    expected.next();
    expected.next();
    if (!check(
            result.valid &&
                result.target_id == 12 &&
                result.hit_chance == 98 &&
                result.hit_roll == 41 &&
                !result.show_miss &&
                result.apply_damage &&
                result.post_hit_audio_sample == 6 &&
                result.packet[0] == 0 &&
                result.packet[1] == 0 &&
                result.packet[2] == 0 &&
                result.packet[3] == 0 &&
                result.packet[4] == 100 &&
                result.packet[5] == 40 &&
                result.packet[6] == -4 &&
                result.packet[13] == 3 &&
                result.packet[14] == 100 &&
                result.packet[30] == 116 &&
                result.packet[31] == 7 &&
                result.packet[34] == 21005 &&
                result.packet[35] == 8 &&
                result.packet[37] == 0 &&
                result.packet[38] == 1 &&
                result.packet[39] == 25 &&
                result.packet[40] == 1 &&
                result.packet[41] == -1 &&
                result.packet[42] == 8 &&
                result.packet[43] == -1 &&
                result.packet[44] == 9 &&
                result.packet[72] == 1 &&
                result.packet[73] == -1 &&
                result.packet[74] == 1234 &&
                result.packet[75] == 8 &&
                result.packet[76] == 0 &&
                random.state() == expected.state(),
            "The player hit packet fields or retail random-call order "
            "differed.")) {
        return false;
    }
    return check(
        result.packet.written_words.test(6) &&
            result.packet.written_words.test(30) &&
            !result.packet.written_words.test(32) &&
            !result.packet.written_words.test(33) &&
            !result.packet.written_words.test(36),
        "The player packet initialized words that retail leaves "
        "outside this attack path.");
}

bool testMissAndDurabilityGates() {
    osf::PlayerAttackImpactStats weak = stats();
    weak.hit_rate = 0;
    osf::RetailRandom miss_random(18);
    const osf::PlayerAttackImpactResult miss =
        osf::resolvePlayerAttackImpact(
            {weak, 3, 100},
            miss_random);
    osf::RetailRandom expected_miss(18);
    expected_miss.next();
    if (!check(
            miss.valid &&
                miss.hit_chance == 20 &&
                miss.hit_roll == 97 &&
                miss.show_miss &&
                !miss.apply_damage &&
                miss.post_hit_audio_sample == -1 &&
                miss.packet.written_words.none() &&
                miss_random.state() ==
                    expected_miss.state(),
            "A missed player attack built a packet or consumed "
            "post-hit random draws.")) {
        return false;
    }

    osf::ItemDefinition weapon;
    weapon.category = 0;
    weapon.maximum_durability = 300;
    osf::RetailRandom wear_random(19);
    const osf::PlayerAttackDurabilityResult wear =
        osf::resolvePlayerAttackDurability(
            &weapon, wear_random);
    osf::RetailRandom no_wear_random(7);
    const osf::PlayerAttackDurabilityResult no_wear =
        osf::resolvePlayerAttackDurability(
            &weapon, no_wear_random);
    osf::RetailRandom no_weapon_random(9);
    const osf::PlayerAttackDurabilityResult no_weapon =
        osf::resolvePlayerAttackDurability(
            nullptr, no_weapon_random);
    weapon.maximum_durability = 0;
    osf::RetailRandom indestructible_random(19);
    const osf::PlayerAttackDurabilityResult
        indestructible =
            osf::resolvePlayerAttackDurability(
                &weapon, indestructible_random);
    return check(
        wear.checked &&
            wear.roll == 0 &&
            wear.lose_durability &&
            no_wear.checked &&
            no_wear.roll == 61 &&
            !no_wear.lose_durability &&
            !no_weapon.checked &&
            no_weapon_random.state() == 9 &&
            indestructible.checked &&
            indestructible.roll == 0 &&
            !indestructible.lose_durability,
        "The post-receiver 30-percent weapon durability gate "
        "differed.");
}

bool testReceiverApplication() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path tables_path =
        std::filesystem::path(
            OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "System" / "Game" /
        "Parameter" / "Table.Tbd";
    if (!std::filesystem::is_regular_file(tables_path)) {
        return true;
    }
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(tables_path, &error),
            "The retail combat tables could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerAttackImpactStats ordinary = stats();
    ordinary.weapon_subtype = 0;
    ordinary.weapon_identifier = -1;
    ordinary.reflection_chance = 0;
    ordinary.reflection_percent = 0;
    ordinary.reaction_motion = 0;
    ordinary.reaction_chance_modifier = 0;
    ordinary.reaction_duration_modifier = 0;
    ordinary.state_words.fill(0);
    ordinary.element_affinities.fill(0);

    osf::EnemyDamageReceiverState enemy;
    enemy.character_number = 14000012;
    enemy.scenario_number = 6;
    enemy.position = {100, 0};
    enemy.judgement = {-20, -20, 19, 19};
    enemy.has_visual = true;
    enemy.current_life = 500;
    enemy.maximum_life = 500;
    enemy.native_element = 0;
    enemy.physical_defense = 10;
    enemy.magical_defense = 20;
    enemy.presentation_action = 7;

    osf::EnemyDamageReceiverContext context;
    context.local_player_slot = 0;
    context.local_player_available = true;
    context.source_player_available = true;
    context.source_player_position = {0, 0};
    osf::RetailRandom random(1);
    const osf::PlayerAttackApplicationResult applied =
        osf::resolvePlayerAttackAgainstEnemy(
            {ordinary, 12, 20},
            enemy,
            {0, 0},
            context,
            nullptr,
            tables,
            random);
    return check(
        applied.impact.apply_damage &&
            applied.receiver.valid &&
            applied.receiver.accepted &&
            applied.receiver.damage.valid &&
            applied.receiver.damage.damage > 0 &&
            applied.receiver.state.current_life ==
                500 -
                    applied.receiver.damage.damage &&
            applied.receiver.state.attributed_damage[0] ==
                applied.receiver.damage.damage &&
            applied.receiver.state.event_number == 4 &&
            !applied.durability.checked,
        "A validated player impact did not pass through the shared "
        "enemy receiver and mutate its returned life state.");
#else
    return true;
#endif
}

bool testRetailStatBuilder() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(
            OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    const std::filesystem::path tables_path =
        data_root / "System" / "Game" / "Parameter" /
        "Table.Tbd";
    const std::filesystem::path items_path =
        data_root / "System" / "Game" / "Parameter" /
        "Item.Ibn";
    if (!std::filesystem::is_regular_file(tables_path) ||
        !std::filesystem::is_regular_file(items_path)) {
        return true;
    }

    std::string error;
    osf::TableDatabase tables;
    osf::ItemDatabase items;
    osf::PlayerData player;
    if (!check(
            tables.load(tables_path, &error) &&
                items.load(items_path, &error) &&
                player.initializeNew(
                    "Mina", 0, tables, &error),
            "The retail player attack-stat fixtures could not "
            "be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const auto found = std::find_if(
        items.definitions(0).begin(),
        items.definitions(0).end(),
        [](const osf::ItemDefinition& definition) {
            return definition.name == "Short Sword";
        });
    if (!check(
            found != items.definitions(0).end(),
            "The retail Short Sword fixture is missing.")) {
        return false;
    }

    osf::PlayerEquipment equipment;
    osf::PlayerInventory inventory;
    if (!check(
            equipment.place(
                osf::EquipmentSlot::main_hand,
                osf::makeInventoryItem(*found),
                *found,
                player.level()).accepted,
            "The retail Short Sword fixture could not be equipped.")) {
        return false;
    }
    const osf::PlayerAttackImpactStats built =
        osf::buildPlayerAttackImpactStats(
            0, player, equipment, inventory, items);
    if (!check(
            player.basePhysicalAttack() ==
                    player.initialParameter(5) &&
                player.basePhysicalDefense() ==
                    player.initialParameter(6) &&
                player.baseHitRate() ==
                    player.initialParameter(9) &&
                player.baseEvasionRate() ==
                    player.initialParameter(10) &&
                built.physical_attack ==
                    player.basePhysicalAttack() + 20 &&
                built.physical_defense ==
                    player.basePhysicalDefense() &&
                built.hit_rate ==
                    player.baseHitRate() + 100 &&
                built.weapon_identifier == found->id &&
                built.weapon_subtype == found->subtype,
            "The named retail base rows or equipped attack-stat "
            "builder differ.")) {
        return false;
    }

    const std::int32_t weight =
        equipment.totalWeight(items);
    equipment.decreaseDurability(
        osf::EquipmentSlot::main_hand,
        found->maximum_durability);
    const osf::PlayerAttackImpactStats broken =
        osf::buildPlayerAttackImpactStats(
            0, player, equipment, inventory, items);
    return check(
        equipment.totalWeight(items) == weight &&
            broken.physical_attack ==
                player.basePhysicalAttack() &&
            broken.hit_rate == player.baseHitRate() &&
            broken.weapon_identifier == found->id,
        "A broken weapon changed carried weight or kept its "
        "derived combat bonuses.");
#else
    return true;
#endif
}

bool findVisibleEnemyPointer(
    osf::WorldScene& world,
    std::int32_t& enemy_id,
    osf::ScreenPosition& point) {
    for (const osf::EnemyActor& enemy :
         world.enemies()) {
        if (enemy_id >= 0 &&
            enemy.id() != enemy_id) {
            continue;
        }
        if (enemy.currentLife() < 1 ||
            !enemy.visible() ||
            !enemy.pointerEnabled()) {
            continue;
        }
        const osf::ScreenPosition anchor =
            osf::calculateRealPosition(
                enemy.position());
        for (std::int32_t y = -enemy.labelHeight();
             y <= 24;
             ++y) {
            for (std::int32_t x = -64;
                 x <= 64;
                 ++x) {
                point = {
                    anchor.x -
                        world.cameraScreenX() + x,
                    anchor.y -
                        world.cameraScreenY() + y,
                };
                if (point.x < 0 || point.x >= 640 ||
                    point.y < 0 || point.y >= 480) {
                    continue;
                }
                world.updatePointerHover(
                    point.x, point.y);
                if (world.hoveredEnemyId() ==
                    enemy.id()) {
                    enemy_id = enemy.id();
                    return true;
                }
            }
        }
    }
    return false;
}

bool testLiveWorldMutationAndAudio() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(
            OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "00000006")) {
        return true;
    }
    osf::WorldScene world;
    osf::PlayerLoadRequest player;
    player.name = "Impact";
    std::string error;
    if (!check(
            world.loadInitialScenario(
                data_root,
                player,
                {6, 4, 0},
                &error),
            "The live player-impact world could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    std::int32_t enemy_id = -1;
    osf::ScreenPosition pointer;
    if (!check(
            findVisibleEnemyPointer(
                world, enemy_id, pointer),
            "The retail combat fixture has no visible selectable "
            "enemy.")) {
        return false;
    }
    const auto enemyLife =
        [&world, enemy_id]() {
            const auto found = std::find_if(
                world.enemies().begin(),
                world.enemies().end(),
                [enemy_id](const osf::EnemyActor& enemy) {
                    return enemy.id() == enemy_id;
                });
            return found == world.enemies().end()
                ? -1
                : found->currentLife();
        };
    const std::int32_t initial_life = enemyLife();
    bool hit = false;
    bool heard_hit = false;
    for (std::int32_t attempt = 0;
         attempt < 10 && !hit;
         ++attempt) {
        if (!findVisibleEnemyPointer(
                world, enemy_id, pointer) ||
            !world.commandWorldInteraction(
                pointer.x, pointer.y)) {
            return check(
                false,
                "A visible retail enemy rejected the player "
                "attack command.");
        }
        bool impact_seen = false;
        for (std::int32_t update = 0;
             update < 5000;
             ++update) {
            world.update();
            if (world.takePlayerAttackImpactTargetId() ==
                enemy_id) {
                impact_seen = true;
            }
            const std::vector<std::int32_t> samples =
                world.takeAudioSamples();
            heard_hit =
                heard_hit ||
                std::find(
                    samples.begin(),
                    samples.end(),
                    6) != samples.end();
            if (impact_seen &&
                world.playerMotion() ==
                    osf::PlayerMotion::idle) {
                break;
            }
        }
        hit = enemyLife() < initial_life;
    }
    return check(
        hit &&
            heard_hit &&
            enemyLife() >= 0 &&
            enemyLife() < initial_life,
        "The live CAF impact did not mutate enemy life and queue "
        "the retail post-hit sample.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testHitPacketAndRandomOrder() &&
                   testMissAndDurabilityGates() &&
                   testReceiverApplication() &&
                   testRetailStatBuilder() &&
                   testLiveWorldMutationAndAudio()
               ? 0
               : 1;
}
