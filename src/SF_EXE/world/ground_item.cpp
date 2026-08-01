#include "ground_item.hpp"
#include "scenario_data.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kGoldCategory = 4;
constexpr std::int32_t kGoldDefinition = 0;
constexpr std::int32_t kMaximumGoldStack = 10000;
constexpr double kGoldDropRadius = 200.0;
constexpr double kGoldDropAngleStep = 0.3141592;
constexpr std::int32_t kScenarioItemCharacterBase = 18000000;
const std::vector<std::int32_t> kEnabledState{1, 1, 1};

}  // namespace

bool createGroundItem(
    std::vector<GroundItem>& items,
    InventoryItem owned_item,
    WorldPosition position) {
    const bool gold =
        owned_item.category == kGoldCategory &&
        owned_item.definition_id == kGoldDefinition;
    if (owned_item.quantity <= 0 ||
        (!gold && owned_item.quantity != 1)) {
        return false;
    }
    GroundItem item;
    item.item = std::move(owned_item);
    item.position = position;
    item.state.initialize(kEnabledState);
    items.push_back(std::move(item));
    return true;
}

bool createGroundItem(
    std::vector<GroundItem>& items,
    std::int32_t category,
    std::int32_t definition_id,
    WorldPosition position,
    std::int32_t quantity) {
    InventoryItem item;
    item.category = category;
    item.definition_id = definition_id;
    item.quantity = quantity;
    return createGroundItem(
        items, std::move(item), position);
}

bool createGroundItems(
    std::vector<GroundItem>& items,
    RetailRandom& random,
    std::int32_t category,
    std::int32_t definition_id,
    WorldPosition position,
    std::int32_t minimum_quantity,
    std::int32_t maximum_quantity) {
    if (category != kGoldCategory ||
        definition_id != kGoldDefinition) {
        return createGroundItem(
            items,
            category,
            definition_id,
            position);
    }

    if (minimum_quantity < 0 ||
        maximum_quantity < minimum_quantity) {
        return false;
    }
    const std::int64_t range =
        static_cast<std::int64_t>(maximum_quantity) -
        minimum_quantity + 1;
    if (range <= 0 ||
        range > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }

    std::int32_t remaining =
        minimum_quantity +
        random.next() % static_cast<std::int32_t>(range);
    double angle = 0.0;
    while (remaining > 0) {
        const std::int32_t quantity =
            std::min(remaining, kMaximumGoldStack);
        const WorldPosition drop_position{
            position.x + static_cast<std::int32_t>(
                             std::cos(angle) * kGoldDropRadius),
            position.y - static_cast<std::int32_t>(
                             std::sin(angle) * kGoldDropRadius),
        };
        if (!createGroundItem(
                items,
                category,
                definition_id,
                drop_position,
                quantity)) {
            return false;
        }
        remaining -= quantity;
        angle += kGoldDropAngleStep;
    }
    return true;
}

bool createScenarioGroundItem(
    std::vector<GroundItem>& items,
    RetailRandom& random,
    const ScenarioItem& source) {
    if (source.id < 0 ||
        source.id >
            std::numeric_limits<std::int32_t>::max() -
                kScenarioItemCharacterBase) {
        return false;
    }

    std::int32_t quantity = 1;
    if (source.category == kGoldCategory &&
        source.definition_id == kGoldDefinition) {
        if (source.minimum_quantity < 0 ||
            source.maximum_quantity <
                source.minimum_quantity) {
            return false;
        }
        const std::int64_t range =
            static_cast<std::int64_t>(
                source.maximum_quantity) -
            source.minimum_quantity + 1;
        if (range <= 0 ||
            range >
                std::numeric_limits<std::int32_t>::max()) {
            return false;
        }
        quantity =
            source.minimum_quantity +
            random.next() %
                static_cast<std::int32_t>(range);
    }

    GroundItem item;
    item.item.category = source.category;
    item.item.definition_id = source.definition_id;
    item.item.quantity = quantity;
    item.position = {source.world_x, source.world_y};
    item.vertical_velocity = 0;
    item.bounce_state = 2;
    item.scenario_character_number =
        kScenarioItemCharacterBase + source.id;
    if (!item.state.initialize(
            source.initial_state_values)) {
        return false;
    }
    items.push_back(std::move(item));
    return true;
}

void restartGroundItemDrop(GroundItem& item) {
    // When FUN_004526a0 cannot insert a picked-up item into the player's
    // owner, it recreates that same item through the ordinary mode-zero
    // world constructor. Keep the instance and position, but restart every
    // field owned by its two-bounce drop presentation.
    item.height = 0;
    item.vertical_velocity = 1600;
    item.vertical_gravity = 280;
    item.bounce_state = 0;
}

GroundItemUpdateEvent updateGroundItem(GroundItem& item) {
    if (item.bounce_state >= 2) {
        return GroundItemUpdateEvent::none;
    }
    item.height += item.vertical_velocity / 10;
    item.vertical_velocity -= item.vertical_gravity;
    if (item.height > 0) {
        return GroundItemUpdateEvent::none;
    }

    item.height = 0;
    if (item.bounce_state == 0) {
        item.bounce_state = 1;
        item.vertical_velocity = 700;
        return GroundItemUpdateEvent::first_impact;
    } else {
        item.bounce_state = 2;
    }
    return GroundItemUpdateEvent::none;
}

bool GroundItem::visible() const {
    return state.visible();
}

bool GroundItem::pointerEnabled() const {
    return state.pointerEnabled();
}

bool GroundItem::judgementEnabled() const {
    return state.judgementEnabled();
}

}  // namespace osf
