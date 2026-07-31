#include "enemy_damage_receiver.hpp"

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
constexpr std::int32_t kHitPresentationAction = 10;
constexpr std::int32_t kDeathPresentationAction = 11;
constexpr std::int32_t kClientNetworkMode = 2;
constexpr std::int32_t kDefaultEvent = 4;
constexpr std::int32_t kReflectionEffect = 20013;
constexpr std::int32_t kReactionEffectBase = 21015;
constexpr std::int32_t kRandomHitEffectBase = 21011;
constexpr std::int32_t kReflectionAudioSample = 61;
constexpr std::int32_t kReactionAudioSample = 119;
constexpr double kRetailPi = 3.141592;
constexpr double kReflectionSpreadScale = 0.001;

const TableData* table(
    const TableDatabase& tables,
    std::int32_t table_number) {
    return tables.find(table_number);
}

bool readTableValue(
    const TableData* values,
    std::int32_t row,
    std::int32_t column,
    std::int32_t& value) {
    if (!values || !values->contains(row, column)) {
        return false;
    }
    value = values->value(row, column);
    return true;
}

CombatEffectSpawnRequest baseEffect(
    const EnemyDamageReceiverState& state,
    std::int32_t effect_number) {
    CombatEffectSpawnRequest request;
    request.valid = true;
    request.effect_number = effect_number;
    request.owner_kind = 4;
    request.source_character_number =
        state.character_number;
    request.target_kind = 0;
    request.target_identifier = 0;
    request.constructor_value_6 = 0;
    request.constructor_value_7 = 0;
    request.direction_radians = 0.0;
    request.has_explicit_origin = false;
    request.has_source_judgement = true;
    request.source_judgement = state.judgement;
    request.constructor_value_12 = 0;
    request.packet_kind = 8;
    request.instance_identifier = -1;
    request.constructor_value_16 = 0;
    request.constructor_value_17 = 0;
    request.constructor_value_18 = 0;
    request.constructor_value_19 = 0;
    request.constructor_value_20 = 0;
    request.constructor_value_21 = 200;
    request.constructor_value_22 = 0;
    return request;
}

CombatEffectSpawnRequest reflectionEffect(
    const EnemyDamageReceiverState& state,
    std::int32_t source_slot,
    double direction) {
    CombatEffectSpawnRequest request =
        baseEffect(state, kReflectionEffect);
    request.target_kind = 1;
    request.target_identifier = source_slot;
    request.constructor_value_6 = 100;
    request.constructor_value_7 = 250;
    request.direction_radians = direction;
    request.has_source_judgement = false;
    return request;
}

CombatEffectSpawnRequest reactionEffect(
    const EnemyDamageReceiverState& state,
    std::int32_t effect_number) {
    CombatEffectSpawnRequest request =
        baseEffect(state, effect_number);
    request.constructor_value_12 =
        state.reaction_duration;
    return request;
}

CombatEffectSpawnRequest configuredEffect(
    const EnemyDamageReceiverState& state,
    std::int32_t effect_number,
    std::int32_t packet_kind) {
    CombatEffectSpawnRequest request =
        baseEffect(state, effect_number);
    request.packet_kind = packet_kind;
    return request;
}

bool validElement(std::int32_t element) {
    return element >= 0 && element < 8;
}

std::int32_t elementPairBase(
    std::int32_t element) {
    return (element / 2) * 2 -
           (element % 2);
}

struct ReactionAffinity {
    bool valid = true;
    std::int32_t chance = 0;
    std::int32_t duration = 0;
};

ReactionAffinity reactionAffinity(
    const EnemyDamageReceiverState& state,
    const CombatPacket& packet,
    const TableDatabase& tables) {
    ReactionAffinity result;
    const std::int32_t pair_base =
        elementPairBase(state.native_element);
    std::int32_t row = -1;
    if (packet[0] == 0) {
        const std::int32_t strength =
            packet[
                static_cast<std::size_t>(
                    pair_base + 7)];
        if (strength >= 1 && strength <= 10) {
            row = strength - 1;
        }
    } else if (packet[32] == pair_base + 1) {
        row = 5;
    }
    if (row < 0) {
        return result;
    }

    const TableData* affinity_table =
        table(tables, kReactionAffinityTable);
    result.valid =
        readTableValue(
            affinity_table, row, 0, result.chance) &&
        readTableValue(
            affinity_table, row, 1, result.duration);
    return result;
}

struct ReactionResult {
    bool valid = true;
};

