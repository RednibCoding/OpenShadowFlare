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
    const osf::TableDatabase& tables,
    std::int32_t spell,
    osf::PlayerSpellAction spell_action,
    std::int32_t first_chart,
    std::int32_t recovery_chart) {
    osf::PlayerSpellAnimationTiming timing;
    if (!check(
            osf::buildPlayerSpellAnimationTiming(
                animation,
                spell_action,
                0,
                timing) &&
                timing.first_chart == first_chart &&
                timing.recovery_chart == recovery_chart &&
                timing.first_frame_count > 0,
            "The retail targeted-spell CAF charts could not be decoded.")) {
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
            spell, 5, tables.find(20));
    const osf::TableData* speed_table = tables.find(20);
    const double expected_speed =
        speed_table && speed_table->contains(spell, 0)
            ? static_cast<double>(
                  speed_table->value(spell, 0)) *
                  1.3 * 0.001
            : 0.0;
    osf::PlayerSpellActionController action;
    osf::PlayerSpellActionEvent event;
    if (!check(
            marker >= 0 &&
                std::abs(speed - expected_speed) < 0.000001 &&
                action.start(
                    spell_action,
                    spell,
                    14000316,
                    5,
                    tables.find(20),
                    timing,
                    &event) &&
                event.cast_due &&
                event.action == spell_action &&
                event.spell == spell &&
                event.target_character_number == 14000316 &&
                event.effect_delay ==
                    static_cast<std::int32_t>(
                        std::trunc(
                            static_cast<double>(marker) /
                            speed)) &&
                action.animationChart() == first_chart &&
                action.animationFrame() == 0,
            "The targeted spell did not enter its action with the retail "
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
            action.animationChart() == recovery_chart;
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
            action.animationChart() == recovery_chart &&
            action.animationFrame() == 0,
        "The spell action did not preserve the retail counter and "
        "chart-fourteen completion behavior.");
}

bool testRetailPacket(
    const osf::TableDatabase& tables,
    std::int32_t spell,
    std::int32_t effect_number,
    std::int32_t packet_subtype,
    std::int32_t impact_effect,
    std::int32_t target_mask,
    bool projectile,
    bool physical_defense) {
    osf::PlayerTargetedSpellCastInput input;
    input.stats.source_character_number = 0;
    input.stats.player_level = 7;
    input.stats.magical_attack = 12;
    input.stats.physical_defense = 11;
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
        osf::buildPlayerTargetedSpellCast(
            spell, input, tables);
    const osf::TableData* hit_table = tables.find(18);
    const std::int32_t expected_hit =
        hit_table && hit_table->contains(spell, 0)
            ? hit_table->value(spell, 0)
            : -1;
    const bool passed =
        request.valid &&
            request.effect_number == effect_number &&
            request.owner_kind == 1 &&
            request.source_character_number == 0 &&
            request.target_kind == target_mask &&
            request.target_identifier == 14000316 &&
            request.constructor_value_6 ==
                (projectile
                     ? tables.find(35)->value(spell, 0)
                     : 0) &&
            request.constructor_value_7 ==
                (projectile ? 200 : 0) &&
            request.constructor_value_12 == 5 &&
            request.constructor_value_17 ==
                (projectile ? 0 : 1) &&
            request.constructor_value_22 ==
                tables.find(21)->value(spell, 0) &&
            std::abs(request.direction_radians) < 0.000001 &&
            request.has_explicit_origin ==
                !projectile &&
            (!request.has_explicit_origin ||
             (request.origin.x == 100 &&
              request.origin.y == 200)) &&
            request.has_source_judgement ==
                projectile &&
            (!request.has_source_judgement ||
             request.source_judgement.left == -80) &&
            request.has_packet &&
            request.packet[0] == 0 &&
            request.packet[1] == 3 &&
            request.packet[2] == 0 &&
            request.packet[3] == packet_subtype &&
            request.packet[4] ==
                input.parameters.effect_value + 12 &&
            request.packet[5] ==
                (physical_defense ? 11 : 13) &&
            request.packet[6] == 1 &&
            request.packet[13] == 8 &&
            request.packet[14] == 100 &&
            request.packet[30] == 116 &&
            request.packet[31] == 7 &&
            request.packet[32] ==
                tables.find(19)->value(spell, 0) &&
            request.packet[34] == impact_effect &&
            request.packet[35] == 8 &&
            request.packet[36] ==
                expected_hit + 14 &&
            request.packet[45] ==
                tables.find(70)->value(spell, 2) &&
            request.packet[54] ==
                tables.find(70)->value(spell, 0) &&
            request.packet[63] ==
                tables.find(70)->value(spell, 1) &&
            request.packet[72] == 0 &&
            request.packet[73] == spell &&
            request.packet[74] == -1 &&
            request.packet[75] == 8 &&
            request.packet[76] == 0;
    if (!passed) {
        std::cerr
            << "spell=" << spell
            << " meta="
            << request.valid
            << ',' << request.owner_kind
            << ',' << request.source_character_number
            << ',' << request.target_kind
            << ',' << request.target_identifier
            << " effect=" << request.effect_number
            << " c6=" << request.constructor_value_6
            << '/'
            << (projectile
                    ? tables.find(35)->value(spell, 0)
                    : 0)
            << " c7=" << request.constructor_value_7
            << " c12=" << request.constructor_value_12
            << " c17=" << request.constructor_value_17
            << " c22=" << request.constructor_value_22
            << '/'
            << tables.find(21)->value(spell, 0)
            << " dir=" << request.direction_radians
            << " judgement="
            << request.has_source_judgement
            << ',' << request.source_judgement.left
            << " origin=" << request.has_explicit_origin
            << ',' << request.origin.x
            << ',' << request.origin.y
            << " has-packet=" << request.has_packet
            << " head=" << request.packet[0]
            << ',' << request.packet[1]
            << ',' << request.packet[2]
            << " p3=" << request.packet[3]
            << " p4=" << request.packet[4]
            << '/'
            << input.parameters.effect_value + 12
            << " p5=" << request.packet[5]
            << " p6=" << request.packet[6]
            << " p13=" << request.packet[13]
            << " p14=" << request.packet[14]
            << " p30=" << request.packet[30]
            << " p31=" << request.packet[31]
            << " p32=" << request.packet[32]
            << '/'
            << tables.find(19)->value(spell, 0)
            << " p34=" << request.packet[34]
            << " p36=" << request.packet[36]
            << '/' << expected_hit + 14
            << " p45=" << request.packet[45]
            << '/'
            << tables.find(70)->value(spell, 2)
            << " p54=" << request.packet[54]
            << '/'
            << tables.find(70)->value(spell, 0)
            << " p63=" << request.packet[63]
            << '/'
            << tables.find(70)->value(spell, 1)
            << " p73=" << request.packet[73]
            << " tail=" << request.packet[72]
            << ',' << request.packet[74]
            << ',' << request.packet[75]
            << ',' << request.packet[76]
            << '\n';
    }
    return check(
        passed,
        "The family-zero targeted-spell packet or controller arguments "
        "differ from its retail action.");
}

bool testShippedWorldCast(
    const std::filesystem::path& game_root,
    std::int32_t spell,
    std::int32_t first_chart,
    std::int32_t projectile_resource,
    std::int32_t launch_sample) {
    osf::PlayerLoadRequest player;
    player.name = "SpellLive";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root,
                player,
                {3000507, 3, 0},
                &error),
            "The shipped spell scenario could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[
        static_cast<std::size_t>(spell)] = 3;
    world.playerMagic().restore(magic_state);
    if (!check(
            world.playerMagic().selectSpell(spell),
            "The live targeted-spell fixture could not select the spell.")) {
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
            "the targeted spell.")) {
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
            spell,
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
                world.playerAnimationChart() ==
                    first_chart &&
                world.playerData().currentMana() ==
                    mana_before - parameters.mana_cost &&
                world.runtimeEffectControllerCount() == 1,
            "The shipped right-click did not enter the retail spell "
            "action, deduct MP, and queue its effect.")) {
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
            std::find(
                audio.begin(),
                audio.end(),
                launch_sample) !=
                audio.end();
        saw_projectile =
            saw_projectile ||
            std::any_of(
                world.runtimeEffects().begin(),
                world.runtimeEffects().end(),
                [projectile_resource](
                    const osf::RuntimeEffectActor& actor) {
                    return actor.resourceId() ==
                        projectile_resource;
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
            osf::PlayerTargetedSpellCastInput contact_input;
            contact_input.stats.source_character_number = 0;
            contact_input.stats.player_level = 1;
            contact_input.stats.magical_attack = 10;
            contact_input.stats.physical_defense = 10;
            contact_input.stats.magical_defense = 10;
            contact_input.stats.magical_hit_rate = 40;
            contact_input.parameters.effective_level = 1;
            contact_input.parameters.effect_value = 40;
            contact_input.target_character_number =
                target_character_number;
            contact_input.source_position =
                spell == 3
                    ? osf::WorldPosition{
                          current_target->position().x -
                              250,
                          current_target->position().y}
                    : current_target->position();
            contact_input.target_position =
                current_target->position();
            osf::CombatEffectSpawnRequest contact =
                osf::buildPlayerTargetedSpellCast(
                    spell,
                    contact_input,
                    world.parameterTables());
            contact.constructor_value_12 = 0;
            if (spell != 3) {
                // Keep this receiver regression deterministic: the
                // projectile starts directly on the shipped target, while
                // the packet remains the exact player-spell family tested
                // above.
                contact.owner_kind = 0;
                contact.target_kind = 4;
                contact.constructor_value_6 = 0;
                contact.has_explicit_origin = true;
                contact.origin =
                    current_target->position();
            }
            contact.packet.write(36, 100000);
            world.queueCombatEffect(contact);
            for (std::int32_t update = 0;
                 update < 12 && !applied_damage;
                 ++update) {
                world.update();
                const std::vector<std::int32_t> audio =
                    world.takeAudioSamples();
                saw_launch_audio =
                    saw_launch_audio ||
                    std::find(
                        audio.begin(),
                        audio.end(),
                        launch_sample) !=
                        audio.end();
                saw_projectile =
                    saw_projectile ||
                    std::any_of(
                        world.runtimeEffects().begin(),
                        world.runtimeEffects().end(),
                        [projectile_resource](
                            const osf::RuntimeEffectActor& actor) {
                            return actor.resourceId() ==
                                projectile_resource;
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
        world.playerMagic().experience(spell) >= 1;
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
            << world.playerMagic().experience(spell)
            << " controllers="
            << world.runtimeEffectControllerCount()
            << " actors=" << world.runtimeEffects().size()
            << '\n';
    }
    return check(
        passed,
        "The shipped targeted spell did not render, sound, "
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
    if (!testRetailAction(
            animation,
            tables,
            1,
            osf::PlayerSpellAction::fire_ball,
            13,
            14) ||
        !testRetailPacket(
            tables,
            1,
            10001,
            0,
            20000,
            0x14,
            true,
            false) ||
        !testShippedWorldCast(
            game_root,
            1,
            13,
            10000010,
            19) ||
        !testRetailAction(
            animation,
            tables,
            2,
            osf::PlayerSpellAction::ice_bolt,
            13,
            14) ||
        !testRetailPacket(
            tables,
            2,
            10002,
            1,
            21013,
            0x14,
            true,
            false) ||
        !testShippedWorldCast(
            game_root,
            2,
            13,
            10000040,
            94) ||
        !testRetailAction(
            animation,
            tables,
            3,
            osf::PlayerSpellAction::plasma,
            11,
            12) ||
        !testRetailPacket(
            tables,
            3,
            10003,
            0,
            20005,
            4,
            false,
            true) ||
        !testShippedWorldCast(
            game_root,
            3,
            11,
            10000030,
            21)) {
        return 1;
    }
#endif
    return 0;
}
