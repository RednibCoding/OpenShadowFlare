#include "enemy_effect_impact.hpp"

#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kEffectTypeBase = 10000;
constexpr std::int32_t kDefaultTargetKind = 19;
constexpr std::int32_t kPlayerTargetKind = 1;
constexpr std::int32_t kScenarioActorTargetKind = 2;

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

bool configureType(
    const EnemyEffectImpactInput& input,
    CombatEffectSpawnRequest& request,
    RetailRandom& random) {
    request.target_kind = kDefaultTargetKind;
    request.target_identifier = -1;

    switch (input.type) {
    case 1:
    case 16:
        request.constructor_value_7 = 250;
        request.constructor_value_12 = 0;
        request.packet.write(3, 0);
        request.packet.write(34, 20000);
        request.packet.write(72, 0);
        return true;
    case 2:
        request.constructor_value_7 = 250;
        request.constructor_value_12 = 0;
        request.packet.write(3, 1);
        request.packet.write(34, 21013);
        request.packet.write(72, 0);
        return true;
    case 3:
        request.constructor_value_7 = 0;
        request.constructor_value_12 = 0;
        request.has_explicit_origin = true;
        request.origin = input.source_position;
        request.packet.write(3, 0);
        request.packet.write(34, 20005);
        request.packet.write(72, 0);
        return true;
    case 4:
        request.constructor_value_7 = 0;
        request.constructor_value_12 = 10;
        request.packet.write(3, 0);
        request.packet.write(34, 20001);
        request.packet.write(72, 0);
        return true;
    case 5:
        request.constructor_value_7 = 0;
        request.constructor_value_12 = 10;
        request.packet.write(3, 1);
        request.packet.write(34, 21013);
        request.packet.write(72, 0);
        return true;
    case 10:
        request.constructor_value_7 = 0;
        request.constructor_value_12 = 0;
        request.has_explicit_origin = true;
        request.origin = input.source_position;
        request.packet.write(3, 0);
        request.packet.write(
            34, random.next() % 4 + 21000);
        request.packet.write(72, 1);
        return true;
    case 11:
        request.constructor_value_7 = 250;
        request.constructor_value_12 = 0;
        request.packet.write(3, 0);
        request.packet.write(34, 20000);
        request.packet.write(72, 0);
        return true;
    case 12:
        request.constructor_value_7 = 0;
        request.constructor_value_12 = 10;
        request.packet.write(3, 1);
        request.packet.write(34, 21013);
        request.packet.write(72, 0);
        if (input.target.found) {
            request.target_kind =
                input.target.kind ==
                        MovementTargetKind::player
                    ? kPlayerTargetKind
                    : kScenarioActorTargetKind;
            request.target_identifier =
                input.target.identifier;
        }
        return true;
    case 13:
        request.constructor_value_7 = 0;
        request.constructor_value_12 = 0;
        request.has_explicit_origin = true;
        request.origin = input.source_position;
        request.packet.write(3, 0);
        request.packet.write(34, 20005);
        request.packet.write(72, 0);
        return true;
    case 14:
        request.constructor_value_7 = 250;
        request.constructor_value_12 = 0;
        request.packet.write(3, 2);
        request.packet.write(34, 21019);
        request.packet.write(72, 0);
        return true;
    case 21:
        request.constructor_value_7 = 250;
        request.constructor_value_12 = 0;
        request.packet.write(3, 0);
        request.packet.write(34, 21000);
        request.packet.write(72, 0);
        return true;
    default:
        return false;
    }
}

}  // namespace

std::int32_t retailEffectParameter(
    const TableDatabase& tables,
    std::int32_t type,
    std::int32_t subtype,
    std::int32_t selector) {
    constexpr std::array<std::int32_t, 5>
        selector_tables{{17, 18, 16, 35, 21}};
    if (selector < 0) {
        return -1;
    }
    if (static_cast<std::size_t>(selector) <
        selector_tables.size()) {
        return tableValue(
            tables,
            selector_tables[
                static_cast<std::size_t>(selector)],
            type,
            subtype - 1);
    }
    return tableValue(tables, 19, type, 0);
}

CombatEffectSpawnRequest resolveEnemyEffectImpact(
    const EnemyEffectImpactInput& input,
    const TableDatabase& tables,
    RetailRandom& random) {
    CombatEffectSpawnRequest request;
    request.effect_number = input.type + kEffectTypeBase;
    request.source_character_number =
        input.source_character_number;
    request.direction_radians = input.direction_radians;
    request.has_source_judgement = true;
    request.source_judgement = input.source_judgement;
    request.constructor_value_17 = input.subtype;
    request.has_packet = true;

    request.packet.write(0, 2);
    request.packet.write(1, 3);
    request.packet.write(
        2, input.source_character_number);
    request.packet.write(4, input.parameter);
    request.packet.write(31, input.packet_word_31);
    const std::int32_t type_value =
        retailEffectParameter(
            tables,
            input.type,
            input.subtype,
            5);
    request.packet.write(32, type_value);
    request.packet.write(35, 8);
    request.packet.write(
        36,
        retailEffectParameter(
            tables,
            input.type,
            input.subtype,
            1) +
        input.additive);
    request.packet.write(37, 0);
    request.packet.write(38, 0);
    request.packet.write(39, 0);
    request.packet.write(40, type_value);
    request.packet.write(41, type_value);
    request.packet.write(42, 0);
    request.packet.write(43, type_value);
    request.packet.write(44, 0);
    for (std::size_t table_index = 0;
         table_index < 9;
         ++table_index) {
        const std::int32_t table_number =
            70 + static_cast<std::int32_t>(
                     table_index);
        const std::int32_t first_column =
            input.subtype * 3 - 3;
        request.packet.write(
            54 + table_index,
            tableValue(
                tables,
                table_number,
                input.type,
                first_column));
        request.packet.write(
            63 + table_index,
            tableValue(
                tables,
                table_number,
                input.type,
                first_column + 1));
        request.packet.write(
            45 + table_index,
            tableValue(
                tables,
                table_number,
                input.type,
                first_column + 2));
    }
    request.packet.write(73, -1);
    request.packet.write(74, -1);
    request.packet.write(75, 8);
    request.packet.write(76, 0);

    request.constructor_value_6 =
        retailEffectParameter(
            tables,
            input.type,
            input.subtype,
            3);
    request.constructor_value_22 =
        retailEffectParameter(
            tables,
            input.type,
            input.subtype,
            4);
    request.valid = configureType(
        input, request, random);
    return request;
}

}  // namespace osf
