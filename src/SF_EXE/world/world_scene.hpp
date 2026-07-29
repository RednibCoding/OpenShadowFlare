#ifndef OPENSHADOWFLARE_WORLD_SCENE_HPP
#define OPENSHADOWFLARE_WORLD_SCENE_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_RPG_SCRIPT/rkc_rpg_script.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "items/item_database.hpp"
#include "items/item_world_resource.hpp"
#include "ground_item.hpp"
#include "npc_actor.hpp"
#include "player_actor.hpp"
#include "scenario_data.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace osf {

class WorldScene {
public:
    WorldScene();

    bool loadInitialScenario(
        const std::filesystem::path& data_root,
        std::int32_t character_gender,
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
    const ItemDatabase& itemDatabase() const;
    const ItemWorldResource* itemWorldResource(
        std::int32_t resource_id) const;
    bool playerPartEnabled(std::size_t part) const;
    bool hasPlayer() const;
    void commandPlayerMovement(
        std::int32_t screen_x,
        std::int32_t screen_y);
    void updatePointerHover(
        std::int32_t screen_x,
        std::int32_t screen_y);
    bool commandWorldInteraction(
        std::int32_t screen_x,
        std::int32_t screen_y);
    std::int32_t hoveredNpcId() const;
    bool conversationActive() const;
    std::int32_t conversationActorId() const;
    std::int32_t conversationMessageId() const;
    const std::string& conversationText() const;
    const gapi::NjpImage& speechPatterns() const;
    void advanceConversation();
    void togglePlayerRun();
    void update();
    std::int32_t playerWorldX() const;
    std::int32_t playerWorldY() const;
    std::int32_t playerDirection() const;
    PlayerMotion playerMotion() const;
    std::int32_t playerAnimationChart() const;
    std::int32_t playerAnimationFrame() const;
    std::int32_t cameraScreenX() const;
    std::int32_t cameraScreenY() const;
    std::int32_t musicTrack() const;
    const ScenarioData& scenario() const;
    const script::ScriptData& scenarioScript() const;

private:
    std::int32_t readScriptOperand(
        const script::Operand& operand) const;
    bool writeScriptOperand(
        const script::Operand& operand,
        std::int32_t value);
    bool executeScriptNativeCommand(
        std::int32_t opcode,
        const std::vector<std::int32_t>& arguments);
    bool queryScriptValue(
        script::ValueQuery query,
        std::int32_t& value) const;
    void showScriptMessage(
        const script::MessageEvent& message);
    std::int32_t npcIndexAtScreenPosition(
        std::int32_t screen_x,
        std::int32_t screen_y) const;
    NpcActor* findScriptNpc(std::int32_t character_number);
    const NpcActor* findScriptNpc(
        std::int32_t character_number) const;
    bool ensureItemWorldResource(std::int32_t resource_id);

    ScenarioData scenario_;
    script::ScriptData scenario_script_;
    script::Interpreter script_interpreter_;
    std::unordered_map<std::uint64_t, std::int32_t>
        script_values_;
    script::MessageEvent conversation_;
    bool conversation_active_ = false;
    std::int32_t conversation_actor_id_ = -1;
    std::int32_t hovered_npc_id_ = -1;
    GroundMap ground_;
    ObjectMap object_map_;
    std::vector<std::unique_ptr<gapi::NjpImage>> map_patterns_;
    gapi::NjpImage player_patterns_;
    gapi::NjpImage player_shadow_patterns_;
    gapi::CafAnimation player_animation_;
    gapi::NjpImage speech_patterns_;
    std::vector<std::uint8_t> player_parts_enabled_;
    std::vector<NpcActor> npcs_;
    std::vector<GroundItem> ground_items_;
    ItemDatabase item_database_;
    std::filesystem::path data_root_;
    std::vector<std::unique_ptr<ItemWorldResource>>
        item_world_resources_;
    RetailRandom item_random_;
    PlayerActor player_;
    bool has_player_ = false;
    std::int32_t music_track_ = -1;
};

}  // namespace osf

#endif
