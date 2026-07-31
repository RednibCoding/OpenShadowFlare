#ifndef OPENSHADOWFLARE_COMPANION_RESPAWN_HPP
#define OPENSHADOWFLARE_COMPANION_RESPAWN_HPP

#include <cstdint>

namespace osf {

class PlayerInventory;

std::int32_t retailCompanionRespawnUpdates(
    const PlayerInventory& inventory);

}  // namespace osf

#endif
