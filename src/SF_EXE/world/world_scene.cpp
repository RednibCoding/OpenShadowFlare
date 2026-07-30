#include "world_scene.hpp"
#include "items/item_audio.hpp"
#include "movement_controller.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kRetailInteractionDistance = 0x9f;

}  // namespace

WorldScene::WorldScene()
    : scenario_script_({
          [this](
              const script::Operand& operand,
              std::int32_t& value) {
              return readScriptWorldOperand(operand, value);
          },
          [this](std::int32_t character_number) {
              const NpcActor* npc =
                  findScriptNpc(character_number);
              return npc ? npc->id() : -1;
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


void WorldScene::clear() {
    scenario_.clear();
    scenario_script_.clear();
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
    player_appearance_.clear();
    npcs_.clear();
    ground_items_.clear();
    pending_audio_samples_.clear();
    quests_.clear();
    missions_.clear();
    item_database_.clear();
    player_equipment_.clear();
    player_belt_.clear();
    player_inventory_.clear();
    player_special_items_.clear();
    item_inventory_patterns_.clear();
    parameter_tables_.clear();
    data_root_.clear();
    item_world_resources_.clear();
    item_random_.seed(1);
    player_data_.clear();
    player_item_controller_.clear();
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

PlayerEquipment& WorldScene::playerEquipment() {
    return player_equipment_;
}

const PlayerEquipment& WorldScene::playerEquipment() const {
    return player_equipment_;
}

PlayerBelt& WorldScene::playerBelt() {
    return player_belt_;
}

const PlayerBelt& WorldScene::playerBelt() const {
    return player_belt_;
}

PlayerInventory& WorldScene::playerInventory() {
    return player_inventory_;
}

const PlayerInventory& WorldScene::playerInventory() const {
    return player_inventory_;
}

PlayerSpecialItems& WorldScene::playerSpecialItems() {
    return player_special_items_;
}

const PlayerSpecialItems& WorldScene::playerSpecialItems() const {
    return player_special_items_;
}

const ItemInventoryResource&
WorldScene::itemInventoryPatterns() const {
    return item_inventory_patterns_;
}

const PlayerData& WorldScene::playerData() const {
    return player_data_;
}

BeltItemUseResult WorldScene::usePlayerBeltPocket(
    std::int32_t pocket) {
    return player_item_controller_.useBeltPocket(
        pocket,
        player_belt_,
        item_database_,
        player_data_);
}

std::int32_t WorldScene::playerMineCount() const {
    return player_item_controller_.mineCount();
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
    return player_appearance_.partEnabled(part);
}

std::int32_t WorldScene::playerPartRedStrength(
    std::size_t part) const {
    return player_appearance_.redStrength(part);
}

std::int32_t WorldScene::playerPartGreenStrength(
    std::size_t part) const {
    return player_appearance_.greenStrength(part);
}

std::int32_t WorldScene::playerPartBlueStrength(
    std::size_t part) const {
    return player_appearance_.blueStrength(part);
}

void WorldScene::refreshPlayerAppearance() {
    player_appearance_.refresh(
        player_visual_.animation().maxPartCount(),
        player_equipment_,
        item_database_);
}

bool WorldScene::hasPlayer() const {
    return has_player_;
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
    constexpr std::int32_t player_blocker_id =
        kNoMovementBlockerId + 1;
    std::vector<MovementBlocker> actor_blockers;
    actor_blockers.reserve(
        npcs_.size() + (has_player_ ? 1u : 0u));
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
        actor_blockers.push_back({
            player_blocker_id,
            player_.position(),
            player_.judgement(),
        });
    }
    for (std::size_t index = 0;
         index < npcs_.size();
         ++index) {
        NpcActor& npc = npcs_[index];
        npc.update(
            ground_, object_map_, &actor_blockers);
        actor_blockers[index].position =
            npc.position();
    }
    for (GroundItem& item : ground_items_) {
        if (updateGroundItem(item) !=
            GroundItemUpdateEvent::first_impact) {
            continue;
        }
        const ItemDefinition* definition =
            item_database_.find(
                item.category,
                item.definition_id);
        if (definition) {
            pending_audio_samples_.push_back(
                retailItemLandingSound(*definition));
        }
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

std::vector<std::int32_t> WorldScene::takeAudioSamples() {
    std::vector<std::int32_t> samples;
    samples.swap(pending_audio_samples_);
    return samples;
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


}  // namespace osf
