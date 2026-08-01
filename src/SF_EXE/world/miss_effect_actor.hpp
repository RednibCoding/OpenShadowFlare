#ifndef OPENSHADOWFLARE_MISS_EFFECT_ACTOR_HPP
#define OPENSHADOWFLARE_MISS_EFFECT_ACTOR_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>

namespace osf {

namespace gapi {
class NjpImage;
}

class MissEffectActor {
public:
    bool initialize(
        WorldPosition position,
        ObjectBounds judgement,
        const gapi::NjpImage& patterns);
    void update();

    WorldPosition position() const;
    const ObjectBounds& judgement() const;
    std::int32_t height() const;
    std::int32_t opacity() const;
    std::int32_t bouncePhase() const;
    bool expired() const;
    const gapi::NjpImage& patterns() const;

private:
    WorldPosition position_;
    ObjectBounds judgement_;
    std::int32_t height_ = 400;
    std::int32_t vertical_velocity_ = 500;
    std::int32_t acceleration_ = -100;
    std::int32_t bounce_phase_ = 0;
    std::int32_t opacity_ = 1000;
    std::int32_t fade_strength_ = 1000;
    bool expired_ = false;
    const gapi::NjpImage* patterns_ = nullptr;
};

}  // namespace osf

#endif
