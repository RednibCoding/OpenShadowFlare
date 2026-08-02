#include "items/item_audio.hpp"
#include "items/item_database.hpp"
#include "items/player_equipment.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/player_actor.hpp"
#include "world/player_attack_action.hpp"
#include "world/player_attack_target.hpp"
#include "world/player_data.hpp"
#include "world/player_voice.hpp"
#include "world/world_scene.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <array>
#include <string>
#include <vector>

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
                osf::retailItemAttackSound(&weapon) == 1 &&
                osf::retailPlayerAttackVoiceSample(0) == 99 &&
                osf::retailPlayerAttackVoiceSample(1) == 96 &&
                osf::retailPlayerComboVoiceSample(0, 0) == 99 &&
                osf::retailPlayerComboVoiceSample(0, 1) == 100 &&
                osf::retailPlayerComboVoiceSample(0, 2) == 101 &&
                osf::retailPlayerComboVoiceSample(1, 0) == 96 &&
                osf::retailPlayerComboVoiceSample(1, 1) == 97 &&
                osf::retailPlayerComboVoiceSample(1, 2) == 98 &&
                osf::retailPlayerDeathVoiceSample(0) == 14 &&
                osf::retailPlayerDeathVoiceSample(1) == 13,
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
    if (!check(
        osf::retailPlayerAttackAction(&weapon) ==
                osf::PlayerAttackAction::ranged_19 &&
            osf::playerAttackActionIsSupported(
                osf::PlayerAttackAction::ranged_19) &&
            osf::playerAttackActionIsRanged(
                osf::PlayerAttackAction::ranged_19),
        "Weapon subtype four did not select supported ranged action "
        "nineteen.")) {
        return false;
    }
    weapon.subtype = 5;
    return check(
        osf::retailPlayerAttackAction(&weapon) ==
                osf::PlayerAttackAction::ranged_20 &&
            osf::playerAttackActionIsSupported(
                osf::PlayerAttackAction::ranged_20) &&
            osf::playerAttackActionIsRanged(
                osf::PlayerAttackAction::ranged_20) &&
            osf::playerAttackActionIsSupported(
                osf::PlayerAttackAction::
                    increased_power_ranged_21) &&
            osf::playerAttackActionIsRanged(
                osf::PlayerAttackAction::
                    increased_power_ranged_21),
        "Weapon subtype five did not select supported ranged action "
        "twenty or leave its action-21 redirect supported.");
}

