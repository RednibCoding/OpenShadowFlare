#ifndef OPENSHADOWFLARE_WORLD_SCENE_HPP
#define OPENSHADOWFLARE_WORLD_SCENE_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_RPG_AICONTROL/rkc_rpg_aicontrol.hpp"
#include "libs/RKC_RPG_SCRIPT/rkc_rpg_script.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "items/item_database.hpp"
#include "items/player_belt.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "items/player_special_items.hpp"
#include "resources/character_visual_resource.hpp"
#include "resources/effect_visual_resource.hpp"
#include "resources/item_inventory_resource.hpp"
#include "resources/item_world_resource.hpp"
#include "resources/object_visual_resource.hpp"
#include "ground_item.hpp"
#include "combat_effect_actor.hpp"
#include "mission_catalog.hpp"
#include "map_exploration.hpp"
#include "npc_actor.hpp"
#include "player_appearance.hpp"
#include "player_actor.hpp"
#include "player_attack_target.hpp"
#include "player_data.hpp"
#include "player_damage_receiver.hpp"
#include "player_item_controller.hpp"
#include "quest_state.hpp"
#include "scenario_world.hpp"
#include "script/scenario_script_runtime.hpp"
#include "transport_catalog.hpp"
#include "world_pointer.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace osf {

enum class GameplayServiceKind {
    none,
    transport,
    toggle_special_items,
};

struct GameplayServiceRequest {
    GameplayServiceKind kind = GameplayServiceKind::none;
    std::int32_t argument = 0;
};

enum class ScenarioTravelResult {
    failed,
    relocated,
    loaded,
};

class WorldScene {
public:
    WorldScene();

    bool loadInitialScenario(
        const std::filesystem::path& data_root,
        const PlayerLoadRequest& player_request,
        std::string* error = nullptr);
    bool loadInitialScenario(
        const std::filesystem::path& data_root,
        const PlayerLoadRequest& player_request,
        const ScenarioStart& start,
        std::string* error = nullptr);
    void clear();

