#ifndef OPENSHADOWFLARE_ENEMY_PRESENTATION_HPP
#define OPENSHADOWFLARE_ENEMY_PRESENTATION_HPP

#include "enemy_presentation_profile.hpp"
#include "enemy_target_selector.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <array>
#include <cstdint>

namespace osf {

constexpr std::uint8_t kEnemyAudioMarkerZero = 1u << 0u;
constexpr std::uint8_t kEnemyAudioMarkerOne = 1u << 1u;
constexpr std::uint8_t kEnemyAudioMarkerTwo = 1u << 2u;

enum class EnemyPresentationFamily : std::int32_t {
    direct = 0,
    effect = 1,
};

struct EnemyPresentationContext {
    WorldPosition position;
    std::int32_t direction = 0;
    std::int32_t event_number = -1;
    std::int32_t resource_id = -1;
    const EnemyPresentationProfile* profile = nullptr;
    const gapi::CafAnimation* animation = nullptr;
    EnemyTargetSearch target_in_range;
    EnemyDefaultTargetSearch default_target;
};

struct EnemyPresentationUpdate {
    bool handled = false;
    bool active = false;
    std::int32_t presentation_action = 7;
    std::int32_t animation_chart = 0;
    std::int32_t animation_frame = 0;
    std::int32_t direction = 0;
    std::uint8_t audio_markers = 0;
    std::array<std::int32_t, 3> audio_samples{{
        -1, -1, -1}};
    bool impact = false;
    EnemyPresentationFamily impact_family =
        EnemyPresentationFamily::direct;
    std::int32_t impact_variant = -1;
    std::int32_t effect_type = 0;
    std::int32_t effect_subtype = 0;
    std::int32_t effect_parameter = 0;
    std::int32_t effect_additive = 0;
    EnemyAiTarget target;
    std::int32_t completion_event = -1;
};

class EnemyPresentationController {
public:
    void reset();
    void select(std::int32_t presentation_action);
    EnemyPresentationUpdate update(
        const EnemyPresentationContext& context);

    std::int32_t presentationAction() const;
    std::int32_t animationChart() const;
    std::int32_t animationFrame() const;
    std::int32_t direction() const;
    std::int32_t elapsedUpdates() const;
    std::int32_t previousAnimationFrame() const;
    const EnemyAiTarget& target() const;

private:
    std::int32_t requested_action_ = 7;
    std::int32_t presentation_action_ = 7;
    std::int32_t animation_chart_ = 0;
    std::int32_t animation_frame_ = 0;
    std::int32_t direction_ = 0;
    std::int32_t elapsed_updates_ = 0;
    std::int32_t previous_animation_frame_ = -1;
    EnemyAiTarget target_;
};

}  // namespace osf

#endif
