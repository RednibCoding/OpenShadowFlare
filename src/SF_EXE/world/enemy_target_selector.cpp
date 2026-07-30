#include "enemy_target_selector.hpp"

#include "actor_direction.hpp"
#include "movement_controller.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace osf {
namespace {

bool living(
    std::int32_t current_life,
    EnemyTargetLifeRequirement requirement) {
    return requirement ==
               EnemyTargetLifeRequirement::ignore ||
           current_life > 0;
}

bool withinRange(
    std::int32_t distance,
    std::int32_t minimum,
    std::int32_t maximum) {
    return (minimum <= distance || minimum == -1) &&
           (distance <= maximum || maximum == -1);
}

std::int32_t targetDistance(
    const EnemyTargetSearchContext& context,
    WorldPosition position,
    const ObjectBounds& bounds) {
    return distanceBetweenBounds(
        position,
        bounds,
        context.position,
        context.bounds);
}

bool nearer(
    const EnemyAiTarget& selected,
    std::int32_t distance) {
    return !selected.found ||
           distance < selected.distance;
}

const EnemyCompanionTargetState* companion(
    const EnemyTargetSearchContext& context,
    std::int32_t character_number) {
    const auto found = std::find_if(
        context.companions.begin(),
        context.companions.end(),
        [character_number](
            const EnemyCompanionTargetState& candidate) {
            return candidate.present &&
                   candidate.character_number ==
                       character_number;
        });
    return found == context.companions.end()
        ? nullptr
        : &*found;
}

}  // namespace

EnemyAiTarget findEnemyTargetInRange(
    const EnemyTargetSearchContext& context,
    std::int32_t minimum_distance,
    std::int32_t maximum_distance,
    EnemyTargetLifeRequirement life_requirement) {
    EnemyAiTarget selected;
    for (std::size_t slot = 0;
         slot < context.players.size();
         ++slot) {
        const EnemyPlayerTargetState& candidate =
            context.players[slot];
        if (!candidate.present ||
            candidate.active_state != 1 ||
            candidate.scenario_id != context.scenario_id ||
            !living(
                candidate.current_life,
                life_requirement)) {
            continue;
        }
        const std::int32_t distance =
            targetDistance(
                context,
                candidate.position,
                candidate.bounds);
        if (!withinRange(
                distance,
                minimum_distance,
                maximum_distance) ||
            !nearer(selected, distance)) {
            continue;
        }
        selected = {
            true,
            MovementTargetKind::player,
            static_cast<std::int32_t>(slot),
            distance,
            candidate.position,
            candidate.combat_defense,
        };
    }
    if (selected.found) {
        return selected;
    }

    for (std::size_t companion_index = 0;
         companion_index <
             kEnemyCompanionTargetCount;
         ++companion_index) {
        const std::int32_t character_number =
            kFirstCompanionCharacterNumber +
            static_cast<std::int32_t>(
                companion_index);
        const EnemyCompanionTargetState* candidate =
            companion(context, character_number);
        if (!candidate ||
            candidate->scenario_id != context.scenario_id ||
            !candidate->script_active ||
            candidate->owner_mode != 0 ||
            !living(
                candidate->current_life,
                life_requirement)) {
            continue;
        }
        const std::int32_t distance =
            targetDistance(
                context,
                candidate->position,
                candidate->bounds);
        if (!withinRange(
                distance,
                minimum_distance,
                maximum_distance) ||
            !nearer(selected, distance)) {
            continue;
        }
        selected = {
            true,
            MovementTargetKind::scenario_actor,
            character_number,
            distance,
            candidate->position,
            candidate->combat_defense,
        };
    }
    return selected;
}

EnemyAiTarget findDefaultEnemyTarget(
    const EnemyTargetSearchContext& context,
    EnemyTargetLifeRequirement life_requirement) {
    EnemyAiTarget selected;
    for (std::size_t slot = 0;
         slot < context.players.size();
         ++slot) {
        const EnemyPlayerTargetState& candidate =
            context.players[slot];
        if (!candidate.present ||
            candidate.active_state == 0 ||
            candidate.scenario_id != context.scenario_id ||
            !living(
                candidate.current_life,
                life_requirement)) {
            continue;
        }
        const std::int32_t distance =
            targetDistance(
                context,
                candidate.position,
                candidate.bounds);
        if (distance ==
                std::numeric_limits<std::int32_t>::max() ||
            !nearer(selected, distance)) {
            continue;
        }
        selected = {
            true,
            MovementTargetKind::player,
            static_cast<std::int32_t>(slot),
            distance,
            candidate.position,
            candidate.combat_defense,
        };
    }
    if (selected.found) {
        return selected;
    }

    for (const EnemyCompanionTargetState& candidate :
         context.companions) {
        if (!candidate.present ||
            candidate.scenario_id != context.scenario_id ||
            candidate.owner_mode != 0 ||
            !living(
                candidate.current_life,
                life_requirement)) {
            continue;
        }
        const std::int32_t distance =
            targetDistance(
                context,
                candidate.position,
                candidate.bounds);
        if (!nearer(selected, distance)) {
            continue;
        }
        selected = {
            true,
            MovementTargetKind::scenario_actor,
            candidate.character_number,
            distance,
            candidate.position,
            candidate.combat_defense,
        };
    }
    return selected;
}

EnemyAiTarget findEnemyDirectImpactTarget(
    const EnemyTargetSearchContext& context,
    std::int32_t maximum_distance,
    std::int32_t direction,
    EnemyTargetLifeRequirement life_requirement) {
    EnemyAiTarget selected;
    for (std::size_t slot = 0;
         slot < context.players.size();
         ++slot) {
        const EnemyPlayerTargetState& candidate =
            context.players[slot];
        if (!candidate.present ||
            candidate.active_state == 0 ||
            candidate.scenario_id != context.scenario_id ||
            !living(
                candidate.current_life,
                life_requirement)) {
            continue;
        }
        const std::int32_t distance =
            targetDistance(
                context,
                candidate.position,
                candidate.bounds);
        const std::int32_t candidate_direction =
            retailDirectionForVector(
                candidate.position.x -
                    context.position.x,
                candidate.position.y -
                    context.position.y);
        const std::int32_t direction_difference =
            (direction - candidate_direction + 8) % 8;
        if (distance > maximum_distance ||
            (direction_difference != 0 &&
             direction_difference != 1 &&
             direction_difference != 7) ||
            !nearer(selected, distance)) {
            continue;
        }
        selected = {
            true,
            MovementTargetKind::player,
            static_cast<std::int32_t>(slot),
            distance,
            candidate.position,
            candidate.combat_defense,
        };
    }
    if (selected.found) {
        return selected;
    }

    for (const EnemyCompanionTargetState& candidate :
         context.companions) {
        if (!candidate.present ||
            candidate.scenario_id != context.scenario_id ||
            !candidate.attack_target_enabled ||
            candidate.owner_mode == 1 ||
            !living(
                candidate.current_life,
                life_requirement)) {
            continue;
        }
        const std::int32_t distance =
            targetDistance(
                context,
                candidate.position,
                candidate.bounds);
        const std::int32_t candidate_direction =
            retailDirectionForVector(
                candidate.position.x -
                    context.position.x,
                candidate.position.y -
                    context.position.y);
        if (distance > maximum_distance ||
            direction != candidate_direction ||
            !nearer(selected, distance)) {
            continue;
        }
        selected = {
            true,
            MovementTargetKind::scenario_actor,
            candidate.character_number,
            distance,
            candidate.position,
            candidate.combat_defense,
        };
    }
    return selected;
}

}  // namespace osf
