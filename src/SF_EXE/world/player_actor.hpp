#ifndef OPENSHADOWFLARE_PLAYER_ACTOR_HPP
#define OPENSHADOWFLARE_PLAYER_ACTOR_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>

namespace osf {

enum class PlayerMotion {
    idle,
    walking,
    running,
};

enum class MovementPace {
    walk,
    run,
};

std::int32_t retailDirectionForVector(
    std::int32_t x,
    std::int32_t y);

class PlayerActor {
public:
    void reset(
        WorldPosition position,
        std::int32_t direction,
        std::int32_t walking_speed_tier = 5);
    void clear();

    void moveTo(WorldPosition destination);
    void cancelMovement();
    void toggleMovementPace();
    void update(
        const GroundMap& ground,
        const ObjectMap& objects);

    WorldPosition position() const;
    WorldPosition destination() const;
    std::int32_t direction() const;
    std::int32_t walkingSpeedTier() const;
    std::int32_t walkingSpeed() const;
    std::int32_t runningSpeed() const;
    MovementPace movementPace() const;
    PlayerMotion motion() const;
    std::int32_t animationChart() const;
    std::int32_t animationFrame() const;

private:
    WorldPosition position_;
    WorldPosition destination_;
    ObjectBounds judgement_{-80, -80, 79, 79};
    std::int32_t direction_ = 0;
    std::int32_t walking_speed_tier_ = 5;
    std::int32_t walking_speed_ = 20;
    std::int32_t running_speed_ = 40;
    std::int32_t action_counter_ = 0;
    std::int32_t animation_chart_ = 0;
    std::int32_t animation_frame_ = 0;
    MovementPace movement_pace_ = MovementPace::walk;
    PlayerMotion motion_ = PlayerMotion::idle;
    PlayerMotion previous_action_ = PlayerMotion::idle;
};

}  // namespace osf

#endif
