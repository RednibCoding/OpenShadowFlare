#include "combat_damage.hpp"

#include "core/retail_integer.hpp"
#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <algorithm>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kPhysicalScaleTable = 7;
constexpr std::int32_t kDefenseScaleTable = 11;
constexpr std::int32_t kTableRowOffset = 10;

std::int32_t pairedElement(
    std::int32_t element) {
    return (element / 2) * 2 -
               (element % 2) +
           1;
}

bool tableValue(
    const TableDatabase& tables,
    std::int32_t table_number,
    std::int32_t row,
    std::int32_t& value) {
    const TableData* table = tables.find(table_number);
    if (!table || !table->contains(row, 0)) {
        return false;
    }
    value = table->value(row, 0);
    return true;
}

bool defenseScale(
    const CombatPacket& packet,
    const CombatDefense& defense,
    const TableDatabase& tables,
    std::int32_t& scale) {
    const std::int32_t packet_element = packet[32];
    const std::int32_t defense_index =
        packet_element + 5;
    if (defense_index < 0 ||
        defense_index >=
            static_cast<std::int32_t>(
                kCombatDefenseWordCount)) {
        return false;
    }
    return tableValue(
        tables,
        kDefenseScaleTable,
        defense[
            static_cast<std::size_t>(defense_index)] +
            kTableRowOffset,
        scale);
}

std::int32_t elementalFactor(
    std::int32_t packet_element,
    std::int32_t defense_element,
    RetailRandom& random) {
    if (pairedElement(defense_element) ==
        packet_element) {
        return random.next() % 4 + 10;
    }
    if (defense_element == packet_element) {
        return random.next() % 4 + 7;
    }
    return 10;
}

std::int32_t scaledElementalDamage(
    std::int32_t base_damage,
    std::int32_t elemental_factor,
    std::int32_t defense_value,
    RetailRandom& random) {
    const std::int32_t random_factor =
        random.next() % 3 + 9;
    const std::int32_t scaled =
        retailMultiply(
            retailMultiply(
                random_factor,
                base_damage),
            elemental_factor) /
        100;
    return retailSubtract(scaled, defense_value);
}

CombatDamageResult finished(
    std::int32_t damage) {
    CombatDamageResult result;
    result.valid = true;
    result.damage = std::max<std::int32_t>(damage, 1);
    return result;
}

CombatDamageResult invalid() {
    return {};
}

CombatDamageResult resolveEffectDamage(
    const CombatPacket& packet,
    const CombatDefense& defense,
    const TableDatabase& tables,
    RetailRandom& random) {
    if (defense[0] == 0) {
        std::int32_t scale = 0;
        if (!defenseScale(
                packet, defense, tables, scale)) {
            return invalid();
        }
        const std::int32_t random_factor =
            random.next() % 3 + 9;
        const std::int32_t offense =
            retailMultiply(
                random_factor, packet[4]) /
            10;
        const std::int32_t reduction =
            retailMultiply(defense[4], scale) /
            10;
        return finished(
            retailSubtract(offense, reduction));
    }
    if (defense[0] != 1 && defense[0] != 2) {
        return finished(1);
    }
    const std::int32_t factor =
        elementalFactor(
            packet[32], defense[13], random);
    return finished(
        scaledElementalDamage(
            packet[4],
            factor,
            defense[4],
            random));
}

std::uint32_t damageDispatchKey(
    const CombatPacket& packet,
    const CombatDefense& defense) {
    return static_cast<std::uint16_t>(packet[0]) |
           (static_cast<std::uint32_t>(
                static_cast<std::uint16_t>(
                    defense[0]))
            << 16U);
}

CombatDamageResult resolvePhysicalScaleDamage(
    const CombatPacket& packet,
    const CombatDefense& defense,
    const TableDatabase& tables,
    RetailRandom& random) {
    CombatDamageResult result;
    result.requests_source_lookup = true;
    result.source_character_number = packet[2];

    const std::int32_t random_factor =
        packet[1] == 1
        ? random.next() % 3 + 10
        : random.next() % 3 + 9;
    if (defense[13] < 0 || defense[13] > 7) {
        return result;
    }
    const std::int32_t packet_index =
        pairedElement(defense[13]) * 2 + 6;
    if (packet_index < 0 ||
        packet_index >=
            static_cast<std::int32_t>(
                kCombatPacketWordCount)) {
        return result;
    }
    std::int32_t scale = 0;
    if (!tableValue(
            tables,
            kPhysicalScaleTable,
            packet[
                static_cast<std::size_t>(
                    packet_index)] +
                kTableRowOffset,
            scale)) {
        return result;
    }

    const std::int32_t scaled =
        retailMultiply(
            retailMultiply(
                packet[4], random_factor),
            scale) /
        100;
    result.valid = true;
    result.damage = std::max<std::int32_t>(
        retailSubtract(scaled, defense[3]), 1);
    return result;
}

CombatDamageResult resolveDefenseScaleDamage(
    const CombatPacket& packet,
    const CombatDefense& defense,
    const TableDatabase& tables,
    RetailRandom& random) {
    std::int32_t scale = 0;
    if (!defenseScale(
            packet, defense, tables, scale)) {
        return invalid();
    }
    const std::int32_t random_factor =
        random.next() % 3 + 9;
    const std::int32_t offense =
        retailMultiply(
            random_factor, packet[4]) /
        10;
    const std::int32_t reduction =
        retailMultiply(defense[3], scale) /
        10;
    return finished(
        retailSubtract(offense, reduction));
}

}  // namespace

CombatDamageResult resolveCombatDamage(
    const CombatPacket& packet,
    const CombatDefense& defense,
    const TableDatabase& tables,
    RetailRandom& random) {
    if (packet[37] == 1) {
        CombatDamageResult result;
        result.valid = true;
        result.damage = packet[4];
        return result;
    }

    if (packet[1] == 3) {
        return resolveEffectDamage(
            packet, defense, tables, random);
    }

    switch (damageDispatchKey(packet, defense)) {
    case 0x00020000:
        return resolvePhysicalScaleDamage(
            packet, defense, tables, random);
    case 0x00000002:
        return resolveDefenseScaleDamage(
            packet, defense, tables, random);
    case 0x00010002:
    case 0x00020001:
        return finished(
            scaledElementalDamage(
                packet[4],
                elementalFactor(
                    packet[32],
                    defense[13],
                    random),
                defense[3],
                random));
    default:
        return finished(1);
    }
}

}  // namespace osf
