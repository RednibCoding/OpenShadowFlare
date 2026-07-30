#ifndef OPENSHADOWFLARE_COMBAT_EFFECT_ACTOR_HPP
#define OPENSHADOWFLARE_COMBAT_EFFECT_ACTOR_HPP

#include "combat_effect_request.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstddef>
#include <cstdint>

namespace osf {

class EffectVisualResource;

std::int32_t retailCombatEffectResourceId(
    std::int32_t effect_number);

class CombatEffectActor {
public:
    bool initialize(
        const CombatEffectSpawnRequest& request,
        WorldPosition position,
        ObjectBounds judgement,
        const EffectVisualResource& visual);
    void update();

    std::int32_t effectNumber() const;
    std::int32_t resourceId() const;
    WorldPosition position() const;
    const ObjectBounds& judgement() const;
    std::int32_t animationChart() const;
    std::int32_t direction() const;
    std::int32_t animationFrame() const;
    std::int32_t displayHeight() const;
    std::int32_t drawStrength() const;
    bool expired() const;
    bool partEnabled(std::size_t part) const;
    const gapi::NjpImage& patterns() const;
    const gapi::CafAnimation& animation() const;

private:
    std::int32_t effect_number_ = -1;
    std::int32_t resource_id_ = -1;
    WorldPosition position_;
    ObjectBounds judgement_;
    std::int32_t direction_ = 8;
    std::int32_t animation_frame_ = 0;
    std::int32_t counter_ = 0;
    std::int32_t duration_ = 0;
    std::int32_t display_height_ = 0;
    std::int32_t draw_strength_ = 1000;
    bool fixed_duration_ = false;
    bool expired_ = false;
    const EffectVisualResource* visual_ = nullptr;
};

}  // namespace osf

#endif
