#ifndef OPENSHADOWFLARE_MOVEMENT_DESTINATION_SELECTOR_HPP
#define OPENSHADOWFLARE_MOVEMENT_DESTINATION_SELECTOR_HPP

#include "core/retail_random.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>
#include <functional>

namespace osf {

enum class MovementTargetKind : std::int32_t {
    none = -1,
    player = 0,
    scenario_actor = 1,
};

enum class MovementDestinationMode : std::int32_t {
    none = -1,
    fixed_point = 0,
    approach_scenario_actor = 1,
    retreat_from_scenario_actor = 2,
    patrol = 3,
    approach_player = 4,
    retreat_from_player = 5,
    rectangle_edge = 6,
};

struct MovementTargetState {
    bool found = false;
    WorldPosition position;
    ObjectBounds bounds;
};

using MovementTargetResolver =
    std::function<MovementTargetState(
        MovementTargetKind kind,
        std::int32_t identifier)>;

struct MovementDestinationRequest {
    MovementDestinationMode mode =
        MovementDestinationMode::none;
    WorldPosition destination;
    ObjectBounds destination_bounds;
    std::int32_t target_identifier = -1;
    std::int32_t speed = 0;
    std::int32_t stop_distance = 0;
    std::int32_t duration = 0;
    std::int32_t random_turn_chance = 0;
    std::int32_t target_refresh_interval = 0;
};

struct MovementDestinationContext {
    WorldPosition position;
    ObjectBounds bounds;
    MovementTargetResolver resolve_target;
};

struct MovementDestinationResult {
    bool active = false;
    WorldPosition destination;
};

class MovementDestinationSelector {
public:
    void reset();
    void initialize(
        const MovementDestinationRequest& request,
        WorldPosition position);
    MovementDestinationResult update(
        const MovementDestinationContext& context,
        RetailRandom& random);

    const MovementDestinationRequest& request() const;
    WorldPosition destination() const;
    WorldPosition targetPosition() const;
    std::int32_t counter() const;

private:
    MovementDestinationRequest request_;
    WorldPosition destination_;
    WorldPosition target_position_;
    std::int32_t counter_ = 0;
};

}  // namespace osf

#endif
