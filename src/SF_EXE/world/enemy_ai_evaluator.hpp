#ifndef OPENSHADOWFLARE_ENEMY_AI_EVALUATOR_HPP
#define OPENSHADOWFLARE_ENEMY_AI_EVALUATOR_HPP

#include "core/retail_random.hpp"
#include "libs/RKC_RPG_AICONTROL/rkc_rpg_aicontrol.hpp"

#include <cstdint>
#include <functional>

namespace osf {

using EnemyAiTargetQuery =
    std::function<bool(
        std::int32_t minimum_distance,
        std::int32_t maximum_distance)>;

struct EnemyAiEvaluationContext {
    std::int32_t current_life = 0;
    std::int32_t maximum_life = 0;
    EnemyAiTargetQuery target_in_range;
};

struct EnemyAiSelection {
    bool selected = false;
    AiActionData action;
};

EnemyAiSelection evaluateEnemyAiEvent(
    const AiControlList& control,
    std::int32_t event_number,
    const EnemyAiEvaluationContext& context,
    RetailRandom& random);

}  // namespace osf

#endif
