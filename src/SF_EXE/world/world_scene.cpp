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
    std::int32_t character_gender,
    std::string* error) {
    clear();

    if (!scenario_.load(
            data_root / "Scenario" / "00000000" / "Scenario.Mct",
            error)) {
        return false;
    }
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
        character_gender == 0 ? "Male" : "Female";
    const std::filesystem::path player_root =
        data_root / "Player" / player_directory;
    std::string player_error;
    if (!player_patterns_.load(
            player_root / "Animation00.Njp",
            &player_error) ||
        !player_shadow_patterns_.load(
            player_root / "Animation00.Sdw",
            &player_error) ||
        !player_animation_.load(
            player_root / "Animation00.Caf",
            &player_error)) {
        setError(
            error,
            "The player animation could not be loaded: " +
                player_error);
        clear();
        return false;
    }

    // The retail appearance refresh at 0x00444ca0 clears this table,
    // enables the base body and shadow, then enables only parts supplied by
    // equipped items. A newly created character has no equipped item parts.
    player_parts_enabled_.assign(
        player_animation_.maxPartCount(), 0);
    if (!player_parts_enabled_.empty()) {
        player_parts_enabled_[0] = 1;
    }
    if (player_parts_enabled_.size() > 1) {
        player_parts_enabled_[1] = 1;
    }

    // The initial values come from scenario 00000000 and the new-character
    // table path. FUN_00450d40 turns both gender tables' agility value 128
    // into tier five; FUN_00450080 then converts that to 20 world units per
    // update.
    player_.reset(
        {entry->world_x, entry->world_y},
        entry->direction,
        5);
    music_track_ = scenario_.musicTrack();
    has_player_ = true;
    return true;
}

void WorldScene::clear() {
    scenario_.clear();
    ground_.clear();
    object_map_.clear();
    map_patterns_.clear();
    player_patterns_.clear();
    player_shadow_patterns_.clear();
    player_animation_.clear();
    player_parts_enabled_.clear();
    player_.clear();
    has_player_ = false;
    music_track_ = -1;
}

const GroundMap& WorldScene::ground() const {
    return ground_;
}

const ObjectMap& WorldScene::objectMap() const {
    return object_map_;
}

const std::vector<std::unique_ptr<gapi::NjpImage>>&
WorldScene::mapPatterns() const {
    return map_patterns_;
}

const gapi::NjpImage& WorldScene::playerPatterns() const {
    return player_patterns_;
}

const gapi::NjpImage& WorldScene::playerShadowPatterns() const {
    return player_shadow_patterns_;
}

const gapi::CafAnimation& WorldScene::playerAnimation() const {
    return player_animation_;
}

bool WorldScene::playerPartEnabled(std::size_t part) const {
    return part < player_parts_enabled_.size() &&
           player_parts_enabled_[part] != 0;
}

bool WorldScene::hasPlayer() const {
    return has_player_;
}

void WorldScene::commandPlayerMovement(
    std::int32_t screen_x,
    std::int32_t screen_y) {
    if (!has_player_) {
        return;
    }
    player_.moveTo(
        calculateWorldPosition({
            cameraScreenX() + screen_x,
            cameraScreenY() + screen_y,
        }));
}

void WorldScene::togglePlayerRun() {
    if (has_player_) {
        player_.toggleMovementPace();
    }
}

void WorldScene::update() {
    if (has_player_) {
        player_.update(ground_, object_map_);
    }
}

std::int32_t WorldScene::playerWorldX() const {
    return player_.position().x;
}

std::int32_t WorldScene::playerWorldY() const {
    return player_.position().y;
}

std::int32_t WorldScene::playerDirection() const {
    return player_.direction();
}

PlayerMotion WorldScene::playerMotion() const {
    return player_.motion();
}

std::int32_t WorldScene::playerAnimationChart() const {
    return player_.animationChart();
}

std::int32_t WorldScene::playerAnimationFrame() const {
    return player_.animationFrame();
}

std::int32_t WorldScene::cameraScreenX() const {
    const ScreenPosition position =
        calculateRealPosition(player_.position());
    return position.x - 320;
}

std::int32_t WorldScene::cameraScreenY() const {
    const ScreenPosition position =
        calculateRealPosition(player_.position());
    return position.y - 240;
}

std::int32_t WorldScene::musicTrack() const {
    return music_track_;
}

const ScenarioData& WorldScene::scenario() const {
    return scenario_;
}

}  // namespace osf
