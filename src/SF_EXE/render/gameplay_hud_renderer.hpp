#ifndef OPENSHADOWFLARE_GAMEPLAY_HUD_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_HUD_RENDERER_HPP

#include <cstdint>

namespace osf {

class PlayerData;
class CompanionActor;
struct PlayerRuntimeProfile;
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
    bool increased_power_ready = false;
    bool increased_power_activation_feedback = false;
    std::int32_t animation_counter = 0;
    bool companion_present = false;
    std::int32_t companion_current_life = 0;
    std::int32_t companion_maximum_life = 0;
    bool companion_inactive = true;
};

GameplayHudValues gameplayHudValues(
    const PlayerData& player,
    const PlayerRuntimeProfile& profile,
    MovementPace movement_pace,
    std::int32_t experience_threshold,
    std::int32_t current_life,
    std::int32_t current_mana,
    bool increased_power_ready = false,
    bool increased_power_activation_feedback = false,
    std::int32_t animation_counter = 0,
    const CompanionActor* companion = nullptr,
    bool companion_inactive = true);

std::int32_t gameplayHudBarWidth(
    std::int32_t current,
    std::int32_t maximum);
std::int32_t gameplayHudExperienceBarWidth(
    std::int32_t experience,
    std::int32_t threshold);
std::int32_t gameplayHudCompanionBarWidth(
    std::int32_t current,
    std::int32_t maximum);

void renderGameplayHud(
    gapi::Backend& renderer,
    const gapi::NjpImage& bar_patterns,
    const GameplayHudValues& values);

}  // namespace osf

#endif
