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

std::int32_t retailItemAttackSound(
    const ItemDefinition* definition) {
    // FUN_00466110 selector four uses sample one for an empty hand and
    // chooses samples one/two at the exact 60-weight boundary otherwise.
    return definition && definition->weight >= 60 ? 2 : 1;
}

}  // namespace osf
