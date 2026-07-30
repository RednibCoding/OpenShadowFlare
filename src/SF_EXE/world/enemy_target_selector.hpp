#ifndef OPENSHADOWFLARE_ENEMY_TARGET_SELECTOR_HPP
#define OPENSHADOWFLARE_ENEMY_TARGET_SELECTOR_HPP

#include "movement_destination_selector.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace osf {

constexpr std::size_t kEnemyPlayerTargetCount = 4;
constexpr std::int32_t kFirstCompanionCharacterNumber =
    16000000;
constexpr std::size_t kEnemyCompanionTargetCount = 4;

enum class EnemyTargetLifeRequirement : std::int32_t {
    ignore = 0,
    living = 1,
};

struct EnemyPlayerTargetState {
    bool present = false;
    std::int32_t active_state = 0;
    std::int32_t scenario_id = -1;
    std::int32_t current_life = 0;
    WorldPosition position;
    ObjectBounds bounds;
};

struct EnemyCompanionTargetState {
    bool present = false;
    std::int32_t character_number = -1;
    std::int32_t scenario_id = -1;
    bool script_active = false;
    std::int32_t current_life = 0;
    std::int32_t owner_mode = 0;
    WorldPosition position;
    ObjectBounds bounds;
};

struct EnemyTargetSearchContext {
    std::int32_t scenario_id = -1;
    WorldPosition position;
    ObjectBounds bounds;
    std::array<
        EnemyPlayerTargetState,
        kEnemyPlayerTargetCount>
        players;
    std::vector<EnemyCompanionTargetState> companions;
};

struct EnemyAiTarget {
    bool found = false;
    MovementTargetKind kind = MovementTargetKind::none;
    std::int32_t identifier = -1;
    std::int32_t distance = 0;
};

using EnemyTargetSearch =
    std::function<EnemyAiTarget(
        std::int32_t minimum_distance,
        std::int32_t maximum_distance)>;
using EnemyDefaultTargetSearch =
    std::function<EnemyAiTarget()>;

EnemyAiTarget findEnemyTargetInRange(
    const EnemyTargetSearchContext& context,
    std::int32_t minimum_distance,
    std::int32_t maximum_distance,
    EnemyTargetLifeRequirement life_requirement);

EnemyAiTarget findDefaultEnemyTarget(
    const EnemyTargetSearchContext& context,
    EnemyTargetLifeRequirement life_requirement);

}  // namespace osf

#endif
