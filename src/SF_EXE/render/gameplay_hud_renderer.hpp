#ifndef OPENSHADOWFLARE_GAMEPLAY_HUD_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_HUD_RENDERER_HPP

#include <cstdint>

namespace osf {

class PlayerData;
enum class MovementPace;

namespace gapi {
class Backend;
class NjpImage;
}

struct GameplayHudValues {
    std::int32_t level = 1;
    std::int32_t current_life = 0;
    std::int32_t maximum_life = 0;
    std::int32_t current_mana = 0;
    std::int32_t maximum_mana = 0;
    std::int32_t experience = 0;
    std::int32_t experience_threshold = 0;
    bool running = false;
};

GameplayHudValues gameplayHudValues(
    const PlayerData& player,
    MovementPace movement_pace,
    std::int32_t experience_threshold);

std::int32_t gameplayHudBarWidth(
    std::int32_t current,
    std::int32_t maximum);
std::int32_t gameplayHudExperienceBarWidth(
    std::int32_t experience,
    std::int32_t threshold);

void renderGameplayHud(
    gapi::Backend& renderer,
    const gapi::NjpImage& bar_patterns,
    const GameplayHudValues& values);

}  // namespace osf

#endif
