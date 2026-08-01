#include "companion_target_selector.hpp"

#include "actor_direction.hpp"
#include "movement_controller.hpp"

namespace osf {
namespace {

bool nearer(
    const CompanionEnemyTarget& selected,
    std::int32_t distance) {
    return !selected.found ||
           distance < selected.distance;
}

CompanionEnemyTarget selectTarget(
    WorldPosition position,
    const ObjectBounds& judgement,
    const std::vector<CompanionEnemyTargetState>& targets,
    std::int32_t maximum_distance,
    std::int32_t required_direction) {
    CompanionEnemyTarget selected;
    for (const CompanionEnemyTargetState& target : targets) {
        if (!target.active ||
            target.current_life <= 0) {
            continue;
        }
        const std::int32_t distance =
            distanceBetweenBounds(
                position,
                judgement,
                target.position,
                target.judgement);
        if (distance > maximum_distance ||
            !nearer(selected, distance)) {
            continue;
        }
        if (required_direction >= 0 &&
            retailDirectionForVector(
                target.position.x - position.x,
                target.position.y - position.y) !=
                required_direction) {
            continue;
        }
        selected = {
            true,
            target.character_number,
            target.position,
            target.judgement,
            distance,
            target.physical_evasion,
        };
    }
    return selected;
}

}  // namespace

CompanionEnemyTarget findCompanionEnemyTarget(
    WorldPosition position,
    const ObjectBounds& judgement,
    const std::vector<CompanionEnemyTargetState>& targets,
    std::int32_t maximum_distance) {
    return selectTarget(
        position,
        judgement,
        targets,
        maximum_distance,
        -1);
}

CompanionEnemyTarget findCompanionForwardEnemyTarget(
    WorldPosition position,
    const ObjectBounds& judgement,
    std::int32_t direction,
    const std::vector<CompanionEnemyTargetState>& targets,
    std::int32_t maximum_distance) {
    return selectTarget(
        position,
        judgement,
        targets,
        maximum_distance,
        direction);
}

}  // namespace osf
