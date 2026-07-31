#include "player_damage_receiver.hpp"

#include "actor_direction.hpp"
#include "core/retail_integer.hpp"
#include "core/retail_random.hpp"
#include "enemy_effect_impact.hpp"
#include "items/item_database.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kEnergyShieldSpell = 9;
constexpr std::int32_t kMagicShieldSpell = 18;
constexpr std::int32_t kCounterBurstSpell = 19;
constexpr std::int32_t kRevivalItemCategory = 4;
constexpr std::int32_t kRevivalItemNumber = 98000000;
constexpr std::int32_t kReactionAffinityTable = 26;
constexpr std::int32_t kReactionDamageTable = 25;
constexpr std::int32_t kHitPresentationAction = 4;
constexpr std::int32_t kDeathPresentationAction = 5;
constexpr std::int32_t kDefaultEvent = 4;
constexpr std::int32_t kRevivalEffect = 21020;
constexpr std::int32_t kMagicShieldEffect = 21029;
constexpr std::int32_t kCounterBurstEffect = 21030;
constexpr std::int32_t kEquipmentReflectionEffect = 20014;
constexpr std::int32_t kReactionEffectBase = 21015;
constexpr std::int32_t kRandomHitEffectBase = 21011;
constexpr std::int32_t kShieldAudioSample = 60;
constexpr std::int32_t kRevivalAudioSample = 17;
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

std::int32_t effectiveSpellLevel(
    const PlayerDamageReceiverState& state,
    std::int32_t spell_number) {
    if (spell_number < 0 ||
        spell_number >=
            static_cast<std::int32_t>(
                state.spell_levels.size())) {
        return 1;
    }
    std::int32_t level =
        state.spell_levels[
            static_cast<std::size_t>(spell_number)];
    if (state.increased_power_updates != 0) {
        level = retailAdd(level, 2);
    }
    level = std::clamp<std::int32_t>(level, 1, 20);
    return std::clamp<std::int32_t>(
        retailAdd(level, state.magic_level_modifier),
        1,
        30);
}

bool suppressesOffHand(
    const PlayerEquipment& equipment,
    const ItemDatabase& items) {
    const InventoryItem* main =
        equipment.item(EquipmentSlot::main_hand);
    if (!main) {
        return false;
    }
    const ItemDefinition* definition =
        items.find(main->category, main->definition_id);
    return definition && definition->suppresses_off_hand;
}

std::int32_t manaCost(
    const PlayerDamageReceiverState& state,
    std::int32_t shield_spell,
    const ItemDatabase& items,
    const TableDatabase& tables,
    bool& valid) {
    const std::int32_t cost =
        retailEffectParameter(
            tables,
            state.selected_magic,
            effectiveSpellLevel(state, shield_spell),
            2);
    if (cost < 0) {
        valid = false;
        return 0;
    }
    return std::max<std::int32_t>(
        retailSubtract(
            cost,
            state.equipment.instanceParameterBonus(
                19, items)),
        1);
}

CombatEffectSpawnRequest baseEffect(
    const PlayerDamageReceiverState& state,
    std::int32_t effect_number) {
    CombatEffectSpawnRequest request;
    request.valid = true;
    request.effect_number = effect_number;
    request.owner_kind = 1;
    request.source_character_number =
        state.defense.character_number;
    request.target_kind = 0;
    request.target_identifier = 0;
    request.has_source_judgement = true;
    request.source_judgement = state.judgement;
    request.packet_kind = 8;
    request.instance_identifier = -1;
    request.constructor_value_21 = 200;
    return request;
}

CombatEffectSpawnRequest configuredEffect(
    const PlayerDamageReceiverState& state,
    std::int32_t effect_number,
    std::int32_t packet_kind) {
    CombatEffectSpawnRequest request =
        baseEffect(state, effect_number);
    request.packet_kind = packet_kind;
    return request;
}

void applyDurability(
    PlayerDamageReceiverResult& result,
    const ItemDatabase& items,
    RetailRandom& random) {
    PlayerEquipment& equipment = result.state.equipment;
    bool broken = false;
    const auto try_slot =
        [&](EquipmentSlot slot,
            std::int32_t chance,
            bool enabled) {
            if (!equipment.item(slot)) {
                return;
            }
            if (random.next() % 100 < chance &&
                enabled &&
                !equipment.decreaseDurability(slot, 1)) {
                broken = true;
            }
        };
    try_slot(EquipmentSlot::helmet, 20, true);
    try_slot(EquipmentSlot::body, 30, true);
    // Retail consumes this draw before checking a two-handed main weapon.
    try_slot(
        EquipmentSlot::off_hand,
        30,
        !suppressesOffHand(equipment, items));
    try_slot(EquipmentSlot::boots, 20, true);
    if (broken) {
        result.equipment_sync_requested = true;
        result.derived_values_refresh_requested = true;
    }
}

