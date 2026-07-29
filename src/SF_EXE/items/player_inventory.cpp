#include "player_inventory.hpp"

#include <algorithm>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kGoldCategory = 4;
constexpr std::int32_t kGoldDefinition = 0;

}  // namespace

void PlayerInventory::clear() {
    items_.clear();
}

bool PlayerInventory::add(
    std::int32_t category,
    std::int32_t definition_id,
    std::int32_t quantity) {
    if (quantity <= 0) {
        return false;
    }

    std::int32_t remaining = quantity;
    if (category == kGoldCategory &&
        definition_id == kGoldDefinition) {
        for (InventoryItem& owned : items_) {
            if (owned.category != category ||
                owned.definition_id != definition_id ||
                owned.quantity >= maximum_gold_stack) {
                continue;
            }
            const std::int32_t moved = std::min(
                remaining,
                maximum_gold_stack - owned.quantity);
            owned.quantity += moved;
            remaining -= moved;
            if (remaining == 0) {
                return true;
            }
        }
    }

    const std::int32_t stack_limit =
        category == kGoldCategory &&
                definition_id == kGoldDefinition
            ? maximum_gold_stack
            : 1;
    while (remaining > 0) {
        const std::int32_t quantity =
            std::min(remaining, stack_limit);
        items_.push_back({
            category,
            definition_id,
            quantity,
        });
        remaining -= quantity;
    }
    return true;
}

const std::vector<InventoryItem>&
PlayerInventory::items() const {
    return items_;
}

}  // namespace osf
