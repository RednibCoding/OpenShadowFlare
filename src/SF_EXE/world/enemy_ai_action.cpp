#include "enemy_ai_action.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace osf {
namespace {

constexpr std::int32_t kWaitAction = 0;
constexpr std::int32_t kPatrolAction = 1;
constexpr std::int32_t kFirstPresentationAction = 2;
constexpr std::int32_t kLastPresentationAction = 8;
constexpr std::int32_t kRetreatAction = 9;
constexpr std::int32_t kApproachAction = 10;
constexpr std::int32_t kReturnToWalkPointAction = 11;
constexpr std::int32_t kLastSupportedAction =
    kReturnToWalkPointAction;
constexpr std::int32_t kIdlePresentation = 7;
constexpr std::int32_t kWalkPresentation = 8;
constexpr std::int32_t kWaitEvent = 11;
constexpr std::int32_t kPatrolEvent = 12;
constexpr std::int32_t kRetreatEvent = 14;
constexpr std::int32_t kApproachEvent = 15;
constexpr std::int32_t kWalkPointDuration = 90;
constexpr std::int32_t kWalkPointStopDistance = 150;
constexpr std::int32_t kRetreatStopDistance = 10000;

constexpr std::size_t kActionDuration = 1;
constexpr std::size_t kMovementSpeed = 3;
constexpr std::size_t kPatrolMovementDuration = 4;
constexpr std::size_t kPatrolIdleDuration = 5;
constexpr std::size_t kTargetRefreshInterval = 7;
constexpr std::size_t kRandomTurnChance = 8;
constexpr std::size_t kTargetConditionEnabled = 3;
constexpr std::size_t kMinimumTargetDistance = 4;
constexpr std::size_t kMaximumTargetDistance = 5;

bool absoluteRange(
    std::int32_t origin,
    std::int32_t minimum_offset,
    std::int32_t maximum_offset,
    std::int32_t& minimum,
    std::int32_t& maximum) {
    const std::int64_t calculated_minimum =
        static_cast<std::int64_t>(origin) +
        minimum_offset;
    const std::int64_t calculated_maximum =
        static_cast<std::int64_t>(origin) +
        maximum_offset;
    if (calculated_minimum <
            std::numeric_limits<std::int32_t>::min() ||
        calculated_minimum >
            std::numeric_limits<std::int32_t>::max() ||
        calculated_maximum <
            std::numeric_limits<std::int32_t>::min() ||
        calculated_maximum >
            std::numeric_limits<std::int32_t>::max()) {
        return false;
    }
    minimum =
        static_cast<std::int32_t>(calculated_minimum);
    maximum =
        static_cast<std::int32_t>(calculated_maximum);
    return true;
}

bool scaledSpeed(
    std::int32_t authored_speed,
    std::int32_t scale,
    std::int32_t& speed) {
    const std::int64_t calculated =
        static_cast<std::int64_t>(authored_speed) *
        scale / 1000;
    if (calculated <
            std::numeric_limits<std::int32_t>::min() ||
        calculated >
            std::numeric_limits<std::int32_t>::max()) {
        return false;
    }
    speed = static_cast<std::int32_t>(calculated);
    return true;
}

bool validTarget(const EnemyAiTarget& target) {
    return !target.found ||
           target.kind == MovementTargetKind::player ||
           target.kind ==
               MovementTargetKind::scenario_actor;
}

}  // namespace

void EnemyAiActionController::reset() {
    action_ = {};
    current_action_ = -1;
    event_number_ = 0;
    action_counter_ = 0;
    patrol_counter_ = 0;
}

void EnemyAiActionController::select(
    const AiActionData& action) {
    action_ = action;
    current_action_ = -1;
    event_number_ = -1;
}

