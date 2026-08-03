#include "companion_status_message.hpp"

#include "companion_profile.hpp"
#include "player_data.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <utility>

namespace osf {
namespace {

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

}  // namespace

bool buildRetailCompanionStatusMessage(
    const TableDatabase& tables,
    const PlayerData& player,
    std::int32_t companion_type,
    std::string& message,
    std::string* error) {
    message.clear();
    if (!player.valid() ||
        companion_type < 0 ||
        static_cast<std::size_t>(companion_type) >=
            player.companionCount()) {
        setError(
            error,
            "The companion status request has no saved progression row.");
        return false;
    }

    CompanionProfile profile;
    if (!decodeCompanionProfile(
            tables,
            companion_type,
            player.companionLevel(companion_type),
            profile,
            error)) {
        return false;
    }

    static constexpr std::array<const char*, 8> attributes{{
        "Fire", "Water", "Earth", "Thunder",
        "Holy", "Dark", "Gel", "Metal",
    }};
    if (profile.native_element < 0 ||
        static_cast<std::size_t>(profile.native_element) >=
            attributes.size()) {
        setError(
            error,
            "The companion status request has an invalid native element.");
        return false;
    }

    // Opcode 3 labels these two fields as magical defense/evasion, but the
    // retail instructions actually read magical hit rate and physical
    // defense. Preserve the displayed values, including that original bug.
    std::ostringstream output;
    output << profile.name << "\n\n"
           << "Level          " << std::setw(5)
           << player.companionLevel() << '\n'
           << "HP             " << std::setw(5)
           << profile.maximum_life << '\n'
           << "Attribute      " << std::setw(5)
           << attributes[static_cast<std::size_t>(
                  profile.native_element)]
           << '\n'
           << "Attack         " << std::setw(5)
           << profile.physical_attack
           << "  Defense        " << std::setw(5)
           << profile.physical_defense << '\n'
           << "Hit Rate       " << std::setw(5)
           << profile.hit_rate
           << "  Evasion Rate   " << std::setw(5)
           << profile.physical_evasion << '\n'
           << "M Defense      " << std::setw(5)
           << profile.magical_hit_rate
           << "  M Evasion Rate " << std::setw(5)
           << profile.physical_defense << '\n'
           << "Attack Speed   " << std::setw(5)
           << profile.attack_speed_rating
           << "  Walking Speed  " << std::setw(5)
           << profile.walking_speed_raw << '\n';

    const std::int32_t level_limit = std::min<std::int32_t>(
        player.level() / 3 + 2, 35);
    if (player.companionLevel() == level_limit) {
        output << (level_limit == 35
                       ? "Experience   Max\n"
                       : "Experience Limit\n");
    } else {
        output << "Experience     " << std::setw(5)
               << player.companionExperience() << '\n';
    }

    message = output.str();
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace osf
