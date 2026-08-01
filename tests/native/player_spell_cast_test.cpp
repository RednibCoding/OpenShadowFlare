#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "core/retail_random.hpp"
#include "items/item_audio.hpp"
#include "world/actor_direction.hpp"
#include "world/combat_effect_actor.hpp"
#include "world/generic_effect_actor.hpp"
#include "world/player_counter_burst.hpp"
#include "world/player_data.hpp"
#include "world/player_energy_shield.hpp"
#include "world/player_heal_spell.hpp"
#include "world/player_magic_shield.hpp"
#include "world/player_moon_spell.hpp"
#include "world/player_resource_rate.hpp"
#include "world/player_runtime_profile.hpp"
#include "world/player_spell_action.hpp"
#include "world/player_spell_cast.hpp"
#include "world/player_spell_parameters.hpp"
#include "world/player_sustained_spell.hpp"
#include "world/retail_save_file.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool writeRetailItemWord(
    osf::InventoryItem& item,
    std::size_t word,
    std::int32_t value) {
    const std::size_t offset = word * 4u;
    if (offset > item.retail_state.size() ||
        item.retail_state.size() - offset < 4u) {
        return false;
    }
    const std::uint32_t encoded =
        static_cast<std::uint32_t>(value);
    item.retail_state[offset] =
        static_cast<std::uint8_t>(encoded);
    item.retail_state[offset + 1] =
        static_cast<std::uint8_t>(encoded >> 8u);
    item.retail_state[offset + 2] =
        static_cast<std::uint8_t>(encoded >> 16u);
    item.retail_state[offset + 3] =
        static_cast<std::uint8_t>(encoded >> 24u);
    return true;
}

struct ExpectedSpellCast {
    std::int32_t effect_number = -1;
    std::int32_t packet_subtype = 0;
    std::int32_t impact_effect = -1;
    std::int32_t target_mask = 0;
    bool requires_target = true;
    bool use_table_travel_speed = true;
    bool use_explicit_origin = false;
    bool use_source_judgement = true;
    bool constructor_uses_level = false;
    bool use_physical_defense = false;
    bool random_ordinary_impact = false;
    std::int32_t packet_value_72 = 0;
    bool use_player_effect_source = true;
    bool use_target_identifier = true;
    std::int32_t packet_type = 3;
    bool physical_percent_damage = false;
    std::int32_t constructor_delay_override = -1;
};

bool testRetailAction(
    const osf::gapi::CafAnimation& animation,
    const osf::TableDatabase& tables,
    std::int32_t spell,
    osf::PlayerSpellAction spell_action,
    std::int32_t first_chart,
    std::int32_t recovery_chart,
    bool dispatch_at_marker = false,
    std::int32_t entry_visual_effect_number = -1) {
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
            "The retail player-spell CAF charts could not be decoded.")) {
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
    const std::int32_t target_character_number =
        osf::playerSpellRequiresCharacterTarget(spell)
            ? 14000316
            : -1;
    if (!check(
            marker >= 0 &&
                std::abs(speed - expected_speed) < 0.000001 &&
                action.start(
                    spell_action,
                    spell,
                    target_character_number,
                    400,
                    200,
                    5,
                    tables.find(20),
                    timing,
                    &event) &&
                event.cast_due ==
                    !dispatch_at_marker &&
                event.action == spell_action &&
                event.spell == spell &&
                event.target_character_number ==
                    target_character_number &&
                event.aim_world_x == 400 &&
                event.aim_world_y == 200 &&
                event.effect_delay ==
                    static_cast<std::int32_t>(
                        std::trunc(
                            static_cast<double>(marker) /
                            speed)) &&
                event.entry_visual_effect_number ==
                    entry_visual_effect_number &&
                action.animationChart() == first_chart &&
                action.animationFrame() == 0,
            "The spell did not enter its action with the retail "
            "speed or effect delay.")) {
        return false;
    }

    std::int32_t completion_update = -1;
    bool saw_recovery = false;
    bool saw_cast = event.cast_due;
    std::int32_t cast_frame =
        event.cast_due ? action.displayedFrame() : -1;
    for (std::int32_t update = 1;
         update < 100 && action.active();
         ++update) {
        event = action.update(5, tables.find(20));
        if (event.cast_due) {
            saw_cast = true;
            cast_frame = action.displayedFrame();
        }
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
            saw_cast &&
            (!dispatch_at_marker ||
             (cast_frame >= marker &&
              cast_frame < timing.first_frame_count)) &&
            saw_recovery &&
            !action.active() &&
            action.animationChart() == recovery_chart &&
            action.animationFrame() == 0,
        "The spell action did not preserve the retail counter and "
        "recovery-chart completion behavior.");
}

bool testRetailSonicBladeAction(
    const osf::gapi::CafAnimation& animation,
    const osf::TableDatabase& tables) {
    struct VariantExpectation {
        std::int32_t subtype;
        osf::PlayerSpellAnimationVariant variant;
        std::int32_t first_chart;
        std::int32_t recovery_chart;
    };
    constexpr std::array<VariantExpectation, 3> variants{{
        {0,
         osf::PlayerSpellAnimationVariant::sonic_blade_subtype_0,
         5,
         6},
        {3,
         osf::PlayerSpellAnimationVariant::sonic_blade_subtype_3,
         15,
         16},
        {1,
         osf::PlayerSpellAnimationVariant::sonic_blade_subtype_1,
         19,
         20},
    }};

    for (const VariantExpectation& expected : variants) {
        osf::PlayerSpellAnimationVariant selected;
        osf::PlayerSpellAnimationTiming timing;
        if (!check(
                osf::playerSonicBladeAnimationVariant(
                    expected.subtype, selected) &&
                    selected == expected.variant &&
                    osf::buildPlayerSpellAnimationTiming(
                        animation,
                        osf::PlayerSpellAction::sonic_blade,
                        selected,
                        0,
                        timing) &&
                    timing.first_chart == expected.first_chart &&
                    timing.recovery_chart ==
                        expected.recovery_chart &&
                    timing.first_frame_count > 0 &&
                    timing.recovery_frame_count > 0,
                "A supported Sonic Blade weapon did not select its "
                "retail CAF chart pair.")) {
            return false;
        }

        osf::PlayerSpellActionController action;
        osf::PlayerSpellActionEvent event;
        if (!check(
                action.start(
                    osf::PlayerSpellAction::sonic_blade,
                    15,
                    14000316,
                    400,
                    200,
                    5,
                    tables.find(20),
                    timing,
                    &event) &&
                    event.entry_visual_effect_number == 21025 &&
                    !event.cast_due &&
                    event.effect_delay == 1 &&
                    action.animationChart() ==
                        expected.first_chart &&
                    std::abs(
                        osf::retailPlayerSonicBladeAnimationSpeed(5) -
                        1.1) < 0.000001,
                "Sonic Blade did not enter with its charge visual, "
                "attack speed, and marker-owned projectile.")) {
            return false;
        }

        bool saw_marker = false;
        bool saw_swing_sound = false;
        bool saw_recovery = false;
        std::int32_t marker_count = 0;
        for (std::int32_t update = 0;
             update < 100 && action.active();
             ++update) {
            event = action.update(5, tables.find(20));
            if (event.cast_due) {
                saw_marker = true;
                ++marker_count;
                if (!check(
                        action.animationChart() ==
                            expected.first_chart,
                        "Sonic Blade dispatched outside its first-chart "
                        "CAF marker.")) {
                    return false;
                }
            }
            saw_swing_sound =
                saw_swing_sound || event.swing_sound_due;
            saw_recovery =
                saw_recovery ||
                action.animationChart() ==
                    expected.recovery_chart;
        }
        if (!check(
                saw_marker && marker_count >= 1 &&
                    saw_swing_sound && saw_recovery &&
                    !action.active() &&
                    action.animationChart() ==
                        expected.recovery_chart &&
                    action.animationFrame() ==
                        timing.recovery_frame_count - 1,
                "Sonic Blade did not launch, sound, recover, and finish "
                "on the retail weapon-animation timeline.")) {
            return false;
        }
    }

    osf::PlayerSpellAnimationVariant rejected;
    return check(
        !osf::playerSonicBladeAnimationVariant(-1, rejected) &&
            !osf::playerSonicBladeAnimationVariant(2, rejected) &&
            !osf::playerSonicBladeAnimationVariant(4, rejected),
        "Sonic Blade accepted an empty, armor, or ranged weapon subtype.");
}

bool testRetailPacket(
    const osf::TableDatabase& tables,
    std::int32_t spell,
    const ExpectedSpellCast& expected) {
    osf::PlayerSpellCastInput input;
    input.stats.source_character_number = 0;
    input.stats.player_level = 7;
    input.stats.physical_attack = 80;
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
    input.target_character_number =
        expected.requires_target ? 14000316 : -1;
    input.source_position = {100, 200};
    input.source_judgement = {-80, -80, 79, 79};
    input.target_position =
        expected.requires_target
            ? osf::WorldPosition{400, 200}
            : osf::WorldPosition{400, 500};
    input.effect_delay = 5;

    osf::RetailRandom packet_random(0x1234u);
    osf::RetailRandom expected_random(0x1234u);
    const osf::CombatEffectSpawnRequest request =
        osf::buildPlayerSpellCast(
            spell,
            input,
            tables,
            expected.random_ordinary_impact
                ? &packet_random
                : nullptr);
    const osf::TableData* hit_table = tables.find(18);
    const std::int32_t expected_hit =
        hit_table && hit_table->contains(spell, 0)
            ? hit_table->value(spell, 0)
            : -1;
    const bool passed =
        request.valid &&
            request.effect_number ==
                expected.effect_number &&
            request.owner_kind == 1 &&
            request.source_character_number ==
                (expected.use_player_effect_source
                     ? 0
                     : -1) &&
            request.target_kind ==
                expected.target_mask &&
            request.target_identifier ==
                (expected.use_target_identifier &&
                         expected.requires_target
                     ? 14000316
                     : -1) &&
            request.constructor_value_6 ==
                (expected.use_table_travel_speed
                     ? tables.find(35)->value(spell, 0)
                     : 0) &&
            request.constructor_value_7 ==
                (expected.use_table_travel_speed
                     ? 200
                     : 0) &&
            request.constructor_value_12 ==
                (expected.constructor_delay_override >= 0
                     ? expected.constructor_delay_override
                     : 5) &&
            request.constructor_value_17 ==
                (expected.constructor_uses_level
                     ? 1
                     : 0) &&
            request.constructor_value_22 ==
                tables.find(21)->value(spell, 0) &&
            std::abs(request.direction_radians) < 0.000001 &&
            request.has_explicit_origin ==
                expected.use_explicit_origin &&
            (!request.has_explicit_origin ||
             (request.origin.x == 100 &&
              request.origin.y == 200)) &&
            request.has_source_judgement ==
                expected.use_source_judgement &&
            (!request.has_source_judgement ||
             request.source_judgement.left == -80) &&
            request.has_packet &&
            request.packet[0] == 0 &&
            request.packet[1] == expected.packet_type &&
            request.packet[2] == 0 &&
            request.packet[3] ==
                expected.packet_subtype &&
            request.packet[4] ==
                (expected.physical_percent_damage
                     ? input.parameters.effect_value * 80 / 100
                     : input.parameters.effect_value + 12) &&
            request.packet[5] ==
                (expected.use_physical_defense
                     ? 11
                     : 13) &&
            request.packet[6] == 1 &&
            request.packet[13] == 8 &&
            request.packet[14] == 100 &&
            request.packet[30] == 116 &&
            request.packet[31] == 7 &&
            request.packet[32] ==
                tables.find(19)->value(spell, 0) &&
            request.packet[34] ==
                (expected.random_ordinary_impact
                     ? expected_random.next() % 4 + 21000
                     : expected.impact_effect) &&
            request.packet[35] == 8 &&
            request.packet[36] ==
                expected_hit + 14 &&
            request.packet[45] ==
                tables.find(70)->value(spell, 2) &&
            request.packet[54] ==
                tables.find(70)->value(spell, 0) &&
            request.packet[63] ==
                tables.find(70)->value(spell, 1) &&
            request.packet[72] ==
                expected.packet_value_72 &&
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
            << (expected.use_table_travel_speed
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
            << (expected.physical_percent_damage
                    ? input.parameters.effect_value * 80 / 100
                    : input.parameters.effect_value + 12)
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
        "The player-spell packet or controller arguments "
        "differ from its retail action.");
}

