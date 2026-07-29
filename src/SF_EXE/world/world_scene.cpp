#include "world_scene.hpp"

#include <algorithm>
#include <cctype>
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

std::uint64_t scriptValueKey(
    const script::Operand& operand) {
    return
        (static_cast<std::uint64_t>(
             static_cast<std::uint32_t>(operand.type))
         << 32u) |
        static_cast<std::uint32_t>(operand.value);
}

}  // namespace

WorldScene::WorldScene()
    : script_interpreter_({
          [this](const script::Operand& operand) {
              return readScriptOperand(operand);
          },
          [this](
              const script::Operand& operand,
              std::int32_t value) {
              return writeScriptOperand(operand, value);
          },
          [this](const script::MessageEvent& message) {
              showScriptMessage(message);
          },
          [this](
              std::int32_t opcode,
              const std::vector<std::int32_t>& arguments) {
              return executeScriptNativeCommand(
                  opcode, arguments);
          },
          [this](
              script::ValueQuery query,
              std::int32_t& value) {
              return queryScriptValue(query, value);
          },
      }) {}

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
    if (!scenario_script_.load(
            data_root / "Scenario" / "00000000" / "Scenario.Scs",
            error)) {
        clear();
        return false;
    }
    script_interpreter_.bind(&scenario_script_);
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

    // This first people slice deliberately brings up one proven record before
    // generalizing actor AI and interaction to the complete group.
    if (!scenario_.people().empty()) {
        NpcActor npc;
        std::string npc_error;
        if (!npc.load(
                data_root,
                scenario_.people().front(),
                &npc_error)) {
            setError(
                error,
                "The first scenario NPC could not be loaded: " +
                    npc_error);
            clear();
            return false;
        }
        npcs_.push_back(std::move(npc));
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
    script_interpreter_.bind(nullptr);
    scenario_script_.clear();
    script_values_.clear();
    conversation_ = {};
    conversation_active_ = false;
    conversation_actor_id_ = -1;
    hovered_npc_id_ = -1;
    ground_.clear();
    object_map_.clear();
    map_patterns_.clear();
    player_patterns_.clear();
    player_shadow_patterns_.clear();
    player_animation_.clear();
    speech_patterns_.clear();
    player_parts_enabled_.clear();
    npcs_.clear();
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

const std::vector<NpcActor>& WorldScene::npcs() const {
    return npcs_;
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

void WorldScene::updatePointerHover(
    std::int32_t screen_x,
    std::int32_t screen_y) {
    if (!has_player_ || conversation_active_) {
        hovered_npc_id_ = -1;
        return;
    }
    const std::int32_t index =
        npcIndexAtScreenPosition(screen_x, screen_y);
    hovered_npc_id_ = index < 0
        ? -1
        : npcs_[static_cast<std::size_t>(index)].id();
}

bool WorldScene::commandWorldInteraction(
    std::int32_t screen_x,
    std::int32_t screen_y) {
    if (!has_player_ || conversation_active_) {
        return false;
    }
    const std::int32_t index =
        npcIndexAtScreenPosition(screen_x, screen_y);
    if (index < 0) {
        return false;
    }
    NpcActor& selected =
        npcs_[static_cast<std::size_t>(index)];

    player_.cancelMovement();
    const std::int32_t script_character_number =
        12000000 + selected.id();
    const script::StepResult result =
        script_interpreter_.startStatus(
            0, script_character_number);
    if (result == script::StepResult::waiting_for_message ||
        result == script::StepResult::complete) {
        hovered_npc_id_ = -1;
    }
    return result == script::StepResult::waiting_for_message ||
           result == script::StepResult::complete;
}

std::int32_t WorldScene::hoveredNpcId() const {
    return hovered_npc_id_;
}

bool WorldScene::conversationActive() const {
    return conversation_active_;
}

std::int32_t WorldScene::conversationActorId() const {
    return conversation_actor_id_;
}

std::int32_t WorldScene::conversationMessageId() const {
    return conversation_.id;
}

const std::string& WorldScene::conversationText() const {
    return conversation_.text;
}

const gapi::NjpImage& WorldScene::speechPatterns() const {
    return speech_patterns_;
}

void WorldScene::advanceConversation() {
    if (!conversation_active_) {
        return;
    }
    conversation_active_ = false;
    conversation_ = {};
    const script::StepResult result =
        script_interpreter_.resume();
    if (result != script::StepResult::waiting_for_message) {
        conversation_active_ = false;
        conversation_actor_id_ = -1;
        for (NpcActor& npc : npcs_) {
            npc.endInteraction();
        }
    }
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
    for (NpcActor& npc : npcs_) {
        npc.update(ground_, object_map_);
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

const script::ScriptData& WorldScene::scenarioScript() const {
    return scenario_script_;
}

std::int32_t WorldScene::readScriptOperand(
    const script::Operand& operand) const {
    const auto found =
        script_values_.find(scriptValueKey(operand));
    return found == script_values_.end() ? 0 : found->second;
}

bool WorldScene::writeScriptOperand(
    const script::Operand& operand,
    std::int32_t value) {
    script_values_.insert_or_assign(
        scriptValueKey(operand), value);
    return true;
}

bool WorldScene::executeScriptNativeCommand(
    std::int32_t opcode,
    const std::vector<std::int32_t>& arguments) {
    if ((opcode != 18 && opcode != 21) ||
        arguments.empty()) {
        return false;
    }
    const std::int32_t character_number =
        arguments.front();
    const auto found = std::find_if(
        npcs_.begin(),
        npcs_.end(),
        [character_number](const NpcActor& npc) {
            return 12000000 + npc.id() ==
                   character_number;
        });
    if (found == npcs_.end()) {
        return false;
    }
    found->beginInteraction(player_.position());
    conversation_actor_id_ = found->id();
    hovered_npc_id_ = -1;
    return true;
}

bool WorldScene::queryScriptValue(
    script::ValueQuery query,
    std::int32_t& value) const {
    if (!has_player_) {
        return false;
    }
    switch (query) {
    case script::ValueQuery::local_player_level:
        value = player_.level();
        return true;
    }
    return false;
}

void WorldScene::showScriptMessage(
    const script::MessageEvent& message) {
    conversation_ = message;
    conversation_active_ = true;
}

std::int32_t WorldScene::npcIndexAtScreenPosition(
    std::int32_t screen_x,
    std::int32_t screen_y) const {
    const std::int32_t camera_x = cameraScreenX();
    const std::int32_t camera_y = cameraScreenY();
    std::int32_t selected = -1;
    std::int32_t selected_distance =
        std::numeric_limits<std::int32_t>::max();
    for (std::size_t index = 0; index < npcs_.size(); ++index) {
        const NpcActor& npc = npcs_[index];
        const ScreenPosition anchor =
            calculateRealPosition(npc.position());
        const std::int32_t relative_x =
            screen_x - (anchor.x - camera_x);
        const std::int32_t relative_y =
            screen_y - (anchor.y - camera_y);
        if (relative_x < -32 || relative_x > 32 ||
            relative_y < -npc.labelHeight() ||
            relative_y > 12) {
            continue;
        }
        const std::int32_t distance =
            relative_x * relative_x +
            relative_y * relative_y;
        if (distance < selected_distance) {
            selected = static_cast<std::int32_t>(index);
            selected_distance = distance;
        }
    }
    return selected;
}

}  // namespace osf
