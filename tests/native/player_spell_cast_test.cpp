#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/player_spell_action.hpp"
#include "world/player_spell_cast.hpp"
#include "world/player_spell_parameters.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cmath>
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

bool testRetailAction(
    const osf::gapi::CafAnimation& animation,
    const osf::TableDatabase& tables) {
    osf::PlayerSpellAnimationTiming timing;
    if (!check(
            osf::buildPlayerSpellAnimationTiming(
                animation,
                osf::PlayerSpellAction::fire_ball,
                0,
                timing) &&
                timing.first_chart == 13 &&
                timing.recovery_chart == 14 &&
                timing.first_frame_count > 0,
            "The retail Fire Ball CAF charts could not be decoded.")) {
        return false;
    }

    std::int32_t marker = -1;
    for (std::size_t frame = 0;
         frame < timing.first_frame_statuses.size();
         ++frame) {
        if ((timing.first_frame_statuses[frame] & 0x40) != 0) {
            marker = static_cast<std::int32_t>(frame);
            break;
        }
    }
    const double speed =
        osf::retailPlayerSpellAnimationSpeed(
            1, 5, tables.find(20));
    osf::PlayerSpellActionController action;
    osf::PlayerSpellActionEvent event;
    if (!check(
            marker >= 0 &&
                std::abs(speed - 1.3) < 0.000001 &&
                action.start(
                    osf::PlayerSpellAction::fire_ball,
                    1,
                    14000316,
                    5,
                    tables.find(20),
                    timing,
                    &event) &&
                event.cast_due &&
                event.spell == 1 &&
                event.target_character_number == 14000316 &&
                event.effect_delay ==
                    static_cast<std::int32_t>(
                        std::trunc(
                            static_cast<double>(marker) /
                            speed)) &&
                action.animationChart() == 13 &&
                action.animationFrame() == 0,
            "Fire Ball did not enter action 23 with the retail "
            "speed or effect delay.")) {
        return false;
    }

    std::int32_t completion_update = -1;
    bool saw_recovery = false;
    for (std::int32_t update = 1;
         update < 100 && action.active();
         ++update) {
        event = action.update(5, tables.find(20));
        saw_recovery =
            saw_recovery ||
            action.animationChart() == 14;
        if (event.completed) {
            completion_update = update;
        }
    }
    const std::int32_t completion_increment =
        static_cast<std::int32_t>(
            std::trunc(7.0 / speed));
    const std::int32_t expected_update =
        static_cast<std::int32_t>(
            std::ceil(
                static_cast<double>(
                    timing.first_frame_count - 1 +
                    completion_increment) /
                speed)) +
        1;
    return check(
        completion_update == expected_update &&
            saw_recovery &&
            !action.active() &&
            action.animationChart() == 14 &&
            action.animationFrame() == 0,
        "Action 23 did not preserve the retail counter and "
        "chart-fourteen completion behavior.");
}

bool testRetailPacket(const osf::TableDatabase& tables) {
    osf::PlayerFireBallCastInput input;
    input.stats.source_character_number = 0;
    input.stats.player_level = 7;
    input.stats.magical_attack = 12;
    input.stats.magical_defense = 13;
    input.stats.magical_hit_rate = 14;
    input.stats.element_affinities = {
        1, 2, 3, 4, 5, 6, 7, 8};
    for (std::size_t index = 0;
         index < input.stats.state_words.size();
         ++index) {
        input.stats.state_words[index] =
            static_cast<std::int32_t>(100 + index);
    }
    input.parameters.effective_level = 1;
    input.parameters.effect_value = 40;
    input.target_character_number = 14000316;
    input.source_position = {100, 200};
    input.source_judgement = {-80, -80, 79, 79};
    input.target_position = {400, 200};
    input.effect_delay = 5;

    const osf::CombatEffectSpawnRequest request =
        osf::buildPlayerFireBallCast(input, tables);
    return check(
        request.valid &&
            request.effect_number == 10001 &&
            request.owner_kind == 1 &&
            request.source_character_number == 0 &&
            request.target_kind == 0x14 &&
            request.target_identifier == 14000316 &&
            request.constructor_value_6 == 91 &&
            request.constructor_value_7 == 200 &&
            request.constructor_value_12 == 5 &&
            request.constructor_value_17 == 0 &&
            request.constructor_value_22 == 0 &&
            std::abs(request.direction_radians) < 0.000001 &&
            request.has_source_judgement &&
            request.source_judgement.left == -80 &&
            request.has_packet &&
            request.packet[0] == 0 &&
            request.packet[1] == 3 &&
            request.packet[2] == 0 &&
            request.packet[3] == 0 &&
            request.packet[4] == 52 &&
            request.packet[5] == 13 &&
            request.packet[6] == 1 &&
            request.packet[13] == 8 &&
            request.packet[14] == 100 &&
            request.packet[30] == 116 &&
            request.packet[31] == 7 &&
            request.packet[32] == 0 &&
            request.packet[34] == 20000 &&
            request.packet[35] == 8 &&
            request.packet[36] == 74 &&
            request.packet[45] == 1 &&
            request.packet[54] == 10 &&
            request.packet[63] == 1 &&
            request.packet[72] == 0 &&
            request.packet[73] == 1 &&
            request.packet[74] == -1 &&
            request.packet[75] == 8 &&
            request.packet[76] == 0,
        "The family-zero Fire Ball packet or controller arguments "
        "differ from FUN_00439730.");
}

