#include "mission_catalog.hpp"

#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kMissionTitleTable = 41;
constexpr std::int32_t kFirstMissionDescriptionTable = 700;

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

}  // namespace

bool MissionCatalog::load(
    const TableDatabase& tables,
    std::string* error) {
    clear();

    const TableData* titles = tables.find(kMissionTitleTable);
    if (!titles || titles->columnCount() < 1) {
        setError(error, "The mission-title table is missing.");
        return false;
    }

    std::vector<MissionDefinition> parsed;
    parsed.reserve(static_cast<std::size_t>(titles->rowCount()));
    for (std::int32_t mission_id = 0;
         mission_id < titles->rowCount();
         ++mission_id) {
        MissionDefinition mission;
        mission.id = mission_id;
        mission.title =
            std::string(titles->text(mission_id, 0));

        const TableData* description = tables.find(
            kFirstMissionDescriptionTable + mission_id);
        if (description && description->columnCount() > 0) {
            mission.description.reserve(
                static_cast<std::size_t>(
                    description->rowCount()));
            for (std::int32_t row = 0;
                 row < description->rowCount();
                 ++row) {
                mission.description.emplace_back(
                    description->text(row, 0));
            }
        }
        parsed.push_back(std::move(mission));
    }
    missions_ = std::move(parsed);
    return true;
}

void MissionCatalog::clear() {
    missions_.clear();
}

const MissionDefinition* MissionCatalog::find(
    std::int32_t mission_id) const {
    if (mission_id < 0 ||
        static_cast<std::size_t>(mission_id) >=
            missions_.size()) {
        return nullptr;
    }
    const MissionDefinition& mission =
        missions_[static_cast<std::size_t>(mission_id)];
    return mission.id == mission_id ? &mission : nullptr;
}

const std::vector<MissionDefinition>&
MissionCatalog::missions() const {
    return missions_;
}

}  // namespace osf
