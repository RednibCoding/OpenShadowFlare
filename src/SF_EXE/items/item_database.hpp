#ifndef OPENSHADOWFLARE_ITEM_DATABASE_HPP
#define OPENSHADOWFLARE_ITEM_DATABASE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf {

struct RetailItemRoll {
    std::int32_t minimum = 0;
    std::int32_t maximum = 0;
    std::int32_t chance = 0;
};

struct ItemDefinition {
    static constexpr std::size_t derived_parameter_count = 10;
    static constexpr std::size_t element_count = 8;
    static constexpr std::size_t instance_parameter_count = 39;

    std::int32_t category = -1;
    std::int32_t id = -1;
    std::int32_t subtype = -1;
    std::int32_t variant = -1;
    std::int32_t loot_episode_mask = 0;
    std::int32_t loot_weight = 0;
    std::int32_t base_price = 0;
    std::int32_t inventory_width = 1;
    std::int32_t inventory_height = 1;
    std::int32_t weight = 0;
    std::int32_t inventory_pattern_group = -1;
    std::int32_t inventory_pattern = -1;
    std::int32_t ground_resource_id = -1;
    std::int32_t ground_animation_chart = -1;
    std::int32_t inventory_palette = -1;
    std::int32_t ground_red_strength = 1000;
    std::int32_t ground_green_strength = 1000;
    std::int32_t ground_blue_strength = 1000;
    std::array<
        std::int32_t,
        derived_parameter_count> derived_parameter_bonuses{};
    std::int32_t maximum_durability = 0;
    std::array<
        std::int32_t,
        element_count> element_strengths{};
    std::array<
        RetailItemRoll,
        instance_parameter_count> instance_parameter_rolls{};
    std::array<
        RetailItemRoll,
        element_count> element_rolls{};
    std::int32_t required_level = 1;
    std::int32_t appearance_part = -1;
    std::int32_t appearance_red_strength = 1000;
    std::int32_t appearance_green_strength = 1000;
    std::int32_t appearance_blue_strength = 1000;
    std::int32_t secondary_appearance_part = -1;
    std::int32_t secondary_appearance_red_strength = 1000;
    std::int32_t secondary_appearance_green_strength = 1000;
    std::int32_t secondary_appearance_blue_strength = 1000;
    bool suppresses_off_hand = false;
    std::int32_t restore_life = 0;
    std::int32_t restore_mana = 0;
    std::int32_t restore_life_percent = 0;
    std::int32_t restore_mana_percent = 0;
    std::int32_t restore_companion_life = 0;
    std::int32_t restore_companion_life_percent = 0;
    std::int32_t consumable_effect = -1;
    std::int32_t consumable_effect_value = 0;
    std::string name;
    std::string description;

    // The rest of each retail record is retained until its fields are named.
    std::vector<std::uint8_t> raw_fields;
};

class ItemDatabase {
public:
    static constexpr std::size_t category_count = 5;

    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);
    bool decode(
        const std::vector<std::uint8_t>& bytes,
        std::string* error = nullptr);
    void clear();

    const ItemDefinition* find(
        std::int32_t category,
        std::int32_t id) const;
    const std::vector<ItemDefinition>& definitions(
        std::size_t category) const;
    std::size_t definitionCount() const;

private:
    std::array<
        std::vector<ItemDefinition>,
        category_count> definitions_;
};

}  // namespace osf

#endif
