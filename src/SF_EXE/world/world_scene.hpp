#ifndef OPENSHADOWFLARE_WORLD_SCENE_HPP
#define OPENSHADOWFLARE_WORLD_SCENE_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_RPG_SCRIPT/rkc_rpg_script.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "items/item_database.hpp"
#include "items/item_world_resource.hpp"
#include "items/player_inventory.hpp"
#include "resources/character_visual_resource.hpp"
#include "resources/item_inventory_resource.hpp"
#include "ground_item.hpp"
#include "mission_catalog.hpp"
#include "map_exploration.hpp"
#include "npc_actor.hpp"
#include "player_actor.hpp"
#include "player_data.hpp"
#include "quest_state.hpp"
#include "scenario_data.hpp"
#include "script/scenario_script_runtime.hpp"
#include "world_pointer.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace osf {

class WorldScene {
public:
    WorldScene();

    bool loadInitialScenario(
        const std::filesystem::path& data_root,
        const PlayerLoadRequest& player_request,
        std::string* error = nullptr);
    void clear();

    const GroundMap& ground() const;
    const ObjectMap& objectMap() const;
    const std::vector<std::unique_ptr<gapi::NjpImage>>&
        mapPatterns() const;
    const gapi::NjpImage& playerPatterns() const;
    const gapi::NjpImage& playerShadowPatterns() const;
    const gapi::CafAnimation& playerAnimation() const;
    const std::vector<NpcActor>& npcs() const;
    const std::vector<GroundItem>& groundItems() const;
    const QuestState& quests() const;
    const MissionCatalog& missions() const;
    const ItemDatabase& itemDatabase() const;
    PlayerInventory& playerInventory();
    const PlayerInventory& playerInventory() const;
    const ItemInventoryResource& itemInventoryPatterns() const;
    const PlayerData& playerData() const;
    const ItemWorldResource* itemWorldResource(
        std::int32_t resource_id) const;
    bool playerPartEnabled(std::size_t part) const;
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
    std::int32_t hoveredNpcId() const;
    std::int32_t hoveredGroundItemId() const;
    std::int32_t pointerScreenX() const;
    std::int32_t pointerScreenY() const;
    bool pointerActive() const;
    const WorldPointerConfiguration& pointerConfiguration() const;
    bool conversationActive() const;
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
    std::int32_t playerWorldX() const;
    std::int32_t playerWorldY() const;
    std::int32_t playerDirection() const;
    PlayerMotion playerMotion() const;
    MovementPace playerMovementPace() const;
    std::int32_t playerAnimationChart() const;
    std::int32_t playerAnimationFrame() const;
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
    const script::ScriptData& scenarioScript() const;

private:
    bool readScriptWorldOperand(
        const script::Operand& operand,
        std::int32_t& value) const;
    bool executeScriptNativeCommand(
        std::int32_t opcode,
        const std::vector<std::int32_t>& arguments);
    bool queryScriptValue(
        script::ValueQuery query,
        std::int32_t& value) const;
    void showScriptMessage(
        const script::MessageEvent& message);
    WorldPointerTarget pointerTargetAtScreenPosition(
        std::int32_t screen_x,
        std::int32_t screen_y) const;
    std::vector<WorldPointerCandidate> pointerCandidatesAtScreenPosition(
        std::int32_t screen_x,
        std::int32_t screen_y) const;
    NpcActor* findScriptNpc(std::int32_t character_number);
    const NpcActor* findScriptNpc(
        std::int32_t character_number) const;
    bool ensureItemWorldResource(std::int32_t resource_id);
    bool prepareGroundItems(std::size_t first_item);
    bool startNpcInteraction(NpcActor& npc);
    bool startGroundItemInteraction(std::int32_t item_id);
    NpcActor* findNpc(std::int32_t id);
    GroundItem* findGroundItem(std::int32_t id);

    ScenarioData scenario_;
    ScenarioScriptRuntime scenario_script_;
    script::MessageEvent conversation_;
    bool conversation_active_ = false;
    std::int32_t conversation_actor_id_ = -1;
    std::int32_t conversation_selected_option_ = -1;
    WorldPointer pointer_;
    WorldPointerTarget pending_interaction_;
    GroundMap ground_;
    ObjectMap object_map_;
    std::vector<std::unique_ptr<gapi::NjpImage>> map_patterns_;
    CharacterVisualResource player_visual_;
    PeopleVisualResources people_visuals_;
    gapi::NjpImage speech_patterns_;
    gapi::NjpImage map_overview_patterns_;
    MapExploration map_exploration_;
    std::vector<std::uint8_t> player_parts_enabled_;
    std::vector<NpcActor> npcs_;
    std::vector<GroundItem> ground_items_;
    QuestState quests_;
    MissionCatalog missions_;
    ItemDatabase item_database_;
    PlayerInventory player_inventory_;
    ItemInventoryResource item_inventory_patterns_;
    TableDatabase parameter_tables_;
    std::filesystem::path data_root_;
    std::vector<std::unique_ptr<ItemWorldResource>>
        item_world_resources_;
    RetailRandom item_random_;
    PlayerData player_data_;
    PlayerActor player_;
    bool has_player_ = false;
    std::int32_t music_track_ = -1;
    std::int32_t next_ground_item_id_ = 0;
    std::int32_t camera_anchor_x_ = 320;
    std::int32_t camera_anchor_y_ = 240;
};

}  // namespace osf

#endif
