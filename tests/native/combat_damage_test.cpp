#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/combat_damage.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

osf::TableDatabase retailTables() {
    osf::TableDatabase tables;
    std::string error;
    if (!tables.load(
            std::string(OPENSHADOWFLARE_SOURCE_DIR) +
                "/tmp/ShadowFlare/System/Game/Parameter/Table.Tbd",
            &error)) {
        std::cerr
            << "The retail Table.Tbd fixture could not be decoded: "
            << error << '\n';
    }
    return tables;
}

bool testDirectOverrideAndUnsupportedPair() {
    const osf::TableDatabase tables = retailTables();
    osf::CombatPacket packet;
    packet.write(4, -7);
    packet.write(37, 1);
    osf::CombatDefense defense;
    osf::RetailRandom random(1);
    const osf::CombatDamageResult direct =
        osf::resolveCombatDamage(
            packet, defense, tables, random);
    if (!check(
            direct.valid &&
                direct.damage == -7 &&
                !direct.requests_source_lookup &&
                random.state() == 1,
            "The direct damage override was clamped or consumed "
            "random state.")) {
        return false;
    }

    packet.write(37, 0);
    packet.write(0, 1);
    packet.write(1, 0);
    const osf::CombatDamageResult unsupported =
        osf::resolveCombatDamage(
            packet, defense, tables, random);
    return check(
        unsupported.valid &&
            unsupported.damage == 1 &&
            random.state() == 1,
        "An unsupported retail packet/defense pair did not return "
        "one damage without a random draw.");
}

bool testScaleTableBranches() {
    const osf::TableDatabase tables = retailTables();

    osf::CombatPacket physical;
    physical.write(0, 2);
    physical.write(1, 0);
    physical.write(4, 100);
    physical.write(32, 0);
    physical.write(37, 0);
    osf::CombatDefense defense;
    defense[0] = 0;
    defense[3] = 20;
    defense[4] = 20;
    defense[5] = 0;

    osf::RetailRandom physical_random(1);
    const osf::CombatDamageResult physical_result =
        osf::resolveCombatDamage(
            physical,
            defense,
            tables,
            physical_random);
    if (!check(
            physical_result.valid &&
                physical_result.damage == 90 &&
                physical_random.state() == 2745024u,
            "The ordinary table-11 damage branch changed its "
            "scale, defense field, or random ordering.")) {
        return false;
    }

    physical.write(1, 3);
    osf::RetailRandom effect_random(1);
    const osf::CombatDamageResult effect_result =
        osf::resolveCombatDamage(
            physical,
            defense,
            tables,
            effect_random);
    return check(
        effect_result.valid &&
            effect_result.damage == 90 &&
            effect_random.state() == 2745024u,
        "The effect-family table-11 branch did not use elemental "
        "defense with the retail random ordering.");
}

bool testElementalBranches() {
    const osf::TableDatabase tables = retailTables();
    osf::CombatPacket packet;
    packet.write(0, 2);
    packet.write(1, 0);
    packet.write(4, 100);
    packet.write(37, 0);
    osf::CombatDefense defense;
    defense[0] = 1;
    defense[3] = 5;
    defense[13] = 2;

    packet.write(32, 3);
    osf::RetailRandom opposing_random(1);
    const osf::CombatDamageResult opposing =
        osf::resolveCombatDamage(
            packet, defense, tables, opposing_random);
    osf::RetailRandom opposing_expected(1);
    opposing_expected.next();
    opposing_expected.next();
    if (!check(
            opposing.valid &&
                opposing.damage == 116 &&
                opposing_random.state() ==
                    opposing_expected.state(),
            "The opposing-element branch changed its multiplier "
            "or two-draw ordering.")) {
        return false;
    }

    packet.write(32, 2);
    osf::RetailRandom exact_random(1);
    const osf::CombatDamageResult exact =
        osf::resolveCombatDamage(
            packet, defense, tables, exact_random);
    if (!check(
            exact.valid &&
                exact.damage == 83 &&
                exact_random.state() ==
                    opposing_expected.state(),
            "The same-element branch changed its reduced "
            "multiplier or two-draw ordering.")) {
        return false;
    }

    packet.write(32, 4);
    osf::RetailRandom other_random(1);
    const osf::CombatDamageResult other =
        osf::resolveCombatDamage(
            packet, defense, tables, other_random);
    if (!check(
            other.valid &&
                other.damage == 105 &&
                other_random.state() == 2745024u,
            "The unrelated-element branch consumed the optional "
            "draw or changed its neutral multiplier.")) {
        return false;
    }

    packet.write(0, 1);
    packet.write(32, 3);
    defense[0] = 2;
    osf::RetailRandom alternate_random(1);
    const osf::CombatDamageResult alternate =
        osf::resolveCombatDamage(
            packet, defense, tables, alternate_random);
    return check(
        alternate.valid &&
            alternate.damage == 116 &&
            alternate_random.state() ==
                opposing_expected.state(),
        "The alternate elemental packet/defense pair did not "
        "share the retail elemental formula.");
}

