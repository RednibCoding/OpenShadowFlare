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
          [this](
              const script::Operand& operand,
              std::int32_t value) {
              return writeScriptWorldOperand(operand, value);
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
    scenario_world_.clear();
    scenario_script_.clear();
    pointer_.reset();
    pending_interaction_ = {};
    player_visual_.clear();
    speech_patterns_.clear();
    player_appearance_.clear();
    pending_audio_samples_.clear();
    quests_.clear();
    missions_.clear();
    transports_.clear();
    item_database_.clear();
    player_equipment_.clear();
    player_belt_.clear();
    player_inventory_.clear();
    player_special_items_.clear();
    item_inventory_patterns_.clear();
    parameter_tables_.clear();
    script_persistent_values_.clear();
    data_root_.clear();
    item_world_resources_.clear();
    item_random_.seed(1);
    player_data_.clear();
    player_item_controller_.clear();
    player_.clear();
    has_player_ = false;
    next_ground_item_id_ = 0;
    camera_anchor_x_ = 320;
    camera_anchor_y_ = 240;
    gameplay_service_request_ = {};
}

const GroundMap& WorldScene::ground() const {
    return scenario_world_.ground();
}

const ObjectMap& WorldScene::objectMap() const {
    return scenario_world_.objectMap();
}

const std::vector<std::unique_ptr<gapi::NjpImage>>&
WorldScene::mapPatterns() const {
    return scenario_world_.mapPatterns();
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

const std::vector<ScenarioObjectActor>&
WorldScene::scenarioObjects() const {
    return scenario_world_.objects();
}

const std::vector<NpcActor>& WorldScene::npcs() const {
    return scenario_world_.people();
}

const TransportCatalog& WorldScene::transports() const {
    return transports_;
}

const std::vector<GroundItem>& WorldScene::groundItems() const {
    return scenario_world_.groundItems();
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
    if (!scenario_script_.messageActive()) {
        scenario_script_.runStatusKind(5);
    }
    NpcActor* interaction_npc = nullptr;
    ScenarioObjectActor* interaction_object = nullptr;
    GroundItem* interaction_item = nullptr;
    if (pending_interaction_.kind ==
        WorldPointerTargetKind::scenario_object) {
        interaction_object =
            findScenarioObject(pending_interaction_.id);
        if (interaction_object) {
            player_.followTo(interaction_object->position());
        }
    } else if (pending_interaction_.kind ==
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
        !interaction_object &&
        !interaction_npc &&
        !interaction_item) {
        pending_interaction_ = {};
    }
    constexpr std::int32_t player_blocker_id =
        kNoMovementBlockerId + 1;
    std::vector<MovementBlocker> actor_blockers;
    actor_blockers.reserve(
        scenario_world_.objects().size() +
        scenario_world_.people().size() +
        (has_player_ ? 1u : 0u));
    for (const ScenarioObjectActor& object :
         scenario_world_.objects()) {
        if (!object.judgementEnabled()) {
            continue;
        }
        actor_blockers.push_back({
            object.movementBlockerId(),
            object.position(),
            object.judgement(),
        });
    }
    std::vector<std::size_t> npc_blocker_indices(
        scenario_world_.people().size(),
        actor_blockers.size());
    constexpr std::size_t no_blocker =
        static_cast<std::size_t>(-1);
    for (const NpcActor& npc : scenario_world_.people()) {
        const std::size_t index =
            static_cast<std::size_t>(
                &npc - scenario_world_.people().data());
        if (!npc.judgementEnabled()) {
            npc_blocker_indices[index] = no_blocker;
            continue;
        }
        npc_blocker_indices[index] = actor_blockers.size();
        actor_blockers.push_back({
            npc.movementBlockerId(),
            npc.position(),
            npc.judgement(),
        });
    }
    if (has_player_) {
        player_.update(
            scenario_world_.ground(),
            scenario_world_.objectMap(),
            &actor_blockers);
        scenario_world_.mapExploration().reveal(
            player_.position());
        actor_blockers.push_back({
            player_blocker_id,
            player_.position(),
            player_.judgement(),
        });
    }
    for (ScenarioObjectActor& object :
         scenario_world_.objects()) {
        object.update();
    }
    for (std::size_t index = 0;
         index < scenario_world_.people().size();
         ++index) {
        NpcActor& npc = scenario_world_.people()[index];
        npc.update(
            scenario_world_.ground(),
            scenario_world_.objectMap(),
            &actor_blockers);
        if (npc_blocker_indices[index] != no_blocker) {
            actor_blockers[
                npc_blocker_indices[index]].position =
                npc.position();
        }
    }
    for (GroundItem& item : scenario_world_.groundItems()) {
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
    interaction_object =
        pending_interaction_.kind ==
                WorldPointerTargetKind::scenario_object
            ? findScenarioObject(pending_interaction_.id)
            : nullptr;
    if (interaction_object &&
        distanceBetweenBounds(
            player_.position(),
            player_.judgement(),
            interaction_object->position(),
            interaction_object->judgement()) <=
            kRetailInteractionDistance) {
        startScenarioObjectInteraction(*interaction_object);
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
    return scenario_world_.musicTrack();
}

const ScenarioData& WorldScene::scenario() const {
    return scenario_world_.data();
}

std::int32_t WorldScene::scenarioId() const {
    return scenario_world_.id();
}

const script::ScriptData& WorldScene::scenarioScript() const {
    return scenario_script_.data();
}


}  // namespace osf
