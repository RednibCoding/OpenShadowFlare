#include "scenario_world.hpp"

#include "resources/resource_memory.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>
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

std::filesystem::path scenarioDirectory(
    const std::filesystem::path& data_root,
    std::int32_t scenario_id) {
    char directory[16]{};
    std::snprintf(
        directory,
        sizeof(directory),
        "%08d",
        scenario_id);
    return data_root / "Scenario" / directory;
}

bool validStart(const ScenarioStart& start) {
    return start.scenario_id >= 0 &&
           start.entry_value >= 0 &&
           start.local_player_number >= 0 &&
           start.local_player_number <= 3 &&
           start.entry_value <=
               (std::numeric_limits<std::int32_t>::max() -
                start.local_player_number) /
                   4;
}

}  // namespace

bool ScenarioWorld::load(
    const std::filesystem::path& data_root,
    const ScenarioStart& start,
    const AiControlDatabase& ai_control,
    RetailRandom& item_random,
    std::string* error) {
    clear();
    if (error) {
        error->clear();
    }
    if (!validStart(start)) {
        setError(error, "The scenario start request is invalid.");
        return false;
    }
    const std::filesystem::path scenario_root =
        scenarioDirectory(data_root, start.scenario_id);
    if (!data_.load(
            scenario_root / "Scenario.Mct",
            error) ||
        !script_data_.load(
            scenario_root / "Scenario.Scs",
            error)) {
        clear();
        return false;
    }

    const std::int32_t entry_key =
        start.local_player_number +
        start.entry_value * 4;
    const ScenarioEntry* selected_entry =
        data_.findEntry(entry_key);
    if (!selected_entry) {
        setError(
            error,
            "The scenario does not contain entry key " +
                std::to_string(entry_key) + ".");
        clear();
        return false;
    }

    const std::string map_name = mapStem(data_.mapPath());
    if (map_name.empty()) {
        setError(error, "The scenario does not name a map.");
        clear();
        return false;
    }
    const std::filesystem::path map_root = data_root / "Map";
    if (!ground_.load(
            map_root / "Ground" / (map_name + ".Gnd"),
            error) ||
        !object_map_.load(
            map_root / "Object" / (map_name + ".Obl"),
            error) ||
        !map_overview_patterns_.load(
            scenario_root / "Scenario.Njp",
            error) ||
        !map_exploration_.initialize(ground_)) {
        if (error && error->empty()) {
            *error = "The scenario map could not be prepared.";
        }
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

    for (std::int32_t resource_id :
         data_.objectResourceIds()) {
        std::string object_resource_error;
        if (!object_visuals_.load(
                data_root,
                resource_id,
                &object_resource_error)) {
            setError(
                error,
                "Scenario object resource " +
                    std::to_string(resource_id) +
                    " could not be loaded: " +
                    object_resource_error);
            clear();
            return false;
        }
    }
    objects_.reserve(data_.objects().size());
    for (const ScenarioObject& object : data_.objects()) {
        ScenarioObjectActor actor;
        std::string object_error;
        const ObjectVisualResource* visual =
            object.resource_id < 0
                ? nullptr
                : object_visuals_.find(object.resource_id);
        if (!actor.initialize(
                object, visual, &object_error)) {
            setError(
                error,
                "Scenario object " +
                    std::to_string(object.id) +
                    " could not be loaded: " +
                    object_error);
            clear();
            return false;
        }
        objects_.push_back(std::move(actor));
    }

    people_.reserve(data_.people().size());
    for (const ScenarioPerson& person : data_.people()) {
        NpcActor actor;
        std::string actor_error;
        const CharacterVisualResource* visual =
            people_visuals_.load(
                data_root,
                person.resource_id,
                &actor_error);
        if (!visual ||
            !actor.initialize(
                person, *visual, &actor_error)) {
            setError(
                error,
                "Scenario person " +
                    std::to_string(person.id) +
                    " could not be loaded: " +
                    actor_error);
            clear();
            return false;
        }
        people_.push_back(std::move(actor));
    }

    enemies_.reserve(data_.enemies().size());
    for (const ScenarioEnemy& enemy :
         data_.enemies()) {
        EnemyActor actor;
        std::string actor_error;
        const CharacterVisualResource* visual =
            enemy.resource_id < 0
                ? nullptr
                : enemy_visuals_.load(
                      data_root,
                      enemy.resource_id,
                      &actor_error);
        const AiControlList* control =
            ai_control.find(enemy.ai_control_name);
        const std::int32_t control_index =
            ai_control.indexOf(control);
        if (!control) {
            actor_error =
                "The AI-control list could not be resolved.";
        }
        if ((enemy.resource_id >= 0 && !visual) ||
            !control ||
            !actor.initialize(
                enemy,
                visual,
                *control,
                control_index,
                &actor_error)) {
            setError(
                error,
                "Scenario enemy " +
                    std::to_string(enemy.id) +
                    " could not be loaded: " +
                    actor_error);
            clear();
            return false;
        }
        const std::size_t index = enemies_.size();
        if (!enemy_indices_.emplace(
                actor.characterNumber(), index).second) {
            setError(
                error,
                "The scenario contains a duplicate enemy character "
                "number.");
            clear();
            return false;
        }
        enemies_.push_back(std::move(actor));
    }

    ground_items_.reserve(data_.items().size());
    for (const ScenarioItem& item : data_.items()) {
        if (!createScenarioGroundItem(
                ground_items_, item_random, item)) {
            setError(
                error,
                "Scenario item " +
                    std::to_string(item.id) +
                    " could not be initialized.");
            clear();
            return false;
        }
    }

    id_ = start.scenario_id;
    music_track_ = data_.musicTrack();
    local_player_number_ =
        start.local_player_number;
    entry_value_ = start.entry_value;
    entry_ = *selected_entry;
    map_exploration_.reveal(
        {entry_.world_x, entry_.world_y});
    if (error) {
        error->clear();
    }
    return true;
}

void ScenarioWorld::clear() {
    id_ = -1;
    music_track_ = -1;
    local_player_number_ = 0;
    entry_value_ = 0;
    entry_ = {};
    data_.clear();
    script_data_.clear();
    ground_.clear();
    object_map_.clear();
    map_patterns_.clear();
    object_visuals_.clear();
    people_visuals_.clear();
    enemy_visuals_.clear();
    map_overview_patterns_.clear();
    map_exploration_.clear();
    objects_.clear();
    people_.clear();
    enemies_.clear();
    enemy_indices_.clear();
    ground_items_.clear();
}

std::uint64_t ScenarioWorld::resourceMemoryUsageBytes() const {
    std::uint64_t bytes = ground_.memoryUsageBytes() +
        object_map_.memoryUsageBytes() +
        decodedMemoryUsageBytes(map_overview_patterns_) +
        decodedMemoryUsageBytes(map_exploration_.mask()) +
        object_visuals_.memoryUsageBytes() +
        people_visuals_.memoryUsageBytes() +
        enemy_visuals_.memoryUsageBytes();
    for (const auto& patterns : map_patterns_) {
        if (patterns) {
            bytes += decodedMemoryUsageBytes(*patterns);
        }
    }
    return bytes;
}

std::int32_t ScenarioWorld::id() const {
    return id_;
}

std::int32_t ScenarioWorld::musicTrack() const {
    return music_track_;
}

std::int32_t ScenarioWorld::localPlayerNumber() const {
    return local_player_number_;
}

std::int32_t ScenarioWorld::entryValue() const {
    return entry_value_;
}

const ScenarioEntry& ScenarioWorld::entry() const {
    return entry_;
}

void ScenarioWorld::setEntry(
    std::int32_t entry_value,
    const ScenarioEntry& entry) {
    entry_value_ = entry_value;
    entry_ = entry;
}

const ScenarioData& ScenarioWorld::data() const {
    return data_;
}

ScenarioData& ScenarioWorld::data() {
    return data_;
}

script::ScriptData ScenarioWorld::takeScriptData() {
    return std::move(script_data_);
}

const GroundMap& ScenarioWorld::ground() const {
    return ground_;
}

GroundMap& ScenarioWorld::ground() {
    return ground_;
}

const ObjectMap& ScenarioWorld::objectMap() const {
    return object_map_;
}

ObjectMap& ScenarioWorld::objectMap() {
    return object_map_;
}

const std::vector<std::unique_ptr<gapi::NjpImage>>&
ScenarioWorld::mapPatterns() const {
    return map_patterns_;
}

const gapi::NjpImage&
ScenarioWorld::mapOverviewPatterns() const {
    return map_overview_patterns_;
}

MapExploration& ScenarioWorld::mapExploration() {
    return map_exploration_;
}

const MapExploration&
ScenarioWorld::mapExploration() const {
    return map_exploration_;
}

std::vector<ScenarioObjectActor>&
ScenarioWorld::objects() {
    return objects_;
}

const std::vector<ScenarioObjectActor>&
ScenarioWorld::objects() const {
    return objects_;
}

std::vector<NpcActor>& ScenarioWorld::people() {
    return people_;
}

const std::vector<NpcActor>&
ScenarioWorld::people() const {
    return people_;
}

std::vector<EnemyActor>& ScenarioWorld::enemies() {
    return enemies_;
}

const std::vector<EnemyActor>&
ScenarioWorld::enemies() const {
    return enemies_;
}

EnemyActor* ScenarioWorld::findEnemyByCharacterNumber(
    std::int32_t character_number) {
    const auto found = enemy_indices_.find(character_number);
    return found == enemy_indices_.end()
               ? nullptr
               : &enemies_[found->second];
}

const EnemyActor* ScenarioWorld::findEnemyByCharacterNumber(
    std::int32_t character_number) const {
    const auto found = enemy_indices_.find(character_number);
    return found == enemy_indices_.end()
               ? nullptr
               : &enemies_[found->second];
}

std::vector<GroundItem>& ScenarioWorld::groundItems() {
    return ground_items_;
}

const std::vector<GroundItem>&
ScenarioWorld::groundItems() const {
    return ground_items_;
}

}  // namespace osf