    const GroundMap& ground() const;
    const ObjectMap& objectMap() const;
    const std::vector<std::unique_ptr<gapi::NjpImage>>&
        mapPatterns() const;
    const gapi::NjpImage& playerPatterns() const;
    const gapi::NjpImage& playerShadowPatterns() const;
    const gapi::CafAnimation& playerAnimation() const;
    const std::vector<ScenarioObjectActor>&
        scenarioObjects() const;
    const std::vector<NpcActor>& npcs() const;
    const std::vector<EnemyActor>& enemies() const;
    const std::vector<CombatEffectActor>&
        combatEffects() const;
    const std::vector<GroundItem>& groundItems() const;
    const QuestState& quests() const;
    const MissionCatalog& missions() const;
    const TransportCatalog& transports() const;
    const ItemDatabase& itemDatabase() const;
    const AiControlDatabase& aiControlDatabase() const;
    PlayerEquipment& playerEquipment();
    const PlayerEquipment& playerEquipment() const;
    PlayerBelt& playerBelt();
    const PlayerBelt& playerBelt() const;
    PlayerInventory& playerInventory();
    const PlayerInventory& playerInventory() const;
    PlayerSpecialItems& playerSpecialItems();
    const PlayerSpecialItems& playerSpecialItems() const;
    const ItemInventoryResource& itemInventoryPatterns() const;
    const PlayerData& playerData() const;
    std::int32_t playerExperienceThreshold() const;
    BeltItemUseResult usePlayerBeltPocket(
        std::int32_t pocket);
    std::int32_t playerMineCount() const;
    const ItemWorldResource* itemWorldResource(
        std::int32_t resource_id) const;
    bool playerPartEnabled(std::size_t part) const;
    std::int32_t playerPartRedStrength(
        std::size_t part) const;
    std::int32_t playerPartGreenStrength(
        std::size_t part) const;
    std::int32_t playerPartBlueStrength(
        std::size_t part) const;
    void refreshPlayerAppearance();
    bool hasPlayer() const;
    void commandPlayerMovement(
        std::int32_t screen_x,
        std::int32_t screen_y);
    void cancelPlayerMovement();
    void updatePointerHover(
        std::int32_t screen_x,
        std::int32_t screen_y);
    void clearPointerHover();
    void configurePointer(
        const WorldPointerConfiguration& configuration);
    bool commandWorldInteraction(
        std::int32_t screen_x,
        std::int32_t screen_y);
    bool dropInventoryItem(
        const InventoryItem& item,
        std::int32_t screen_x,
        std::int32_t screen_y);
    bool interactionPending() const;
    std::int32_t hoveredScenarioObjectId() const;
    std::int32_t hoveredNpcId() const;
    std::int32_t hoveredEnemyId() const;
    std::int32_t hoveredGroundItemId() const;
    std::int32_t pointerScreenX() const;
    std::int32_t pointerScreenY() const;
    bool pointerActive() const;
    const WorldPointerConfiguration& pointerConfiguration() const;
    bool conversationActive() const;
    GameplayServiceRequest takeGameplayServiceRequest();
    ScenarioTravelResult activateTransportDestination(
        std::int32_t row,
        std::string* error = nullptr);
    ScenarioTravelResult transitionScenario(
        const ScenarioStart& start,
        std::string* error = nullptr);
    std::int32_t conversationActorId() const;
    std::int32_t conversationMessageId() const;
    const std::string& conversationText() const;
    bool conversationRequiresSelection() const;
    std::int32_t conversationInitialSelection() const;
    std::int32_t conversationSelectedOption() const;
    void selectConversationOption(std::int32_t option);
    const gapi::NjpImage& speechPatterns() const;
    const gapi::NjpImage& mapOverviewPatterns() const;
    const MapExploration& mapExploration() const;
    void advanceConversation();
    void chooseConversationOption(std::int32_t option);
    void togglePlayerRun();
    void update();
    std::vector<std::int32_t> takeAudioSamples();
    std::int32_t playerWorldX() const;
    std::int32_t playerWorldY() const;
    std::int32_t playerDirection() const;
    PlayerMotion playerMotion() const;
    MovementPace playerMovementPace() const;
    std::int32_t playerAnimationChart() const;
    std::int32_t playerAnimationFrame() const;
    std::int32_t playerAttackTargetId() const;
    std::int32_t takePlayerAttackImpactTargetId();
    std::int32_t cameraScreenX() const;
    std::int32_t cameraScreenY() const;
    std::int32_t renderCameraScreenX(double alpha) const;
    std::int32_t renderCameraScreenY(double alpha) const;
    void setCameraAnchor(
        std::int32_t screen_x,
        std::int32_t screen_y);
    WorldPosition playerRenderPosition(double alpha) const;
    const ObjectBounds& playerJudgement() const;
    std::int32_t musicTrack() const;
    const ScenarioData& scenario() const;
    std::int32_t scenarioId() const;
    const script::ScriptData& scenarioScript() const;

private:
    bool readScriptWorldOperand(
        const script::Operand& operand,
        std::int32_t& value) const;
    bool writeScriptWorldOperand(
        const script::Operand& operand,
        std::int32_t value);
    bool executeScriptNativeCommand(
        std::int32_t opcode,
        const std::vector<std::int32_t>& arguments);
    bool queryScriptValue(
        script::ValueQuery query,
        std::int32_t& value) const;
    WorldPointerTarget pointerTargetAtScreenPosition(
        std::int32_t screen_x,
        std::int32_t screen_y) const;
    std::vector<WorldPointerCandidate> pointerCandidatesAtScreenPosition(
        std::int32_t screen_x,
        std::int32_t screen_y) const;
    NpcActor* findScriptNpc(std::int32_t character_number);
    const NpcActor* findScriptNpc(
        std::int32_t character_number) const;
    ScenarioObjectActor* findScriptObject(
        std::int32_t character_number);
    const ScenarioObjectActor* findScriptObject(
        std::int32_t character_number) const;
    EnemyActor* findScriptEnemy(
        std::int32_t character_number);
    const EnemyActor* findScriptEnemy(
        std::int32_t character_number) const;
    GroundItem* findScriptGroundItem(
        std::int32_t character_number);
    const GroundItem* findScriptGroundItem(
        std::int32_t character_number) const;
    bool ensureItemWorldResource(
        std::int32_t resource_id,
        std::string* error = nullptr);
    bool prepareGroundItems(std::size_t first_item);
    bool prepareGroundItems(
        std::vector<GroundItem>& ground_items,
        std::size_t first_item,
        std::int32_t& next_item_id,
        std::string* error = nullptr);
    bool startNpcInteraction(NpcActor& npc);
    bool startScenarioObjectInteraction(
        ScenarioObjectActor& object);
    bool startGroundItemInteraction(std::int32_t item_id);
    ScenarioObjectActor* findScenarioObject(
        std::int32_t id);
    NpcActor* findNpc(std::int32_t id);
    EnemyActor* findEnemy(std::int32_t id);
    const EnemyActor* findEnemy(std::int32_t id) const;
    GroundItem* findGroundItem(std::int32_t id);
    PlayerAttackTargetSnapshot attackTargetSnapshot(
        const EnemyActor& enemy) const;
    bool commandPlayerAttack(EnemyActor& enemy);
    bool readyPlayerAttack(EnemyActor& enemy);
    std::int32_t playerAttackSpeedTier() const;
    void handlePlayerAttackEvent(
        const PlayerAttackActionEvent& event);
    void applyPlayerAttackImpact(EnemyActor& enemy);
    void handleEnemyDeathStart(
        EnemyActor& enemy,
        CombatEffectSpawnRequest effect);
    EnemyActorUpdate updateEnemyActor(
        EnemyActor& enemy,
        const std::vector<MovementBlocker>& blockers);
    PlayerDamageReceiverState playerDamageReceiverState() const;
    void applyPlayerDamageReceiverState(
        const PlayerDamageReceiverState& state);
    void applyEnemyDirectImpact(
        EnemyActor& enemy,
        const EnemyDirectImpactResult& impact);
    void queueCombatEffect(
        const CombatEffectSpawnRequest& request);
    void spawnPendingCombatEffects();
    WorldPosition combatEffectOrigin(
        const CombatEffectSpawnRequest& request) const;
    ObjectBounds combatEffectJudgement(
        const CombatEffectSpawnRequest& request) const;

