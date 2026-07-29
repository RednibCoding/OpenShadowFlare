#include "world_scene.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <utility>

namespace osf {
namespace {


void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

bool endsWithIgnoreCase(
    const std::string& value,
    const std::string& suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }
    return std::equal(
        suffix.rbegin(),
        suffix.rend(),
        value.rbegin(),
        [](char left, char right) {
            return std::tolower(
                       static_cast<unsigned char>(left)) ==
                   std::tolower(
                       static_cast<unsigned char>(right));
        });
}

std::vector<std::string> readPatternList(
    const std::filesystem::path& path) {
    std::ifstream file(path);
    std::vector<std::string> result;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        result.push_back(std::move(line));
    }
    return result;
}

std::string mapStem(const std::string& map_path) {
    std::string normalized = map_path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return std::filesystem::path(normalized).stem().string();
}

}  // namespace

bool WorldScene::loadInitialScenario(
    const std::filesystem::path& data_root,
    const PlayerLoadRequest& player_request,
    std::string* error) {
    clear();

    if (!parameter_tables_.load(
            data_root / "System" / "Game" / "Parameter" /
                "Table.Tbd",
            error) ||
        !missions_.load(parameter_tables_, error) ||
        !player_data_.load(
            player_request, parameter_tables_, error)) {
        clear();
        return false;
    }
    if (!scenario_.load(
            data_root / "Scenario" / "00000000" / "Scenario.Mct",
            error)) {
        return false;
    }
    if (!scenario_script_.load(
            data_root / "Scenario" / "00000000" / "Scenario.Scs",
            error)) {
        clear();
        return false;
    }
    if (!item_database_.load(
            data_root / "System" / "Game" / "Parameter" /
                "Item.Ibn",
            error)) {
        clear();
        return false;
    }
    if (!item_inventory_patterns_.load(
            data_root,
            error)) {
        clear();
        return false;
    }
    data_root_ = data_root;
    const std::string map_name = mapStem(scenario_.mapPath());
    if (map_name.empty()) {
        setError(error, "The scenario does not name a map.");
        clear();
        return false;
    }
    const ScenarioEntry* entry = scenario_.findEntry(0);
    if (!entry) {
        setError(
            error,
            "The scenario does not contain entry point 0.");
        clear();
        return false;
    }

    const std::filesystem::path map_root = data_root / "Map";
    if (!ground_.load(
            map_root / "Ground" / (map_name + ".Gnd"), error)) {
        clear();
        return false;
    }
    if (!object_map_.load(
            map_root / "Object" / (map_name + ".Obl"), error)) {
        clear();
        return false;
    }
    if (!speech_patterns_.load(
            data_root / "System" / "Game" / "Pattern" /
                "Hukidasi.njp",
            error)) {
        clear();
        return false;
    }
    if (!map_overview_patterns_.load(
            data_root / "Scenario" / "00000000" /
                "Scenario.Njp",
            error) ||
        !map_exploration_.initialize(ground_)) {
        setError(
            error,
            "The scenario map could not be prepared.");
        clear();
        return false;
    }

    const std::vector<std::string> pattern_names =
        readPatternList(
            map_root / "Pattern" / (map_name + ".Lst"));
    if (pattern_names.empty()) {
        setError(error, "The map pattern list could not be read.");
        clear();
        return false;
    }
    map_patterns_.resize(pattern_names.size());
    for (std::size_t index = 0;
         index < pattern_names.size();
         ++index) {
        if (!endsWithIgnoreCase(pattern_names[index], ".njp") &&
            !endsWithIgnoreCase(pattern_names[index], ".sdw")) {
            continue;
        }
        auto image = std::make_unique<gapi::NjpImage>();
        std::string image_error;
        if (!image->load(
                map_root / "Pattern" / pattern_names[index],
                &image_error)) {
            setError(
                error,
                "A map pattern could not be loaded: " +
                    pattern_names[index] + " (" +
                    image_error + ")");
            clear();
            return false;
        }
        map_patterns_[index] = std::move(image);
    }

    const char* player_directory =
        player_data_.gender() == 1 ? "Female" : "Male";
    const std::filesystem::path player_root =
        data_root / "Player" / player_directory;
    std::string player_error;
    if (!player_visual_.load(
            player_root, "Animation00", &player_error)) {
        setError(
            error,
            "The player animation could not be loaded: " +
                player_error);
        clear();
        return false;
    }

    refreshPlayerAppearance();

    npcs_.reserve(scenario_.people().size());
    for (const ScenarioPerson& person : scenario_.people()) {
        NpcActor npc;
        std::string npc_error;
        const CharacterVisualResource* visual =
            people_visuals_.load(
                data_root,
                person.resource_id,
                &npc_error);
        if (!visual ||
            !npc.initialize(
                person, *visual, &npc_error)) {
            setError(
                error,
                "Scenario person " + std::to_string(person.id) +
                    " could not be loaded: " + npc_error);
            clear();
            return false;
        }
        npcs_.push_back(std::move(npc));
    }

    player_.reset(
        {entry->world_x, entry->world_y},
        entry->direction,
        player_data_.walkingSpeedTier());
    map_exploration_.reveal(player_.position());
    music_track_ = scenario_.musicTrack();
    has_player_ = true;
    return true;
}


}  // namespace osf
