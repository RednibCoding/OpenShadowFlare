#ifndef OPENSHADOWFLARE_ENEMY_AI_ACTION_HPP
#define OPENSHADOWFLARE_ENEMY_AI_ACTION_HPP

#include "core/retail_random.hpp"
#include "libs/RKC_RPG_AICONTROL/rkc_rpg_aicontrol.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>
#include <functional>

namespace osf {

enum class EnemyAiTargetKind : std::int32_t {
    none = -1,
    player = 0,
    scenario_actor = 1,
};

struct EnemyAiTarget {
    bool found = false;
    EnemyAiTargetKind kind = EnemyAiTargetKind::none;
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

enum class EnemyAiMovementMode : std::int32_t {
    none = -1,
    fixed_point = 0,
    approach_scenario_actor = 1,
    retreat_from_scenario_actor = 2,
    patrol = 3,
    approach_player = 4,
    retreat_from_player = 5,
};

struct EnemyAiMovementRequest {
    bool begin = false;
    EnemyAiMovementMode mode =
        EnemyAiMovementMode::none;
    WorldPosition destination;
    EnemyAiTargetKind target_kind =
        EnemyAiTargetKind::none;
    std::int32_t target_identifier = -1;
    std::int32_t speed = 0;
    std::int32_t stop_distance = 0;
    std::int32_t duration = 0;
    std::int32_t animation_chart = 0;
    std::int32_t random_turn_chance = 0;
    std::int32_t target_refresh_interval = 0;
};

struct EnemyAiActionUpdate {
    bool handled = false;
    std::int32_t event_number = -1;
    std::int32_t requested_presentation_action = -1;
    bool clear_current_presentation = false;
    EnemyAiMovementRequest movement;
};

class EnemyAiActionController {
public:
    void reset();
    void select(const AiActionData& action);
    EnemyAiActionUpdate update(
        const EnemyAiActionContext& context,
        RetailRandom& random);

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