bool testRetailCombo(
    const osf::gapi::CafAnimation& animation,
    osf::PlayerComboAttackKind kind,
    const std::array<std::int32_t, 3>& expected_charts,
    const std::array<osf::PlayerAttackAction, 3>& expected_actions) {
    osf::PlayerAttackActionController attack;
    osf::PlayerAttackActionEvent event;
    if (!check(
            attack.startCombo(kind, 5, animation, 0, &event) &&
                attack.animationChart() == expected_charts[0] &&
                attack.animationFrame() == 0 &&
                event.combo_step == 0 &&
                event.action == expected_actions[0] &&
                event.target_id == -1,
            "The right-click combo did not enter its first retail phase.")) {
        return false;
    }

    std::array<std::int32_t, 3> impacts{};
    std::array<std::int32_t, 3> sounds{};
    std::array<bool, 3> saw_chart{};
    saw_chart[0] = true;
    bool saw_lunge = false;
    bool completed = false;
    for (std::int32_t update = 0;
         update < 160 && attack.active();
         ++update) {
        event = attack.update();
        if (event.combo_step < 0 || event.combo_step > 2) {
            return check(
                false,
                "The right-click combo published an invalid phase.");
        }
        const std::size_t step =
            static_cast<std::size_t>(event.combo_step);
        if (event.action != expected_actions[step]) {
            return check(
                false,
                "A combo phase published the wrong retail action.");
        }
        impacts[step] += event.impact_due ? 1 : 0;
        sounds[step] += event.swing_sound_due ? 1 : 0;
        saw_lunge = saw_lunge || event.lunge_distance > 0;
        for (std::size_t chart = 0;
             chart < expected_charts.size();
             ++chart) {
            saw_chart[chart] = saw_chart[chart] ||
                attack.animationChart() == expected_charts[chart];
        }
        completed = completed || event.completed;
    }
    return check(
        impacts == std::array<std::int32_t, 3>{1, 1, 1} &&
            sounds == std::array<std::int32_t, 3>{1, 1, 1} &&
            saw_chart == std::array<bool, 3>{true, true, true} &&
            saw_lunge && completed && !attack.active(),
        "The three right-click phases did not each swing, impact, "
        "lunge, and complete once.");
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

bool testRangedActionTiming(
    const osf::gapi::CafAnimation& animation) {
    osf::PlayerAttackAnimationTiming timing;
    if (!check(
            osf::buildPlayerAttackAnimationTiming(
                animation,
                osf::PlayerAttackAction::ranged_20,
                1,
                timing) &&
                timing.first_chart == 10 &&
                timing.recovery_chart == -1 &&
                timing.first_frame_count == 17 &&
                timing.recovery_frame_count == 0,
            "Retail ranged action twenty did not select its single "
            "chart-ten animation.")) {
        return false;
    }

    std::int32_t marker = -1;
    for (std::size_t index = 0;
         index < timing.first_frame_statuses.size();
         ++index) {
        if ((timing.first_frame_statuses[index] & 0x40) != 0) {
            marker = static_cast<std::int32_t>(index);
            break;
        }
    }
    osf::PlayerAttackActionController attack;
    osf::PlayerAttackAnimationTiming redirected_timing;
    if (!check(
            marker >= 0 &&
                osf::buildPlayerAttackAnimationTiming(
                    animation,
                    osf::PlayerAttackAction::
                        increased_power_ranged_21,
                    1,
                    redirected_timing) &&
                redirected_timing.first_chart == 10 &&
                redirected_timing.first_frame_count ==
                    timing.first_frame_count &&
                attack.start(
                    osf::PlayerAttackAction::
                        increased_power_ranged_21,
                    44,
                    5,
                    timing),
            "The retail ranged CAF marker or action start is missing.")) {
        return false;
    }
    std::int32_t marker_update = -1;
    std::int32_t sound_update = -1;
    std::int32_t completion_update = -1;
    for (std::int32_t update = 1;
         update <= timing.first_frame_count + 2;
         ++update) {
        const osf::PlayerAttackActionEvent event =
            attack.update();
        if (event.impact_due) {
            marker_update = update;
        }
        if (event.swing_sound_due) {
            sound_update = update;
        }
        if (event.completed) {
            completion_update = update;
            break;
        }
    }
    const bool faithful =
        marker == 3 &&
            marker_update == 4 &&
            sound_update == 6 &&
            completion_update == 17 &&
            attack.animationChart() == 10 &&
            attack.animationFrame() ==
                timing.first_frame_count - 1;
    if (!faithful) {
        std::cerr
            << "marker=" << marker
            << " marker update=" << marker_update
            << " sound update=" << sound_update
            << " completion update=" << completion_update
            << " frame count=" << timing.first_frame_count
            << " final frame=" << attack.animationFrame()
            << '\n';
    }
    return check(
        faithful,
        "Ranged chart-ten marker, sample-three counter, or completion "
        "timing differs.");
}

bool testRetailDeathHold(
    const osf::gapi::CafAnimation& animation) {
    const std::int32_t death_frames =
        animation.charts()[4].directions[8].frame_count;
    if (!check(
            death_frames > 0,
            "The retail death chart has no frames.")) {
        return false;
    }

    osf::PlayerActor player;
    player.reset({100, 200}, 1, 5);
    player.applyDamagePresentation({
        5, 0, 1, 0, 0, false, 0, 0.0, 1, 4,
    });
    osf::GroundMap ground;
    osf::ObjectMap objects;
    std::int32_t death_voice_requests = 0;
    for (std::int32_t update = 0;
         update < death_frames + 119;
         ++update) {
        player.update(
            ground, objects, nullptr, -1, &animation);
        if (player.takeDeathVoiceRequest()) {
            ++death_voice_requests;
        }
        if (player.takeRespawnRequest()) {
            return check(
                false,
                "The player requested revival before the retail "
                "120-update final-frame hold completed.");
        }
    }
    player.update(ground, objects, nullptr, -1, &animation);
    if (player.takeDeathVoiceRequest()) {
        ++death_voice_requests;
    }
    return check(
        player.motion() == osf::PlayerMotion::defeated &&
            player.animationChart() == 4 &&
            player.animationFrame() == death_frames - 1 &&
            death_voice_requests == 1 &&
            player.takeRespawnRequest() &&
            !player.takeRespawnRequest(),
        "The player death action did not publish one revival request "
        "after the retail final-frame hold.");
}

bool testLiveRightClickCombo(
    const std::filesystem::path& game_root) {
    osf::PlayerLoadRequest request;
    request.name = "ComboLive";
    request.gender =
        osf::playerGenderValue(osf::PlayerGender::male);
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root,
                request,
                {3000507, 3, 0},
                &error),
            "The shipped right-click combo scenario could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
            world.playerMagic().targeting() &&
                world.playerMagic().selectedSpell() == -1,
            "Gameplay entry did not select retail's normal-attack "
            "command.")) {
        return false;
    }

    const osf::ItemDefinition* one_handed = nullptr;
    for (const osf::ItemDefinition& definition :
         world.itemDatabase().definitions(0)) {
        if (definition.subtype == 0) {
            one_handed = &definition;
            break;
        }
    }
    if (!check(
            one_handed &&
                world.playerEquipment()
                    .place(
                        osf::EquipmentSlot::main_hand,
                        osf::makeInventoryItem(*one_handed),
                        *one_handed,
                        world.playerData().level())
                    .accepted,
            "The live combo fixture could not equip a one-handed weapon.")) {
        return false;
    }
    world.refreshPlayerAppearance();
    if (!check(
            world.commandPlayerMagic(480, 240) &&
                world.playerMotion() == osf::PlayerMotion::attacking &&
                world.playerAnimationChart() == 5 &&
                world.playerAttackTargetId() == -1,
            "Normal-target right-click did not start the targetless combo.")) {
        return false;
    }

    std::vector<std::int32_t> voices;
    std::int32_t initial_updates = 0;
    for (std::int32_t update = 0;
         update < 160 &&
         world.playerMotion() == osf::PlayerMotion::attacking;
         ++update) {
        world.update();
        ++initial_updates;
        for (std::int32_t sample : world.takeAudioSamples()) {
            if (sample >= 96 && sample <= 98) {
                voices.push_back(sample);
            }
        }
    }
    if (!check(
            voices == std::vector<std::int32_t>{96, 97, 98} &&
                world.playerMotion() == osf::PlayerMotion::idle,
            "The gameplay right-click path did not finish all three "
            "voiced combo phases.")) {
        return false;
    }

    if (!check(
            world.transitionScenario(
                {world.scenarioId(), 3, 0}, &error) ==
                osf::ScenarioTravelResult::relocated &&
                world.commandPlayerMagic(480, 240),
            "The combo timing fixture could not reproduce a same-entry "
            "revival transition.")) {
        std::cerr << error << '\n';
        return false;
    }
    std::int32_t relocated_updates = 0;
    while (world.playerMotion() == osf::PlayerMotion::attacking &&
           relocated_updates < 160) {
        world.update();
        ++relocated_updates;
    }
    return check(
        relocated_updates == initial_updates &&
            world.playerMotion() == osf::PlayerMotion::idle,
        "Loading and same-entry revival used different right-click combo "
        "timing.");
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
    const std::filesystem::path game_root =
        source_root / "tmp" / "ShadowFlare";
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
    if (!testRetailCombo(
            animation,
            osf::PlayerComboAttackKind::one_handed,
            {5, 7, 8},
            {
                osf::PlayerAttackAction::combo_weapon_11,
                osf::PlayerAttackAction::combo_weapon_14,
                osf::PlayerAttackAction::combo_weapon_17,
            }) ||
        !testRetailCombo(
            animation,
            osf::PlayerComboAttackKind::two_handed,
            {15, 17, 18},
            {
                osf::PlayerAttackAction::combo_weapon_12,
                osf::PlayerAttackAction::combo_weapon_15,
                osf::PlayerAttackAction::combo_weapon_18,
            })) {
        return false;
    }
    if (!testRangedActionTiming(animation)) {
        return false;
    }
    if (!testRetailDeathHold(animation)) {
        return false;
    }
    if (!testLiveRightClickCombo(game_root)) {
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
    if (!testRangedActionTiming(female_animation)) {
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
            player.jobLevel(16) == 1 &&
            player.jobLevel(5) == 0 &&
            osf::retailPlayerAttackSpeedTier(
                player.baseAttackSpeed(),
                player.baseWeightCapacity() + 1,
                player.baseWeightCapacity(),
                tables.find(4)) == 0,
        "The new-character job history or retail attack-speed tier "
        "differs.")) {
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
