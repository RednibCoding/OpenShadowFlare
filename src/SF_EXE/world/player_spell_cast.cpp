#include "player_spell_cast.hpp"

#include "actor_direction.hpp"
#include "core/retail_random.hpp"
#include "enemy_effect_impact.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kPlayerOwnerKind = 1;
constexpr std::int32_t kEnemyAndObjectTargetMask = 0x14;

struct PlayerSpellDescriptor {
    std::int32_t effect_number = -1;
    std::int32_t packet_subtype = 0;
    std::int32_t impact_effect = -1;
    std::int32_t target_mask =
        kEnemyAndObjectTargetMask;
    bool requires_target = true;
    bool use_table_travel_speed = true;
    bool use_explicit_origin = false;
    bool use_source_judgement = true;
    bool constructor_uses_level = false;
    bool use_physical_defense = false;
    bool random_ordinary_impact = false;
    std::int32_t packet_value_72 = 0;
};

bool playerSpellDescriptor(
    std::int32_t spell,
    PlayerSpellDescriptor& descriptor) {
    switch (spell) {
    case 1:
        descriptor = {10001, 0, 20000};
        return true;
    case 2:
        descriptor = {10002, 1, 21013};
        return true;
    case 3:
        descriptor = {
            10003,
            0,
            20005,
            4,
            true,
            false,
            true,
            false,
            true,
            true,
        };
        return true;
    case 4:
        descriptor = {
            10004,
            0,
            20001,
            4,
            false,
            false,
            false,
            true,
            false,
            false,
        };
        return true;
    case 5:
        descriptor = {
            10005,
            1,
            21013,
            4,
            false,
            false,
            false,
            true,
            false,
            false,
        };
        return true;
    case 10:
        descriptor = {
            10010,
            0,
            -1,
            4,
            true,
            false,
            true,
            false,
            true,
            true,
            true,
            1,
        };
        return true;
    default:
        return false;
    }
}

std::int32_t tableValue(
    const TableDatabase& tables,
    std::int32_t table_number,
    std::int32_t row,
    std::int32_t column) {
    const TableData* table = tables.find(table_number);
    return table && table->contains(row, column)
        ? table->value(row, column)
        : -1;
}

}  // namespace

bool playerSpellRequiresCharacterTarget(
    std::int32_t spell) {
    PlayerSpellDescriptor descriptor;
    return playerSpellDescriptor(spell, descriptor) &&
           descriptor.requires_target;
}

CombatEffectSpawnRequest buildPlayerSpellCast(
    std::int32_t spell,
    const PlayerSpellCastInput& input,
    const TableDatabase& tables,
    RetailRandom* random) {
    CombatEffectSpawnRequest request;
    PlayerSpellDescriptor descriptor;
    if (input.stats.source_character_number < 0 ||
        input.parameters.effective_level < 1 ||
        !playerSpellDescriptor(spell, descriptor) ||
        (descriptor.random_ordinary_impact && !random) ||
        (descriptor.requires_target &&
         input.target_character_number < 0)) {
        return request;
    }

    request.valid = true;
    request.effect_number = descriptor.effect_number;
    request.owner_kind = kPlayerOwnerKind;
    request.source_character_number =
        input.stats.source_character_number;
    request.target_kind = descriptor.target_mask;
    request.target_identifier =
        input.target_character_number;
    request.constructor_value_6 =
        descriptor.use_table_travel_speed
            ? retailEffectParameter(
                  tables,
                  spell,
                  input.parameters.effective_level,
                  3)
            : 0;
    request.constructor_value_7 =
        descriptor.use_table_travel_speed ? 200 : 0;
    request.direction_radians =
        descriptor.requires_target
            ? retailAngleForVector(
                  input.target_position.x -
                      input.source_position.x,
                  input.target_position.y -
                      input.source_position.y)
            : 0.0;
    request.has_explicit_origin =
        descriptor.use_explicit_origin;
    request.origin = input.source_position;
    request.has_source_judgement =
        descriptor.use_source_judgement;
    if (request.has_source_judgement) {
        request.source_judgement =
            input.source_judgement;
    }
    request.constructor_value_12 =
        input.effect_delay;
    request.has_packet = true;
    request.packet_kind = 8;
    request.instance_identifier = -1;
    request.constructor_value_16 = 0;
    request.constructor_value_17 =
        descriptor.constructor_uses_level
            ? input.parameters.effective_level
            : 0;
    request.constructor_value_18 = 0;
    request.constructor_value_19 = 0;
    request.constructor_value_20 = 0;
    request.constructor_value_21 = 200;
    request.constructor_value_22 =
        retailEffectParameter(
            tables,
            spell,
            input.parameters.effective_level,
            4);

    CombatPacket& packet = request.packet;
    packet.write(0, 0);
    packet.write(1, 3);
    packet.write(
        2, input.stats.source_character_number);
    packet.write(3, descriptor.packet_subtype);
    packet.write(
        4,
        input.parameters.effect_value +
            input.stats.magical_attack);
    if (packet[4] < 1) {
        packet.write(4, 1);
    }
    packet.write(
        5,
        descriptor.use_physical_defense
            ? input.stats.physical_defense
            : input.stats.magical_defense);
    for (std::size_t index = 0;
         index < input.stats.element_affinities.size();
         ++index) {
        packet.write(
            6 + index,
            input.stats.element_affinities[index]);
    }
    for (std::size_t index = 0;
         index < input.stats.state_words.size();
         ++index) {
        packet.write(
            14 + index,
            input.stats.state_words[index]);
    }
    packet.write(31, input.stats.player_level);
    const std::int32_t type_value =
        retailEffectParameter(
            tables,
            spell,
            input.parameters.effective_level,
            5);
    packet.write(32, type_value);
    packet.write(
        34,
        descriptor.random_ordinary_impact
            ? random->next() % 4 + 21000
            : descriptor.impact_effect);
    packet.write(35, 8);
    packet.write(
        36,
        retailEffectParameter(
            tables,
            spell,
            input.parameters.effective_level,
            1) +
            input.stats.magical_hit_rate);
    packet.write(37, 0);
    packet.write(38, 0);
    packet.write(39, 0);
    packet.write(40, type_value);
    packet.write(41, type_value);
    packet.write(42, 0);
    packet.write(43, type_value);
    packet.write(44, 0);
    for (std::size_t table_index = 0;
         table_index < 9;
         ++table_index) {
        const std::int32_t table_number =
            70 + static_cast<std::int32_t>(
                     table_index);
        const std::int32_t first_column =
            input.parameters.effective_level * 3 - 3;
        packet.write(
            54 + table_index,
            tableValue(
                tables,
                table_number,
                spell,
                first_column));
        packet.write(
            63 + table_index,
            tableValue(
                tables,
                table_number,
                spell,
                first_column + 1));
        packet.write(
            45 + table_index,
            tableValue(
                tables,
                table_number,
                spell,
                first_column + 2));
    }
    packet.write(72, descriptor.packet_value_72);
    packet.write(73, spell);
    packet.write(74, -1);
    packet.write(75, 8);
    packet.write(76, 0);
    return request;
}

}  // namespace osf
