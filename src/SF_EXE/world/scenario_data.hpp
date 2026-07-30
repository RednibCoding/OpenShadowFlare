#ifndef OPENSHADOWFLARE_SCENARIO_DATA_HPP
#define OPENSHADOWFLARE_SCENARIO_DATA_HPP

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

struct ScenarioObject {
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

struct ScenarioPerson {
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
    std::int32_t unknown_tail_value = 0;
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
    std::vector<ScenarioEntry> entries_;
    std::array<std::int32_t, 3> footer_values_{};
};

}  // namespace osf

#endif
