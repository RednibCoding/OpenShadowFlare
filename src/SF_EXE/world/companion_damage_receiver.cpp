#include "companion_damage_receiver.hpp"

#include "actor_direction.hpp"
#include "combat_effect_number.hpp"
#include "core/retail_integer.hpp"
#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kReactionAffinityTable = 24;
constexpr std::int32_t kReactionDamageTable = 25;
constexpr std::int32_t kHitPresentationAction = 5;
constexpr std::int32_t kDeathPresentationAction = 6;
constexpr std::int32_t kDefaultEvent = 4;
constexpr std::int32_t kReactionEffectBase = 21015;
constexpr std::int32_t kRandomHitEffectBase = 21011;
constexpr std::int32_t kReactionAudioSample = 119;
constexpr double kRetailPi = 3.141592;

bool readTableValue(
    const TableDatabase& tables,
    std::int32_t table_number,
    std::int32_t row,
    std::int32_t column,
    std::int32_t& value) {
    const TableData* table = tables.find(table_number);
    if (!table || !table->contains(row, column)) {
        return false;
    }
    value = table->value(row, column);
    return true;
}

std::int32_t pairedElementBase(
    std::int32_t element) {
    return (element / 2) * 2 -
           (element % 2);
}

bool validElement(std::int32_t element) {
    return element >= 0 && element < 8;
}

CombatEffectSpawnRequest baseEffect(
    const CompanionDamageReceiverState& state,
    std::int32_t effect_number,
    std::int32_t owner_kind) {
    CombatEffectSpawnRequest request;
    request.valid = true;
    request.effect_number = effect_number;
    request.owner_kind = owner_kind;
    request.source_character_number =
        state.character_number;
    request.target_kind = 0;
    request.target_identifier = 0;
    request.has_source_judgement = true;
    request.source_judgement = state.judgement;
    request.packet_kind = 8;
    request.instance_identifier = -1;
    request.constructor_value_21 = 200;
    request.constructor_value_22 = 0;
    return request;
}

bool resolveReaction(
    CompanionDamageReceiverState& state,
    const CombatPacket& packet,
    WorldPosition impact_origin,
    std::int32_t damage,
    const TableDatabase& tables,
    RetailRandom& random) {
    if (!validElement(state.native_element) ||
        state.maximum_life <= 0) {
        return false;
    }

    std::int32_t affinity_chance = 0;
    std::int32_t affinity_duration = 0;
    const std::int32_t pair_base =
        pairedElementBase(state.native_element);
    std::int32_t affinity_row = -1;
    if (packet[0] == 0) {
        const std::int32_t strength =
            packet[
                static_cast<std::size_t>(
                    pair_base + 7)];
        if (strength >= 1 && strength <= 10) {
            affinity_row = strength - 1;
        }
    } else if (packet[32] == pair_base + 1) {
        affinity_row = 5;
    }
    if (affinity_row >= 0 &&
        (!readTableValue(
             tables,
             kReactionAffinityTable,
             affinity_row,
             0,
             affinity_chance) ||
         !readTableValue(
             tables,
             kReactionAffinityTable,
             affinity_row,
             1,
             affinity_duration))) {
        return false;
    }

    std::int32_t damage_row =
        retailMultiply(damage, 50) /
        state.maximum_life;
    damage_row =
        std::min<std::int32_t>(damage_row, 49);
    if (damage_row < 0) {
        return false;
    }

    std::int32_t chance = packet[41];
    if (chance == -1 &&
        !readTableValue(
            tables,
            kReactionDamageTable,
            damage_row,
            0,
            chance)) {
        return false;
    }
    if (packet[1] == 3) {
        chance =
            packet[
                static_cast<std::size_t>(
                    state.native_element + 54)];
    }
    chance = retailAdd(
        chance,
        retailAdd(packet[42], affinity_chance));
    chance = std::max<std::int32_t>(chance, 0);
    if (chance <= random.next() % 100) {
        return true;
    }

    std::int32_t duration = packet[43];
    if (duration == -1 &&
        !readTableValue(
            tables,
            kReactionDamageTable,
            damage_row,
            1,
            duration)) {
        return false;
    }
    if (packet[1] == 3) {
        duration =
            packet[
                static_cast<std::size_t>(
                    state.native_element + 63)];
    }
    duration = retailAdd(
        duration,
        retailAdd(packet[44], affinity_duration));
    duration = std::max<std::int32_t>(duration, 1);

    bool motion = packet[40] != 0;
    if (packet[1] == 3 &&
        packet[
            static_cast<std::size_t>(
                state.native_element + 45)] == 0) {
        motion = false;
    }
    if (!motion) {
        duration =
            std::min<std::int32_t>(duration, 15);
    }

    if (state.presentation_action !=
            kHitPresentationAction ||
        state.reaction_stage != 2) {
        state.presentation_action =
            kHitPresentationAction;
        state.reaction_stage = 0;
        state.presentation_counter = 0;
        state.action_lock = 1;
        state.reaction_duration =
            std::max<std::int32_t>(
                retailAdd(duration, packet[76]), 1);
        state.reaction_additive = packet[76];
        state.reaction_motion = motion;
        state.reaction_angle =
            retailAngleForVector(
                state.position.x - impact_origin.x,
                state.position.y - impact_origin.y);
        if (!motion) {
            state.direction =
                retailDirectionForAngle(
                    state.reaction_angle -
                    kRetailPi);
        }
    }
    return true;
}