EnemyAiActionUpdate EnemyAiActionController::update(
    const EnemyAiActionContext& context) {
    EnemyAiActionUpdate result;
    const std::int32_t action_number =
        action_.action_number;
    if (action_number < kWaitAction ||
        action_number > kLastSupportedAction) {
        return result;
    }

    result.handled = true;
    std::int32_t next_current_action =
        current_action_;
    std::int32_t next_action_counter =
        action_counter_;
    std::int32_t next_patrol_counter =
        patrol_counter_;
    const bool entering =
        current_action_ != action_number;
    if (entering) {
        next_current_action = action_number;
        next_action_counter = 0;
    }

    if (action_number >= kFirstPresentationAction &&
        action_number <= kLastPresentationAction) {
        if (entering &&
            action_number < kLastPresentationAction) {
            result.clear_current_presentation = true;
            result.requested_presentation_action =
                action_number - 1;
        }
    } else if (action_number == kWaitAction) {
        if (entering &&
            context.presentation_action !=
                kIdlePresentation) {
            result.requested_presentation_action =
                kIdlePresentation;
        }
        event_number_ =
            action_.parameters[kActionDuration] <=
                    next_action_counter
                ? 0
                : kWaitEvent;
    } else if (action_number == kPatrolAction) {
        const std::int32_t movement_duration =
            action_.parameters[kPatrolMovementDuration];
        const std::int32_t idle_duration =
            action_.parameters[kPatrolIdleDuration];
        const std::int64_t complete_cycle =
            static_cast<std::int64_t>(
                movement_duration) +
            idle_duration;
        if (complete_cycle <
                std::numeric_limits<std::int32_t>::min() ||
            complete_cycle >
                std::numeric_limits<std::int32_t>::max()) {
            result.handled = false;
            return result;
        }
        const std::int32_t cycle_duration =
            static_cast<std::int32_t>(complete_cycle);

        if (entering) {
            next_patrol_counter = cycle_duration;
        } else if (
            context.presentation_action !=
                kWalkPresentation &&
            next_patrol_counter < movement_duration) {
            next_patrol_counter = movement_duration;
        }

        if (next_patrol_counter == cycle_duration) {
            std::int32_t minimum_x = 0;
            std::int32_t maximum_x = 0;
            std::int32_t minimum_y = 0;
            std::int32_t maximum_y = 0;
            std::int32_t speed = 0;
            if (!scaledSpeed(
                    action_.parameters[kMovementSpeed],
                    context.movement_speed_scale,
                    speed)) {
                result.handled = false;
                return result;
            }
            result.movement.mode =
                MovementDestinationMode::patrol;
            if (!absoluteRange(
                    context.spawn_position.x,
                    context.patrol_bounds.left,
                    context.patrol_bounds.right,
                    minimum_x,
                    maximum_x) ||
                !absoluteRange(
                    context.spawn_position.y,
                    context.patrol_bounds.top,
                    context.patrol_bounds.bottom,
                    minimum_y,
                    maximum_y)) {
                result.handled = false;
                return result;
            }
            result.movement.destination_bounds = {
                minimum_x,
                minimum_y,
                maximum_x,
                maximum_y,
            };
            result.movement.speed = speed;
            result.movement.duration =
                movement_duration;
            if (context.presentation_action !=
                kWalkPresentation) {
                result.requested_presentation_action =
                    kWalkPresentation;
            }
            next_patrol_counter = 0;
        }

        event_number_ =
            action_.parameters[kActionDuration] <=
                    next_action_counter
                ? 1
                : kPatrolEvent;
    } else if (
        action_number == kRetreatAction ||
        action_number == kApproachAction) {
        const std::int32_t completion_event =
            action_number;
        const std::int32_t holding_event =
            action_number == kRetreatAction
            ? kRetreatEvent
            : kApproachEvent;
        if (!entering &&
            context.presentation_action !=
                kWalkPresentation) {
            event_number_ = completion_event;
        } else if (entering) {
            std::int32_t speed = 0;
            if (!scaledSpeed(
                    action_.parameters[kMovementSpeed],
                    context.movement_speed_scale,
                    speed)) {
                result.handled = false;
                return result;
            }

            EnemyAiTarget target;
            if (action_.conditions[
                    kTargetConditionEnabled] == 1) {
                if (context.target_in_range) {
                    target = context.target_in_range(
                        action_.conditions[
                            kMinimumTargetDistance],
                        action_.conditions[
                            kMaximumTargetDistance]);
                }
            } else if (context.default_target) {
                target = context.default_target();
            }

            if (!validTarget(target)) {
                result.handled = false;
                return result;
            }
            if (!target.found) {
                event_number_ = completion_event;
            } else {
                result.movement.mode =
                    action_number == kRetreatAction
                    ? (target.kind ==
                               MovementTargetKind::player
                           ? MovementDestinationMode::
                                 retreat_from_player
                           : MovementDestinationMode::
                                 retreat_from_scenario_actor)
                    : (target.kind ==
                               MovementTargetKind::player
                           ? MovementDestinationMode::
                                 approach_player
                           : MovementDestinationMode::
                                 approach_scenario_actor);
                result.movement.target_identifier =
                    target.identifier;
                result.movement.speed = speed;
                result.movement.stop_distance =
                    action_number == kRetreatAction
                    ? kRetreatStopDistance
                    : 0;
                result.movement.random_turn_chance =
                    action_.parameters[
                        kRandomTurnChance];
                result.movement.target_refresh_interval =
                    action_.parameters[
                        kTargetRefreshInterval];
                if (context.presentation_action !=
                    kWalkPresentation) {
                    result.requested_presentation_action =
                        kWalkPresentation;
                }
                event_number_ =
                    action_.parameters[kActionDuration] <=
                            next_action_counter
                        ? completion_event
                        : holding_event;
            }
        } else {
            event_number_ =
                action_.parameters[kActionDuration] <=
                        next_action_counter
                    ? completion_event
                    : holding_event;
        }
    } else if (
        action_number == kReturnToWalkPointAction) {
        if (!entering &&
            context.presentation_action !=
                kWalkPresentation) {
            event_number_ = 0;
        } else {
            if (entering) {
                std::int32_t speed = 0;
                if (!scaledSpeed(
                        context.walk_point_speed,
                        context.movement_speed_scale,
                        speed)) {
                    result.handled = false;
                    return result;
                }
                result.movement.mode =
                    MovementDestinationMode::fixed_point;
                result.movement.destination =
                    context.walk_point;
                result.movement.speed = speed;
                result.movement.stop_distance =
                    kWalkPointStopDistance;
                if (context.presentation_action !=
                    kWalkPresentation) {
                    result.requested_presentation_action =
                        kWalkPresentation;
                }
            }
            event_number_ =
                next_action_counter >
                        kWalkPointDuration
                    ? 0
                    : -1;
        }
    }

    current_action_ = next_current_action;
    action_counter_ = next_action_counter;
    patrol_counter_ = next_patrol_counter;
    result.event_number = event_number_;
    ++action_counter_;
    ++patrol_counter_;
    return result;
}

std::int32_t EnemyAiActionController::currentAction() const {
    return current_action_;
}

std::int32_t EnemyAiActionController::eventNumber() const {
    return event_number_;
}

std::int32_t EnemyAiActionController::actionCounter() const {
    return action_counter_;
}

std::int32_t EnemyAiActionController::patrolCounter() const {
    return patrol_counter_;
}

const AiActionData&
EnemyAiActionController::actionData() const {
    return action_;
}

}  // namespace osf