ReactionResult resolveReaction(
    EnemyDamageReceiverState& state,
    const CombatPacket& packet,
    WorldPosition impact_origin,
    std::int32_t damage,
    const TableDatabase& tables,
    RetailRandom& random) {
    ReactionResult result;
    if (!state.has_visual) {
        return result;
    }
    if (!validElement(state.native_element) ||
        state.maximum_life <= 0) {
        result.valid = false;
        return result;
    }

    const ReactionAffinity affinity =
        reactionAffinity(state, packet, tables);
    if (!affinity.valid) {
        result.valid = false;
        return result;
    }

    std::int32_t damage_row =
        retailMultiply(damage, 50) /
        state.maximum_life;
    damage_row = std::min<std::int32_t>(
        damage_row, 49);
    if (damage_row < 0) {
        result.valid = false;
        return result;
    }

    const TableData* damage_table =
        table(tables, kReactionDamageTable);
    std::int32_t chance = packet[41];
    if (chance == -1 &&
        !readTableValue(
            damage_table, damage_row, 0, chance)) {
        result.valid = false;
        return result;
    }
    if (packet[1] == 3) {
        chance =
            packet[
                static_cast<std::size_t>(
                    state.native_element + 54)];
    }
    chance = retailAdd(
        chance,
        retailSubtract(
            packet[42],
            state.reaction_chance_defense));
    chance = retailAdd(chance, affinity.chance);
    chance = std::max<std::int32_t>(chance, 0);
    if (chance <= random.next() % 100) {
        return result;
    }

    std::int32_t duration = packet[43];
    if (duration == -1 &&
        !readTableValue(
            damage_table, damage_row, 1, duration)) {
        result.valid = false;
        return result;
    }
    if (packet[1] == 3) {
        duration =
            packet[
                static_cast<std::size_t>(
                    state.native_element + 63)];
    }
    duration = retailAdd(
        duration,
        retailSubtract(
            packet[44],
            state.reaction_duration_defense));
    duration = retailAdd(duration, affinity.duration);
    duration = std::max<std::int32_t>(duration, 1);

    bool displacement_suppressed = packet[40] != 0;
    if (packet[1] == 3 &&
        packet[
            static_cast<std::size_t>(
                state.native_element + 45)] == 0) {
        displacement_suppressed = false;
    }
    if (!displacement_suppressed) {
        duration = std::min<std::int32_t>(
            duration, 15);
    }

    if (state.presentation_action !=
            kHitPresentationAction ||
        state.reaction_stage != 2) {
        state.reaction_duration = retailAdd(
            duration, packet[76]);
        if (state.reaction_duration < 1) {
            state.reaction_duration = 1;
        }
        state.reaction_stage = 0;
        state.presentation_action =
            kHitPresentationAction;
        state.presentation_counter = 0;
        state.action_lock = 1;
        state.reaction_displacement_suppressed =
            displacement_suppressed;
        state.reaction_additive = packet[76];
        if (state.always_suppress_reaction_displacement) {
            state.reaction_displacement_suppressed = true;
        }
        state.reaction_angle =
            retailAngleForVector(
                state.position.x - impact_origin.x,
                state.position.y - impact_origin.y);
        if (!state.reaction_displacement_suppressed) {
            state.direction =
                retailDirectionForAngle(
                    state.reaction_angle -
                    kRetailPi);
        }
    }
    return result;
}

EnemyDamageNetworkRequest networkRequest(
    const EnemyDamageReceiverState& state,
    const CombatPacket& packet,
    std::int32_t source_slot,
    std::int32_t damage,
    EnemyDamageNetworkAction action) {
    EnemyDamageNetworkRequest request;
    request.action = action;
    request.scenario_number = state.scenario_number;
    request.source_slot = source_slot;
    request.target_character_number =
        state.character_number;
    request.effect_family = packet[1] == 3;
    request.damage = damage;
    request.source_is_character_number =
        packet[2] > 9;
    request.target_position = state.position;
    return request;
}

void addKillStatuses(
    const EnemyDamageReceiverContext& context,
    EnemyDamageReceiverResult& result) {
    if (!context.local_player_available) {
        return;
    }
    if (context.apply_status_7_on_kill) {
        result.local_player_statuses.push_back(
            {7, 1});
    }
    if (context.apply_status_8_on_kill) {
        result.local_player_statuses.push_back(
            {8, 1});
    }
    if (context.apply_status_9_on_kill) {
        result.local_player_statuses.push_back(
            {9, 1});
    }
}

void applyLocalDamage(
    EnemyDamageReceiverResult& result,
    const CombatPacket& packet,
    std::int32_t source_slot,
    const EnemyDamageReceiverContext& context) {
    const std::int32_t damage = result.damage.damage;
    if (damage == 0) {
        return;
    }

    EnemyDamageReceiverState& state = result.state;
    if (context.network_mode == kClientNetworkMode) {
        result.network = networkRequest(
            state,
            packet,
            source_slot,
            damage,
            EnemyDamageNetworkAction::send_to_server);
        state.current_life =
            retailSubtract(state.current_life, damage);
        if (state.current_life < 1) {
            state.current_life = 1;
        }
        return;
    }

    const std::int32_t attributed =
        std::min<std::int32_t>(
            state.current_life, damage);
    state.attributed_damage[
        static_cast<std::size_t>(source_slot)] =
        retailAdd(
            state.attributed_damage[
                static_cast<std::size_t>(
                    source_slot)],
            attributed);
    state.current_life =
        retailSubtract(state.current_life, damage);
    if (state.current_life < 0) {
        state.current_life = 0;
    }

    if (state.current_life < 1) {
        if (state.presentation_action !=
                kHitPresentationAction ||
            state.reaction_stage != 2) {
            state.reaction_stage = 0;
        }
        state.death_counter = 0;
        state.defeated_by_effect =
            packet[1] == 3;
        state.defeat_source_character_number =
            packet[2];
        result.kill_requested = true;
        addKillStatuses(context, result);
        return;
    }

    result.network = networkRequest(
        state,
        packet,
        source_slot,
        damage,
        EnemyDamageNetworkAction::
            broadcast_to_clients);
}

