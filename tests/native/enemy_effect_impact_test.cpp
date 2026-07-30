#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/enemy_effect_impact.hpp"

#include <array>
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

osf::EnemyEffectImpactInput inputForType(
    std::int32_t type) {
    osf::EnemyEffectImpactInput input;
    input.source_character_number = 14000042;
    input.source_position = {1200, 3400};
    input.source_judgement = {-20, -30, 21, 31};
    input.direction_radians = 0.75;
    input.type = type;
    input.subtype = 10;
    input.parameter = 123;
    input.additive = 7;
    input.packet_word_31 = 456;
    return input;
}

bool testTableExpansionAndConstructorPacket(
    const osf::TableDatabase& tables) {
    osf::RetailRandom random(1);
    const osf::EnemyEffectSpawnRequest request =
        osf::resolveEnemyEffectImpact(
            inputForType(2), tables, random);
    if (!check(
            request.valid &&
                request.effect_number == 10002 &&
                request.owner_kind == 4 &&
                request.source_character_number ==
                    14000042 &&
                request.target_kind == 19 &&
                request.target_identifier == -1 &&
                request.constructor_value_6 == 100 &&
                request.constructor_value_7 == 250 &&
                request.direction_radians == 0.75 &&
                !request.has_explicit_origin &&
                request.has_source_judgement &&
                request.source_judgement.left == -20 &&
                request.constructor_value_12 == 0 &&
                request.packet_kind == 8 &&
                request.instance_identifier == -1 &&
                request.constructor_value_17 == 10 &&
                request.constructor_value_21 == 200 &&
                request.constructor_value_22 == 0,
            "Effect type two did not preserve the retail "
            "constructor arguments.")) {
        return false;
    }

    const auto& packet = request.packet;
    if (!check(
            packet[0] == 2 &&
                packet[1] == 3 &&
                packet[2] == 14000042 &&
                packet[3] == 1 &&
                packet[4] == 123 &&
                packet[31] == 456 &&
                packet[32] == 1 &&
                packet[34] == 21013 &&
                packet[35] == 8 &&
                packet[36] == 103 &&
                packet[37] == 0 &&
                packet[38] == 0 &&
                packet[39] == 0 &&
                packet[40] == 1 &&
                packet[41] == 1 &&
                packet[42] == 0 &&
                packet[43] == 1 &&
                packet[44] == 0 &&
                packet[45] == 1 &&
                packet[54] == 100 &&
                packet[63] == 28 &&
                packet[72] == 0 &&
                packet[73] == -1 &&
                packet[74] == -1 &&
                packet[75] == 8 &&
                packet[76] == 0,
            "Effect packet scalar fields or the first table-70 "
            "triple differ from the retail stack packet.")) {
        return false;
    }

    for (std::size_t index = 0; index < 9; ++index) {
        const osf::TableData* table =
            tables.find(
                70 + static_cast<std::int32_t>(index));
        if (!check(
                table &&
                    packet[54 + index] ==
                        table->value(2, 27) &&
                    packet[63 + index] ==
                        table->value(2, 28) &&
                    packet[45 + index] ==
                        table->value(2, 29),
                "One of tables 70 through 78 was placed in the "
                "wrong packet bank.")) {
            return false;
        }
    }
    if (!check(
            request.packet.written_words.count() == 50 &&
                request.packet.written_words.test(0) &&
                request.packet.written_words.test(72) &&
                !request.packet.written_words.test(5) &&
                !request.packet.written_words.test(33),
            "The effect packet did not distinguish retail-written "
            "fields from untouched stack words.")) {
        return false;
    }
    return check(
        random.state() == 1,
        "A non-random effect type consumed retail random state.");
}

bool testScalarSelectorTable(
    const osf::TableDatabase& tables) {
    constexpr std::array<std::int32_t, 6>
        expected{{69, 96, 30, 100, 0, 1}};
    for (std::int32_t selector = 0;
         selector < 6;
         ++selector) {
        if (!check(
                osf::retailEnemyEffectParameter(
                    tables,
                    2,
                    10,
                    selector) ==
                    expected[
                        static_cast<std::size_t>(
                            selector)],
                "The effect scalar selector used the wrong "
                "Table.Tbd table or cell.")) {
            return false;
        }
    }
    return check(
        osf::retailEnemyEffectParameter(
            tables, 2, 10, 30) == 1 &&
            osf::retailEnemyEffectParameter(
                tables, 2, 10, -1) == -1,
        "The effect scalar selector did not preserve its "
        "table-19 fallback or contain a negative selector.");
}