bool testEffectElementalDefense() {
    const osf::TableDatabase tables = retailTables();
    osf::CombatPacket packet;
    packet.write(0, 2);
    packet.write(1, 3);
    packet.write(4, 100);
    packet.write(32, 3);
    packet.write(37, 0);
    osf::CombatDefense defense;
    defense[0] = 2;
    defense[4] = 6;
    defense[13] = 2;
    osf::RetailRandom random(1);
    const osf::CombatDamageResult result =
        osf::resolveCombatDamage(
            packet, defense, tables, random);
    return check(
        result.valid && result.damage == 115,
        "The elemental effect branch used physical defense "
        "instead of the effect-defense word.");
}

bool testRetailIntegerWrapping() {
    const osf::TableDatabase tables = retailTables();
    osf::CombatPacket packet;
    packet.write(0, 2);
    packet.write(1, 0);
    packet.write(4, 200000000);
    packet.write(32, 4);
    packet.write(37, 0);
    osf::CombatDefense defense;
    defense[0] = 1;
    defense[13] = 2;
    osf::RetailRandom random(1);
    const osf::CombatDamageResult result =
        osf::resolveCombatDamage(
            packet, defense, tables, random);
    return check(
        result.valid && result.damage == 5251635,
        "Combat multiplication did not retain retail 32-bit "
        "wrapping on a large packet value.");
}

bool testPhysicalScaleAndLookupRequest() {
    const osf::TableDatabase tables = retailTables();
    osf::CombatPacket packet;
    packet.write(0, 0);
    packet.write(1, 1);
    packet.write(2, 14000012);
    packet.write(4, 100);
    packet.write(12, 0);
    packet.write(37, 0);
    osf::CombatDefense defense;
    defense[0] = 2;
    defense[3] = 5;
    defense[13] = 2;
    osf::RetailRandom random(1);
    const osf::CombatDamageResult result =
        osf::resolveCombatDamage(
            packet, defense, tables, random);
    if (!check(
            result.valid &&
                result.damage == 115 &&
                result.requests_source_lookup &&
                result.source_character_number == 14000012 &&
                random.state() == 2745024u,
            "The table-7 physical branch lost its source lookup, "
            "special factor, or packet element value.")) {
        return false;
    }

    packet.write(1, 0);
    defense[3] = 500;
    osf::RetailRandom clamped_random(1);
    const osf::CombatDamageResult clamped =
        osf::resolveCombatDamage(
            packet, defense, tables, clamped_random);
    if (!check(
            clamped.valid && clamped.damage == 1,
            "A calculated retail damage value was not clamped "
            "to one.")) {
        return false;
    }

    defense[13] = 8;
    osf::RetailRandom invalid_element_random(1);
    const osf::CombatDamageResult invalid_element =
        osf::resolveCombatDamage(
            packet,
            defense,
            tables,
            invalid_element_random);
    return check(
        !invalid_element.valid &&
            invalid_element.requests_source_lookup &&
            invalid_element_random.state() == 2745024u,
        "An unsafe native-element packet index was not contained "
        "after the retail lookup and random ordering.");
}

bool testInvalidPortableLookupsPreserveOrdering() {
    osf::TableDatabase no_tables;
    osf::CombatPacket packet;
    packet.write(0, 2);
    packet.write(1, 3);
    packet.write(4, 100);
    packet.write(32, 0);
    packet.write(37, 0);
    osf::CombatDefense defense;
    defense[0] = 0;
    defense[5] = 0;
    osf::RetailRandom effect_random(1);
    const osf::CombatDamageResult effect =
        osf::resolveCombatDamage(
            packet,
            defense,
            no_tables,
            effect_random);
    if (!check(
            !effect.valid &&
                effect_random.state() == 1,
            "A missing table-11 lookup consumed the later random "
            "draw or returned fabricated damage.")) {
        return false;
    }

    packet.write(0, 0);
    packet.write(1, 1);
    packet.write(2, 7);
    packet.write(12, 0);
    defense[0] = 2;
    defense[13] = 2;
    osf::RetailRandom physical_random(1);
    const osf::CombatDamageResult physical =
        osf::resolveCombatDamage(
            packet,
            defense,
            no_tables,
            physical_random);
    return check(
        !physical.valid &&
            physical.requests_source_lookup &&
            physical.source_character_number == 7 &&
            physical_random.state() == 2745024u,
        "A missing table-7 lookup did not preserve the preceding "
        "source lookup and random draw.");
}

}  // namespace

int main() {
    return testDirectOverrideAndUnsupportedPair() &&
                   testScaleTableBranches() &&
                   testElementalBranches() &&
                   testEffectElementalDefense() &&
                   testRetailIntegerWrapping() &&
                   testPhysicalScaleAndLookupRequest() &&
                   testInvalidPortableLookupsPreserveOrdering()
        ? 0
        : 1;
}