void addPacketEffects(
    CompanionDamageReceiverResult& result,
    const CombatPacket& packet,
    RetailRandom& random) {
    CompanionDamageReceiverState& state = result.state;
    if (packet[3] == 1 &&
        state.action_lock == 1 &&
        state.reaction_duration != 0) {
        state.reaction_stage = 1;
        CombatEffectSpawnRequest effect =
            baseEffect(
                state,
                random.next() % 4 +
                    kReactionEffectBase,
                4);
        effect.constructor_value_12 =
            state.reaction_duration;
        result.effects.push_back(effect);
        result.audio_samples.push_back(
            kReactionAudioSample);
    }
    if (packet[3] == 2 &&
        state.action_lock == 1 &&
        state.reaction_duration != 0) {
        state.reaction_stage = 2;
    }
    if (packet[34] != -1 &&
        (!isDeathSplatterEffect(packet[34]) ||
         state.current_life <= 0)) {
        CombatEffectSpawnRequest effect =
            baseEffect(state, packet[34], 2);
        effect.packet_kind = packet[35];
        result.effects.push_back(effect);
    }
    if (packet[74] != -1) {
        CombatEffectSpawnRequest effect =
            baseEffect(state, packet[74], 2);
        effect.packet_kind = packet[75];
        result.effects.push_back(effect);
    }
    if (packet[72] != 0 &&
        random.next() % 100 < 20) {
        const std::int32_t packet_kind =
            random.next() % 8;
        const std::int32_t effect_number =
            random.next() % 2 +
            kRandomHitEffectBase;
        CombatEffectSpawnRequest effect =
            baseEffect(
                state, effect_number, 2);
        effect.packet_kind = packet_kind;
        result.effects.push_back(effect);
    }
}

bool ignoredAction(std::int32_t action) {
    return action == 7 ||
           action == 8 ||
           action == 10;
}

}  // namespace

CompanionDamageReceiverResult resolveCompanionDamage(
    const CompanionDamageReceiverState& state,
    const CombatPacket& packet,
    WorldPosition impact_origin,
    const CompanionDamageReceiverContext& context,
    const TableDatabase& tables,
    RetailRandom& random) {
    CompanionDamageReceiverResult result;
    result.state = state;
    if (state.current_life < 1 ||
        ignoredAction(state.presentation_action)) {
        return result;
    }
    result.accepted = true;

    CombatDefense defense;
    defense[0] = 1;
    defense[1] = state.character_number;
    defense[3] = state.physical_defense;
    defense[4] = state.magical_defense;
    defense[13] = state.native_element;
    if (packet[4] < 1) {
        result.damage.valid = true;
        result.damage.damage = 0;
    } else {
        result.damage =
            resolveCombatDamage(
                packet, defense, tables, random);
        if (!result.damage.valid) {
            result.valid = false;
            return result;
        }
    }

    if (context.local_player_slot ==
        state.character_number % 10) {
        result.state.current_life =
            retailSubtract(
                result.state.current_life,
                result.damage.damage);
        if (result.state.current_life <= 0) {
            if (result.state.presentation_action !=
                    kHitPresentationAction ||
                result.state.reaction_stage != 2) {
                result.state.reaction_stage = 0;
            }
            result.state.presentation_action =
                kDeathPresentationAction;
            result.state.presentation_counter = 0;
            result.state.action_lock = 1;
        }
    }

    if (result.state.presentation_action !=
            kHitPresentationAction ||
        result.state.reaction_stage != 2) {
        result.state.reaction_duration = 0;
    }
    if (result.state.current_life > 0 &&
        !resolveReaction(
            result.state,
            packet,
            impact_origin,
            result.damage.damage,
            tables,
            random)) {
        CompanionDamageReceiverResult invalid;
        invalid.valid = false;
        invalid.accepted = true;
        invalid.state = state;
        return invalid;
    }

    addPacketEffects(result, packet, random);
    if (result.state.current_life <= 0) {
        result.state.presentation_action =
            kDeathPresentationAction;
        result.state.presentation_counter = 0;
        result.state.action_lock = 1;
    }
    if (result.state.event_number == 0) {
        result.state.event_number = kDefaultEvent;
    }
    return result;
}

}  // namespace osf
