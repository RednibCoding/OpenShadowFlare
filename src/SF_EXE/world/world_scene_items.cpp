#include "world_scene.hpp"
#include "movement_controller.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <utility>

namespace osf {

bool WorldScene::ensureItemWorldResource(
    std::int32_t resource_id) {
    if (resource_id < 0 || resource_id > 99999999) {
        return false;
    }
    const std::size_t index =
        static_cast<std::size_t>(resource_id);
    if (index < item_world_resources_.size() &&
        item_world_resources_[index]) {
        return true;
    }
    auto resource = std::make_unique<ItemWorldResource>();
    if (!resource->load(data_root_, resource_id)) {
        return false;
    }
    if (item_world_resources_.size() <= index) {
        item_world_resources_.resize(index + 1u);
    }
    item_world_resources_[index] = std::move(resource);
    return true;
}

bool WorldScene::prepareGroundItems(
    std::size_t first_item) {
    if (first_item > ground_items_.size()) {
        return false;
    }
    const std::int32_t first_id =
        next_ground_item_id_;
    for (std::size_t index = first_item;
         index < ground_items_.size();
         ++index) {
        GroundItem& item = ground_items_[index];
        const ItemDefinition* definition =
            item_database_.find(
                item.category, item.definition_id);
        if (!definition ||
            !ensureItemWorldResource(
                definition->ground_resource_id)) {
            ground_items_.resize(first_item);
            next_ground_item_id_ = first_id;
            return false;
        }
        item.resource_id =
            definition->ground_resource_id;
        item.animation_chart =
            definition->ground_animation_chart;
        item.red_strength =
            definition->ground_red_strength;
        item.green_strength =
            definition->ground_green_strength;
        item.blue_strength =
            definition->ground_blue_strength;
        item.id = next_ground_item_id_++;
    }
    return true;
}

}  // namespace osf