bool testRetailSonicBladeEffects(
    const osf::TableDatabase& tables) {
    const osf::CombatEffectSpawnRequest charge =
        osf::buildPlayerSpellEntryVisual(
            21025, 0, {-80, -80, 79, 79});
    if (!check(
            charge.valid &&
                charge.effect_number == 21025 &&
                charge.owner_kind == 1 &&
                charge.source_character_number == 0 &&
                charge.target_kind == 0 &&
                charge.target_identifier == -1 &&
                charge.has_source_judgement &&
                charge.source_judgement.left == -80 &&
                !charge.has_packet &&
                charge.packet_kind == 8 &&
                charge.constructor_value_21 == 200 &&
                osf::retailCombatEffectResourceId(21025) ==
                    11000100,
            "Sonic Blade's action-entry charge visual differs from "
            "retail effect 21025.")) {
        return false;
    }

    osf::PlayerSpellCastInput input;
    input.stats.source_character_number = 0;
    input.stats.player_level = 7;
    input.stats.physical_attack = 80;
    input.stats.magical_attack = 12;
    input.stats.physical_defense = 11;
    input.stats.magical_defense = 13;
    input.stats.magical_hit_rate = 14;
    input.parameters.effective_level = 1;
    input.parameters.effect_value = 40;
    input.target_character_number = 14000316;
    input.source_position = {100, 200};
    input.source_judgement = {-80, -80, 79, 79};
    input.target_position = {400, 200};
    input.effect_delay = 99;
    const osf::CombatEffectSpawnRequest projectile =
        osf::buildPlayerSpellCast(
            15, input, tables);
    osf::RuntimeEffectActorSpawnRequest actor;
    if (!check(
        projectile.valid &&
            projectile.constructor_value_12 == 1 &&
            osf::buildGenericEffectActor(
                projectile, input.source_position, actor) &&
            actor.controller_effect_number == 10015 &&
            actor.resource_id == 10000090 &&
            actor.position.x == 300 &&
            actor.position.y == 200 &&
            actor.judgement.left == -80 &&
            actor.judgement.top == -80 &&
            actor.judgement.right == 79 &&
            actor.judgement.bottom == 79 &&
            actor.display_height == 155 &&
            actor.lifetime == 7 &&
            actor.travel_speed ==
                tables.find(35)->value(15, 0) &&
            actor.collide_with_environment &&
            actor.expire_on_environment_collision &&
            actor.target_collision_start == 0 &&
            actor.expire_on_target &&
            actor.target_audio.bank == 0 &&
            actor.target_audio.sample == 20 &&
            actor.animation_direction == 1 &&
            actor.has_packet &&
            actor.packet[1] == 0 &&
            actor.packet[4] == 32 &&
            actor.packet[5] == 11 &&
            actor.packet[34] == 21024 &&
            actor.packet[72] == 1 &&
            actor.packet[73] == 15,
        "Sonic Blade's marker projectile lost its retail resource, "
        "geometry, lifetime, collision, audio, or physical packet.")) {
        return false;
    }

    osf::RuntimeEffectActorSpawnRequest contact_actor = actor;
    contact_actor.resource_id = -1;
    contact_actor.visible = false;
    contact_actor.collide_with_environment = false;
    contact_actor.travel_speed = 0;
    contact_actor.position = {400, 200};
    contact_actor.packet.write(36, 100000);
    osf::RuntimeEffectActor runtime_actor;
    osf::RuntimeEffectTargetSnapshot target;
    target.kind = osf::RuntimeEffectTargetKind::enemy;
    target.character_number = 14000316;
    target.identifier = 14000316;
    target.position = contact_actor.position;
    target.judgement = {-80, -80, 79, 79};
    target.current_life = 100;
    target.physical_evasion = 0;
    osf::RetailRandom random(1);
    osf::GroundMap ground;
    osf::ObjectMap objects;
    if (!check(
            runtime_actor.initialize(contact_actor, nullptr),
            "The deterministic Sonic Blade contact fixture could not "
            "initialize.")) {
        return false;
    }
    const osf::RuntimeEffectActorUpdate contact =
        runtime_actor.update(
            ground, objects, {target}, random);
    return check(
        contact.target_contacts.size() == 1 &&
            contact.target_contacts.front().identifier == 14000316 &&
            contact.target_contacts.front().receiver_action ==
                osf::RuntimeEffectReceiverAction::apply_packet &&
            contact.audio.size() == 1 &&
            contact.audio.front().sound.bank == 0 &&
            contact.audio.front().sound.sample == 20 &&
            contact.expired,
        "Sonic Blade did not apply its physical packet, contact sample, "
        "and first-target expiry at collision time.");
}

bool testRetailHealResolution(
    const osf::TableDatabase& tables) {
    const osf::TableData* heal_table = tables.find(17);
    if (!check(
            heal_table && heal_table->contains(6, 0),
            "The retail Heal percentage could not be read.")) {
        return false;
    }
    const std::int32_t heal_percent =
        heal_table->value(6, 0);
    const osf::PlayerHealSpellResolution damaged =
        osf::resolvePlayerHealSpell({
            0,
            50,
            140,
            heal_percent,
            {-80, -80, 79, 79},
        });
    const std::int32_t expected_amount =
        std::min(
            heal_percent * 140 / 100,
            90);
    if (!check(
            damaged.valid &&
                damaged.healed_amount == expected_amount &&
                damaged.restored_life ==
                    50 + expected_amount &&
                damaged.award_practice &&
                damaged.audio_sample == 17 &&
                damaged.visual.valid &&
                damaged.visual.effect_number == 21020 &&
                damaged.visual.owner_kind == 1 &&
                damaged.visual.source_character_number == 0 &&
                damaged.visual.target_kind == 0 &&
                damaged.visual.target_identifier == 0 &&
                damaged.visual.constructor_value_6 == 0 &&
                damaged.visual.constructor_value_7 == 0 &&
                damaged.visual.direction_radians == 0.0 &&
                !damaged.visual.has_explicit_origin &&
                damaged.visual.has_source_judgement &&
                damaged.visual.source_judgement.left == -80 &&
                damaged.visual.constructor_value_12 == 0 &&
                !damaged.visual.has_packet &&
                damaged.visual.packet_kind == 8 &&
                damaged.visual.instance_identifier == -1 &&
                damaged.visual.constructor_value_21 == 200 &&
                damaged.visual.constructor_value_22 == 0,
            "Heal did not preserve its retail percentage, visual, "
            "audio, or practice result.")) {
        return false;
    }

    const osf::PlayerHealSpellResolution full =
        osf::resolvePlayerHealSpell({
            0,
            140,
            140,
            heal_percent,
            {-80, -80, 79, 79},
        });
    return check(
        full.valid &&
            full.restored_life == 140 &&
            full.healed_amount == 0 &&
            !full.award_practice &&
            full.audio_sample == -1 &&
            full.visual.valid &&
            full.visual.effect_number == 21020,
        "A full-life Heal lost its visual or incorrectly produced "
        "restoration, audio, or practice.");
}

bool testRetailMoonRules(
    const osf::TableDatabase& tables) {
    constexpr std::int32_t level = 1;
    const osf::TableData* moon_table = tables.find(200);
    if (!check(
            moon_table && moon_table->contains(13, level - 1),
            "The retail Moon parameter table could not be read.")) {
        return false;
    }

    osf::PlayerSustainedSpell moon;
    if (!check(
            moon.toggle(200, level, tables) &&
                moon.active() &&
                moon.effectiveLevel() == level &&
                moon.manaChangeRate() ==
                    moon_table->value(0, level - 1) &&
                moon.manaChangeRate() < 0,
            "Moon did not activate with its retail Table 200 rate.")) {
        return false;
    }

    osf::CompanionProfile base;
    base.attack_speed_rating = 90;
    base.walking_speed_raw = 105;
    base.running_speed_raw = 155;
    base.walking_speed = 21;
    base.running_speed = 31;
    base.physical_attack = 40;
    base.maximum_life = 120;
    base.hit_rate = 50;
    base.physical_defense = 30;
    base.physical_evasion = 25;
    base.magical_attack = 20;
    base.magical_hit_rate = 35;
    base.magical_defense = 15;
    base.magical_evasion = 18;
    base.parameter_17 = 12;
    const osf::CompanionProfile modified =
        osf::applyPlayerMoonCompanionModifiers(
            base, moon, tables);
    const auto adjusted =
        [moon_table](std::int32_t value, std::int32_t row) {
            return value +
                   moon_table->value(row, 0) * value / 100;
        };
    if (!check(
            modified.attack_speed_rating ==
                    std::clamp(adjusted(90, 1), 0, 255) &&
                modified.walking_speed_raw ==
                    std::clamp(adjusted(105, 2), 0, 255) &&
                modified.running_speed_raw ==
                    std::clamp(adjusted(155, 3), 0, 255) &&
                modified.walking_speed ==
                    modified.walking_speed_raw / 5 &&
                modified.running_speed ==
                    modified.running_speed_raw / 5 &&
                modified.physical_attack ==
                    std::max(adjusted(40, 4), 1) &&
                modified.maximum_life ==
                    std::max(adjusted(120, 5), 1) &&
                modified.hit_rate ==
                    std::max(adjusted(50, 6), 1) &&
                modified.physical_defense ==
                    std::max(adjusted(30, 7), 1) &&
                modified.physical_evasion ==
                    std::max(adjusted(25, 8), 1) &&
                modified.magical_attack ==
                    std::max(adjusted(20, 9), 1) &&
                modified.magical_hit_rate ==
                    std::max(adjusted(35, 10), 1) &&
                modified.magical_evasion ==
                    std::max(adjusted(18, 11), 1) &&
                modified.magical_defense ==
                    std::max(adjusted(15, 12), 1) &&
                modified.parameter_17 ==
                    std::max(adjusted(12, 13), 1),
            "Moon did not apply the thirteen retail companion modifiers.")) {
        return false;
    }

    std::int32_t mana = 160;
    bool drained = false;
    bool deactivated = false;
    osf::PlayerResourceRateController mana_rate;
    for (std::int32_t update = 0;
         update < 20000 && !deactivated;
         ++update) {
        const osf::PlayerResourceRateUpdate result =
            mana_rate.update(
                mana, 160, moon.manaChangeRate(), 0);
        drained = drained || result.value < mana;
        mana = result.value;
        if (mana == 0) {
            deactivated = moon.deactivate();
        }
    }
    if (!check(
            drained && deactivated && mana == 0 && !moon.active(),
            "Moon did not drain MP on the retail three-update cadence "
            "or switch off at zero.")) {
        return false;
    }
    return check(
        moon.toggle(200, level, tables) && moon.active(),
        "Moon could not be activated again after automatic shutdown.");
}

