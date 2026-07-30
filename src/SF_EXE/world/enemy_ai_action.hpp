#ifndef OPENSHADOWFLARE_ENEMY_AI_ACTION_HPP
#define OPENSHADOWFLARE_ENEMY_AI_ACTION_HPP

#include "core/retail_random.hpp"
#include "libs/RKC_RPG_AICONTROL/rkc_rpg_aicontrol.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>

namespace osf {

struct EnemyAiActionContext {
    WorldPosition spawn_position;
    ObjectBounds patrol_bounds;
    std::int32_t movement_speed_scale = 0;
    std::int32_t presentation_action = 7;
};

struct EnemyAiActionUpdate {
    bool handled = false;
    std::int32_t event_number = -1;
    std::int32_t requested_presentation_action = -1;
    bool begin_patrol = false;
    WorldPosition patrol_destination;
    std::int32_t movement_speed = 0;
    std::int32_t movement_duration = 0;
    std::int32_t movement_animation_chart = 0;
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
