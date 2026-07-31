#include "player_spell_cast.hpp"

#include "actor_direction.hpp"
#include "enemy_effect_impact.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kFireBallSpell = 1;
constexpr std::int32_t kFireBallEffect = 10001;
constexpr std::int32_t kPlayerOwnerKind = 1;
constexpr std::int32_t kEnemyAndObjectTargetMask = 0x14;

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

CombatEffectSpawnRequest buildPlayerFireBallCast(
    const PlayerFireBallCastInput& input,
    const TableDatabase& tables) {
    CombatEffectSpawnRequest request;
    if (input.stats.source_character_number < 0 ||
        input.target_character_number < 0 ||
        input.parameters.effective_level < 1) {
        return request;
    }

    request.valid = true;
    request.effect_number = kFireBallEffect;
    request.owner_kind = kPlayerOwnerKind;
    request.source_character_number =
        input.stats.source_character_number;
    request.target_kind = kEnemyAndObjectTargetMask;
    request.target_identifier =
        input.target_character_number;
    request.constructor_value_6 =
        retailEffectParameter(
            tables,
            kFireBallSpell,
            input.parameters.effective_level,
            3);
    request.constructor_value_7 = 200;
    request.direction_radians =
        retailAngleForVector(
            input.target_position.x -
                input.source_position.x,
            input.target_position.y -
                input.source_position.y);
    request.has_source_judgement = true;
    request.source_judgement =
        input.source_judgement;
    request.constructor_value_12 =
        input.effect_delay;
    request.has_packet = true;
    request.packet_kind = 8;
    request.instance_identifier = -1;
    request.constructor_value_16 = 0;
    request.constructor_value_17 = 0;
    request.constructor_value_18 = 0;
    request.constructor_value_19 = 0;
    request.constructor_value_20 = 0;
    request.constructor_value_21 = 200;
    request.constructor_value_22 =
        retailEffectParameter(
            tables,
            kFireBallSpell,
            input.parameters.effective_level,
            4);

    CombatPacket& packet = request.packet;
    packet.write(0, 0);
    packet.write(1, 3);
    packet.write(
        2, input.stats.source_character_number);
    packet.write(3, 0);
    packet.write(
        4,
        input.parameters.effect_value +
            input.stats.magical_attack);
    if (packet[4] < 1) {
        packet.write(4, 1);
    }
    packet.write(5, input.stats.magical_defense);
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
            kFireBallSpell,
            input.parameters.effective_level,
            5);
    packet.write(32, type_value);
    packet.write(34, 20000);
    packet.write(35, 8);
    packet.write(
        36,
        retailEffectParameter(
            tables,
            kFireBallSpell,
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
                kFireBallSpell,
                first_column));
        packet.write(
            63 + table_index,
            tableValue(
                tables,
                table_number,
                kFireBallSpell,
                first_column + 1));
        packet.write(
            45 + table_index,
            tableValue(
                tables,
                table_number,
                kFireBallSpell,
                first_column + 2));
    }
    packet.write(72, 0);
    packet.write(73, kFireBallSpell);
    packet.write(74, -1);
    packet.write(75, 8);
    packet.write(76, 0);
    return request;
}

}  // namespace osf
