#ifndef OPENSHADOWFLARE_ITEM_AUDIO_HPP
#define OPENSHADOWFLARE_ITEM_AUDIO_HPP

#include <cstdint>

namespace osf {

struct ItemDefinition;

std::int32_t retailItemMoveSound(
    const ItemDefinition& definition);
std::int32_t retailItemEquipSound(
    const ItemDefinition& definition);
std::int32_t retailItemLandingSound(
    const ItemDefinition& definition);

}  // namespace osf

#endif