bool consumeRevivalItem(
    PlayerSpecialItems& special_items) {
    const auto& entries = special_items.items();
    for (std::size_t index = 0;
         index < entries.size();
         ++index) {
        if (entries[index].category ==
                kRevivalItemCategory &&
            entries[index].definition_id ==
                kRevivalItemNumber) {
            special_items.take(index);
            return true;
        }
    }
    return false;
}

std::int32_t pairedElement(
    std::int32_t element) {
    return (element / 2) * 2 -
               (element % 2) +
           1;
}

bool validElement(std::int32_t element) {
    return element >= 0 && element < 8;
}

bool resolveReaction(
    PlayerDamageReceiverResult& result,
    const CombatPacket& packet,
    WorldPosition impact_origin,
    const ItemDatabase& items,
    const TableDatabase& tables,
    RetailRandom& random) {
    PlayerDamageReceiverState& state = result.state;
    if (!validElement(packet[32]) ||
        state.maximum_life <= 0) {
        return false;
    }
    const CombatDefense defense =
        buildPlayerCombatDefense(
            state.defense,
            state.equipment,
            state.inventory,
            items);
    const std::int32_t affinity =
        defense[
            static_cast<std::size_t>(
                packet[32] + 5)];
    std::int32_t affinity_chance = 0;
    std::int32_t affinity_duration = 0;
    if (affinity < 0) {
        if (affinity < -10 ||
            !readTableValue(
                tables,
                kReactionAffinityTable,
                affinity + 10,
                0,
                affinity_chance) ||
            !readTableValue(
                tables,
                kReactionAffinityTable,
                affinity + 10,
                1,
                affinity_duration)) {
            return false;
        }
    }

    std::int32_t damage_row =
        retailMultiply(result.damage.damage, 50) /
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
        if (affinity < 0) {
            chance =
                packet[
                    static_cast<std::size_t>(
                        54 + pairedElement(packet[32]))];
        } else if (affinity == 0) {
            chance = packet[62];
        } else {
            chance =
                packet[
                    static_cast<std::size_t>(
                        54 + packet[32])];
        }
    }
    chance = retailAdd(
        retailAdd(
            chance,
            retailAdd(packet[42], affinity_chance)),
        -state.equipment.instanceParameterBonus(
            14, items));
    chance = std::max<std::int32_t>(chance, 0);
    if (state.presentation_action >= 22 &&
        state.presentation_action <= 36) {
        chance = 9999;
    }
    if (packet[1] != 3 &&
        state.presentation_action >= 11 &&
        state.presentation_action <= 14) {
        chance = 0;
    }
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
        if (affinity < 0) {
            duration =
                packet[
                    static_cast<std::size_t>(
                        63 + pairedElement(packet[32]))];
        } else if (affinity == 0) {
            duration = packet[71];
        } else {
            duration =
                packet[
                    static_cast<std::size_t>(
                        63 + packet[32])];
        }
    }
    duration = retailAdd(
        retailAdd(
            duration,
            retailAdd(packet[44], affinity_duration)),
        -state.equipment.instanceParameterBonus(
            15, items));
    duration = std::max<std::int32_t>(duration, 1);

    bool motion = packet[40] != 0;
    if (packet[1] == 3) {
        std::int32_t motion_word = 53;
        if (affinity < 0) {
            motion_word =
                45 + pairedElement(packet[32]);
        } else if (affinity > 0) {
            motion_word = 45 + packet[32];
        }
        motion =
            packet[
                static_cast<std::size_t>(
                    motion_word)] != 0;
    }
    if (!motion) {
        duration =
            std::min<std::int32_t>(duration, 15);
    }
    if (state.equipment.instanceParameterBonus(
            16, items) != 0) {
        motion = true;
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
    PlayerDamageReceiverResult& result,
    const CombatPacket& packet,
    RetailRandom& random) {
    PlayerDamageReceiverState& state = result.state;
    if (packet[3] == 1 &&
        state.action_lock == 1 &&
        state.reaction_duration != 0) {
        state.reaction_stage = 1;
        CombatEffectSpawnRequest effect =
            configuredEffect(
                state,
                random.next() % 4 +
                    kReactionEffectBase,
                8);
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
    if (packet[34] != -1) {
        result.effects.push_back(
            configuredEffect(
                state, packet[34], packet[35]));
    }
    if (packet[74] != -1) {
        result.effects.push_back(
            configuredEffect(
                state, packet[74], packet[75]));
    }
    if (packet[72] != 0 &&
        random.next() % 100 < 20) {
        const std::int32_t packet_kind =
            random.next() % 8;
        const std::int32_t effect_number =
            random.next() % 2 +
            kRandomHitEffectBase;
        CombatEffectSpawnRequest effect =
            configuredEffect(
                state,
                effect_number,
                packet_kind);
        effect.source_character_number =
            state.effect_owner_identifier;
        result.effects.push_back(effect);
    }
}

bool applyShield(
    PlayerDamageReceiverResult& result,
    const CombatPacket& packet,
    const ItemDatabase& items,
    const TableDatabase& tables) {
    PlayerDamageReceiverState& state = result.state;
    if (!state.magic_shield_active ||
        packet[1] != 3) {
        return true;
    }
    const std::int32_t reduction =
        retailEffectParameter(
            tables,
            kMagicShieldSpell,
            effectiveSpellLevel(
                state, kMagicShieldSpell),
            0);
    if (reduction < 0) {
        return false;
    }
    result.damage.damage =
        retailMultiply(
            100 - reduction,
            result.damage.damage) /
        100;
    if (result.damage.damage == 0) {
        result.damage.damage = 1;
    } else if (result.damage.damage >= 20) {
        result.spell_training.push_back(
            {kMagicShieldSpell, 0});
    }
    result.effects.push_back(
        baseEffect(state, kMagicShieldEffect));
    result.audio_samples.push_back(
        kShieldAudioSample);
    bool valid = true;
    const std::int32_t cost =
        manaCost(
            state,
            kMagicShieldSpell,
            items,
            tables,
            valid);
    if (!valid) {
        return false;
    }
    state.current_mana =
        retailSubtract(state.current_mana, cost);
    if (state.current_mana < 1) {
        state.current_mana = 0;
        state.magic_shield_active = false;
    }
    return true;
}

void tryReflection(
    PlayerDamageReceiverResult& result,
    const CombatPacket& incoming,
    const PlayerDamageReceiverContext& context,
    const ItemDatabase& items,
    const TableDatabase& tables,
    RetailRandom& random) {
    PlayerDamageReceiverState& state = result.state;
    if (state.current_life < 1 ||
        incoming[38] != 1) {
        return;
    }
    const std::array<std::int32_t, 2> equipment =
        state.equipment.conditionalInstanceParameterBonus(
            20, 21, items);
    std::int32_t reflected_percent =
        random.next() % 100 < equipment[0]
            ? equipment[1]
            : 0;
    if (state.counter_burst_active) {
        const std::int32_t spell_percent =
            retailEffectParameter(
                tables,
                kCounterBurstSpell,
                effectiveSpellLevel(
                    state, kCounterBurstSpell),
                0);
        if (spell_percent < 0) {
            result.valid = false;
            return;
        }
        reflected_percent =
            retailAdd(
                reflected_percent, spell_percent);
    }
    const PlayerReflectionTarget& target =
        context.reflection_target;
    if ((reflected_percent == 0 &&
         !state.counter_burst_active) ||
        incoming[0] != 2 ||
        !target.found ||
        target.actor_kind != 2 ||
        target.active_value <= 0) {
        return;
    }

    PlayerReflectedDamageRequest& reflection =
        result.reflection;
    reflection.valid = true;
    reflection.target_character_number =
        target.character_number;
    reflection.impact_origin = state.position;
    reflection.packet.write(0, 0);
    reflection.packet.write(1, 0);
    reflection.packet.write(
        2, state.defense.character_number);
    std::int32_t reflected_damage =
        retailMultiply(
            result.damage.damage,
            reflected_percent) /
        100;
    if (target.damage_scale_value == 100) {
        reflected_damage /= 2;
    }
    reflection.packet.write(
        4,
        std::max<std::int32_t>(
            reflected_damage, 1));
    reflection.packet.write(
        5, state.defense.physical_defense);
    reflection.packet.write(31, state.level);
    reflection.packet.write(
        34, random.next() % 3 + 20015);
    reflection.packet.write(37, 1);
    reflection.packet.write(72, 1);
    reflection.packet.write(74, -1);
    result.audio_samples.push_back(
        kShieldAudioSample);

    if (!state.counter_burst_active) {
        CombatEffectSpawnRequest effect =
            baseEffect(
                state,
                kEquipmentReflectionEffect);
        effect.direction_radians =
            retailAngleForVector(
                target.position.x - state.position.x,
                target.position.y - state.position.y);
        result.effects.push_back(effect);
        return;
    }

    if (result.damage.damage >= 20) {
        result.spell_training.push_back(
            {kCounterBurstSpell, 0});
    }
    result.effects.push_back(
        baseEffect(state, kCounterBurstEffect));
    bool valid = true;
    const std::int32_t cost =
        manaCost(
            state,
            kCounterBurstSpell,
            items,
            tables,
            valid);
    if (!valid) {
        result.valid = false;
        return;
    }
    state.current_mana =
        retailSubtract(state.current_mana, cost);
    if (state.current_mana < 1) {
        state.current_mana = 0;
        state.counter_burst_active = false;
    }
}

}  // namespace

