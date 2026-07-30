#include "items/item_audio.hpp"
#include "items/item_database.hpp"
#include "items/player_equipment.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/player_actor.hpp"
#include "world/player_attack_action.hpp"
#include "world/player_attack_target.hpp"
#include "world/player_data.hpp"

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

osf::PlayerAttackAnimationTiming basicTiming() {
    osf::PlayerAttackAnimationTiming timing;
    timing.first_chart = 5;
    timing.recovery_chart = 6;
    timing.first_frame_count = 10;
    timing.recovery_frame_count = 9;
    timing.first_frame_statuses.assign(10, 0);
    timing.first_frame_statuses[7] = 0x40;
    return timing;
}

bool testActionSelectionAndAudio() {
    osf::ItemDefinition weapon;
    weapon.category = 0;
    weapon.weight = 59;
    if (!check(
            osf::retailPlayerAttackAction(nullptr) ==
                    osf::PlayerAttackAction::basic &&
                osf::retailItemAttackSound(nullptr) == 1 &&
                osf::retailItemAttackSound(&weapon) == 1,
            "The empty-hand action or light attack sound differs.")) {
        return false;
    }
    weapon.weight = 60;
    if (!check(
            osf::retailItemAttackSound(&weapon) == 2,
            "The retail 60-weight attack-sound boundary differs.")) {
        return false;
    }

    weapon.subtype = 0;
    if (!check(
            osf::retailPlayerAttackAction(&weapon) ==
                osf::PlayerAttackAction::weapon_8,
            "Weapon subtype zero did not select action eight.")) {
        return false;
    }
    weapon.subtype = 3;
    if (!check(
            osf::retailPlayerAttackAction(&weapon) ==
                osf::PlayerAttackAction::weapon_9,
            "Weapon subtype three did not select action nine.")) {
        return false;
    }
    weapon.subtype = 1;
    if (!check(
            osf::retailPlayerAttackAction(&weapon) ==
                osf::PlayerAttackAction::weapon_10,
            "Weapon subtype one did not select action ten.")) {
        return false;
    }
    weapon.subtype = 4;
    return check(
        osf::retailPlayerAttackAction(&weapon) ==
                osf::PlayerAttackAction::ranged_19 &&
            !osf::playerAttackActionIsSupported(
                osf::PlayerAttackAction::ranged_19),
        "The still-deferred ranged action was treated as melee.");
}

bool testBasicActionTiming() {
    osf::PlayerAttackActionController attack;
    osf::PlayerAttackActionEvent event;
    if (!check(
            attack.start(
                osf::PlayerAttackAction::basic,
                17,
                4,
                basicTiming(),
                &event) &&
                attack.active() &&
                attack.animationChart() == 5 &&
                attack.animationFrame() == 0 &&
                !event.impact_due &&
                !event.swing_sound_due,
            "The retail basic attack did not enter chart five frame zero.")) {
        return false;
    }

    std::int32_t impacts = 0;
    std::int32_t sounds = 0;
    std::int32_t completion_update = -1;
    for (std::int32_t update = 1; update <= 24; ++update) {
        event = attack.update();
        impacts += event.impact_due ? 1 : 0;
        sounds += event.swing_sound_due ? 1 : 0;
        if (event.impact_due &&
            (update != 8 || event.target_id != 17)) {
            return check(
                false,
                "The basic attack crossed its CAF impact marker "
                "on the wrong update.");
        }
        if (event.swing_sound_due && update != 5) {
            return check(
                false,
                "The basic attack emitted its swing sound on the "
                "wrong counter.");
        }
        if (event.completed) {
            completion_update = update;
            break;
        }
    }
    return check(
        impacts == 1 &&
            sounds == 1 &&
            completion_update == 19 &&
            !attack.active() &&
            attack.animationChart() == 6 &&
            attack.animationFrame() == 8,
        "The basic attack/recovery timing or final frame differs.");
}

bool testWeaponAndFastMarkerTiming() {
    osf::PlayerAttackActionController attack;
    if (!check(
            attack.start(
                osf::PlayerAttackAction::weapon_8,
                8,
                4,
                basicTiming()),
            "The chart-five weapon action did not start.")) {
        return false;
    }

    std::int32_t impact_update = -1;
    std::int32_t sound_update = -1;
    std::int32_t completion_update = -1;
    for (std::int32_t update = 1; update <= 24; ++update) {
        const osf::PlayerAttackActionEvent event =
            attack.update();
        if (event.impact_due) {
            impact_update = update;
        }
        if (event.swing_sound_due) {
            sound_update = update;
        }
        if (event.completed) {
            completion_update = update;
            break;
        }
    }
    if (!check(
            impact_update == 7 &&
                sound_update == 6 &&
                completion_update == 18,
            "Action eight did not preserve its distinct counter order.")) {
        return false;
    }

    attack.start(
        osf::PlayerAttackAction::weapon_8,
        9,
        9,
        basicTiming());
    bool crossed_marker = false;
    for (std::int32_t update = 0; update < 8; ++update) {
        crossed_marker =
            attack.update().impact_due || crossed_marker;
    }
    if (!check(
        crossed_marker,
        "A fast attack skipped a crossed CAF impact marker.")) {
        return false;
    }

    attack.start(
        osf::PlayerAttackAction::weapon_8,
        10,
        4,
        basicTiming());
    attack.update();
    attack.update(9);
    return check(
        attack.displayedFrame() == 3,
        "An active attack retained a stale attack-speed tier.");
}

