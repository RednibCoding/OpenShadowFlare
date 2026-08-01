#ifndef OPENSHADOWFLARE_GAMEPLAY_BLACKJACK_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_BLACKJACK_RENDERER_HPP

#include <cstdint>

namespace osf {

class GameplayBlackjack;
class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}

void renderGameplayBlackjack(
    gapi::Backend& renderer,
    const gapi::NjpImage& card_patterns,
    const gapi::NjpImage& status_patterns,
    const GameplayBlackjack& blackjack,
    const WorldScene& world,
    std::uint32_t gameplay_counter);

}  // namespace osf

#endif