    ScenarioWorld scenario_world_;
    ScenarioScriptRuntime scenario_script_;
    WorldPointer pointer_;
    WorldPointerTarget pending_interaction_;
    PlayerAttackTargetController player_attack_target_;
    CharacterVisualResource player_visual_;
    EffectVisualResources effect_visuals_;
    gapi::NjpImage speech_patterns_;
    PlayerAppearance player_appearance_;
    std::vector<std::int32_t> pending_audio_samples_;
    std::vector<CombatEffectSpawnRequest>
        pending_combat_effects_;
    std::vector<CombatEffectActor> combat_effects_;
    QuestState quests_;
    MissionCatalog missions_;
    TransportCatalog transports_;
    ItemDatabase item_database_;
    PlayerEquipment player_equipment_;
    PlayerBelt player_belt_;
    PlayerInventory player_inventory_;
    PlayerSpecialItems player_special_items_;
    ItemInventoryResource item_inventory_patterns_;
    TableDatabase parameter_tables_;
    AiControlDatabase ai_control_database_;
    std::unordered_map<std::uint64_t, std::int32_t>
        script_persistent_values_;
    std::filesystem::path data_root_;
    std::vector<std::unique_ptr<ItemWorldResource>>
        item_world_resources_;
    RetailRandom item_random_;
    PlayerData player_data_;
    PlayerItemController player_item_controller_;
    PlayerActor player_;
    bool has_player_ = false;
    std::int32_t pending_player_attack_impact_target_id_ = -1;
    std::int32_t next_ground_item_id_ = 0;
    std::int32_t camera_anchor_x_ = 320;
    std::int32_t camera_anchor_y_ = 240;
    GameplayServiceRequest gameplay_service_request_;
};

}  // namespace osf

#endif
