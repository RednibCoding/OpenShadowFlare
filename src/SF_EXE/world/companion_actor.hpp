#ifndef OPENSHADOWFLARE_COMPANION_ACTOR_HPP
#define OPENSHADOWFLARE_COMPANION_ACTOR_HPP

#include "companion_profile.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "movement_controller.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {

class CharacterVisualResource;

namespace gapi {
class NjpImage;
}

enum class CompanionMotion {
    idle,
    walking,
    running,
};

class CompanionActor {
public:
    bool initialize(
        const CompanionProfile& profile,
        const CharacterVisualResource& visual,
        std::int32_t owner_slot,
        WorldPosition position,
        std::int32_t direction);
    void clear();
    void relocate(
        WorldPosition position,
        std::int32_t direction);
    void updateFollow(
        WorldPosition owner_position,
        const ObjectBounds& owner_bounds,
        const GroundMap& ground,
        const ObjectMap& objects,
        const std::vector<MovementBlocker>*
            dynamic_blockers = nullptr);

    bool valid() const;
    std::int32_t characterNumber() const;
    std::int32_t movementBlockerId() const;
    const CompanionProfile& profile() const;
    WorldPosition position() const;
    WorldPosition renderPosition(double alpha) const;
    const ObjectBounds& judgement() const;
    std::int32_t direction() const;
    CompanionMotion motion() const;
    std::int32_t animationChart() const;
    std::int32_t animationFrame() const;
    std::int32_t currentLife() const;
    std::int32_t maximumLife() const;
    bool partEnabled(std::size_t part) const;
    std::int32_t partRedStrength(std::size_t part) const;
    std::int32_t partGreenStrength(std::size_t part) const;
    std::int32_t partBlueStrength(std::size_t part) const;
    const gapi::NjpImage& patterns() const;
    const gapi::NjpImage& shadowPatterns() const;
    const gapi::CafAnimation& animation() const;
    bool visible() const;
    bool judgementEnabled() const;

private:
    void selectMotion(CompanionMotion motion);

    CompanionProfile profile_;
    std::int32_t owner_slot_ = -1;
    WorldPosition position_;
    WorldPosition previous_position_;
    ObjectBounds judgement_{-80, -80, 79, 79};
    std::int32_t direction_ = 0;
    CompanionMotion motion_ = CompanionMotion::idle;
    std::int32_t action_counter_ = 0;
    std::int32_t close_linger_counter_ = 0;
    std::int32_t current_life_ = 0;
    MovementController movement_controller_;
    const CharacterVisualResource* visual_ = nullptr;
};

}  // namespace osf

#endif
