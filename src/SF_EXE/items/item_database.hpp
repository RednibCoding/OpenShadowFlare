#ifndef OPENSHADOWFLARE_ITEM_DATABASE_HPP
#define OPENSHADOWFLARE_ITEM_DATABASE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf {

struct ItemDefinition {
    std::int32_t category = -1;
    std::int32_t id = -1;
    std::int32_t subtype = -1;
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
