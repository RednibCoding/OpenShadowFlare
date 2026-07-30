#include "item_audio.hpp"

#include "item_database.hpp"

namespace osf {

std::int32_t retailItemMoveSound(
    const ItemDefinition& definition) {
    // FUN_00466110 selects the inventory pickup/drop sample from the
    // concrete item category and weight.
    if (definition.category == 2) {
        return 93;
    }
    if (definition.category == 4 &&
        definition.id == 0) {
        return 85;
    }
    return definition.weight < 60 ? 48 : 47;
}

std::int32_t retailItemEquipSound(
    const ItemDefinition& definition) {
    return definition.category == 2 ? 93 : 49;
}

std::int32_t retailItemLandingSound(
    const ItemDefinition& definition) {
    // FUN_00466110 selector 2 is used on the first ground impact.
    if (definition.category == 2) {
        return 93;
    }
    if (definition.category == 4 &&
        definition.id == 0) {
        return 85;
    }
    return 15;
}

}  // namespace osf