bool testEveryRetailEffectType(
    const osf::TableDatabase& tables) {
    struct Expected {
        std::int32_t type;
        std::int32_t duration;
        std::int32_t constructor_value_12;
        std::int32_t packet_mode;
        std::int32_t visual;
        std::int32_t packet_flag;
    };
    constexpr std::array<Expected, 12> expected{{
        {1, 250, 0, 0, 20000, 0},
        {2, 250, 0, 1, 21013, 0},
        {3, 0, 0, 0, 20005, 0},
        {4, 0, 10, 0, 20001, 0},
        {5, 0, 10, 1, 21013, 0},
        {10, 0, 0, 0, 21001, 1},
        {11, 250, 0, 0, 20000, 0},
        {12, 0, 10, 1, 21013, 0},
        {13, 0, 0, 0, 20005, 0},
        {14, 250, 0, 2, 21019, 0},
        {16, 250, 0, 0, 20000, 0},
        {21, 250, 0, 0, 21000, 0},
    }};

    for (const Expected& values : expected) {
        osf::RetailRandom random(1);
        const osf::EnemyEffectSpawnRequest request =
            osf::resolveEnemyEffectImpact(
                inputForType(values.type),
                tables,
                random);
        if (!check(
                request.valid &&
                    request.constructor_value_7 ==
                        values.duration &&
                    request.constructor_value_12 ==
                        values.constructor_value_12 &&
                    request.packet[3] ==
                        values.packet_mode &&
                    request.packet[34] ==
                        values.visual &&
                    request.packet[72] ==
                        values.packet_flag,
                "A shipped effect type did not reproduce its "
                "retail constructor switch.")) {
            std::cerr
                << "effect type: " << values.type << '\n';
            return false;
        }
        const bool random_type = values.type == 10;
        if (!check(
                random.state() ==
                    (random_type ? 2745024u : 1u),
                "An effect type consumed the wrong number of "
                "retail random draws.")) {
            return false;
        }
    }

    osf::RetailRandom random(1);
    const osf::EnemyEffectSpawnRequest invalid =
        osf::resolveEnemyEffectImpact(
            inputForType(-1), tables, random);
    return check(
        !invalid.valid &&
            random.state() == 1,
        "The catalog's disabled type-minus-one effect became a "
        "spawn or consumed random state.");
}

bool testTypeTwelveTarget(
    const osf::TableDatabase& tables) {
    osf::EnemyEffectImpactInput input =
        inputForType(12);
    input.target = {
        true,
        osf::MovementTargetKind::player,
        3,
        40,
        {10, 20},
    };
    osf::RetailRandom random(1);
    osf::EnemyEffectSpawnRequest request =
        osf::resolveEnemyEffectImpact(
            input, tables, random);
    if (!check(
            request.target_kind == 1 &&
                request.target_identifier == 3,
            "Effect type twelve did not attach to its retail "
            "player slot.")) {
        return false;
    }

    input.target.kind =
        osf::MovementTargetKind::scenario_actor;
    input.target.identifier =
        osf::kFirstCompanionCharacterNumber + 2;
    request = osf::resolveEnemyEffectImpact(
        input, tables, random);
    return check(
        request.target_kind == 2 &&
            request.target_identifier ==
                osf::kFirstCompanionCharacterNumber + 2,
        "Effect type twelve did not attach to its retail "
        "scenario-actor character number.");
}

bool testExplicitOrigins(
    const osf::TableDatabase& tables) {
    for (const std::int32_t type : {3, 10, 13}) {
        osf::RetailRandom random(1);
        const osf::EnemyEffectSpawnRequest request =
            osf::resolveEnemyEffectImpact(
                inputForType(type), tables, random);
        if (!check(
                request.has_explicit_origin &&
                    request.origin.x == 1200 &&
                    request.origin.y == 3400,
                "A retail source-position effect omitted its "
                "explicit constructor origin.")) {
            return false;
        }
    }
    return true;
}

bool testMissingTablesRemainExplicit() {
    osf::TableDatabase tables;
    osf::RetailRandom random(1);
    const osf::EnemyEffectSpawnRequest request =
        osf::resolveEnemyEffectImpact(
            inputForType(1), tables, random);
    return check(
        request.valid &&
            request.constructor_value_6 == -1 &&
            request.constructor_value_22 == -1 &&
            request.packet[32] == -1 &&
            request.packet[36] == 6 &&
            request.packet[45] == -1 &&
            request.packet[54] == -1 &&
            request.packet[63] == -1,
        "Missing parameter tables were silently confused with "
        "retail zero values.");
}

}  // namespace

int main() {
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                std::filesystem::path(
                    OPENSHADOWFLARE_SOURCE_DIR) /
                    "tmp" / "ShadowFlare" / "System" /
                    "Game" / "Parameter" / "Table.Tbd",
                &error),
            "The retail effect parameter tables could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return 1;
    }
    if (!testTableExpansionAndConstructorPacket(tables) ||
        !testScalarSelectorTable(tables) ||
        !testEveryRetailEffectType(tables) ||
        !testTypeTwelveTarget(tables) ||
        !testExplicitOrigins(tables) ||
        !testMissingTablesRemainExplicit()) {
        return 1;
    }
    return 0;
}
