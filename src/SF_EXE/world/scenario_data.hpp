#ifndef OPENSHADOWFLARE_SCENARIO_DATA_HPP
#define OPENSHADOWFLARE_SCENARIO_DATA_HPP

#include "enemy_presentation_profile.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf {

struct ScenarioEntry {
    std::int32_t key = 0;
    std::int32_t world_x = 0;
    std::int32_t world_y = 0;
    std::int32_t direction = 0;
};

struct ScenarioEntity {
    std::int32_t id = 0;
    std::int32_t resource_id = 0;
    std::string name;
    std::uint32_t name_color = 0;
    std::int32_t label_height = 0;
    std::int32_t world_x = 0;
    std::int32_t world_y = 0;
    std::int32_t judgement_left = 0;
    std::int32_t judgement_top = 0;
    std::int32_t judgement_right = 0;
    std::int32_t judgement_bottom = 0;
    std::int32_t direction = 0;
    std::vector<std::int32_t> initial_state_values;
    std::vector<std::int32_t> part_visibility;
    std::vector<std::int16_t> red_strength;
    std::vector<std::int16_t> green_strength;
    std::vector<std::int16_t> blue_strength;
    std::int32_t unknown_common_value = 0;
};

struct ScenarioObject : ScenarioEntity {
    std::int32_t visual_mode = 0;
    std::int32_t static_pattern = -1;
    std::int32_t animation_chart = -1;
    bool draw_status_bit_80 = false;
    std::int32_t height = 0;
    std::int32_t unknown_tail_5 = 0;
    std::int32_t unknown_tail_6 = 0;
    std::int32_t draw_flags = 0;
    std::int32_t draw_strength = 0;
    std::int32_t unknown_tail_9 = 0;
    std::int32_t red_draw_strength = 0;
    std::int32_t green_draw_strength = 0;
    std::int32_t blue_draw_strength = 0;
};

struct ScenarioPerson : ScenarioEntity {
    std::int32_t walk_speed = 0;
    std::int32_t walk_duration = 0;
    std::int32_t idle_duration = 0;
    bool wander_bounds_relative = false;
    std::int32_t wander_left = 0;
    std::int32_t wander_top = 0;
    std::int32_t wander_right = 0;
    std::int32_t wander_bottom = 0;
    bool wandering_enabled = false;
    bool scripted_turning_enabled = false;
    // Retail copies this final PEOPLE value into the actor's portable
    // initialization block. The PEOPLE update and render paths do not read
    // it, so retain it without assigning guessed behavior.
    std::int32_t reserved_behavior_value = 0;
};

struct ScenarioEnemy : ScenarioEntity {
    // Retail keeps a fixed 32-byte AI-control name between two still mostly
    // unnamed parameter blocks, resolves that name through RKC_RPG_AICONTROL,
    // then rearranges the values into its runtime enemy initializer. Keep
    // every raw value while exposing only fields proven by executable
    // consumers.
    std::array<std::int32_t, 15> pre_ai_values{};
    std::string ai_control_name;
    std::array<std::int32_t, 56> post_ai_values{};
    std::int32_t patrol_left = 0;
    std::int32_t patrol_top = 0;
    std::int32_t patrol_right = 0;
    std::int32_t patrol_bottom = 0;
    std::int32_t maximum_life = 0;
    std::int32_t native_element = 0;
    std::int32_t physical_defense = 0;
    std::int32_t physical_evasion = 0;
    std::int32_t magical_defense = 0;
    std::int32_t experience_reward = 0;
    std::int32_t loot_table_row = -1;
    std::int32_t gold_drop_chance = 0;
    std::int32_t gold_minimum = 0;
    std::int32_t gold_maximum = 0;
    std::int32_t reaction_chance_defense = 0;
    std::int32_t reaction_duration_defense = 0;
    bool always_suppress_reaction_displacement = false;
    std::int32_t movement_speed_scale = 0;
    EnemyPresentationProfile presentation;
};

struct ScenarioItem : ScenarioEntity {
    std::int32_t category = 0;
    std::int32_t definition_id = 0;
    std::int32_t minimum_quantity = 0;
    std::int32_t maximum_quantity = 0;
};

class ScenarioData {
public:
    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);
    bool decode(
        const std::vector<std::uint8_t>& bytes,
        std::string* error = nullptr);
    void clear();

    const std::string& controllerPath() const;
    const std::string& mapPath() const;
    const std::string& title() const;
    std::int32_t musicTrack() const;
    const std::vector<std::int32_t>& objectResourceIds() const;
    const std::vector<std::int32_t>& peopleResourceIds() const;
    const std::vector<std::int32_t>& enemyResourceIds() const;
    const std::vector<ScenarioObject>& objects() const;
    const std::vector<ScenarioPerson>& people() const;
    const std::vector<ScenarioEnemy>& enemies() const;
    const std::vector<ScenarioItem>& items() const;
    const std::vector<ScenarioEntry>& entries() const;
    const std::array<std::int32_t, 3>& footerValues() const;
    const ScenarioEntry* findEntry(std::int32_t key) const;

private:
    std::string controller_path_;
    std::string map_path_;
    std::string title_;
    std::int32_t music_track_ = -1;
    std::vector<std::int32_t> object_resource_ids_;
    std::vector<std::int32_t> people_resource_ids_;
    std::vector<std::int32_t> enemy_resource_ids_;
    std::vector<ScenarioObject> objects_;
    std::vector<ScenarioPerson> people_;
    std::vector<ScenarioEnemy> enemies_;
    std::vector<ScenarioItem> items_;
    std::vector<ScenarioEntry> entries_;
    std::array<std::int32_t, 3> footer_values_{};
};

}  // namespace osf

#endif