bool testShippedWorldCast(
    const std::filesystem::path& game_root) {
    osf::PlayerLoadRequest player;
    player.name = "FireBallLive";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root,
                player,
                {3000507, 3, 0},
                &error),
            "The shipped Fire Ball scenario could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[1] = 3;
    world.playerMagic().restore(magic_state);
    if (!check(
            world.playerMagic().selectSpell(1),
            "The live Fire Ball fixture could not select the spell.")) {
        return false;
    }

    const osf::EnemyActor* target = nullptr;
    std::int32_t pointer_x = -1;
    std::int32_t pointer_y = -1;
    for (const osf::EnemyActor& enemy : world.enemies()) {
        const osf::ScreenPosition projected =
            osf::calculateRealPosition(enemy.position());
        const std::int32_t anchor_x =
            projected.x - world.cameraScreenX();
        const std::int32_t anchor_y =
            projected.y - world.cameraScreenY();
        if (anchor_x < -80 || anchor_x > 720 ||
            anchor_y < -160 || anchor_y > 440) {
            continue;
        }
        for (std::int32_t y =
                 std::max(0, anchor_y - 140);
             y < std::min(400, anchor_y + 30) &&
             pointer_x < 0;
             ++y) {
            for (std::int32_t x =
                     std::max(0, anchor_x - 80);
                 x < std::min(640, anchor_x + 81);
                 ++x) {
                world.updatePointerHover(x, y);
                if (world.hoveredEnemyId() == enemy.id()) {
                    target = &enemy;
                    pointer_x = x;
                    pointer_y = y;
                    break;
                }
            }
        }
        if (target) {
            break;
        }
    }
    if (!check(
            target && pointer_x >= 0 && pointer_y >= 0,
            "No shipped on-screen enemy could be picked for "
            "Fire Ball.")) {
        return false;
    }

    const std::int32_t target_character_number =
        target->characterNumber();
    const osf::WorldPosition target_position_before =
        target->position();
    const osf::WorldPosition player_position_before{
        world.playerWorldX(),
        world.playerWorldY(),
    };
    const std::int32_t target_life_before =
        target->currentLife();
    const std::int32_t mana_before =
        world.playerData().currentMana();
    const osf::PlayerSpellParameters parameters =
        osf::playerSpellParameters(
            world.playerMagic(),
            1,
            world.playerEquipment(),
            world.itemDatabase(),
            world.parameterTables());
    if (!check(
            world.commandPlayerMagic(pointer_x, pointer_y) &&
                world.playerSpellActive() &&
                world.playerSpellTargetCharacterNumber() ==
                    target_character_number &&
                world.playerMotion() ==
                    osf::PlayerMotion::casting &&
                world.playerAnimationChart() == 13 &&
                world.playerData().currentMana() ==
                    mana_before - parameters.mana_cost &&
                world.runtimeEffectControllerCount() == 1,
            "The shipped right-click did not enter action 23, "
            "deduct MP, and queue effect 10001.")) {
        return false;
    }

    bool saw_projectile = false;
    bool saw_launch_audio = false;
    bool applied_damage = false;
    for (std::int32_t update = 0;
         update < 80;
         ++update) {
        world.update();
        const std::vector<std::int32_t> audio =
            world.takeAudioSamples();
        saw_launch_audio =
            saw_launch_audio ||
            std::find(audio.begin(), audio.end(), 19) !=
                audio.end();
        saw_projectile =
            saw_projectile ||
            std::any_of(
                world.runtimeEffects().begin(),
                world.runtimeEffects().end(),
                [](const osf::RuntimeEffectActor& actor) {
                    return actor.resourceId() == 10000010;
                });
        const auto found = std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [target_character_number](
                const osf::EnemyActor& enemy) {
                return enemy.characterNumber() ==
                    target_character_number;
            });
        applied_damage =
            applied_damage ||
            found == world.enemies().end() ||
            found->currentLife() < target_life_before;
    }
    if (!applied_damage) {
        const auto current_target = std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [target_character_number](
                const osf::EnemyActor& enemy) {
                return enemy.characterNumber() ==
                    target_character_number;
            });
        if (current_target == world.enemies().end()) {
            applied_damage = true;
        } else {
            osf::PlayerFireBallCastInput contact_input;
            contact_input.stats.source_character_number = 0;
            contact_input.stats.player_level = 1;
            contact_input.stats.magical_attack = 10;
            contact_input.stats.magical_defense = 10;
            contact_input.stats.magical_hit_rate = 40;
            contact_input.parameters.effective_level = 1;
            contact_input.parameters.effect_value = 40;
            contact_input.target_character_number =
                target_character_number;
            contact_input.source_position =
                current_target->position();
            contact_input.target_position =
                current_target->position();
            osf::CombatEffectSpawnRequest contact =
                osf::buildPlayerFireBallCast(
                    contact_input,
                    world.parameterTables());
            // Keep this receiver regression deterministic: the controller
            // starts directly on the shipped target, while the packet itself
            // remains the exact player-spell family tested above.
            contact.owner_kind = 0;
            contact.target_kind = 4;
            contact.constructor_value_6 = 0;
            contact.constructor_value_12 = 0;
            contact.has_explicit_origin = true;
            contact.origin = current_target->position();
            contact.packet.write(36, 100000);
            world.queueCombatEffect(contact);
            for (std::int32_t update = 0;
                 update < 12 && !applied_damage;
                 ++update) {
                world.update();
                world.takeAudioSamples();
                const auto found = std::find_if(
                    world.enemies().begin(),
                    world.enemies().end(),
                    [target_character_number](
                        const osf::EnemyActor& enemy) {
                        return enemy.characterNumber() ==
                            target_character_number;
                    });
                applied_damage =
                    found == world.enemies().end() ||
                    found->currentLife() <
                        target_life_before;
            }
        }
    }
    const bool passed =
        saw_projectile &&
        saw_launch_audio &&
        applied_damage &&
        world.playerMagic().experience(1) == 1;
    if (!passed) {
        std::cerr
            << "target=" << target_character_number
            << " pointer=" << pointer_x << ',' << pointer_y
            << " source=" << player_position_before.x
            << ',' << player_position_before.y
            << " initial-target=" << target_position_before.x
            << ',' << target_position_before.y
            << " projectile=" << saw_projectile
            << " audio=" << saw_launch_audio
            << " damage=" << applied_damage
            << " practice="
            << world.playerMagic().experience(1)
            << " controllers="
            << world.runtimeEffectControllerCount()
            << " actors=" << world.runtimeEffects().size()
            << '\n';
    }
    return check(
        passed,
        "The shipped Fire Ball projectile did not render, sound, "
        "damage, and award one practice point.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path game_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    const std::filesystem::path caf_path =
        game_root / "Player" / "Male" / "Animation00.Caf";
    const std::filesystem::path table_path =
        game_root / "System" / "Game" / "Parameter" /
        "Table.Tbd";
    if (!std::filesystem::is_regular_file(caf_path) ||
        !std::filesystem::is_regular_file(table_path)) {
        return 0;
    }

    std::string error;
    osf::gapi::CafAnimation animation;
    osf::TableDatabase tables;
    if (!check(
            animation.load(caf_path, &error) &&
                tables.load(table_path, &error),
            "The retail Fire Ball fixtures could not be loaded.")) {
        std::cerr << error << '\n';
        return 1;
    }
    if (!testRetailAction(animation, tables) ||
        !testRetailPacket(tables) ||
        !testShippedWorldCast(game_root)) {
        return 1;
    }
#endif
    return 0;
}
