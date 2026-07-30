#include "enemy_ai_evaluator.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {
namespace {

constexpr std::size_t kPriority = 0;
constexpr std::size_t kSelectionWeight = 2;
constexpr std::size_t kLifeConditionEnabled = 0;
constexpr std::size_t kMinimumLifePercent = 1;
constexpr std::size_t kMaximumLifePercent = 2;
constexpr std::size_t kTargetConditionEnabled = 3;
constexpr std::size_t kMinimumTargetDistance = 4;
constexpr std::size_t kMaximumTargetDistance = 5;

bool eligible(
    const AiActionData& action,
    const EnemyAiEvaluationContext& context) {
    if (action.conditions[kTargetConditionEnabled] == 1 &&
        (!context.target_in_range ||
         !context.target_in_range(
             action.conditions[kMinimumTargetDistance],
             action.conditions[kMaximumTargetDistance]))) {
        return false;
    }

    if (action.conditions[kLifeConditionEnabled] != 1) {
        return true;
    }
    if (context.maximum_life <= 0) {
        return false;
    }
    const std::int32_t life_percent =
        static_cast<std::int32_t>(
            static_cast<std::int64_t>(
                context.current_life) *
            100 /
            context.maximum_life);
    const std::int32_t minimum =
        action.conditions[kMinimumLifePercent];
    const std::int32_t maximum =
        action.conditions[kMaximumLifePercent];
    return (minimum == -1 || life_percent >= minimum) &&
           (maximum == -1 || life_percent <= maximum);
}

bool fallsBackToDefault(std::int32_t event_number) {
    return (event_number >= 1 && event_number <= 10) ||
           event_number == 16 ||
           event_number == 17;
}

EnemyAiSelection evaluateOneEvent(
    const AiControlList& control,
    std::int32_t event_number,
    const EnemyAiEvaluationContext& context,
    RetailRandom& random) {
    const AiEventData* event = control.event(event_number);
    if (!event) {
        return {};
    }

    std::int32_t highest_priority = -1;
    std::vector<const AiActionData*> candidates;
    for (const AiActionData& action : event->actions()) {
        if (!eligible(action, context)) {
            continue;
        }
        if (highest_priority < action.parameters[kPriority]) {
            candidates.clear();
            highest_priority =
                action.parameters[kPriority];
        }
        // The DLL event is a linked list. Its evaluator inserts every
        // retained copy at list position zero, so weighted traversal runs
        // in reverse file order. It also keeps a later lower-priority
        // candidate unless another new maximum clears the list.
        candidates.insert(candidates.begin(), &action);
    }

    std::int64_t total_weight = 0;
    for (const AiActionData* action : candidates) {
        total_weight +=
            action->parameters[kSelectionWeight];
    }
    if (total_weight <= 0) {
        return {};
    }

    const std::int64_t draw =
        random.next() % total_weight;
    std::int64_t accumulated_weight = 0;
    for (const AiActionData* action : candidates) {
        accumulated_weight +=
            action->parameters[kSelectionWeight];
        if (draw < accumulated_weight) {
            return {true, *action};
        }
    }
    return {};
}

}  // namespace

EnemyAiSelection evaluateEnemyAiEvent(
    const AiControlList& control,
    std::int32_t event_number,
    const EnemyAiEvaluationContext& context,
    RetailRandom& random) {
    if (event_number == -1) {
        return {};
    }
    EnemyAiSelection selection =
        evaluateOneEvent(
            control, event_number, context, random);
    if (!selection.selected &&
        fallsBackToDefault(event_number)) {
        selection = evaluateOneEvent(
            control, 0, context, random);
    }
    return selection;
}

}  // namespace osf