bool testPlayerMovementLock(
    const osf::gapi::CafAnimation& animation) {
    osf::PlayerActor player;
    player.reset({100, 200}, 1, 5);
    player.moveTo({1000, 200});
    player.faceToward({200, 200});
    if (!check(
            player.beginAttack(
                osf::PlayerAttackAction::basic,
                33,
                4,
                animation),
            "The player actor could not start the retail attack CAF.")) {
        return false;
    }
    player.moveTo({2000, 200});
    player.cancelMovement();

    osf::GroundMap ground;
    osf::ObjectMap objects;
    player.update(ground, objects);
    if (!check(
            player.position().x == 100 &&
                player.position().y == 200 &&
                player.motion() == osf::PlayerMotion::attacking &&
                player.attackTargetId() == 33,
            "Movement input displaced or cancelled an active attack.")) {
        return false;
    }

    for (std::int32_t update = 0;
         update < 32 && player.attackActive();
         ++update) {
        player.update(ground, objects);
    }
    return check(
        !player.attackActive() &&
            player.motion() == osf::PlayerMotion::idle &&
            player.position().x == 100 &&
            player.position().y == 200,
        "The player did not unlock cleanly after attack recovery.");
}

bool testRetailAssetsAndSpeedTable() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path source_root =
        OPENSHADOWFLARE_SOURCE_DIR;
    const std::filesystem::path male_caf =
        source_root / "tmp" / "ShadowFlare" / "Player" /
        "Male" / "Animation00.Caf";
    const std::filesystem::path female_caf =
        source_root / "tmp" / "ShadowFlare" / "Player" /
        "Female" / "Animation00.Caf";
    const std::filesystem::path table_path =
        source_root / "tmp" / "ShadowFlare" / "System" /
        "Game" / "Parameter" / "Table.Tbd";
    const std::filesystem::path item_path =
        source_root / "tmp" / "ShadowFlare" / "System" /
        "Game" / "Parameter" / "Item.Ibn";
    if (!std::filesystem::is_regular_file(male_caf) ||
        !std::filesystem::is_regular_file(female_caf) ||
        !std::filesystem::is_regular_file(table_path) ||
        !std::filesystem::is_regular_file(item_path)) {
        return true;
    }

    std::string error;
    osf::gapi::CafAnimation animation;
    osf::PlayerAttackAnimationTiming timing;
    if (!check(
            animation.load(male_caf, &error) &&
                osf::buildPlayerAttackAnimationTiming(
                    animation,
                    osf::PlayerAttackAction::basic,
                    0,
                    timing) &&
                timing.first_chart == 5 &&
                timing.recovery_chart == 6 &&
                timing.first_frame_count == 10 &&
                timing.recovery_frame_count == 9 &&
                timing.first_frame_statuses.size() >= 8 &&
                (timing.first_frame_statuses[7] & 0x40) != 0,
            "The male retail attack charts or impact marker changed.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!testPlayerMovementLock(animation)) {
        return false;
    }
    osf::gapi::CafAnimation female_animation;
    osf::PlayerAttackAnimationTiming female_timing;
    if (!check(
            female_animation.load(female_caf, &error) &&
                osf::buildPlayerAttackAnimationTiming(
                    female_animation,
                    osf::PlayerAttackAction::basic,
                    0,
                    female_timing) &&
                female_timing.first_frame_count == 10 &&
                female_timing.recovery_frame_count == 9 &&
                female_timing.first_frame_statuses.size() >= 8 &&
                (female_timing.first_frame_statuses[7] & 0x40) != 0,
            "The female retail attack charts or impact marker changed.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::TableDatabase tables;
    osf::PlayerData player;
    if (!check(
            tables.load(table_path, &error) &&
                player.initializeNew("Mina", 0, tables, &error),
            "The retail attack-speed fixture could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const std::int32_t tier =
        osf::retailPlayerAttackSpeedTier(
            player.baseAttackSpeed(),
            0,
            player.baseWeightCapacity(),
            tables.find(4));
    if (!check(
        tier == 5 &&
            osf::retailPlayerAttackSpeedTier(
                player.baseAttackSpeed(),
                player.baseWeightCapacity() + 1,
                player.baseWeightCapacity(),
                tables.find(4)) == 0,
        "The new-character or overweight retail attack-speed tier differs.")) {
        return false;
    }

    osf::ItemDatabase items;
    if (!check(
            items.load(item_path, &error),
            "The equipment attack-speed fixture could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const osf::ItemDefinition* two_handed = nullptr;
    const osf::ItemDefinition* off_hand = nullptr;
    for (const osf::ItemDefinition& definition :
         items.definitions(0)) {
        if (definition.suppresses_off_hand) {
            two_handed = &definition;
            break;
        }
    }
    for (const osf::ItemDefinition& definition :
         items.definitions(1)) {
        if (definition.subtype == 2) {
            off_hand = &definition;
            break;
        }
    }
    if (!check(
            two_handed && off_hand,
            "The retail two-hand/off-hand fixture is missing.")) {
        return false;
    }
    osf::PlayerEquipment equipment;
    equipment.place(
        osf::EquipmentSlot::main_hand,
        osf::makeInventoryItem(*two_handed),
        *two_handed,
        100);
    equipment.place(
        osf::EquipmentSlot::off_hand,
        osf::makeInventoryItem(*off_hand),
        *off_hand,
        100);
    return check(
        equipment.totalWeight(items) == two_handed->weight &&
            equipment.derivedParameterBonus(8, items) ==
                two_handed->derived_parameter_bonuses[8],
        "A suppressed off hand still changed attack weight or speed.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    if (!testActionSelectionAndAudio() ||
        !testBasicActionTiming() ||
        !testWeaponAndFastMarkerTiming() ||
        !testRetailAssetsAndSpeedTable()) {
        return 1;
    }
    return 0;
}
