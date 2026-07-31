#include "companion_profile.hpp"

#include "core/retail_integer.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <array>
#include <cstddef>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kCompanionCatalogTable = 60;
constexpr std::int32_t kFirstCompanionParameterTable = 800;
constexpr std::int32_t kCompanionParameterRows = 19;

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

}  // namespace

bool decodeCompanionProfile(
    const TableDatabase& tables,
    std::int32_t companion_type,
    std::int32_t companion_level,
    CompanionProfile& profile,
    std::string* error) {
    profile = {};
    const TableData* catalog =
        tables.find(kCompanionCatalogTable);
    const TableData* parameters =
        tables.find(
            kFirstCompanionParameterTable +
            companion_type);
    if (!catalog ||
        !catalog->contains(companion_type, 6) ||
        !parameters ||
        parameters->rowCount() <
            kCompanionParameterRows ||
        companion_level < 1 ||
        companion_level > parameters->columnCount()) {
        setError(
            error,
            "The selected companion profile is not present in "
            "the retail parameter tables.");
        return false;
    }

    std::array<std::int32_t, kCompanionParameterRows>
        values{};
    for (std::int32_t column = 0;
         column < companion_level;
         ++column) {
        for (std::int32_t row = 0;
             row < kCompanionParameterRows;
             ++row) {
            std::int32_t& value =
                values[static_cast<std::size_t>(row)];
            value = retailAdd(
                value, parameters->value(row, column));
        }
    }

    profile.type = companion_type;
    profile.level = companion_level;
    profile.name =
        std::string(catalog->text(companion_type, 0));
    profile.resource_id =
        catalog->value(companion_type, 1);
    profile.red_strength =
        catalog->value(companion_type, 2);
    profile.green_strength =
        catalog->value(companion_type, 3);
    profile.blue_strength =
        catalog->value(companion_type, 4);
    profile.attack_speed_rating = values[0];
    profile.native_element = values[13];
    profile.walking_speed = values[1] / 5;
    profile.running_speed = values[2] / 5;
    profile.maximum_life = values[3];
    profile.physical_attack = values[5];
    profile.hit_rate = values[6];
    profile.physical_defense = values[7];
    profile.physical_evasion = values[8];
    profile.magical_attack = values[9];
    profile.magical_hit_rate = values[10];
    profile.magical_defense = values[11];
    profile.magical_evasion = values[12];
    profile.parameter_17 = values[17];
    profile.experience_threshold = values[18];
    if (profile.name.empty() ||
        profile.resource_id < 0 ||
        profile.maximum_life < 1) {
        setError(
            error,
            "The selected companion profile contains invalid "
            "retail values.");
        profile = {};
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace osf
