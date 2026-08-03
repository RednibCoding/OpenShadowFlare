#include "world_scene.hpp"
#include "movement_controller.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <utility>

namespace osf {

bool WorldScene::ensureItemWorldResource(
    std::int32_t resource_id,
    std::string* error) {
    if (resource_id < 0 || resource_id > 99999999) {
        if (error) {
            *error =
                "The ground-item resource ID is invalid.";
        }
        return false;
    }
    const std::size_t index =
        static_cast<std::size_t>(resource_id);
    if (index < item_world_resources_.size() &&
        item_world_resources_[index]) {
        return true;
    }
    auto resource = std::make_unique<ItemWorldResource>();
    if (!resource->load(
            data_root_, resource_id, error)) {
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
    return prepareGroundItems(
        scenario_world_.groundItems(),
        first_item,
        next_ground_item_id_);
}

bool WorldScene::prepareGroundItems(
    std::vector<GroundItem>& ground_items,
    std::size_t first_item,
    std::int32_t& next_item_id,
    std::string* error) {
    if (first_item > ground_items.size()) {
        if (error) {
            *error =
                "The first ground-item index is invalid.";
        }
        return false;
    }
    const std::int32_t first_id =
        next_item_id;
    for (std::size_t index = first_item;
         index < ground_items.size();
         ++index) {
        GroundItem& item = ground_items[index];
        const ItemDefinition* definition =
            item_database_.find(
                item.item.category,
                item.item.definition_id);
        if (!definition) {
            if (error) {
                *error =
                    "Ground-item category " +
                    std::to_string(item.item.category) +
                    ", definition " +
                    std::to_string(
                        item.item.definition_id) +
                    " is absent from Item.Ibn.";
            }
            ground_items.resize(first_item);
            next_item_id = first_id;
            return false;
        }
        if (item.item.retail_state.empty()) {
            item.item = makeInventoryItem(
                *definition, item.item.quantity);
        }
        std::string resource_error;
        if (!ensureItemWorldResource(
                definition->ground_resource_id,
                &resource_error)) {
            if (error) {
                *error =
                    "Ground-item category " +
                    std::to_string(item.item.category) +
                    ", definition " +
                    std::to_string(
                        item.item.definition_id) +
                    " could not load resource " +
                    std::to_string(
                        definition->ground_resource_id) +
                    ": " + resource_error;
            }
            ground_items.resize(first_item);
            next_item_id = first_id;
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
        item.id = next_item_id++;
    }
    return true;
}

void WorldScene::releaseUnusedItemWorldResources() {
    std::vector<std::uint8_t> required(
        item_world_resources_.size(), 0);
    for (const GroundItem& item : scenario_world_.groundItems()) {
        if (item.resource_id >= 0 &&
            static_cast<std::size_t>(item.resource_id) <
                required.size()) {
            required[static_cast<std::size_t>(item.resource_id)] = 1;
        }
    }
    for (std::size_t index = 0;
         index < item_world_resources_.size();
         ++index) {
        if (required[index] == 0) {
            item_world_resources_[index].reset();
        }
    }
    while (!item_world_resources_.empty() &&
           !item_world_resources_.back()) {
        item_world_resources_.pop_back();
    }
    item_world_resources_.shrink_to_fit();
}

}  // namespace osf
