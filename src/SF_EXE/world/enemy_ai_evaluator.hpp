#ifndef OPENSHADOWFLARE_ENEMY_AI_EVALUATOR_HPP
#define OPENSHADOWFLARE_ENEMY_AI_EVALUATOR_HPP

#include "core/retail_random.hpp"
#include "enemy_target_selector.hpp"
#include "libs/RKC_RPG_AICONTROL/rkc_rpg_aicontrol.hpp"

#include <cstdint>

namespace osf {

struct EnemyAiEvaluationContext {
    std::int32_t current_life = 0;
    std::int32_t maximum_life = 0;
    EnemyTargetSearch target_in_range;
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