PlayerDamageReceiverResult resolvePlayerDamage(
    const PlayerDamageReceiverState& state,
    const CombatPacket& packet,
    WorldPosition impact_origin,
    const PlayerDamageReceiverContext& context,
    const ItemDatabase& items,
    const TableDatabase& tables,
    RetailRandom& random) {
    PlayerDamageReceiverResult result;
    result.state = state;
    const auto invalid =
        [&]() {
            PlayerDamageReceiverResult failed;
            failed.valid = false;
            failed.accepted = true;
            failed.state = state;
            return failed;
        };
    const bool local_player =
        context.local_player_character_number ==
        state.defense.character_number;

    CombatDefense defense =
        buildPlayerCombatDefense(
            state.defense,
            state.equipment,
            state.inventory,
            items);
    if (state.increased_power_updates != 0) {
        defense[3] =
            retailMultiply(defense[3], 12) / 10;
    }
    if (local_player &&
        state.energy_shield_active &&
        packet[1] != 3) {
        const std::int32_t scale =
            retailEffectParameter(
                tables,
                kEnergyShieldSpell,
                effectiveSpellLevel(
                    state, kEnergyShieldSpell),
                0);
        if (scale < 0) {
            return invalid();
        }
        defense[3] =
            retailMultiply(scale, defense[3]) /
            100;
    }

    if (packet[4] < 1) {
        result.damage.valid = true;
        result.damage.damage = 0;
    } else {
        result.damage =
            resolveCombatDamage(
                packet, defense, tables, random);
        if (!result.damage.valid) {
            return invalid();
        }
    }

    if (local_player) {
        if (!applyShield(
                result, packet, items, tables)) {
            return invalid();
        }
        PlayerDamageReceiverState& updated =
            result.state;
        if (!updated.energy_shield_active ||
            packet[1] == 3 ||
            updated.current_mana == 0) {
            updated.current_life =
                retailSubtract(
                    updated.current_life,
                    result.damage.damage);
        } else {
            updated.current_mana =
                std::max<std::int32_t>(
                    retailSubtract(
                        updated.current_mana,
                        result.damage.damage),
                    0);
        }

        if (updated.current_life < 1 &&
            consumeRevivalItem(
                updated.special_items)) {
            updated.current_life =
                updated.maximum_life;
            updated.current_mana =
                updated.maximum_mana;
            result.revived = true;
            result.audio_samples.push_back(
                kRevivalAudioSample);
            result.effects.push_back(
                baseEffect(updated, kRevivalEffect));
        }
        if (updated.current_life < 1) {
            if (updated.presentation_action !=
                    kHitPresentationAction ||
                updated.reaction_stage != 2) {
                updated.reaction_stage = 0;
            }
            updated.presentation_action =
                kDeathPresentationAction;
            updated.presentation_counter = 0;
            updated.action_lock = 1;
        }
        applyDurability(result, items, random);
    }

    if (result.state.presentation_action !=
            kHitPresentationAction ||
        result.state.reaction_stage != 2) {
        result.state.reaction_duration = 0;
    }

    tryReflection(
        result,
        packet,
        context,
        items,
        tables,
        random);
    if (!result.valid) {
        return invalid();
    }

    if (result.state.current_life > 0 &&
        !resolveReaction(
            result,
            packet,
            impact_origin,
            items,
            tables,
        random)) {
        return invalid();
    }

    addPacketEffects(result, packet, random);
    if (result.state.event_number == 0) {
        result.state.event_number = kDefaultEvent;
    }
    return result;
}

}  // namespace osf