bool testRetailBerserkerRules(
    const std::filesystem::path& game_root,
    const osf::TableDatabase& tables) {
    constexpr std::int32_t level = 1;
    const osf::TableData* berserker_table = tables.find(201);
    if (!check(
            berserker_table &&
                berserker_table->contains(12, level - 1),
            "The retail Berserker parameter table could not be read.")) {
        return false;
    }

    osf::PlayerLoadRequest player;
    player.name = "BerserkerRules";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(game_root, player, &error),
            "Remote Town could not prepare the Berserker rules fixture.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerSustainedSpell inactive;
    const osf::PlayerRuntimeProfile base =
        osf::buildPlayerRuntimeProfile(
            world.playerData(),
            world.playerEquipment(),
            world.itemDatabase(),
            inactive,
            tables);
    osf::PlayerSustainedSpell berserker;
    if (!check(
            berserker.toggle(201, level, tables) &&
                berserker.active() &&
                berserker.effectiveLevel() == level &&
                berserker.manaChangeRate() ==
                    berserker_table->value(0, level - 1),
            "Berserker did not activate with its retail Table 201 rate.")) {
        return false;
    }
    const osf::PlayerRuntimeProfile modified =
        osf::buildPlayerRuntimeProfile(
            world.playerData(),
            world.playerEquipment(),
            world.itemDatabase(),
            berserker,
            tables);
    const auto adjusted =
        [berserker_table](
            std::int32_t value,
            std::int32_t row) {
            return value +
                   berserker_table->value(row, 0) * value /
                       100;
        };
    if (!check(
            modified.attack_speed_raw ==
                    std::clamp(adjusted(
                        base.attack_speed_raw, 1), 0, 255) &&
                modified.walking_speed_raw ==
                    std::clamp(adjusted(
                        base.walking_speed_raw, 2), 0, 255) &&
                modified.maximum_life ==
                    std::max(adjusted(base.maximum_life, 3), 1) &&
                modified.maximum_mana ==
                    std::max(adjusted(base.maximum_mana, 4), 1) &&
                modified.physical_attack ==
                    std::max(adjusted(base.physical_attack, 5), 1) &&
                modified.physical_defense ==
                    std::max(adjusted(base.physical_defense, 6), 1) &&
                modified.hit_rate ==
                    std::max(adjusted(base.hit_rate, 7), 1) &&
                modified.physical_evasion ==
                    std::max(adjusted(base.physical_evasion, 8), 1) &&
                modified.magical_attack ==
                    std::max(adjusted(base.magical_attack, 9), 1) &&
                modified.magical_defense ==
                    std::max(adjusted(base.magical_defense, 10), 1) &&
                modified.magical_hit_rate ==
                    std::max(adjusted(base.magical_hit_rate, 11), 1) &&
                modified.magical_evasion ==
                    std::max(adjusted(base.magical_evasion, 12), 1),
            "Berserker did not apply all twelve retail player modifiers.")) {
        return false;
    }

    osf::PlayerSustainedSpell moon;
    moon.toggle(200, level, tables);
    osf::PlayerResourceRateController mana_rate;
    std::int32_t mana = 160;
    bool drained = false;
    for (std::int32_t update = 0;
         update < 200 && !drained;
         ++update) {
        const osf::PlayerResourceRateUpdate result =
            mana_rate.update(
                mana,
                160,
                moon.manaChangeRate() +
                    berserker.manaChangeRate(),
                0);
        drained = result.value < mana;
        mana = result.value;
    }
    if (!check(
            drained && mana_rate.updateCounter() > 1,
            "Moon and Berserker did not share the retail MP accumulator.")) {
        return false;
    }

    osf::PlayerResourceRateController life_rate;
    const osf::PlayerResourceRateUpdate first_life =
        life_rate.update(80, 100, 100, 1);
    const osf::PlayerResourceRateUpdate second_life =
        life_rate.update(first_life.value, 100, 100, 1);
    const osf::PlayerResourceRateUpdate third_life =
        life_rate.update(second_life.value, 100, 100, 1);
    const osf::PlayerResourceRateUpdate fourth_life =
        life_rate.update(third_life.value, 100, 100, 1);
    osf::PlayerResourceRateController defeated_life_rate;
    const osf::PlayerResourceRateUpdate defeated_life =
        defeated_life_rate.update(0, 100, 100, 1, false);
    osf::PlayerResourceRateController draining_life_rate;
    const osf::PlayerResourceRateUpdate minimum_life =
        draining_life_rate.update(1, 100, -100, 1);
    if (!check(
            first_life.value == 81 && first_life.changed &&
                second_life.value == 81 &&
                third_life.value == 81 &&
                fourth_life.value == 82 &&
                defeated_life.value == 0 &&
                minimum_life.value == 1,
            "The retail life-rate cadence or living-player clamps differ.")) {
        return false;
    }

    const osf::InventoryItem* starter_body =
        world.playerEquipment().item(
            osf::EquipmentSlot::body);
    const osf::ItemDefinition* starter_body_definition =
        starter_body
            ? world.itemDatabase().find(
                  starter_body->category,
                  starter_body->definition_id)
            : nullptr;
    osf::PlayerEquipment rate_equipment;
    osf::InventoryItem rate_item =
        starter_body ? *starter_body : osf::InventoryItem{};
    rate_item.retail_state.resize(200u);
    if (!check(
            starter_body_definition &&
                writeRetailItemWord(rate_item, 17, 35) &&
                writeRetailItemWord(rate_item, 18, -25) &&
                rate_equipment.place(
                    osf::EquipmentSlot::body,
                    rate_item,
                    *starter_body_definition,
                    world.playerData().level()).accepted &&
                rate_equipment.instanceParameterBonus(
                    17, world.itemDatabase()) == 35 &&
                rate_equipment.instanceParameterBonus(
                    18, world.itemDatabase()) == -25,
            "Equipped rolled parameters 17 and 18 did not map to the "
            "retail life and mana rates.")) {
        return false;
    }

    const osf::PlayerSustainedSpellShutdown shutdown =
        osf::deactivateSustainedSpellsAtZeroMana(
            0, moon, berserker);
    if (!check(
            shutdown.moon_deactivated &&
                shutdown.berserker_deactivated &&
                !moon.active() && !berserker.active(),
            "Zero MP did not switch off both retail sustained spells.")) {
        return false;
    }
    moon.toggle(200, level, tables);
    berserker.toggle(201, level, tables);

    osf::PlayerMagic magic;
    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[7] = 3;
    magic_state.availability[8] = 3;
    magic.restore(magic_state);
    const osf::PlayerSustainedSpellTraining hero_training =
        osf::trainActiveSustainedSpellsOnOwnedKill(
            magic, moon, berserker, 0, 0, tables);
    const osf::PlayerSustainedSpellTraining companion_training =
        osf::trainActiveSustainedSpellsOnOwnedKill(
            magic, moon, berserker, 10, 0, tables);
    const std::int32_t moon_after_owned = magic.experience(7);
    const std::int32_t berserker_after_owned = magic.experience(8);
    const osf::PlayerSustainedSpellTraining foreign_training =
        osf::trainActiveSustainedSpellsOnOwnedKill(
            magic, moon, berserker, 1, 0, tables);
    if (!check(
            hero_training.moon_trained &&
                hero_training.berserker_trained &&
                companion_training.moon_trained &&
                companion_training.berserker_trained &&
                !foreign_training.moon_trained &&
                !foreign_training.berserker_trained &&
                magic.experience(7) == moon_after_owned &&
                magic.experience(8) == berserker_after_owned,
            "Sustained-spell practice did not follow retail local-owner "
            "kill attribution.")) {
        return false;
    }
    return check(
        !berserker.toggle(201, level, tables) &&
            !berserker.active(),
        "The second Berserker marker did not toggle the spell off.");
}

bool testRetailEnergyShieldRules(
    const osf::TableDatabase& tables) {
    osf::PlayerEnergyShield shield;
    if (!check(
            !shield.toggle(0) &&
                !shield.active() &&
                shield.toggle(1) &&
                shield.active(),
            "Energy Shield did not preserve its retail zero-MP "
            "activation guard.")) {
        return false;
    }
    const std::int32_t frame_before = shield.auraFrame();
    shield.updateAura(false);
    const std::int32_t hidden_frame = shield.auraFrame();
    shield.updateAura(true);
    if (!check(
            hidden_frame == frame_before &&
                shield.auraFrame() == frame_before + 1,
            "Energy Shield did not advance its aura only while displayed.")) {
        return false;
    }

    osf::PlayerMagic magic;
    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[9] = 3;
    magic.restore(magic_state);
    const bool hero_training =
        osf::trainEnergyShieldOnOwnedKill(
            magic, shield, 0, 0, tables);
    const bool companion_training =
        osf::trainEnergyShieldOnOwnedKill(
            magic, shield, 10, 0, tables);
    const std::int32_t owned_experience =
        magic.experience(9);
    const bool foreign_training =
        osf::trainEnergyShieldOnOwnedKill(
            magic, shield, 1, 0, tables);
    return check(
        hero_training && companion_training &&
            !foreign_training &&
            magic.experience(9) == owned_experience &&
            !shield.toggle(1) && !shield.active() &&
            !shield.deactivate(),
        "Energy Shield practice or toggle-off behavior differs from "
        "retail local-owner rules.");
}

bool testRetailMagicShieldRules() {
    osf::PlayerMagicShield shield;
    if (!check(
            shield.toggle() &&
                shield.active() &&
                shield.auraFrame() == 0,
            "Magic Shield did not toggle on and reset its retail aura "
            "counter.")) {
        return false;
    }
    shield.updateAura(false);
    const std::int32_t hidden_frame = shield.auraFrame();
    shield.updateAura(true);
    if (!check(
            hidden_frame == 0 &&
                shield.auraFrame() == 1,
            "Magic Shield advanced its aura while it was not displayed.")) {
        return false;
    }
    shield.restoreActive(false);
    if (!check(
            !shield.active() &&
                shield.auraFrame() == 1,
            "Damage-time Magic Shield shutdown changed its aura counter.")) {
        return false;
    }
    shield.restoreActive(true);
    return check(
        !shield.toggle() &&
            !shield.active() &&
            shield.auraFrame() == 0 &&
            !shield.deactivate() &&
            osf::retailCombatEffectResourceId(21029) == 11000241,
        "Magic Shield toggle-off or hit-effect resource mapping differs "
        "from retail.");
}

bool testRetailCounterBurstRules() {
    osf::PlayerCounterBurst counter;
    if (!check(
            counter.toggle() &&
                counter.active() &&
                counter.auraFrame() == 0,
            "Counter Burst did not toggle on and reset its retail aura "
            "counter.")) {
        return false;
    }
    counter.updateAura(false);
    const std::int32_t hidden_frame = counter.auraFrame();
    counter.updateAura(true);
    if (!check(
            hidden_frame == 0 &&
                counter.auraFrame() == 1,
            "Counter Burst advanced its aura while it was not "
            "displayed.")) {
        return false;
    }
    counter.restoreActive(false);
    if (!check(
            !counter.active() &&
                counter.auraFrame() == 1,
            "Damage-time Counter Burst shutdown changed its aura "
            "counter.")) {
        return false;
    }
    counter.restoreActive(true);
    return check(
        !counter.toggle() &&
            !counter.active() &&
            counter.auraFrame() == 0 &&
            !counter.deactivate() &&
            osf::retailCombatEffectResourceId(21030) == 11000251,
        "Counter Burst toggle-off or hit-effect resource mapping "
        "differs from retail.");
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
            osf::PlayerSpellCastInput contact_input;
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
                spell == 3 || spell == 10 || spell == 13
                    ? osf::WorldPosition{
                          current_target->position().x -
                              (spell == 13 ? 350 : 250),
                          current_target->position().y}
                    : current_target->position();
            contact_input.target_position =
                current_target->position();
            osf::RetailRandom contact_random(1);
            osf::CombatEffectSpawnRequest contact =
                osf::buildPlayerSpellCast(
                    spell,
                    contact_input,
                    world.parameterTables(),
                    &contact_random);
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
                    spell == 10 || spell == 13
                        ? contact_input.source_position
                        : current_target->position();
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

bool testShippedSonicBladeCast(
    const std::filesystem::path& game_root) {
    constexpr std::int32_t spell = 15;
    osf::PlayerLoadRequest player;
    player.name = "SonicBladeLive";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root,
                player,
                {3000507, 3, 0},
                &error),
            "The shipped Sonic Blade scenario could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[spell] = 3;
    world.playerMagic().restore(magic_state);
    if (!check(
            world.playerMagic().selectSpell(spell),
            "The live Sonic Blade fixture could not select the spell.")) {
        return false;
    }

    const osf::EnemyActor* target = nullptr;
    std::int32_t pointer_x = -1;
    std::int32_t pointer_y = -1;
    std::int64_t target_distance_squared =
        std::numeric_limits<std::int64_t>::max();
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
        std::int32_t candidate_x = -1;
        std::int32_t candidate_y = -1;
        for (std::int32_t y = std::max(0, anchor_y - 140);
             y < std::min(400, anchor_y + 30) && candidate_x < 0;
             ++y) {
            for (std::int32_t x = std::max(0, anchor_x - 80);
                 x < std::min(640, anchor_x + 81);
                 ++x) {
                world.updatePointerHover(x, y);
                if (world.hoveredEnemyId() == enemy.id()) {
                    candidate_x = x;
                    candidate_y = y;
                    break;
                }
            }
        }
        if (candidate_x >= 0) {
            const std::int64_t dx =
                static_cast<std::int64_t>(enemy.position().x) -
                world.playerWorldX();
            const std::int64_t dy =
                static_cast<std::int64_t>(enemy.position().y) -
                world.playerWorldY();
            const std::int64_t distance_squared =
                dx * dx + dy * dy;
            if (distance_squared < target_distance_squared) {
                target = &enemy;
                pointer_x = candidate_x;
                pointer_y = candidate_y;
                target_distance_squared = distance_squared;
            }
        }
    }
    if (!check(
            target && pointer_x >= 0 && pointer_y >= 0,
            "No shipped enemy could prepare the Sonic Blade command.")) {
        return false;
    }

    const osf::PlayerSpellParameters parameters =
        osf::playerSpellParameters(
            world.playerMagic(),
            spell,
            world.playerEquipment(),
            world.itemDatabase(),
            world.parameterTables());
    const std::int32_t mana_without_weapon =
        world.playerData().currentMana();
    if (!check(
            !world.playerEquipment().item(
                osf::EquipmentSlot::main_hand) &&
                world.commandPlayerMagic(pointer_x, pointer_y) &&
                !world.playerSpellActive() &&
                world.playerData().currentMana() ==
                    mana_without_weapon &&
                world.combatEffects().empty() &&
                world.runtimeEffects().empty(),
            "An unarmed Sonic Blade command spent MP or created an "
            "action/effect.")) {
        return false;
    }

    const osf::ItemDefinition* weapon = nullptr;
    for (const osf::ItemDefinition& definition :
         world.itemDatabase().definitions(0)) {
        if (definition.subtype == 0 &&
            definition.required_level <=
                world.playerData().level()) {
            weapon = &definition;
            break;
        }
    }
    if (!check(
            weapon &&
                world.playerEquipment()
                    .place(
                        osf::EquipmentSlot::main_hand,
                        osf::makeInventoryItem(*weapon),
                        *weapon,
                        world.playerData().level())
                    .accepted,
            "A retail subtype-zero weapon could not prepare Sonic Blade.")) {
        return false;
    }
    world.refreshPlayerAppearance();

    const std::int32_t target_character_number =
        target->characterNumber();
    const std::int32_t mana_before =
        world.playerData().currentMana();
    const std::int32_t weapon_sample =
        osf::retailItemAttackSound(weapon);
    if (!check(
            world.commandPlayerMagic(pointer_x, pointer_y) &&
                world.playerSpellActive() &&
                world.playerSpellTargetCharacterNumber() ==
                    target_character_number &&
                world.playerAnimationChart() == 5 &&
                world.playerData().currentMana() ==
                    mana_before - parameters.mana_cost &&
                world.runtimeEffectControllerCount() == 0,
            "The equipped Sonic Blade command did not enter action 37 "
            "with its weapon chart and retail MP cost.")) {
        return false;
    }

    bool saw_charge = false;
    bool saw_projectile = false;
    bool heard_launch = false;
    bool heard_weapon = false;
    bool saw_recovery = false;
    for (std::int32_t update = 0; update < 100; ++update) {
        world.update();
        const std::vector<std::int32_t> audio =
            world.takeAudioSamples();
        heard_launch =
            heard_launch ||
            std::find(audio.begin(), audio.end(), 154) !=
                audio.end();
        heard_weapon =
            heard_weapon ||
            std::find(
                audio.begin(), audio.end(), weapon_sample) !=
                audio.end();
        saw_charge =
            saw_charge ||
            std::any_of(
                world.combatEffects().begin(),
                world.combatEffects().end(),
                [](const osf::CombatEffectActor& effect) {
                    return effect.effectNumber() == 21025 &&
                           effect.resourceId() == 11000100;
                });
        saw_projectile =
            saw_projectile ||
            std::any_of(
                world.runtimeEffects().begin(),
                world.runtimeEffects().end(),
                [](const osf::RuntimeEffectActor& actor) {
                    return actor.controllerEffectNumber() == 10015 &&
                           actor.resourceId() == 10000090 &&
                           actor.displayHeight() == 155 &&
                           actor.lifetime() == 7;
                });
        saw_recovery =
            saw_recovery || world.playerAnimationChart() == 6;
        if (!world.playerSpellActive() &&
            world.runtimeEffects().empty() &&
            world.combatEffects().empty()) {
            break;
        }
    }

    const bool passed =
        saw_charge && saw_projectile && heard_launch &&
        heard_weapon && saw_recovery &&
        !world.playerSpellActive();
    if (!passed) {
        std::cerr
            << "charge=" << saw_charge
            << " projectile=" << saw_projectile
            << " launch=" << heard_launch
            << " weapon=" << heard_weapon
            << " recovery=" << saw_recovery
            << " active=" << world.playerSpellActive()
            << " effects=" << world.runtimeEffects().size()
            << ',' << world.combatEffects().size()
            << " distance2=" << target_distance_squared
            << " speed="
            << world.parameterTables().find(35)->value(15, 0)
            << '\n';
    }
    return check(
        passed,
        "The shipped Sonic Blade action did not show its charge and "
        "projectile, play both sounds, and recover.");
}

