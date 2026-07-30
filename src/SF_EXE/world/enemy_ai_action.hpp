#ifndef OPENSHADOWFLARE_ENEMY_AI_ACTION_HPP
#define OPENSHADOWFLARE_ENEMY_AI_ACTION_HPP

#include "libs/RKC_RPG_AICONTROL/rkc_rpg_aicontrol.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "movement_destination_selector.hpp"

#include <cstdint>
#include <functional>

namespace osf {

struct EnemyAiTarget {
    bool found = false;
    MovementTargetKind kind = MovementTargetKind::none;
    std::int32_t identifier = -1;
};

using EnemyAiTargetSearch =
    std::function<EnemyAiTarget(
        std::int32_t minimum_distance,
        std::int32_t maximum_distance)>;
using EnemyAiDefaultTargetSearch =
    std::function<EnemyAiTarget()>;

struct EnemyAiActionContext {
    WorldPosition spawn_position;
    ObjectBounds patrol_bounds;
    std::int32_t movement_speed_scale = 0;
    std::int32_t presentation_action = 7;
    WorldPosition walk_point;
    std::int32_t walk_point_speed = 0;
    EnemyAiTargetSearch target_in_range;
    EnemyAiDefaultTargetSearch default_target;
};

struct EnemyAiActionUpdate {
    bool handled = false;
    std::int32_t event_number = -1;
    std::int32_t requested_presentation_action = -1;
    bool clear_current_presentation = false;
    MovementDestinationRequest movement;
};

class EnemyAiActionController {
public:
    void reset();
    void select(const AiActionData& action);
    EnemyAiActionUpdate update(
        const EnemyAiActionContext& context);

    std::int32_t currentAction() const;
    std::int32_t eventNumber() const;
    std::int32_t actionCounter() const;
    std::int32_t patrolCounter() const;
    const AiActionData& actionData() const;

private:
    AiActionData action_;
    std::int32_t current_action_ = -1;
    std::int32_t event_number_ = 0;
    std::int32_t action_counter_ = 0;
    std::int32_t patrol_counter_ = 0;
};

}  // namespace osf

#endif
