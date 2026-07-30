#ifndef OPENSHADOWFLARE_SCENARIO_WORLD_HPP
#define OPENSHADOWFLARE_SCENARIO_WORLD_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_RPG_AICONTROL/rkc_rpg_aicontrol.hpp"
#include "libs/RKC_RPG_SCRIPT/rkc_rpg_script.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "enemy_actor.hpp"
#include "ground_item.hpp"
#include "map_exploration.hpp"
#include "npc_actor.hpp"
#include "resources/character_visual_resource.hpp"
#include "resources/object_visual_resource.hpp"
#include "scenario_data.hpp"
#include "scenario_object_actor.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace osf {

struct ScenarioStart {
    std::int32_t scenario_id = 0;
    std::int32_t entry_value = 0;
    std::int32_t local_player_number = 0;
};

class ScenarioWorld {
public:
    bool load(
        const std::filesystem::path& data_root,
        const ScenarioStart& start,
        const AiControlDatabase& ai_control,
        RetailRandom& item_random,
        std::string* error = nullptr);
    void clear();

    std::int32_t id() const;
    std::int32_t musicTrack() const;
    std::int32_t localPlayerNumber() const;
    const ScenarioEntry& entry() const;
    const ScenarioData& data() const;
    ScenarioData& data();
    script::ScriptData takeScriptData();
    const GroundMap& ground() const;
    GroundMap& ground();
    const ObjectMap& objectMap() const;
    ObjectMap& objectMap();
    const std::vector<std::unique_ptr<gapi::NjpImage>>&
        mapPatterns() const;
    const gapi::NjpImage& mapOverviewPatterns() const;
    MapExploration& mapExploration();
    const MapExploration& mapExploration() const;
    std::vector<ScenarioObjectActor>& objects();
    const std::vector<ScenarioObjectActor>& objects() const;
    std::vector<NpcActor>& people();
    const std::vector<NpcActor>& people() const;
    std::vector<EnemyActor>& enemies();
    const std::vector<EnemyActor>& enemies() const;
    std::vector<GroundItem>& groundItems();
    const std::vector<GroundItem>& groundItems() const;

private:
    std::int32_t id_ = -1;
    std::int32_t music_track_ = -1;
    std::int32_t local_player_number_ = 0;
    ScenarioEntry entry_;
    ScenarioData data_;
    script::ScriptData script_data_;
    GroundMap ground_;
    ObjectMap object_map_;
    std::vector<std::unique_ptr<gapi::NjpImage>> map_patterns_;
    ObjectVisualResources object_visuals_;
    CharacterVisualResources people_visuals_{"PEOPLE"};
    CharacterVisualResources enemy_visuals_{"ENEMY"};
    gapi::NjpImage map_overview_patterns_;
    MapExploration map_exploration_;
    std::vector<ScenarioObjectActor> objects_;
    std::vector<NpcActor> people_;
    std::vector<EnemyActor> enemies_;
    std::vector<GroundItem> ground_items_;
};

}  // namespace osf

#endif