bool testShippedHellFireCast(
    const std::filesystem::path& game_root) {
    constexpr std::int32_t spell = 4;
    osf::PlayerLoadRequest player;
    player.name = "HellFireLive";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root,
                player,
                {3000507, 3, 0},
                &error),
            "The shipped Hell Fire scenario could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[spell] = 3;
    world.playerMagic().restore(magic_state);
    if (!check(
            world.playerMagic().selectSpell(spell),
            "The live Hell Fire fixture could not select the spell.")) {
        return false;
    }

    const osf::EnemyActor* pointed_enemy = nullptr;
    std::int32_t pointer_x = -1;
    std::int32_t pointer_y = -1;
    for (const osf::EnemyActor& enemy : world.enemies()) {
        const osf::ScreenPosition projected =
            osf::calculateRealPosition(enemy.position());
        const std::int32_t anchor_x =
            projected.x - world.cameraScreenX();
        const std::int32_t anchor_y =
            projected.y - world.cameraScreenY();
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
                    pointed_enemy = &enemy;
                    pointer_x = x;
                    pointer_y = y;
                    break;
                }
            }
        }
        if (pointed_enemy) {
            break;
        }
    }
    if (!check(
            pointed_enemy && pointer_x >= 0,
            "No shipped enemy could prove Hell Fire's ground command.")) {
        return false;
    }

    const std::int32_t target_character_number =
        pointed_enemy->characterNumber();
    const std::int32_t target_life_before =
        pointed_enemy->currentLife();
    const std::int32_t mana_before =
        world.playerData().currentMana();
    const osf::WorldPosition aimed_position =
        osf::calculateWorldPosition({
            world.cameraScreenX() + pointer_x,
            world.cameraScreenY() + pointer_y,
        });
    const std::int32_t expected_direction =
        osf::retailDirectionForVector(
            aimed_position.x - world.playerWorldX(),
            aimed_position.y - world.playerWorldY());
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
                world.playerSpellTargetCharacterNumber() == -1 &&
                world.playerDirection() ==
                    expected_direction &&
                world.playerMotion() ==
                    osf::PlayerMotion::casting &&
                world.playerAnimationChart() == 13 &&
                world.playerData().currentMana() ==
                    mana_before - parameters.mana_cost &&
                world.runtimeEffectControllerCount() == 1,
            "A pointed enemy incorrectly diverted Hell Fire from the "
            "retail ground/self command.")) {
        return false;
    }

    bool saw_warning = false;
    bool saw_first_burst = false;
    bool saw_second_burst = false;
    bool saw_first_audio = false;
    bool saw_second_audio = false;
    bool applied_damage = false;
    for (std::int32_t update = 0; update < 80; ++update) {
        world.update();
        const std::vector<std::int32_t> audio =
            world.takeAudioSamples();
        saw_first_audio =
            saw_first_audio ||
            std::find(audio.begin(), audio.end(), 29) !=
                audio.end();
        saw_second_audio =
            saw_second_audio ||
            std::find(audio.begin(), audio.end(), 23) !=
                audio.end();
        for (const osf::RuntimeEffectActor& actor :
             world.runtimeEffects()) {
            saw_warning =
                saw_warning ||
                actor.resourceId() == 10000002;
            saw_first_burst =
                saw_first_burst ||
                (actor.resourceId() == 10000000 &&
                 actor.animationChart() == 1);
            saw_second_burst =
                saw_second_burst ||
                (actor.resourceId() == 10000000 &&
                 actor.animationChart() == 0);
        }
        const auto target = std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [target_character_number](
                const osf::EnemyActor& enemy) {
                return enemy.characterNumber() ==
                    target_character_number;
            });
        applied_damage =
            applied_damage ||
            target == world.enemies().end() ||
            target->currentLife() < target_life_before;
    }

    if (!applied_damage ||
        world.playerMagic().experience(spell) < 1) {
        const auto target = std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [target_character_number](
                const osf::EnemyActor& enemy) {
                return enemy.characterNumber() ==
                    target_character_number;
            });
        if (target != world.enemies().end()) {
            osf::PlayerSpellCastInput input;
            input.stats.source_character_number = 0;
            input.stats.player_level = 1;
            input.stats.magical_attack = 10;
            input.stats.physical_defense = 10;
            input.stats.magical_defense = 10;
            input.stats.magical_hit_rate = 40;
            input.parameters.effective_level = 1;
            input.parameters.effect_value = 40;
            input.target_character_number = -1;
            input.source_position = target->position();
            input.source_judgement = {};
            input.target_position = target->position();
            input.effect_delay = 4;
            osf::CombatEffectSpawnRequest contact =
                osf::buildPlayerSpellCast(
                    spell,
                    input,
                    world.parameterTables());
            contact.owner_kind = 0;
            contact.has_explicit_origin = true;
            contact.origin = target->position();
            contact.packet.write(36, 100000);
            world.queueCombatEffect(contact);
            for (std::int32_t update = 0;
                 update < 8 && !applied_damage;
                 ++update) {
                world.update();
                world.takeAudioSamples();
                const auto current = std::find_if(
                    world.enemies().begin(),
                    world.enemies().end(),
                    [target_character_number](
                        const osf::EnemyActor& enemy) {
                        return enemy.characterNumber() ==
                            target_character_number;
                    });
                applied_damage =
                    current == world.enemies().end() ||
                    current->currentLife() <
                        target_life_before;
            }
        }
    }

    return check(
        saw_warning &&
            saw_first_burst &&
            saw_second_burst &&
            saw_first_audio &&
            saw_second_audio &&
            applied_damage &&
            world.playerMagic().experience(spell) >= 1,
        "The shipped Hell Fire command did not preserve its warning, "
        "burst layers, sounds, area contact, or practice award.");
}

bool testShippedIceBlastCast(
    const std::filesystem::path& game_root) {
    constexpr std::int32_t spell = 5;
    osf::PlayerLoadRequest player;
    player.name = "IceBlastLive";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root,
                player,
                {3000507, 3, 0},
                &error),
            "The shipped Ice Blast scenario could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[spell] = 3;
    world.playerMagic().restore(magic_state);
    if (!check(
            world.playerMagic().selectSpell(spell),
            "The live Ice Blast fixture could not select the spell.")) {
        return false;
    }

    const osf::EnemyActor* pointed_enemy = nullptr;
    std::int32_t pointer_x = -1;
    std::int32_t pointer_y = -1;
    for (const osf::EnemyActor& enemy : world.enemies()) {
        const osf::ScreenPosition projected =
            osf::calculateRealPosition(enemy.position());
        const std::int32_t anchor_x =
            projected.x - world.cameraScreenX();
        const std::int32_t anchor_y =
            projected.y - world.cameraScreenY();
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
                    pointed_enemy = &enemy;
                    pointer_x = x;
                    pointer_y = y;
                    break;
                }
            }
        }
        if (pointed_enemy) {
            break;
        }
    }
    if (!check(
            pointed_enemy && pointer_x >= 0,
            "No shipped enemy could prove Ice Blast's ground command.")) {
        return false;
    }

    const std::int32_t target_character_number =
        pointed_enemy->characterNumber();
    const std::int32_t target_life_before =
        pointed_enemy->currentLife();
    const osf::WorldPosition player_position_before{
        world.playerWorldX(),
        world.playerWorldY(),
    };
    const std::int32_t mana_before =
        world.playerData().currentMana();
    const osf::WorldPosition aimed_position =
        osf::calculateWorldPosition({
            world.cameraScreenX() + pointer_x,
            world.cameraScreenY() + pointer_y,
        });
    const std::int32_t expected_direction =
        osf::retailDirectionForVector(
            aimed_position.x - world.playerWorldX(),
            aimed_position.y - world.playerWorldY());
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
                world.playerSpellTargetCharacterNumber() == -1 &&
                world.playerDirection() == expected_direction &&
                world.playerMotion() ==
                    osf::PlayerMotion::casting &&
                world.playerAnimationChart() == 11 &&
                world.playerData().currentMana() ==
                    mana_before - parameters.mana_cost &&
                world.runtimeEffectControllerCount() == 1,
            "A pointed enemy incorrectly diverted Ice Blast from the "
            "retail ground/self command.")) {
        return false;
    }

    bool saw_first = false;
    bool saw_second = false;
    bool saw_third = false;
    bool first_captured_player = false;
    bool heard_pulse = false;
    bool applied_damage = false;
    for (std::int32_t update = 0; update < 100; ++update) {
        world.update();
        const std::vector<std::int32_t> audio =
            world.takeAudioSamples();
        heard_pulse =
            heard_pulse ||
            std::find(audio.begin(), audio.end(), 22) !=
                audio.end();
        for (const osf::RuntimeEffectActor& actor :
             world.runtimeEffects()) {
            if (actor.resourceId() == 10000051) {
                saw_first = true;
                first_captured_player =
                    first_captured_player ||
                    (actor.position().x ==
                         player_position_before.x &&
                     actor.position().y ==
                         player_position_before.y);
            }
            saw_second =
                saw_second ||
                actor.resourceId() == 10000050;
            saw_third =
                saw_third ||
                actor.resourceId() == 10000052;
        }
        const auto target = std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [target_character_number](
                const osf::EnemyActor& enemy) {
                return enemy.characterNumber() ==
                    target_character_number;
            });
        applied_damage =
            applied_damage ||
            target == world.enemies().end() ||
            target->currentLife() < target_life_before;
    }

    if (!applied_damage ||
        world.playerMagic().experience(spell) < 1) {
        const auto target = std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [target_character_number](
                const osf::EnemyActor& enemy) {
                return enemy.characterNumber() ==
                    target_character_number;
            });
        if (target != world.enemies().end()) {
            osf::PlayerSpellCastInput input;
            input.stats.source_character_number = 0;
            input.stats.player_level = 1;
            input.stats.magical_attack = 10;
            input.stats.physical_defense = 10;
            input.stats.magical_defense = 10;
            input.stats.magical_hit_rate = 40;
            input.parameters.effective_level = 1;
            input.parameters.effect_value = 40;
            input.target_character_number = -1;
            input.source_position = target->position();
            input.source_judgement = {};
            input.target_position = target->position();
            input.effect_delay = 4;
            osf::CombatEffectSpawnRequest contact =
                osf::buildPlayerSpellCast(
                    spell,
                    input,
                    world.parameterTables());
            contact.owner_kind = 0;
            contact.has_explicit_origin = true;
            contact.origin = target->position();
            contact.packet.write(36, 100000);
            world.queueCombatEffect(contact);
            for (std::int32_t update = 0;
                 update < 80 && !applied_damage;
                 ++update) {
                world.update();
                world.takeAudioSamples();
                const auto current = std::find_if(
                    world.enemies().begin(),
                    world.enemies().end(),
                    [target_character_number](
                        const osf::EnemyActor& enemy) {
                        return enemy.characterNumber() ==
                            target_character_number;
                    });
                applied_damage =
                    current == world.enemies().end() ||
                    current->currentLife() <
                        target_life_before;
            }
        }
    }

    return check(
        saw_first &&
            saw_second &&
            saw_third &&
            first_captured_player &&
            heard_pulse &&
            applied_damage &&
            world.playerMagic().experience(spell) >= 1,
        "The shipped Ice Blast command did not preserve its "
        "self-centered layers, pulse audio, area contact, or practice.");
}

