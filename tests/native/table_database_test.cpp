#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

void appendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value) {
    const std::uint32_t data = static_cast<std::uint32_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(data));
    bytes.push_back(static_cast<std::uint8_t>(data >> 8u));
    bytes.push_back(static_cast<std::uint8_t>(data >> 16u));
    bytes.push_back(static_cast<std::uint8_t>(data >> 24u));
}

}  // namespace

int main() {
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                std::string(OPENSHADOWFLARE_SOURCE_DIR) +
                    "/tmp/ShadowFlare/System/Game/Parameter/Table.Tbd",
                &error),
            "The retail Table.Tbd fixture could not be decoded.")) {
        std::cerr << error << '\n';
        return 1;
    }

    const osf::TableData* male = tables.find(900);
    const osf::TableData* female = tables.find(901);
    const osf::TableData* transport = tables.find(40);
    const osf::TableData* effect_waves = tables.find(205);
    if (!check(
            tables.tables().size() == 138 &&
                female && male && transport && effect_waves &&
                female->rowCount() == 13 &&
                female->columnCount() == 5 &&
                male->rowCount() == 13 &&
                male->columnCount() == 5 &&
                transport->rowCount() == 51 &&
                transport->columnCount() == 3 &&
                effect_waves->rowCount() == 1 &&
                effect_waves->columnCount() == 30 &&
                effect_waves->value(0, 19) == 5 &&
                transport->text(0, 0) == "Remote Town" &&
                transport->value(0, 1) == 0 &&
                transport->value(0, 2) == 50,
            "The retail table catalog or player table dimensions changed.")) {
        return 1;
    }
    if (!check(
            female->value(0, 0) == 100 &&
                female->value(1, 0) == 128 &&
                female->value(2, 0) == 140 &&
                female->value(3, 0) == 160 &&
                male->value(0, 0) == 100 &&
                male->value(1, 0) == 128 &&
                male->value(2, 0) == 150 &&
                male->value(3, 0) == 150,
            "The retail new-character parameters changed.")) {
        return 1;
    }
    if (!check(
            !male->contains(-1, 0) &&
                !male->contains(0, 5) &&
                male->value(99, 99) == 0 &&
                male->text(99, 99).empty(),
            "Out-of-range table access was not contained.")) {
        return 1;
    }

    std::vector<std::uint8_t> payload;
    appendI32(payload, 1);
    appendI32(payload, 42);
    appendI32(payload, 1);
    appendI32(payload, 2);
    appendI32(payload, 7);
    appendI32(payload, -3);
    appendI32(payload, 2);
    payload.push_back(static_cast<std::uint8_t>(~'H'));
    payload.push_back(static_cast<std::uint8_t>(~'i'));
    appendI32(payload, 0);

    const std::uint8_t header[16] = {
        'T', 'A', 'B', 'L', 'E', ' ', 'D', 'A',
        'T', 'A', ' ', 'V', '0', '0', '0', 0x1a,
    };
    std::vector<std::uint8_t> plain(
        header, header + sizeof(header));
    appendI32(plain, 0);
    appendI32(
        plain, static_cast<std::int32_t>(payload.size()));
    plain.insert(plain.end(), payload.begin(), payload.end());
    if (!check(
            tables.decode(plain.data(), plain.size(), &error) &&
                tables.find(42) &&
                tables.find(42)->value(0, 0) == 7 &&
                tables.find(42)->value(0, 1) == -3 &&
                tables.find(42)->text(0, 0) == "Hi",
            "An uncompressed table database was not decoded.")) {
        return 1;
    }

    const std::uint8_t invalid[] = {0, 1, 2, 3};
    if (!check(
            !tables.decode(invalid, sizeof(invalid), &error) &&
                tables.tables().empty(),
            "An invalid table database was accepted.")) {
        return 1;
    }
    return 0;
}
