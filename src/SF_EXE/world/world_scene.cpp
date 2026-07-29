#include "world_scene.hpp"
#include "libs/RKC_RPGSCRN/display_hit_test.hpp"
#include "movement_controller.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kRetailInteractionDistance = 0x9f;
constexpr std::int32_t kRetailHeightScale = 20;

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

WorldScene::WorldScene()
    : scenario_script_({
          [this](
              const script::Operand& operand,
              std::int32_t& value) {
              return readScriptWorldOperand(operand, value);
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

    // The retail appearance refresh at 0x00444ca0 clears this table,
    // enables the base body and shadow, then enables only parts supplied by
    // equipped items. A newly created character has no equipped item parts.
    player_parts_enabled_.assign(
        player_visual_.animation().maxPartCount(), 0);
    if (!player_parts_enabled_.empty()) {
        player_parts_enabled_[0] = 1;
    }
    if (player_parts_enabled_.size() > 1) {
        player_parts_enabled_[1] = 1;
    }

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

void WorldScene::clear() {
    scenario_.clear();
    scenario_script_.clear();
    conversation_ = {};
    conversation_active_ = false;
    conversation_actor_id_ = -1;
    conversation_selected_option_ = -1;
    pointer_.reset();
    pending_interaction_ = {};
    ground_.clear();
    object_map_.clear();
    map_patterns_.clear();
    player_visual_.clear();
    people_visuals_.clear();
    speech_patterns_.clear();
    map_overview_patterns_.clear();
    map_exploration_.clear();
    player_parts_enabled_.clear();
    npcs_.clear();
    ground_items_.clear();
    quests_.clear();
    missions_.clear();
    item_database_.clear();
    player_inventory_.clear();
    item_inventory_patterns_.clear();
    parameter_tables_.clear();
    data_root_.clear();
    item_world_resources_.clear();
    item_random_.seed(1);
    player_data_.clear();
    player_.clear();
    has_player_ = false;
    music_track_ = -1;
    next_ground_item_id_ = 0;
    camera_anchor_x_ = 320;
    camera_anchor_y_ = 240;
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
    return player_visual_.patterns();
}

const gapi::NjpImage& WorldScene::playerShadowPatterns() const {
    return player_visual_.shadowPatterns();
}

const gapi::CafAnimation& WorldScene::playerAnimation() const {
    return player_visual_.animation();
}

const std::vector<NpcActor>& WorldScene::npcs() const {
    return npcs_;
}

const std::vector<GroundItem>& WorldScene::groundItems() const {
    return ground_items_;
}

const QuestState& WorldScene::quests() const {
    return quests_;
}

const MissionCatalog& WorldScene::missions() const {
    return missions_;
}

const ItemDatabase& WorldScene::itemDatabase() const {
    return item_database_;
}

PlayerInventory& WorldScene::playerInventory() {
    return player_inventory_;
}

const PlayerInventory& WorldScene::playerInventory() const {
    return player_inventory_;
}

const ItemInventoryResource&
WorldScene::itemInventoryPatterns() const {
    return item_inventory_patterns_;
}

const PlayerData& WorldScene::playerData() const {
    return player_data_;
}

const ItemWorldResource* WorldScene::itemWorldResource(
    std::int32_t resource_id) const {
    if (resource_id < 0 ||
        static_cast<std::size_t>(resource_id) >=
            item_world_resources_.size()) {
        return nullptr;
    }
    return item_world_resources_[
        static_cast<std::size_t>(resource_id)].get();
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
    pending_interaction_ = {};
    player_.moveTo(
        calculateWorldPosition({
            cameraScreenX() + screen_x,
            cameraScreenY() + screen_y,
        }));
}

void WorldScene::cancelPlayerMovement() {
    pending_interaction_ = {};
    player_.cancelMovement();
}

void WorldScene::updatePointerHover(
    std::int32_t screen_x,
    std::int32_t screen_y) {
    if (!has_player_ || conversation_active_) {
        pointer_.clearSelection();
        return;
    }
    pointer_.update(
        screen_x,
        screen_y,
        pointerCandidatesAtScreenPosition(
            screen_x, screen_y));
}

void WorldScene::clearPointerHover() {
    pointer_.clearSelection();
}

void WorldScene::configurePointer(
    const WorldPointerConfiguration& configuration) {
    pointer_.configure(configuration);
}

bool WorldScene::commandWorldInteraction(
    std::int32_t screen_x,
    std::int32_t screen_y) {
    if (!has_player_ || conversation_active_) {
        return false;
    }
    const WorldPointerTarget target =
        pointerTargetAtScreenPosition(screen_x, screen_y);
    if (target.kind == WorldPointerTargetKind::none) {
        return false;
    }
    if (target.kind ==
        WorldPointerTargetKind::ground_item) {
        pending_interaction_ = target;
        GroundItem* item = findGroundItem(target.id);
        if (!item) {
            pending_interaction_ = {};
            return false;
        }
        if (distanceBetweenBounds(
                player_.position(),
                player_.judgement(),
                item->position,
                {}) > kRetailInteractionDistance) {
            player_.followTo(item->position);
            return true;
        }
        return startGroundItemInteraction(item->id);
    }

    NpcActor* selected = findNpc(target.id);
    if (!selected) {
        return false;
    }
    pending_interaction_ = target;
    if (distanceBetweenBounds(
            player_.position(),
            player_.judgement(),
            selected->position(),
            selected->judgement()) >
            kRetailInteractionDistance) {
        player_.followTo(selected->position());
        return true;
    }
    return startNpcInteraction(*selected);
}

bool WorldScene::dropInventoryItem(
    const InventoryItem& item,
    std::int32_t screen_x,
    std::int32_t screen_y) {
    if (!has_player_) {
        return false;
    }

    const ItemDefinition* definition =
        item_database_.find(
            item.category, item.definition_id);
    if (!definition ||
        !ensureItemWorldResource(
            definition->ground_resource_id)) {
        return false;
    }

    const WorldPosition pointer_world =
        calculateWorldPosition({
            cameraScreenX() + screen_x,
            cameraScreenY() + screen_y,
        });
    const WorldPosition player_position =
        player_.position();
    const std::int32_t direction =
        retailDirectionForVector(
            pointer_world.x - player_position.x,
            pointer_world.y - player_position.y);

    WorldPosition drop_position = player_position;
    constexpr std::int32_t kRetailDropDistance = 200;
    switch (direction) {
    case 0:
        drop_position.x += kRetailDropDistance;
        drop_position.y += kRetailDropDistance;
        break;
    case 1:
        drop_position.x += kRetailDropDistance;
        break;
    case 2:
        drop_position.x += kRetailDropDistance;
        drop_position.y -= kRetailDropDistance;
        break;
    case 3:
        drop_position.y -= kRetailDropDistance;
        break;
    case 4:
        drop_position.x -= kRetailDropDistance;
        drop_position.y -= kRetailDropDistance;
        break;
    case 5:
        drop_position.x -= kRetailDropDistance;
        break;
    case 6:
        drop_position.x -= kRetailDropDistance;
        drop_position.y += kRetailDropDistance;
        break;
    case 7:
        drop_position.y += kRetailDropDistance;
        break;
    default:
        return false;
    }

    const std::size_t first_item =
        ground_items_.size();
    const std::int32_t first_id =
        next_ground_item_id_;
    if (!createGroundItem(
            ground_items_,
            item.category,
            item.definition_id,
            drop_position,
            item.quantity) ||
        !prepareGroundItems(first_item)) {
        ground_items_.resize(first_item);
        next_ground_item_id_ = first_id;
        return false;
    }
    pending_interaction_ = {};
    player_.cancelMovement();
    pointer_.clearSelection();
    return true;
}

bool WorldScene::interactionPending() const {
    return pending_interaction_.kind !=
        WorldPointerTargetKind::none;
}

bool WorldScene::startNpcInteraction(NpcActor& selected) {
    pending_interaction_ = {};
    player_.cancelMovement();
    player_.faceToward(selected.position());
    const std::int32_t script_character_number =
        12000000 + selected.id();
    const script::StepResult result =
        scenario_script_.startStatus(
            0, script_character_number);
    if (result == script::StepResult::waiting_for_message ||
        result == script::StepResult::complete) {
        pointer_.clearSelection();
    }
    return result == script::StepResult::waiting_for_message ||
           result == script::StepResult::complete;
}

std::int32_t WorldScene::hoveredNpcId() const {
    return pointer_.target().kind ==
                   WorldPointerTargetKind::npc
               ? pointer_.target().id
               : -1;
}

std::int32_t WorldScene::hoveredGroundItemId() const {
    return pointer_.target().kind ==
                   WorldPointerTargetKind::ground_item
               ? pointer_.target().id
               : -1;
}

std::int32_t WorldScene::pointerScreenX() const {
    return pointer_.screenX();
}

std::int32_t WorldScene::pointerScreenY() const {
    return pointer_.screenY();
}

bool WorldScene::pointerActive() const {
    return pointer_.active();
}

const WorldPointerConfiguration&
WorldScene::pointerConfiguration() const {
    return pointer_.configuration();
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

bool WorldScene::conversationRequiresSelection() const {
    return conversation_.selection_required;
}

std::int32_t WorldScene::conversationInitialSelection() const {
    return conversation_.initial_selection;
}

std::int32_t WorldScene::conversationSelectedOption() const {
    return conversation_selected_option_;
}

void WorldScene::selectConversationOption(
    std::int32_t option) {
    if (conversation_active_ &&
        conversation_.selection_required &&
        option >= 0) {
        conversation_selected_option_ = option;
    }
}

const gapi::NjpImage& WorldScene::speechPatterns() const {
    return speech_patterns_;
}

const gapi::NjpImage&
WorldScene::mapOverviewPatterns() const {
    return map_overview_patterns_;
}

const MapExploration& WorldScene::mapExploration() const {
    return map_exploration_;
}

void WorldScene::advanceConversation() {
    if (!conversation_active_) {
        return;
    }
    conversation_active_ = false;
    conversation_ = {};
    conversation_selected_option_ = -1;
    const script::StepResult result =
        scenario_script_.resume();
    if (result != script::StepResult::waiting_for_message) {
        conversation_active_ = false;
        conversation_actor_id_ = -1;
        for (NpcActor& npc : npcs_) {
            npc.endInteraction();
        }
    }
}

void WorldScene::chooseConversationOption(
    std::int32_t option) {
    if (!conversation_active_ ||
        !conversation_.selection_required ||
        option < 0) {
        return;
    }
    conversation_active_ = false;
    conversation_ = {};
    conversation_selected_option_ = -1;
    const script::StepResult result =
        scenario_script_.resume(option);
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
    NpcActor* interaction_npc = nullptr;
    GroundItem* interaction_item = nullptr;
    if (pending_interaction_.kind ==
        WorldPointerTargetKind::npc) {
        interaction_npc =
            findNpc(pending_interaction_.id);
        if (interaction_npc) {
            player_.followTo(interaction_npc->position());
        }
    } else if (
        pending_interaction_.kind ==
        WorldPointerTargetKind::ground_item) {
        interaction_item =
            findGroundItem(pending_interaction_.id);
        if (interaction_item) {
            player_.followTo(interaction_item->position);
        }
    }
    if (pending_interaction_.kind !=
            WorldPointerTargetKind::none &&
        !interaction_npc && !interaction_item) {
        pending_interaction_ = {};
    }
    std::vector<MovementBlocker> actor_blockers;
    actor_blockers.reserve(npcs_.size());
    for (const NpcActor& npc : npcs_) {
        actor_blockers.push_back({
            npc.id(),
            npc.position(),
            npc.judgement(),
        });
    }
    if (has_player_) {
        player_.update(
            ground_, object_map_, &actor_blockers);
        map_exploration_.reveal(player_.position());
    }
    for (NpcActor& npc : npcs_) {
        npc.update(ground_, object_map_);
    }
    for (GroundItem& item : ground_items_) {
        updateGroundItem(item);
    }
    interaction_npc =
        pending_interaction_.kind ==
                WorldPointerTargetKind::npc
            ? findNpc(pending_interaction_.id)
            : nullptr;
    if (interaction_npc &&
        distanceBetweenBounds(
            player_.position(),
            player_.judgement(),
            interaction_npc->position(),
            interaction_npc->judgement()) <=
            kRetailInteractionDistance) {
        startNpcInteraction(*interaction_npc);
        return;
    }
    interaction_item =
        pending_interaction_.kind ==
                WorldPointerTargetKind::ground_item
            ? findGroundItem(pending_interaction_.id)
            : nullptr;
    if (interaction_item &&
        distanceBetweenBounds(
            player_.position(),
            player_.judgement(),
            interaction_item->position,
            {}) <= kRetailInteractionDistance) {
        startGroundItemInteraction(interaction_item->id);
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

MovementPace WorldScene::playerMovementPace() const {
    return player_.movementPace();
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
    return position.x - camera_anchor_x_;
}

std::int32_t WorldScene::cameraScreenY() const {
    const ScreenPosition position =
        calculateRealPosition(player_.position());
    return position.y - camera_anchor_y_;
}

std::int32_t WorldScene::renderCameraScreenX(
    double alpha) const {
    const ScreenPosition position =
        calculateRealPosition(player_.renderPosition(alpha));
    return position.x - camera_anchor_x_;
}

std::int32_t WorldScene::renderCameraScreenY(
    double alpha) const {
    const ScreenPosition position =
        calculateRealPosition(player_.renderPosition(alpha));
    return position.y - camera_anchor_y_;
}

void WorldScene::setCameraAnchor(
    std::int32_t screen_x,
    std::int32_t screen_y) {
    camera_anchor_x_ = screen_x;
    camera_anchor_y_ = screen_y;
}

WorldPosition WorldScene::playerRenderPosition(
    double alpha) const {
    return player_.renderPosition(alpha);
}

const ObjectBounds& WorldScene::playerJudgement() const {
    return player_.judgement();
}

std::int32_t WorldScene::musicTrack() const {
    return music_track_;
}

const ScenarioData& WorldScene::scenario() const {
    return scenario_;
}

const script::ScriptData& WorldScene::scenarioScript() const {
    return scenario_script_.data();
}

bool WorldScene::readScriptWorldOperand(
    const script::Operand& operand,
    std::int32_t& value) const {
    if (operand.type != 6 && operand.type != 7) {
        return false;
    }
    const NpcActor* npc = findScriptNpc(operand.value);
    value = !npc
                ? 0
                : (operand.type == 6
                       ? npc->position().x
                       : npc->position().y);
    return true;
}

bool WorldScene::executeScriptNativeCommand(
    std::int32_t opcode,
    const std::vector<std::int32_t>& arguments) {
    if (opcode == 10) {
        if (arguments.size() < 6) {
            return false;
        }
        const std::size_t first_item = ground_items_.size();
        if (!createGroundItems(
                ground_items_,
                item_random_,
                arguments[0],
                arguments[1],
                {arguments[2], arguments[3]},
                arguments[4],
                arguments[5])) {
            return false;
        }
        return prepareGroundItems(first_item);
    }

    if (opcode == 48) {
        if (arguments.empty()) {
            return false;
        }
        quests_.selectNotice(arguments[0]);
        return true;
    }

    if (opcode == 62) {
        if (arguments.size() < 3) {
            return false;
        }
        // Argument two requests the retail server broadcast when a quest is
        // completed. The initial scenario is strictly single-player, but the
        // interpreter still evaluates and preserves that argument.
        return quests_.applyScriptUpdate(
            arguments[0], arguments[1]);
    }

    if ((opcode != 18 && opcode != 19 && opcode != 21) ||
        arguments.empty()) {
        return false;
    }
    NpcActor* npc = findScriptNpc(arguments.front());
    if (!npc) {
        return false;
    }
    if (opcode == 19) {
        npc->endInteraction();
        return true;
    }
    npc->beginInteraction(player_.position());
    conversation_actor_id_ = npc->id();
    pointer_.clearSelection();
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
        value = player_data_.level();
        return true;
    }
    return false;
}

void WorldScene::showScriptMessage(
    const script::MessageEvent& message) {
    conversation_ = message;
    conversation_active_ = true;
    conversation_selected_option_ =
        message.selection_required
            ? message.initial_selection
            : -1;
    const NpcActor* speaker =
        findScriptNpc(message.character_number);
    if (speaker) {
        conversation_actor_id_ = speaker->id();
    }
}

WorldPointerTarget WorldScene::pointerTargetAtScreenPosition(
    std::int32_t screen_x,
    std::int32_t screen_y) const {
    WorldPointer resolver;
    resolver.configure(pointer_.configuration());
    resolver.update(
        screen_x,
        screen_y,
        pointerCandidatesAtScreenPosition(
            screen_x, screen_y));
    return resolver.target();
}

std::vector<WorldPointerCandidate>
WorldScene::pointerCandidatesAtScreenPosition(
    std::int32_t screen_x,
    std::int32_t screen_y) const {
    const std::int32_t camera_x = cameraScreenX();
    const std::int32_t camera_y = cameraScreenY();
    const ScreenPosition point{screen_x, screen_y};
    const std::int32_t half_size =
        worldPointerHalfSize(pointer_.configuration());
    const DisplayHitRectangle hit_rectangle{
        screen_x - half_size,
        screen_y - half_size,
        screen_x + half_size,
        screen_y + half_size,
    };
    const WorldPosition pointer_world =
        calculateWorldPosition({
            camera_x + screen_x,
            camera_y + screen_y,
        });
    const auto pointerDistanceSquared =
        [pointer_world](WorldPosition position) {
            const std::int64_t delta_x =
                static_cast<std::int64_t>(position.x) -
                pointer_world.x;
            const std::int64_t delta_y =
                static_cast<std::int64_t>(position.y) -
                pointer_world.y;
            return delta_x * delta_x + delta_y * delta_y;
        };
    std::vector<WorldPointerCandidate> candidates;
    candidates.reserve(npcs_.size() + ground_items_.size());
    for (const NpcActor& npc : npcs_) {
        const auto part_enabled =
            [&npc](std::size_t part) {
                return npc.partEnabled(part);
            };
        if (!displayAnimationIntersectsRectangle(
                npc.animation(),
                npc.patterns(),
                npc.position(),
                npc.animationChart(),
                npc.direction(),
                npc.animationFrame(),
                part_enabled,
                camera_x,
                camera_y,
                hit_rectangle)) {
            continue;
        }
        candidates.push_back({
            {WorldPointerTargetKind::npc, npc.id()},
            {
                0,
                npc.position(),
                npc.judgement(),
                0,
            },
            0,
            displayAnimationContainsPoint(
                npc.animation(),
                npc.patterns(),
                npc.position(),
                npc.animationChart(),
                npc.direction(),
                npc.animationFrame(),
                part_enabled,
                camera_x,
                camera_y,
                point),
            pointerDistanceSquared(npc.position()),
        });
    }
    for (const GroundItem& item : ground_items_) {
        const ItemWorldResource* resource =
            itemWorldResource(item.resource_id);
        const auto part_enabled = [](std::size_t) {
            return true;
        };
        const std::int32_t display_height =
            item.height * kRetailHeightScale / 100;
        if (!resource ||
            !displayAnimationIntersectsRectangle(
                resource->animation(),
                resource->patterns(),
                item.position,
                item.animation_chart,
                8,
                0,
                part_enabled,
                camera_x,
                camera_y,
                hit_rectangle,
                display_height)) {
            continue;
        }
        candidates.push_back({
            {WorldPointerTargetKind::ground_item, item.id},
            {
                0,
                item.position,
                {},
                0,
            },
            3,
            displayAnimationContainsPoint(
                resource->animation(),
                resource->patterns(),
                item.position,
                item.animation_chart,
                8,
                0,
                part_enabled,
                camera_x,
                camera_y,
                point,
                display_height),
            pointerDistanceSquared(item.position),
        });
    }
    return candidates;
}

NpcActor* WorldScene::findScriptNpc(
    std::int32_t character_number) {
    const auto found = std::find_if(
        npcs_.begin(),
        npcs_.end(),
        [character_number](const NpcActor& npc) {
            return 12000000 + npc.id() ==
                   character_number;
        });
    return found == npcs_.end() ? nullptr : &*found;
}

const NpcActor* WorldScene::findScriptNpc(
    std::int32_t character_number) const {
    const auto found = std::find_if(
        npcs_.begin(),
        npcs_.end(),
        [character_number](const NpcActor& npc) {
            return 12000000 + npc.id() ==
                   character_number;
        });
    return found == npcs_.end() ? nullptr : &*found;
}

NpcActor* WorldScene::findNpc(std::int32_t id) {
    const auto found = std::find_if(
        npcs_.begin(),
        npcs_.end(),
        [id](const NpcActor& npc) {
            return npc.id() == id;
        });
    return found == npcs_.end() ? nullptr : &*found;
}

GroundItem* WorldScene::findGroundItem(std::int32_t id) {
    const auto found = std::find_if(
        ground_items_.begin(),
        ground_items_.end(),
        [id](const GroundItem& item) {
            return item.id == id;
        });
    return found == ground_items_.end()
        ? nullptr
        : &*found;
}

bool WorldScene::startGroundItemInteraction(
    std::int32_t item_id) {
    const auto found = std::find_if(
        ground_items_.begin(),
        ground_items_.end(),
        [item_id](const GroundItem& item) {
            return item.id == item_id;
        });
    if (found == ground_items_.end()) {
        pending_interaction_ = {};
        return false;
    }

    pending_interaction_ = {};
    player_.cancelMovement();
    const ItemDefinition* definition =
        item_database_.find(
            found->category,
            found->definition_id);
    if (definition &&
        player_inventory_.add(
            *definition,
            found->quantity)) {
        if (pointer_.target().kind ==
                WorldPointerTargetKind::ground_item &&
            pointer_.target().id == item_id) {
            pointer_.clearSelection();
        }
        ground_items_.erase(found);
    }
    return true;
}

bool WorldScene::ensureItemWorldResource(
    std::int32_t resource_id) {
    if (resource_id < 0 || resource_id > 99999999) {
        return false;
    }
    const std::size_t index =
        static_cast<std::size_t>(resource_id);
    if (index < item_world_resources_.size() &&
        item_world_resources_[index]) {
        return true;
    }
    auto resource = std::make_unique<ItemWorldResource>();
    if (!resource->load(data_root_, resource_id)) {
        return false;
    }
    if (item_world_resources_.size() <= index) {
        item_world_resources_.resize(index + 1u);
    }
    item_world_resources_[index] = std::move(resource);
    return true;
}

bool WorldScene::prepareGroundItems(
    std::size_t first_item) {
    if (first_item > ground_items_.size()) {
        return false;
    }
    const std::int32_t first_id =
        next_ground_item_id_;
    for (std::size_t index = first_item;
         index < ground_items_.size();
         ++index) {
        GroundItem& item = ground_items_[index];
        const ItemDefinition* definition =
            item_database_.find(
                item.category, item.definition_id);
        if (!definition ||
            !ensureItemWorldResource(
                definition->ground_resource_id)) {
            ground_items_.resize(first_item);
            next_ground_item_id_ = first_id;
            return false;
        }
        item.resource_id =
            definition->ground_resource_id;
        item.animation_chart =
            definition->ground_animation_chart;
        item.red_strength =
            definition->ground_red_strength;
        item.green_strength =
            definition->ground_green_strength;
        item.blue_strength =
            definition->ground_blue_strength;
        item.id = next_ground_item_id_++;
    }
    return true;
}

}  // namespace osf