bool testShippedHealCast(
    const std::filesystem::path& game_root,
    const osf::TableDatabase& tables) {
    constexpr std::int32_t spell = 6;
    osf::PlayerData saved_player;
    std::string error;
    if (!check(
            saved_player.initializeNew(
                "HealLive", 0, tables, &error),
            "The live Heal player could not be initialized.")) {
        std::cerr << error << '\n';
        return false;
    }
    saved_player.setCurrentLife(
        saved_player.baseMaximumLife() - 1);

    const auto unique =
        std::chrono::high_resolution_clock::now()
            .time_since_epoch()
            .count();
    const std::filesystem::path save_path =
        std::filesystem::temp_directory_path() /
        ("openshadowflare-heal-" +
         std::to_string(unique) + ".sav");
    std::error_code cleanup_error;
    std::filesystem::remove(save_path, cleanup_error);
    if (!check(
            osf::writeRetailSave(
                save_path,
                saved_player,
                0x5a,
                &error),
            "The live Heal save fixture could not be written.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerLoadRequest player;
    player.source = osf::PlayerDataSource::retail_save;
    player.save_path = save_path;
    osf::WorldScene world;
    const bool loaded = world.loadInitialScenario(
        game_root, player, &error);
    std::filesystem::remove(save_path, cleanup_error);
    if (!check(
            loaded,
            "Remote Town could not load the damaged Heal fixture.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[spell] = 3;
    world.playerMagic().restore(magic_state);
    if (!check(
            world.playerMagic().selectSpell(spell),
            "The live Heal fixture could not select the spell.")) {
        return false;
    }

    const std::int32_t life_before =
        world.playerData().currentLife();
    const std::int32_t maximum_life =
        world.playerData().baseMaximumLife();
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
            life_before == maximum_life - 1 &&
                world.commandPlayerMagic(400, 240) &&
                world.playerSpellActive() &&
                world.playerSpellTargetCharacterNumber() == -1 &&
                world.playerAnimationChart() == 11 &&
                world.playerData().currentMana() ==
                    mana_before - parameters.mana_cost &&
                world.playerData().currentLife() == life_before &&
                world.playerMagic().experience(spell) == 0 &&
                world.combatEffects().empty(),
            "Heal resolved before its retail CAF marker or lost its "
            "targetless action and MP cost.")) {
        return false;
    }

    bool saw_visual = false;
    bool heard_heal = false;
    for (std::int32_t update = 0; update < 100; ++update) {
        world.update();
        const std::vector<std::int32_t> audio =
            world.takeAudioSamples();
        heard_heal =
            heard_heal ||
            std::find(audio.begin(), audio.end(), 17) !=
                audio.end();
        saw_visual =
            saw_visual ||
            std::any_of(
                world.combatEffects().begin(),
                world.combatEffects().end(),
                [](const osf::CombatEffectActor& effect) {
                    return effect.effectNumber() == 21020 &&
                           effect.resourceId() == 11000060;
                });
        if (!world.playerSpellActive() &&
            world.combatEffects().empty()) {
            break;
        }
    }
    if (!check(
            saw_visual &&
                heard_heal &&
                world.playerData().currentLife() == maximum_life &&
                world.playerMagic().experience(spell) == 1,
            "The shipped Heal marker did not restore life, show "
            "resource 11000060, play sample 17, and award practice.")) {
        return false;
    }

    const std::int32_t full_experience =
        world.playerMagic().experience(spell);
    const std::int32_t full_mana_before =
        world.playerData().currentMana();
    if (!check(
            full_mana_before >= parameters.mana_cost &&
                world.commandPlayerMagic(400, 240),
            "The full-life Heal command could not be started.")) {
        return false;
    }
    bool saw_full_visual = false;
    bool heard_full_heal = false;
    for (std::int32_t update = 0; update < 100; ++update) {
        world.update();
        const std::vector<std::int32_t> audio =
            world.takeAudioSamples();
        heard_full_heal =
            heard_full_heal ||
            std::find(audio.begin(), audio.end(), 17) !=
                audio.end();
        saw_full_visual =
            saw_full_visual ||
            std::any_of(
                world.combatEffects().begin(),
                world.combatEffects().end(),
                [](const osf::CombatEffectActor& effect) {
                    return effect.effectNumber() == 21020 &&
                           effect.resourceId() == 11000060;
                });
        if (!world.playerSpellActive() &&
            world.combatEffects().empty()) {
            break;
        }
    }
    return check(
        world.playerData().currentMana() ==
                full_mana_before - parameters.mana_cost &&
            world.playerData().currentLife() == maximum_life &&
            world.playerMagic().experience(spell) ==
                full_experience &&
            saw_full_visual &&
            !heard_full_heal,
        "A full-life Heal lost its visual or incorrectly restored, "
        "played sample 17, or awarded practice.");
}

bool testShippedMoonCast(
    const std::filesystem::path& game_root) {
    constexpr std::int32_t spell = 7;
    osf::PlayerLoadRequest player;
    player.name = "MoonLive";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root, player, &error),
            "Remote Town could not prepare the Moon fixture.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[spell] = 3;
    world.playerMagic().restore(magic_state);
    if (!check(
            world.playerMagic().selectSpell(spell),
            "The live Moon fixture could not select the spell.")) {
        return false;
    }

    const osf::CompanionProfile base =
        world.companion().profile();
    const osf::PlayerSpellParameters parameters =
        osf::playerSpellParameters(
            world.playerMagic(),
            spell,
            world.playerEquipment(),
            world.itemDatabase(),
            world.parameterTables());
    const std::int32_t mana_before =
        world.playerData().currentMana();
    if (!check(
            world.commandPlayerMagic(400, 240) &&
                world.playerSpellActive() &&
                !world.playerMoonActive() &&
                !world.companionMoonAuraVisible() &&
                world.playerAnimationChart() == 11 &&
                world.playerSpellTargetCharacterNumber() == -1 &&
                world.playerData().currentMana() ==
                    mana_before - parameters.mana_cost,
            "Moon resolved before its retail CAF marker or lost its "
            "targetless action and MP cost.")) {
        return false;
    }

    bool activated = false;
    for (std::int32_t update = 0; update < 100; ++update) {
        world.update();
        world.takeAudioSamples();
        if (world.playerMoonActive()) {
            activated = true;
            break;
        }
    }
    osf::PlayerSustainedSpell expected_moon;
    expected_moon.toggle(
        200,
        parameters.effective_level,
        world.parameterTables());
    const osf::CompanionProfile expected =
        osf::applyPlayerMoonCompanionModifiers(
            base, expected_moon, world.parameterTables());
    const osf::CompanionProfile active =
        world.companion().profile();
    if (!check(
            activated &&
                world.companionMoonAuraVisible() &&
                world.companionMoonAuraVisual() != nullptr &&
                active.attack_speed_rating ==
                    expected.attack_speed_rating &&
                active.walking_speed_raw ==
                    expected.walking_speed_raw &&
                active.running_speed_raw ==
                    expected.running_speed_raw &&
                active.maximum_life == expected.maximum_life &&
                active.physical_attack ==
                    expected.physical_attack &&
                active.hit_rate == expected.hit_rate &&
                active.physical_defense ==
                    expected.physical_defense &&
                active.physical_evasion ==
                    expected.physical_evasion &&
                active.magical_attack ==
                    expected.magical_attack &&
                active.magical_hit_rate ==
                    expected.magical_hit_rate &&
                active.magical_defense ==
                    expected.magical_defense &&
                active.magical_evasion ==
                    expected.magical_evasion &&
                active.parameter_17 == expected.parameter_17,
            "The Moon marker did not enable its companion aura and "
            "Table 200 runtime profile.")) {
        return false;
    }

    while (world.playerSpellActive()) {
        world.update();
        world.takeAudioSamples();
    }
    if (!check(
            world.playerData().currentMana() >=
                    parameters.mana_cost &&
                world.commandPlayerMagic(400, 240),
            "The active Moon spell could not start its toggle-off cast.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 100 && world.playerMoonActive();
         ++update) {
        world.update();
        world.takeAudioSamples();
    }
    const osf::CompanionProfile restored =
        world.companion().profile();
    return check(
        !world.playerMoonActive() &&
            !world.companionMoonAuraVisible() &&
            restored.attack_speed_rating ==
                base.attack_speed_rating &&
            restored.walking_speed_raw ==
                base.walking_speed_raw &&
            restored.running_speed_raw ==
                base.running_speed_raw &&
            restored.maximum_life == base.maximum_life &&
            restored.physical_attack == base.physical_attack &&
            restored.hit_rate == base.hit_rate &&
            restored.physical_defense == base.physical_defense &&
            restored.physical_evasion == base.physical_evasion &&
            restored.magical_attack == base.magical_attack &&
            restored.magical_hit_rate == base.magical_hit_rate &&
            restored.magical_defense == base.magical_defense &&
            restored.magical_evasion == base.magical_evasion &&
            restored.parameter_17 == base.parameter_17,
        "The second Moon marker did not remove its aura and restore "
        "the base companion profile.");
}

bool testShippedBerserkerCast(
    const std::filesystem::path& game_root) {
    constexpr std::int32_t spell = 8;
    osf::PlayerLoadRequest player;
    player.name = "BerserkerLive";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(game_root, player, &error),
            "Remote Town could not prepare the Berserker fixture.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[spell] = 3;
    world.playerMagic().restore(magic_state);
    if (!check(
            world.playerMagic().selectSpell(spell),
            "The live Berserker fixture could not select the spell.")) {
        return false;
    }

    const osf::PlayerRuntimeProfile base =
        world.playerRuntimeProfile();
    const osf::PlayerSpellParameters parameters =
        osf::playerSpellParameters(
            world.playerMagic(),
            spell,
            world.playerEquipment(),
            world.itemDatabase(),
            world.parameterTables());
    const std::int32_t mana_before =
        world.playerData().currentMana();
    if (!check(
            world.commandPlayerMagic(400, 240) &&
                world.playerSpellActive() &&
                !world.playerBerserkerActive() &&
                world.playerBerserkerVisual() == nullptr &&
                world.playerAnimationChart() == 11 &&
                world.playerSpellTargetCharacterNumber() == -1 &&
                world.playerData().currentMana() ==
                    mana_before - parameters.mana_cost,
            "Berserker resolved before its retail CAF marker or lost "
            "its targetless action and MP cost.")) {
        return false;
    }

    bool activated = false;
    for (std::int32_t update = 0; update < 100; ++update) {
        world.update();
        world.takeAudioSamples();
        if (world.playerBerserkerActive()) {
            activated = true;
            break;
        }
    }
    const osf::PlayerRuntimeProfile active =
        world.playerRuntimeProfile();
    if (!check(
            activated &&
                world.playerBerserkerVisual() != nullptr &&
                active.attack_speed_raw > base.attack_speed_raw &&
                active.walking_speed_raw > base.walking_speed_raw &&
                active.magical_attack > base.magical_attack &&
                active.physical_defense < base.physical_defense &&
                active.physical_evasion < base.physical_evasion,
            "The Berserker marker did not enable its Powerup visual and "
            "Table 201 runtime profile.")) {
        return false;
    }
    const std::int32_t frame_before =
        world.playerBerserkerFrame();
    world.update();
    world.takeAudioSamples();
    if (!check(
            world.playerBerserkerFrame() > frame_before,
            "The active Berserker Powerup animation did not advance.")) {
        return false;
    }

    while (world.playerSpellActive()) {
        world.update();
        world.takeAudioSamples();
    }
    if (!check(
            world.playerData().currentMana() >=
                    parameters.mana_cost &&
                world.commandPlayerMagic(400, 240),
            "The active Berserker spell could not start its toggle-off "
            "cast.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 100 && world.playerBerserkerActive();
         ++update) {
        world.update();
        world.takeAudioSamples();
    }
    const osf::PlayerRuntimeProfile restored =
        world.playerRuntimeProfile();
    return check(
        !world.playerBerserkerActive() &&
            restored.attack_speed_raw == base.attack_speed_raw &&
            restored.walking_speed_raw == base.walking_speed_raw &&
            restored.maximum_life == base.maximum_life &&
            restored.maximum_mana == base.maximum_mana &&
            restored.physical_attack == base.physical_attack &&
            restored.physical_defense == base.physical_defense &&
            restored.hit_rate == base.hit_rate &&
            restored.physical_evasion == base.physical_evasion &&
            restored.magical_attack == base.magical_attack &&
            restored.magical_defense == base.magical_defense &&
            restored.magical_hit_rate == base.magical_hit_rate &&
            restored.magical_evasion == base.magical_evasion,
        "The second Berserker marker did not remove its aura and restore "
        "the base player profile.");
}

bool testShippedEnergyShieldCast(
    const std::filesystem::path& game_root) {
    constexpr std::int32_t spell = 9;
    osf::PlayerLoadRequest player;
    player.name = "EnergyShieldLive";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(game_root, player, &error),
            "Remote Town could not prepare the Energy Shield fixture.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[spell] = 3;
    world.playerMagic().restore(magic_state);
    if (!check(
            world.playerMagic().selectSpell(spell),
            "The live Energy Shield fixture could not select the spell.")) {
        return false;
    }

    const osf::PlayerSpellParameters parameters =
        osf::playerSpellParameters(
            world.playerMagic(),
            spell,
            world.playerEquipment(),
            world.itemDatabase(),
            world.parameterTables());
    const std::int32_t mana_before =
        world.playerData().currentMana();
    if (!check(
            mana_before > parameters.mana_cost &&
                world.commandPlayerMagic(400, 240) &&
                world.playerSpellActive() &&
                !world.playerEnergyShieldActive() &&
                world.playerEnergyShieldVisual() == nullptr &&
                world.playerAnimationChart() == 11 &&
                world.playerSpellTargetCharacterNumber() == -1 &&
                world.playerData().currentMana() ==
                    mana_before - parameters.mana_cost,
            "Energy Shield resolved before its retail CAF marker or "
            "lost its targetless action and MP cost.")) {
        return false;
    }

    for (std::int32_t update = 0;
         update < 100 && !world.playerEnergyShieldActive();
         ++update) {
        world.update();
        world.takeAudioSamples();
    }
    if (!check(
            world.playerEnergyShieldActive() &&
                world.playerEnergyShieldVisual() != nullptr,
            "The Energy Shield marker did not enable its Powerup aura.")) {
        return false;
    }
    const std::int32_t frame_before =
        world.playerEnergyShieldFrame();
    world.update();
    world.takeAudioSamples();
    if (!check(
            world.playerEnergyShieldFrame() > frame_before,
            "The active Energy Shield Powerup animation did not advance.")) {
        return false;
    }

    while (world.playerSpellActive()) {
        world.update();
        world.takeAudioSamples();
    }
    if (!check(
            world.playerData().currentMana() >=
                    parameters.mana_cost &&
                world.commandPlayerMagic(400, 240),
            "The active Energy Shield could not start its toggle-off cast.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 100 && world.playerEnergyShieldActive();
         ++update) {
        world.update();
        world.takeAudioSamples();
    }
    if (!check(
            !world.playerEnergyShieldActive(),
            "The second Energy Shield marker did not toggle it off.")) {
        return false;
    }

    while (world.playerSpellActive()) {
        world.update();
        world.takeAudioSamples();
    }
    osf::PlayerData exact_mana_player;
    if (!check(
            exact_mana_player.initializeNew(
                "EnergyShieldExact",
                0,
                world.parameterTables(),
                &error),
            "The exact-cost Energy Shield player could not be prepared.")) {
        return false;
    }
    exact_mana_player.setCurrentMana(parameters.mana_cost);
    const auto unique =
        std::chrono::high_resolution_clock::now()
            .time_since_epoch()
            .count();
    const std::filesystem::path save_path =
        std::filesystem::temp_directory_path() /
        ("openshadowflare-energy-shield-" +
         std::to_string(unique) + ".sav");
    std::error_code cleanup_error;
    std::filesystem::remove(save_path, cleanup_error);
    if (!check(
            osf::writeRetailSave(
                save_path,
                exact_mana_player,
                0x5a,
                &error),
            "The exact-cost Energy Shield save could not be written.")) {
        return false;
    }
    osf::PlayerLoadRequest exact_request;
    exact_request.source = osf::PlayerDataSource::retail_save;
    exact_request.save_path = save_path;
    osf::WorldScene exact_world;
    const bool exact_loaded = exact_world.loadInitialScenario(
        game_root, exact_request, &error);
    std::filesystem::remove(save_path, cleanup_error);
    if (!check(
            exact_loaded,
            "The exact-cost Energy Shield save could not be loaded.")) {
        return false;
    }
    exact_world.playerMagic().restore(magic_state);
    exact_world.playerMagic().selectSpell(spell);
    if (!check(
            exact_world.commandPlayerMagic(400, 240) &&
                exact_world.playerData().currentMana() == 0,
            "The exact-cost Energy Shield command was not accepted.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 100 && exact_world.playerSpellActive();
         ++update) {
        exact_world.update();
        exact_world.takeAudioSamples();
    }
    if (!check(
            !exact_world.playerEnergyShieldActive(),
            "Energy Shield activated after its cast consumed the last MP.")) {
        return false;
    }

    osf::WorldScene shutdown_world;
    if (!check(
            shutdown_world.loadInitialScenario(
                game_root, player, &error),
            "The Energy Shield shutdown fixture could not be loaded.")) {
        return false;
    }
    shutdown_world.playerMagic().restore(magic_state);
    shutdown_world.playerMagic().selectSpell(spell);
    if (!check(
            shutdown_world.commandPlayerMagic(400, 240),
            "Energy Shield could not be recast for its shutdown check.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 100 && !shutdown_world.playerEnergyShieldActive();
         ++update) {
        shutdown_world.update();
        shutdown_world.takeAudioSamples();
    }
    const osf::InventoryItem* body =
        shutdown_world.playerEquipment().item(
            osf::EquipmentSlot::body);
    const osf::ItemDefinition* body_definition =
        body
            ? shutdown_world.itemDatabase().find(
                  body->category, body->definition_id)
            : nullptr;
    std::optional<osf::InventoryItem> draining_body =
        shutdown_world.playerEquipment().take(
            osf::EquipmentSlot::body);
    if (draining_body) {
        draining_body->retail_state.resize(200u);
    }
    if (!check(
            shutdown_world.playerEnergyShieldActive() &&
                body_definition && draining_body &&
                writeRetailItemWord(*draining_body, 18, -10000) &&
                shutdown_world.playerEquipment().place(
                    osf::EquipmentSlot::body,
                    *draining_body,
                    *body_definition,
                    shutdown_world.playerData().level()).accepted,
            "The zero-MP Energy Shield shutdown fixture could not be "
            "prepared.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 4 && shutdown_world.playerEnergyShieldActive();
         ++update) {
        shutdown_world.update();
        shutdown_world.takeAudioSamples();
    }
    return check(
        shutdown_world.playerData().currentMana() == 0 &&
            !shutdown_world.playerEnergyShieldActive(),
        "Energy Shield did not shut off when the player reached zero MP.");
}

bool testShippedIdentifyCast(
    const std::filesystem::path& game_root) {
    osf::PlayerLoadRequest player;
    player.name = "IdentifyLive";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root, player, &error),
            "Remote Town could not prepare Identify.")) {
        std::cerr << error << '\n';
        return false;
    }

    const osf::ItemDefinition* ordinary =
        world.itemDatabase().find(0, 0);
    const osf::ItemDefinition* unidentified =
        world.itemDatabase().find(0, 10);
    world.playerInventory().clear();
    if (!check(
            ordinary && unidentified &&
                ordinary->variant == 0 &&
                unidentified->variant == 1 &&
                world.playerInventory().add(*ordinary) &&
                world.playerInventory().add(*unidentified) &&
                world.playerInventory().items().size() == 2 &&
                world.playerInventory().items()[0].identified == 1 &&
                world.playerInventory().items()[1].identified == 0,
            "The Identify backpack fixture could not be prepared.")) {
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[17] = 3;
    world.playerMagic().restore(magic_state);
    if (!check(
            world.playerMagic().selectSpell(17),
            "Identify could not be selected.")) {
        return false;
    }

    const osf::PlayerSpellParameters parameters =
        osf::playerSpellParameters(
            world.playerMagic(),
            17,
            world.playerEquipment(),
            world.itemDatabase(),
            world.parameterTables());
    const std::int32_t mana_before =
        world.playerData().currentMana();
    if (!check(
            world.commandPlayerMagic(400, 240) &&
                world.playerSpellActive() &&
                world.playerSpellTargetCharacterNumber() == -1 &&
                world.playerMotion() == osf::PlayerMotion::casting &&
                world.playerAnimationChart() == 11 &&
                world.playerData().currentMana() ==
                    mana_before - parameters.mana_cost &&
                !world.playerIdentifyModeActive(),
            "Identify did not enter action 39 with its retail MP cost.")) {
        return false;
    }

    bool saw_entry_visual = false;
    bool saw_inventory_request = false;
    for (std::int32_t update = 0;
         update < 100 &&
         (world.playerSpellActive() ||
          !saw_inventory_request);
         ++update) {
        world.update();
        saw_entry_visual =
            saw_entry_visual ||
            std::any_of(
                world.combatEffects().begin(),
                world.combatEffects().end(),
                [](const osf::CombatEffectActor& effect) {
                    return effect.effectNumber() == 21028 &&
                           effect.resourceId() == 11000230;
                });
        const osf::GameplayServiceRequest service =
            world.takeGameplayServiceRequest();
        saw_inventory_request =
            saw_inventory_request ||
            service.kind ==
                osf::GameplayServiceKind::identify_item;
    }
    if (!check(
            saw_entry_visual &&
                saw_inventory_request &&
                world.playerIdentifyModeActive() &&
                !world.playerSpellActive(),
            "Identify did not show effect 21028 and enter item-selection "
            "mode at its CAF marker.")) {
        return false;
    }

    const std::int32_t held_mana =
        world.playerData().currentMana();
    if (!check(
            world.commandPlayerMagic(400, 240) &&
                !world.playerSpellActive() &&
                world.playerData().currentMana() == held_mana &&
                !world.identifyPlayerInventoryItem(0) &&
                world.playerIdentifyModeActive(),
            "An active Identify mode repeated its cast or accepted an "
            "already identified item.")) {
        return false;
    }

    const std::int32_t experience_before =
        world.playerMagic().experience(17);
    if (!check(
            world.identifyPlayerInventoryItem(1) &&
                !world.playerIdentifyModeActive() &&
                world.playerInventory().items()[1].identified == 1 &&
                world.playerMagic().experience(17) ==
                    experience_before + 1 &&
                !world.identifyPlayerInventoryItem(1),
            "Identifying a backpack item did not persist its flag and "
            "award exactly one practice point.")) {
        return false;
    }

    const_cast<osf::PlayerData&>(world.playerData()).setCurrentMana(
        std::max(parameters.mana_cost - 1, 0));
    const std::int32_t insufficient_mana =
        world.playerData().currentMana();
    return check(
        world.commandPlayerMagic(400, 240) &&
            !world.playerSpellActive() &&
            !world.playerIdentifyModeActive() &&
            world.playerData().currentMana() ==
                insufficient_mana,
        "An insufficient-MP Identify command created an action or mode.");
}

bool testShippedMagicShieldCast(
    const std::filesystem::path& game_root) {
    osf::PlayerLoadRequest player;
    player.name = "MagicShieldLive";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root, player, &error),
            "Remote Town could not prepare Magic Shield.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[18] = 3;
    world.playerMagic().restore(magic_state);
    if (!check(
            world.playerMagic().selectSpell(18),
            "Magic Shield could not be selected.")) {
        return false;
    }
    const osf::PlayerSpellParameters parameters =
        osf::playerSpellParameters(
            world.playerMagic(),
            18,
            world.playerEquipment(),
            world.itemDatabase(),
            world.parameterTables());
    const std::int32_t maximum_mana =
        world.playerRuntimeProfile().maximum_mana;
    const_cast<osf::PlayerData&>(world.playerData()).setCurrentMana(
        maximum_mana, maximum_mana);
    const std::int32_t mana_before =
        world.playerData().currentMana();
    if (!check(
            world.commandPlayerMagic(400, 240) &&
                world.playerSpellActive() &&
                world.playerMotion() == osf::PlayerMotion::casting &&
                world.playerAnimationChart() == 11 &&
                world.playerData().currentMana() ==
                    mana_before - parameters.mana_cost &&
                !world.playerMagicShieldActive(),
            "Magic Shield did not enter action 40 with its retail MP "
            "charge.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 100 &&
         (world.playerSpellActive() ||
          !world.playerMagicShieldActive());
         ++update) {
        world.update();
    }
    if (!check(
            world.playerMagicShieldActive() &&
                !world.playerSpellActive() &&
                world.playerMagicShieldVisual() &&
                !world.playerMagicShieldVisual()
                     ->animation().charts().empty() &&
                world.transitionScenario({0, 0, 0}) ==
                    osf::ScenarioTravelResult::relocated &&
                world.playerMagicShieldActive(),
            "Magic Shield did not activate at its marker, load resource "
            "11000240, or survive ordinary scenario relocation.")) {
        return false;
    }

    const std::int32_t mana_before_toggle_off =
        world.playerData().currentMana();
    if (!check(
            world.commandPlayerMagic(400, 240),
            "The active Magic Shield could not begin its toggle-off cast.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 100 &&
         (world.playerSpellActive() ||
          world.playerMagicShieldActive());
         ++update) {
        world.update();
    }
    if (!check(
            !world.playerMagicShieldActive() &&
                !world.playerSpellActive() &&
                world.playerData().currentMana() ==
                    mana_before_toggle_off - parameters.mana_cost,
            "Magic Shield did not charge MP again and toggle off at its "
            "marker.")) {
        return false;
    }

    const_cast<osf::PlayerData&>(world.playerData()).setCurrentMana(
        parameters.mana_cost, maximum_mana);
    if (!check(
            world.commandPlayerMagic(400, 240) &&
                world.playerData().currentMana() == 0,
            "The exact-cost Magic Shield cast was rejected.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 100 &&
         !world.playerMagicShieldActive();
         ++update) {
        world.update();
    }
    if (!check(
            world.playerMagicShieldActive(),
            "An exact-cost Magic Shield cast did not expose its marker "
            "frame.")) {
        return false;
    }
    world.update();
    return check(
        !world.playerMagicShieldActive(),
        "Zero mana did not disable Magic Shield on the next player "
        "update.");
}

bool testShippedCounterBurstCast(
    const std::filesystem::path& game_root) {
    osf::PlayerLoadRequest player;
    player.name = "CounterBurstLive";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root, player, &error),
            "Remote Town could not prepare Counter Burst.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[18] = 3;
    magic_state.availability[19] = 3;
    world.playerMagic().restore(magic_state);
    const osf::PlayerSpellParameters magic_parameters =
        osf::playerSpellParameters(
            world.playerMagic(),
            18,
            world.playerEquipment(),
            world.itemDatabase(),
            world.parameterTables());
    const osf::PlayerSpellParameters counter_parameters =
        osf::playerSpellParameters(
            world.playerMagic(),
            19,
            world.playerEquipment(),
            world.itemDatabase(),
            world.parameterTables());
    const std::int32_t maximum_mana =
        world.playerRuntimeProfile().maximum_mana;
    const auto restore_mana = [&]() {
        const_cast<osf::PlayerData&>(world.playerData())
            .setCurrentMana(maximum_mana, maximum_mana);
    };
    const auto finish_cast = [&]() {
        for (std::int32_t update = 0;
             update < 100 && world.playerSpellActive();
             ++update) {
            world.update();
        }
        return !world.playerSpellActive();
    };

    if (!check(
            world.playerMagic().selectSpell(18),
            "Magic Shield could not prepare Counter Burst's mutual "
            "exclusion check.")) {
        return false;
    }
    restore_mana();
    if (!check(
            world.commandPlayerMagic(400, 240) &&
                finish_cast() &&
                world.playerMagicShieldActive(),
            "Magic Shield could not establish the opposing live flag.")) {
        return false;
    }

    if (!check(
            world.playerMagic().selectSpell(19),
            "Counter Burst could not be selected.")) {
        return false;
    }
    restore_mana();
    const std::int32_t mana_before =
        world.playerData().currentMana();
    if (!check(
            world.commandPlayerMagic(400, 240) &&
                world.playerSpellActive() &&
                world.playerMotion() == osf::PlayerMotion::casting &&
                world.playerAnimationChart() == 11 &&
                world.playerData().currentMana() ==
                    mana_before - counter_parameters.mana_cost &&
                world.playerMagicShieldActive() &&
                !world.playerCounterBurstActive(),
            "Counter Burst did not enter action 41 with its retail MP "
            "charge.")) {
        return false;
    }
    if (!check(
            finish_cast() &&
                world.playerCounterBurstActive() &&
                !world.playerMagicShieldActive() &&
                world.playerCounterBurstVisual() &&
                !world.playerCounterBurstVisual()
                     ->animation().charts().empty() &&
                world.transitionScenario({0, 0, 0}) ==
                    osf::ScenarioTravelResult::relocated &&
                world.playerCounterBurstActive(),
            "Counter Burst did not activate at its marker, exclude "
            "Magic Shield, load resource 11000250, or survive ordinary "
            "scenario relocation.")) {
        return false;
    }

    restore_mana();
    const std::int32_t mana_before_toggle_off =
        world.playerData().currentMana();
    if (!check(
            world.commandPlayerMagic(400, 240) &&
                finish_cast() &&
                !world.playerCounterBurstActive() &&
                world.playerData().currentMana() ==
                    mana_before_toggle_off -
                        counter_parameters.mana_cost,
            "Counter Burst did not charge MP again and toggle off at "
            "its marker.")) {
        return false;
    }

    const_cast<osf::PlayerData&>(world.playerData()).setCurrentMana(
        counter_parameters.mana_cost, maximum_mana);
    if (!check(
            world.commandPlayerMagic(400, 240) &&
                world.playerData().currentMana() == 0,
            "The exact-cost Counter Burst cast was rejected.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 100 &&
         !world.playerCounterBurstActive();
         ++update) {
        world.update();
    }
    if (!check(
            world.playerCounterBurstActive(),
            "An exact-cost Counter Burst cast did not expose its marker "
            "frame.")) {
        return false;
    }
    world.update();
    if (!check(
            !world.playerCounterBurstActive(),
            "Zero mana did not disable Counter Burst on the next player "
            "update.")) {
        return false;
    }
    if (!check(
            finish_cast(),
            "The exact-cost Counter Burst action did not finish.")) {
        return false;
    }

    restore_mana();
    if (!check(
            world.commandPlayerMagic(400, 240) &&
                finish_cast() &&
                world.playerCounterBurstActive(),
            "Counter Burst could not be reactivated for the reverse "
            "exclusion check.")) {
        return false;
    }
    if (!check(
            world.playerMagic().selectSpell(18),
            "Magic Shield could not be reselected.")) {
        return false;
    }
    restore_mana();
    return check(
        world.commandPlayerMagic(400, 240) &&
            finish_cast() &&
            world.playerMagicShieldActive() &&
            !world.playerCounterBurstActive() &&
            world.playerData().currentMana() ==
                maximum_mana - magic_parameters.mana_cost,
        "Magic Shield did not exclude an active Counter Burst at its "
        "marker.");
}

bool testShippedExplosionCast(
    const std::filesystem::path& game_root) {
    constexpr std::int32_t spell = 20;
    osf::PlayerLoadRequest player;
    player.name = "ExplosionLive";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root,
                player,
                {3000507, 3, 0},
                &error),
            "The shipped Explosion scenario could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[spell] = 3;
    magic_state.availability[21] = 3;
    world.playerMagic().restore(magic_state);
    if (!check(
            world.playerMagic().selectSpell(spell) &&
                world.hasCompanion(),
            "The live Explosion fixture could not select the spell "
            "or find its owned companion.")) {
        return false;
    }

    const osf::EnemyActor* target = nullptr;
    std::int32_t pointer_x = -1;
    std::int32_t pointer_y = -1;
    osf::WorldPosition destination;
    for (const osf::EnemyActor& enemy : world.enemies()) {
        for (std::int32_t offset_y = -240;
             offset_y <= 240 && !target;
             offset_y += 40) {
            for (std::int32_t offset_x = -240;
                 offset_x <= 240;
                 offset_x += 40) {
                const osf::WorldPosition candidate{
                    enemy.position().x + offset_x,
                    enemy.position().y + offset_y,
                };
                const osf::ScreenPosition projected =
                    osf::calculateRealPosition(candidate);
                const std::int32_t x =
                    projected.x - world.cameraScreenX();
                const std::int32_t y =
                    projected.y - world.cameraScreenY();
                if (x < 0 || x >= 640 || y < 0 || y >= 400) {
                    continue;
                }
                const osf::WorldPosition actual =
                    osf::calculateWorldPosition({
                        world.cameraScreenX() + x,
                        world.cameraScreenY() + y,
                    });
                if (!osf::positionIsWalkable(
                        world.ground(),
                        world.objectMap(),
                        actual,
                        world.companion().judgement())) {
                    continue;
                }
                target = &enemy;
                pointer_x = x;
                pointer_y = y;
                destination = actual;
                break;
            }
        }
        if (target) {
            break;
        }
    }
    if (!check(
            target && pointer_x >= 0 && pointer_y >= 0,
            "No shipped on-screen enemy could prepare Explosion's "
            "ground destination.")) {
        return false;
    }

    const std::int32_t target_character_number =
        target->characterNumber();
    const std::int32_t target_life_before =
        target->currentLife();
    const osf::PlayerSpellParameters parameters =
        osf::playerSpellParameters(
            world.playerMagic(),
            spell,
            world.playerEquipment(),
            world.itemDatabase(),
            world.parameterTables());
    const std::int32_t mana_before =
        world.playerData().currentMana();
    if (!check(
            mana_before >= parameters.mana_cost &&
                world.commandPlayerMagic(pointer_x, pointer_y) &&
                world.playerSpellActive() &&
                world.playerSpellTargetCharacterNumber() == -1 &&
                world.playerAnimationChart() == 11 &&
                world.playerData().currentMana() ==
                    mana_before - parameters.mana_cost &&
                world.runtimeEffectControllerCount() == 0,
            "Explosion did not enter player action 42, spend MP, and "
            "defer its companion command to the CAF marker.")) {
        return false;
    }

    bool saw_departure = false;
    bool saw_arrival = false;
    bool relocated = false;
    bool saw_first_visual = false;
    bool saw_second_visual = false;
    bool heard_first_sample = false;
    bool heard_second_sample = false;
    std::int32_t relocate_sample_count = 0;
    bool heard_impact_sample = false;
    bool applied_damage = false;
    for (std::int32_t update = 0; update < 160; ++update) {
        world.update();
        saw_departure =
            saw_departure ||
            (world.companion().explosionActive() &&
             world.companion().animationChart() == 6);
        saw_arrival =
            saw_arrival ||
            (world.companion().explosionActive() &&
             world.companion().animationChart() == 7);
        relocated = relocated ||
            (world.companion().position().x == destination.x &&
             world.companion().position().y == destination.y);
        for (const osf::RuntimeEffectActor& actor :
             world.runtimeEffects()) {
            if (actor.controllerEffectNumber() != 21031 ||
                actor.resourceId() != 10000000) {
                continue;
            }
            saw_first_visual =
                saw_first_visual ||
                (actor.animationChart() == 1 &&
                 actor.redStrength() == 500 &&
                 actor.greenStrength() == 500 &&
                 actor.blueStrength() == 1200);
            saw_second_visual =
                saw_second_visual ||
                (actor.animationChart() == 0 &&
                 actor.redStrength() == 500 &&
                 actor.greenStrength() == 500 &&
                 actor.blueStrength() == 1200);
        }
        const std::vector<std::int32_t> audio =
            world.takeAudioSamples();
        heard_first_sample =
            heard_first_sample ||
            std::find(audio.begin(), audio.end(), 29) != audio.end();
        heard_second_sample =
            heard_second_sample ||
            std::find(audio.begin(), audio.end(), 23) != audio.end();
        relocate_sample_count += static_cast<std::int32_t>(
            std::count(audio.begin(), audio.end(), 45));
        heard_impact_sample =
            heard_impact_sample ||
            std::find(audio.begin(), audio.end(), 46) != audio.end();
        const auto current = std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [target_character_number](const osf::EnemyActor& enemy) {
                return enemy.characterNumber() ==
                    target_character_number;
            });
        applied_damage = applied_damage ||
            current == world.enemies().end() ||
            current->currentLife() < target_life_before;
        if (!world.playerSpellActive() &&
            !world.companion().explosionActive() &&
            saw_first_visual && saw_second_visual) {
            break;
        }
    }

    const bool passed =
        saw_departure && saw_arrival && relocated &&
            saw_first_visual && saw_second_visual &&
            heard_first_sample && heard_second_sample &&
            relocate_sample_count == 2 &&
            heard_impact_sample &&
            applied_damage &&
            world.playerMagic().experience(spell) >= 1 &&
            !world.companion().explosionActive() &&
            world.companion().presentationAction() == 2;
    if (!passed) {
        std::cerr
            << "departure=" << saw_departure
            << " arrival=" << saw_arrival
            << " relocated=" << relocated
            << " visual=" << saw_first_visual
            << ',' << saw_second_visual
            << " audio=" << heard_first_sample
            << ',' << heard_second_sample
            << ',' << relocate_sample_count
            << ',' << heard_impact_sample
            << " damage=" << applied_damage
            << " practice="
            << world.playerMagic().experience(spell)
            << " active="
            << world.companion().explosionActive()
            << " presentation="
            << world.companion().presentationAction()
            << " destination=" << destination.x
            << ',' << destination.y
            << " companion="
            << world.companion().position().x
            << ',' << world.companion().position().y
            << '\n';
    }
    return check(
        passed,
        "The live Explosion did not relocate its companion, play both "
        "visual/audio layers, damage the area, train, and unlock.");
}

bool testGroundSpellInsufficientMana(
    const std::filesystem::path& game_root,
    std::int32_t spell) {
    osf::PlayerLoadRequest player;
    player.name = "GroundSpellMana";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root, player, &error),
            "Remote Town could not prepare the ground-spell MP check.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[spell] = 3;
    world.playerMagic().restore(magic_state);
    if (!world.playerMagic().selectSpell(spell)) {
        return false;
    }
    const osf::PlayerSpellParameters parameters =
        osf::playerSpellParameters(
            world.playerMagic(),
            spell,
            world.playerEquipment(),
            world.itemDatabase(),
            world.parameterTables());
    while (world.playerData().currentMana() >=
           parameters.mana_cost) {
        if (!check(
                world.commandPlayerMagic(400, 240) &&
                    world.playerSpellActive() &&
                    world.playerSpellTargetCharacterNumber() == -1,
                "A valid ground-spell MP-draining command was rejected.")) {
            return false;
        }
        for (std::int32_t update = 0;
             update < 100 && world.playerSpellActive();
             ++update) {
            world.update();
            world.takeAudioSamples();
        }
        if (!check(
                !world.playerSpellActive(),
                "The ground spell did not finish before the next MP check.")) {
            return false;
        }
        for (std::int32_t update = 0;
             update < 40 &&
             world.runtimeEffectControllerCount() != 0;
             ++update) {
            world.update();
            world.takeAudioSamples();
        }
    }

    const std::int32_t mana_before =
        world.playerData().currentMana();
    const std::size_t controllers_before =
        world.runtimeEffectControllerCount();
    return check(
        mana_before < parameters.mana_cost &&
            world.commandPlayerMagic(400, 240) &&
            !world.playerSpellActive() &&
            world.playerData().currentMana() ==
                mana_before &&
            world.runtimeEffectControllerCount() ==
                controllers_before,
        "An insufficient-MP ground command created an "
        "action, effect, or mana change.");
}

bool testTargetedSpellInsufficientMana(
    const std::filesystem::path& game_root,
    std::int32_t spell) {
    osf::PlayerLoadRequest player;
    player.name = "TargetSpellMana";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                game_root,
                player,
                {3000507, 3, 0},
                &error),
            "The targeted-spell MP fixture could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerMagicState magic_state;
    magic_state.availability.fill(0);
    magic_state.levels.fill(1);
    magic_state.experience.fill(0);
    magic_state.bar_slots.fill(-1);
    magic_state.availability[spell] = 3;
    world.playerMagic().restore(magic_state);
    if (!world.playerMagic().selectSpell(spell)) {
        return false;
    }

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
        for (std::int32_t y = std::max(0, anchor_y - 140);
             y < std::min(400, anchor_y + 30) && pointer_x < 0;
             ++y) {
            for (std::int32_t x = std::max(0, anchor_x - 80);
                 x < std::min(640, anchor_x + 81);
                 ++x) {
                world.updatePointerHover(x, y);
                if (world.hoveredEnemyId() == enemy.id()) {
                    pointer_x = x;
                    pointer_y = y;
                    break;
                }
            }
        }
        if (pointer_x >= 0) {
            break;
        }
    }
    if (!check(
            pointer_x >= 0 && pointer_y >= 0,
            "No shipped enemy could prepare the targeted-spell "
            "MP check.")) {
        return false;
    }

    const osf::PlayerSpellParameters parameters =
        osf::playerSpellParameters(
            world.playerMagic(),
            spell,
            world.playerEquipment(),
            world.itemDatabase(),
            world.parameterTables());
    // PlayerData is deliberately exposed read-only by WorldScene. The
    // underlying scene is mutable here; lower only this fixture's MP so the
    // command guard can be exercised without changing the runtime API.
    const_cast<osf::PlayerData&>(world.playerData()).setCurrentMana(
        std::max(0, parameters.mana_cost - 1));
    const std::int32_t mana_before =
        world.playerData().currentMana();
    const std::size_t controllers_before =
        world.runtimeEffectControllerCount();
    return check(
        mana_before < parameters.mana_cost &&
            world.commandPlayerMagic(pointer_x, pointer_y) &&
            !world.playerSpellActive() &&
            world.playerData().currentMana() == mana_before &&
            world.runtimeEffectControllerCount() == controllers_before,
        "An insufficient-MP targeted command created an "
        "action, effect, or mana change.");
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
            {
                10001, 0, 20000, 0x14,
                true, true, false, true, false, false,
            }) ||
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
            {
                10002, 1, 21013, 0x14,
                true, true, false, true, false, false,
            }) ||
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
            {
                10003, 0, 20005, 4,
                true, false, true, false, true, true,
            }) ||
        !testShippedWorldCast(
            game_root,
            3,
            11,
            10000030,
            21) ||
        !testRetailAction(
            animation,
            tables,
            4,
            osf::PlayerSpellAction::hell_fire,
            13,
            14) ||
        !testRetailPacket(
            tables,
            4,
            {
                10004, 0, 20001, 4,
                false, false, false, true, false, false,
            }) ||
        !testShippedHellFireCast(game_root) ||
        !testGroundSpellInsufficientMana(game_root, 4) ||
        !testRetailAction(
            animation,
            tables,
            5,
            osf::PlayerSpellAction::ice_blast,
            11,
            12) ||
        !testRetailPacket(
            tables,
            5,
            {
                10005, 1, 21013, 4,
                false, false, false, true, false, false,
            }) ||
        !testShippedIceBlastCast(game_root) ||
        !testGroundSpellInsufficientMana(game_root, 5) ||
        !testRetailAction(
            animation,
            tables,
            6,
            osf::PlayerSpellAction::heal,
            11,
            12,
            true) ||
        !testRetailHealResolution(tables) ||
        !testShippedHealCast(game_root, tables) ||
        !testGroundSpellInsufficientMana(game_root, 6) ||
        !testRetailAction(
            animation,
            tables,
            7,
            osf::PlayerSpellAction::moon,
            11,
            12,
            true) ||
        !testRetailMoonRules(tables) ||
        !testShippedMoonCast(game_root) ||
        !testRetailAction(
            animation,
            tables,
            8,
            osf::PlayerSpellAction::berserker,
            11,
            12,
            true) ||
        !testRetailBerserkerRules(game_root, tables) ||
        !testShippedBerserkerCast(game_root) ||
        !testGroundSpellInsufficientMana(game_root, 8) ||
        !testRetailAction(
            animation,
            tables,
            9,
            osf::PlayerSpellAction::energy_shield,
            11,
            12,
            true) ||
        !testRetailEnergyShieldRules(tables) ||
        !testShippedEnergyShieldCast(game_root) ||
        !testGroundSpellInsufficientMana(game_root, 9) ||
        !testRetailAction(
            animation,
            tables,
            10,
            osf::PlayerSpellAction::earth_spear,
            11,
            12) ||
        !testRetailPacket(
            tables,
            10,
            {
                10010, 0, -1, 4,
                true, false, true, false, true, true,
                true, 1,
            }) ||
        !testShippedWorldCast(
            game_root,
            10,
            11,
            10000060,
            22) ||
        !testTargetedSpellInsufficientMana(game_root, 10) ||
        !testRetailAction(
            animation,
            tables,
            11,
            osf::PlayerSpellAction::flame_strike,
            13,
            14) ||
        !testRetailPacket(
            tables,
            11,
            {
                10011, 0, 20000, 0x14,
                true, true, false, true, true, false,
                false, 0,
            }) ||
        !testShippedWorldCast(
            game_root,
            11,
            13,
            10000010,
            19) ||
        !testTargetedSpellInsufficientMana(game_root, 11) ||
        !testRetailAction(
            animation,
            tables,
            12,
            osf::PlayerSpellAction::dread_deathscythe,
            13,
            14) ||
        !testRetailPacket(
            tables,
            12,
            {
                10012, 1, 21013, 0x14,
                true, true, false, true, true, false,
                false, 0,
            }) ||
        !testShippedWorldCast(
            game_root,
            12,
            13,
            10000081,
            94) ||
        !testTargetedSpellInsufficientMana(game_root, 12) ||
        !testRetailAction(
            animation,
            tables,
            13,
            osf::PlayerSpellAction::lightning_storm,
            11,
            12) ||
        !testRetailPacket(
            tables,
            13,
            {
                10013, 0, 20005, 4,
                true, false, true, false, true, true,
                false, 0, false, false,
            }) ||
        !testShippedWorldCast(
            game_root,
            13,
            11,
            10000030,
            21) ||
        !testTargetedSpellInsufficientMana(game_root, 13) ||
        !testRetailAction(
            animation,
            tables,
            14,
            osf::PlayerSpellAction::medusa,
            13,
            14) ||
        !testRetailPacket(
            tables,
            14,
            {
                10014, 2, 21019, 0x14,
                true, true, false, true, false, false,
                false, 0,
            }) ||
        !testShippedWorldCast(
            game_root,
            14,
            13,
            10000070,
            22) ||
        !testTargetedSpellInsufficientMana(game_root, 14)) {
        return 1;
    }
    if (!testRetailSonicBladeAction(animation, tables) ||
        !testRetailPacket(
            tables,
            15,
            {
                10015, 0, 21024, 0x14,
                true, true, false, true, false, true,
                false, 1, true, true, 0, true, 1,
            }) ||
        !testRetailSonicBladeEffects(tables) ||
        !testShippedSonicBladeCast(game_root) ||
        !testTargetedSpellInsufficientMana(game_root, 15)) {
        return 1;
    }
    if (!testRetailAction(
            animation,
            tables,
            16,
            osf::PlayerSpellAction::mud_javelin,
            13,
            14) ||
        !testRetailPacket(
            tables,
            16,
            {
                10016, 3, -1, 0x14,
                true, true, false, true, false, false,
                true, 0,
            }) ||
        !testShippedWorldCast(
            game_root,
            16,
            13,
            10000110,
            19) ||
        !testTargetedSpellInsufficientMana(game_root, 16)) {
        return 1;
    }
    if (!testRetailAction(
            animation,
            tables,
            17,
            osf::PlayerSpellAction::identify,
            11,
            12,
            true,
            21028) ||
        !testShippedIdentifyCast(game_root)) {
        return 1;
    }
    if (!testRetailMagicShieldRules() ||
        !testRetailAction(
            animation,
            tables,
            18,
            osf::PlayerSpellAction::magic_shield,
            11,
            12,
            true) ||
        !testShippedMagicShieldCast(game_root) ||
        !testGroundSpellInsufficientMana(game_root, 18)) {
        return 1;
    }
    if (!testRetailCounterBurstRules() ||
        !testRetailAction(
            animation,
            tables,
            19,
            osf::PlayerSpellAction::counter_burst,
            11,
            12,
            true) ||
        !testShippedCounterBurstCast(game_root) ||
        !testGroundSpellInsufficientMana(game_root, 19)) {
        return 1;
    }
    if (!testRetailAction(
            animation,
            tables,
            20,
            osf::PlayerSpellAction::explosion,
            11,
            12,
            true) ||
        !testShippedExplosionCast(game_root) ||
        !testGroundSpellInsufficientMana(game_root, 20)) {
        return 1;
    }
#endif
    return 0;
}
