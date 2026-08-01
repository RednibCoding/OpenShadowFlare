#include "miss_effect_actor.hpp"

#include "core/retail_integer.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"

namespace osf {
namespace {

constexpr std::int32_t kBaseHeight = 400;

}  // namespace

bool MissEffectActor::initialize(
    WorldPosition position,
    ObjectBounds judgement,
    const gapi::NjpImage& patterns) {
    *this = {};
    if (patterns.patterns().empty()) {
        return false;
    }
    position_ = position;
    judgement_ = judgement;
    height_ = kBaseHeight;
    vertical_velocity_ = 500;
    acceleration_ = -100;
    opacity_ = 1000;
    fade_strength_ = 1000;
    patterns_ = &patterns;
    return true;
}

void MissEffectActor::update() {
    if (expired_ || !patterns_) {
        return;
    }

    height_ = retailAdd(
        height_, vertical_velocity_ / 10);
    if (height_ < kBaseHeight) {
        height_ = kBaseHeight;
        if (bounce_phase_ == 0) {
            vertical_velocity_ = 300;
        }
        if (bounce_phase_ == 1) {
            vertical_velocity_ = 200;
        }
        if (bounce_phase_ != 3) {
            bounce_phase_ =
                retailAdd(bounce_phase_, 1);
        }
    }
    vertical_velocity_ =
        retailAdd(vertical_velocity_, acceleration_);

    if (bounce_phase_ == 3) {
        opacity_ = fade_strength_;
        fade_strength_ =
            retailAdd(fade_strength_, -100);
        if (fade_strength_ == 0) {
            expired_ = true;
        }
    }
}

WorldPosition MissEffectActor::position() const {
    return position_;
}

const ObjectBounds& MissEffectActor::judgement() const {
    return judgement_;
}

std::int32_t MissEffectActor::height() const {
    return height_;
}

std::int32_t MissEffectActor::opacity() const {
    return opacity_;
}

std::int32_t MissEffectActor::bouncePhase() const {
    return bounce_phase_;
}

bool MissEffectActor::expired() const {
    return expired_;
}

const gapi::NjpImage& MissEffectActor::patterns() const {
    return *patterns_;
}

}  // namespace osf