void addReflectionEffects(
    EnemyDamageReceiverResult& result,
    const EnemyDamageReceiverContext& context,
    std::int32_t source_slot,
    RetailRandom& random) {
    const double source_angle =
        retailAngleForVector(
            context.source_player_position.x -
                result.state.position.x,
            context.source_player_position.y -
                result.state.position.y);
    for (std::int32_t effect = 0;
         effect < 5;
         ++effect) {
        const double direction =
            static_cast<double>(
                random.next() % 3000 - 1500) *
                kReflectionSpreadScale +
            source_angle +
            kRetailPi;
        result.effects.push_back(
            reflectionEffect(
                result.state,
                source_slot,
                direction));
    }
    result.audio_samples.push_back(
        kReflectionAudioSample);
}

void addPacketEffects(
    EnemyDamageReceiverResult& result,
    const CombatPacket& packet,
    RetailRandom& random) {
    EnemyDamageReceiverState& state = result.state;
    if (packet[3] == 1 &&
        state.action_lock == 1 &&
        state.reaction_duration != 0) {
        state.reaction_stage = 1;
        result.effects.push_back(
            reactionEffect(
                state,
                random.next() % 4 +
                    kReactionEffectBase));
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
         state.current_life < 1)) {
        result.effects.push_back(
            configuredEffect(
                state, packet[34], packet[35]));
    }
    if (packet[74] != -1) {
        result.effects.push_back(
            configuredEffect(
                state, packet[74], packet[75]));
    }
    if (state.has_visual &&
        packet[72] != 0 &&
        random.next() % 100 < 20) {
        const std::int32_t packet_kind =
            random.next() % 8;
        const std::int32_t effect_number =
            random.next() % 2 +
            kRandomHitEffectBase;
        result.effects.push_back(
            configuredEffect(
                state,
                effect_number,
                packet_kind));
    }
}

}  // namespace

EnemyDamageReceiverResult resolveEnemyDamage(
    const EnemyDamageReceiverState& state,
    const CombatPacket& packet,
    WorldPosition impact_origin,
    const EnemyDamageReceiverContext& context,
    const TableDatabase& tables,
    RetailRandom& random) {
    EnemyDamageReceiverResult result;
    result.state = state;
    if (state.current_life < 1 || packet[2] < 0) {
        return result;
    }
    result.accepted = true;

    if (packet[0] == 0) {
        ++result.source_player_lookup_count;
    } else {
        result.source_scenario_actor_lookup_requested = true;
        result.source_owner_lookup_requested = true;
    }
    const std::int32_t source_slot =
        packet[2] % 10;

    CombatDefense defense;
    defense[0] = 2;
    defense[1] = state.character_number;
    defense[3] = state.physical_defense;
    defense[4] = state.magical_defense;
    defense[13] = state.native_element;
    if (packet[4] < 1) {
        result.damage.valid = true;
        result.damage.damage = 0;
    } else {
        result.damage = resolveCombatDamage(
            packet, defense, tables, random);
        if (result.damage.requests_source_lookup) {
            ++result.source_player_lookup_count;
        }
        if (!result.damage.valid) {
            result.valid = false;
            return result;
        }
    }

    if (result.state.presentation_action !=
            kHitPresentationAction ||
        result.state.reaction_stage != 2) {
        result.state.reaction_duration = 0;
    }

    if ((packet[0] == 0 || packet[0] == 1) &&
        packet[73] != -1 &&
        context.local_player_slot == source_slot &&
        context.local_player_available) {
        result.local_player_statuses.push_back(
            {packet[73], 0});
    }

    const ReactionResult reaction =
        resolveReaction(
            result.state,
            packet,
            impact_origin,
            result.damage.damage,
            tables,
            random);
    if (!reaction.valid) {
        result.valid = false;
        result.state = state;
        result.local_player_statuses.clear();
        result.network = {};
        result.kill_requested = false;
        result.effects.clear();
        result.audio_samples.clear();
        return result;
    }

    if (context.local_player_slot == source_slot) {
        applyLocalDamage(
            result,
            packet,
            source_slot,
            context);
    }

    if (packet[39] != 0 &&
        packet[0] == 0 &&
        context.source_player_available) {
        addReflectionEffects(
            result, context, source_slot, random);
    }

    addPacketEffects(result, packet, random);

    if (result.state.current_life < 1) {
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
