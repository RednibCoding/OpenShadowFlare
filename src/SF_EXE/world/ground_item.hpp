#ifndef OPENSHADOWFLARE_GROUND_ITEM_HPP
#define OPENSHADOWFLARE_GROUND_ITEM_HPP

#include "core/retail_random.hpp"
#include "items/player_inventory.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "scenario_entity_state.hpp"

#include <cstdint>
#include <vector>

namespace osf {

struct ScenarioItem;

struct GroundItem {
    InventoryItem item;
    WorldPosition position;
    std::int32_t resource_id = -1;
    std::int32_t animation_chart = -1;
    std::int32_t height = 0;
    std::int32_t vertical_velocity = 1600;
    std::int32_t vertical_gravity = 280;
    std::int32_t bounce_state = 0;
    std::int32_t red_strength = 1000;
    std::int32_t green_strength = 1000;
    std::int32_t blue_strength = 1000;
    std::int32_t id = -1;
    std::int32_t scenario_character_number = -1;
    ObjectBounds judgement{-20, -20, 19, 19};
    ScenarioEntityState state;

    bool visible() const;
    bool pointerEnabled() const;
    bool judgementEnabled() const;
};

enum class GroundItemUpdateEvent {
    none,
    first_impact,
};

bool createGroundItem(
    std::vector<GroundItem>& items,
    InventoryItem item,
    WorldPosition position);
bool createGroundItem(
    std::vector<GroundItem>& items,
    std::int32_t category,
    std::int32_t definition_id,
    WorldPosition position,
    std::int32_t quantity = 1);
bool createGroundItems(
    std::vector<GroundItem>& items,
    RetailRandom& random,
    std::int32_t category,
    std::int32_t definition_id,
    WorldPosition position,
    std::int32_t minimum_quantity,
    std::int32_t maximum_quantity);
bool createScenarioGroundItem(
    std::vector<GroundItem>& items,
    RetailRandom& random,
    const ScenarioItem& source);
void restartGroundItemDrop(GroundItem& item);
GroundItemUpdateEvent updateGroundItem(GroundItem& item);

}  // namespace osf

#endif
