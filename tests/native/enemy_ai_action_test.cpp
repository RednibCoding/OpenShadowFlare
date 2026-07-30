#include "world/enemy_ai_action.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

osf::AiActionData action(
    std::int32_t number,
    std::int32_t duration) {
    osf::AiActionData result;
    result.action_number = number;
    result.parameters[1] = duration;
    return result;
}

bool testRetailWaitAction() {
    osf::EnemyAiActionController controller;
    controller.select(action(0, 2));
    osf::EnemyAiActionContext context;
    context.presentation_action = 8;

    osf::EnemyAiActionUpdate update =
        controller.update(context);
    if (!check(
            update.handled &&
                update.event_number == 11 &&
                update.requested_presentation_action == 7 &&
                update.movement.mode ==
                    osf::MovementDestinationMode::none &&
                controller.currentAction() == 0 &&
                controller.actionCounter() == 1,
            "Enemy AI action zero did not enter retail's timed idle "
            "state.")) {
        return false;
    }

    context.presentation_action = 7;
    update = controller.update(context);
    if (!check(
            update.event_number == 11 &&
                update.requested_presentation_action == -1 &&
                controller.actionCounter() == 2,
            "Enemy AI action zero left its holding event too early.")) {
        return false;
    }
    update = controller.update(context);
    if (!check(
            update.event_number == 0 &&
                controller.eventNumber() == 0 &&
                controller.actionCounter() == 3,
            "Enemy AI action zero did not return to event zero on "
            "its inclusive duration boundary.")) {
        return false;
    }

    controller.select(action(0, -1));
    update = controller.update(context);
    return check(
        update.event_number == 0 &&
            controller.actionCounter() == 1,
        "A retail minus-one wait duration did not complete on entry.");
}

bool testRetailPatrolAction() {
    osf::AiActionData patrol = action(1, 5);
    patrol.parameters[3] = 15;
    patrol.parameters[4] = 3;
    patrol.parameters[5] = 2;
    patrol.parameters[6] = 4;

    osf::EnemyAiActionController controller;
    controller.select(patrol);
    osf::EnemyAiActionContext context;
    context.spawn_position = {100, 100};
    context.patrol_bounds = {-10, -10, 10, 10};
    context.movement_speed_scale = 3000;
    context.presentation_action = 7;

    osf::EnemyAiActionUpdate update =
        controller.update(context);
    if (!check(
            update.handled &&
                update.event_number == 12 &&
                update.requested_presentation_action == 8 &&
                update.movement.mode ==
                    osf::MovementDestinationMode::patrol &&
                update.movement.destination_bounds.left == 90 &&
                update.movement.destination_bounds.top == 90 &&
                update.movement.destination_bounds.right == 110 &&
                update.movement.destination_bounds.bottom == 110 &&
                update.movement.speed == 45 &&
                update.movement.duration == 3 &&
                update.movement.animation_chart == 4 &&
                controller.actionCounter() == 1 &&
                controller.patrolCounter() == 1,
            "Enemy AI action one did not start the authored retail "
            "patrol cycle.")) {
        return false;
    }

    context.presentation_action = 8;
    update = controller.update(context);
    if (!check(
            update.movement.mode ==
                osf::MovementDestinationMode::none &&
                controller.patrolCounter() == 2,
            "An active patrol restarted before its movement phase "
            "completed.")) {
        return false;
    }

    context.presentation_action = 7;
    update = controller.update(context);
    if (!check(
            update.movement.mode ==
                osf::MovementDestinationMode::none &&
                controller.patrolCounter() == 4,
            "An early patrol arrival did not clamp to the authored "
            "movement duration.")) {
        return false;
    }
    update = controller.update(context);
    if (!check(
            update.movement.mode ==
                osf::MovementDestinationMode::none &&
                controller.patrolCounter() == 5,
            "The patrol idle phase did not retain retail cadence.")) {
        return false;
    }
    update = controller.update(context);
    if (!check(
            update.movement.mode ==
                osf::MovementDestinationMode::patrol &&
                controller.actionCounter() == 5 &&
                controller.patrolCounter() == 1,
            "The next patrol request did not start at the complete "
            "movement-plus-idle boundary.")) {
        return false;
    }
    update = controller.update(context);
    return check(
        update.event_number == 1 &&
            controller.eventNumber() == 1 &&
            controller.actionCounter() == 6,
        "Enemy AI action one did not return to event one on its "
        "inclusive action-duration boundary.");
}

bool testUnsupportedActionStaysDormant() {
    osf::EnemyAiActionController controller;
    controller.select(action(12, 0));
    osf::EnemyAiActionContext context;
    const osf::EnemyAiActionUpdate update =
        controller.update(context);
    return check(
        !update.handled &&
            controller.currentAction() == -1 &&
            controller.eventNumber() == -1 &&
            controller.actionCounter() == 0,
        "An unreconstructed enemy action was partially executed.");
}

bool testRetailTargetMovementActions() {
    osf::AiActionData retreat = action(9, 2);
    retreat.parameters[3] = 15;
    retreat.parameters[7] = 4;
    retreat.parameters[8] = 25;
    retreat.conditions[3] = 1;
    retreat.conditions[4] = 100;
    retreat.conditions[5] = 500;

    osf::EnemyAiActionController controller;
    controller.select(retreat);
    osf::EnemyAiActionContext context;
    context.movement_speed_scale = 3000;
    context.presentation_action = 7;
    std::int32_t searched_minimum = -1;
    std::int32_t searched_maximum = -1;
    context.target_in_range =
        [&](std::int32_t minimum, std::int32_t maximum) {
            searched_minimum = minimum;
            searched_maximum = maximum;
            return osf::EnemyAiTarget{
                true, osf::MovementTargetKind::player, 3};
        };
    osf::EnemyAiActionUpdate update =
        controller.update(context);
    if (!check(
            update.handled &&
                update.event_number == 14 &&
                update.requested_presentation_action == 8 &&
                update.movement.mode ==
                    osf::MovementDestinationMode::
                        retreat_from_player &&
                update.movement.target_identifier == 3 &&
                update.movement.speed == 45 &&
                update.movement.stop_distance == 10000 &&
                update.movement.random_turn_chance == 25 &&
                update.movement.target_refresh_interval == 4 &&
                searched_minimum == 100 &&
                searched_maximum == 500,
            "Enemy action nine did not enter retail target-retreat "
            "movement.")) {
        return false;
    }

    context.presentation_action = 8;
    update = controller.update(context);
    if (!check(
            update.event_number == 14 &&
                update.movement.mode ==
                    osf::MovementDestinationMode::none,
            "Enemy action nine left its holding event too early.")) {
        return false;
    }
    update = controller.update(context);
    if (!check(
            update.event_number == 9 &&
                controller.actionCounter() == 3,
            "Enemy action nine did not complete on its inclusive "
            "duration boundary.")) {
        return false;
    }

    controller.select(retreat);
    context.presentation_action = 8;
    context.target_in_range =
        [](std::int32_t, std::int32_t) {
            return osf::EnemyAiTarget{
                true,
                osf::MovementTargetKind::scenario_actor,
                14000008};
        };
    update = controller.update(context);
    if (!check(
                update.movement.mode ==
                    osf::MovementDestinationMode::
                        retreat_from_scenario_actor,
            "Enemy action nine did not use the native non-player "
            "retreat mode.")) {
        return false;
    }

    osf::AiActionData approach = action(10, 10);
    approach.parameters[3] = 20;
    approach.parameters[7] = 3;
    approach.parameters[8] = 40;
    controller.select(approach);
    context.presentation_action = 7;
    bool used_default_target = false;
    context.default_target = [&] {
        used_default_target = true;
        return osf::EnemyAiTarget{
            true,
            osf::MovementTargetKind::scenario_actor,
            14000007};
    };
    update = controller.update(context);
    if (!check(
                update.event_number == 15 &&
                update.movement.mode ==
                    osf::MovementDestinationMode::
                        approach_scenario_actor &&
                update.movement.target_identifier == 14000007 &&
                update.movement.speed == 60 &&
                update.movement.stop_distance == 0 &&
                update.movement.random_turn_chance == 40 &&
                update.movement.target_refresh_interval == 3 &&
                used_default_target,
            "Enemy action ten did not enter retail target-approach "
            "movement.")) {
        return false;
    }

    controller.select(approach);
    context.presentation_action = 8;
    context.default_target = [] {
        return osf::EnemyAiTarget{
            true, osf::MovementTargetKind::player, 2};
    };
    update = controller.update(context);
    if (!check(
                update.movement.mode ==
                    osf::MovementDestinationMode::
                        approach_player,
            "Enemy action ten did not use the native player-target "
            "approach mode.")) {
        return false;
    }

    context.presentation_action = 7;
    update = controller.update(context);
    if (!check(
            update.event_number == 10 &&
                update.movement.mode ==
                    osf::MovementDestinationMode::none,
            "A stopped target movement did not publish its native "
            "completion event.")) {
        return false;
    }

    controller.select(approach);
    context.default_target = [] {
        return osf::EnemyAiTarget{};
    };
    update = controller.update(context);
    return check(
        update.event_number == 10 &&
            update.movement.mode ==
                osf::MovementDestinationMode::none &&
            controller.currentAction() == 10 &&
            controller.actionCounter() == 1,
        "A target movement without an eligible target did not "
        "complete immediately.");
}

bool testRetailWalkPointAction() {
    osf::EnemyAiActionController controller;
    controller.select(action(11, 0));
    osf::EnemyAiActionContext context;
    context.presentation_action = 7;
    context.walk_point = {123, 456};
    context.walk_point_speed = 10;
    context.movement_speed_scale = 3000;

    osf::EnemyAiActionUpdate update =
        controller.update(context);
    if (!check(
            update.handled &&
                update.event_number == -1 &&
                update.requested_presentation_action == 8 &&
                update.movement.mode ==
                    osf::MovementDestinationMode::fixed_point &&
                update.movement.destination.x == 123 &&
                update.movement.destination.y == 456 &&
                update.movement.speed == 30 &&
                update.movement.stop_distance == 150,
            "Enemy action eleven did not start its retail walk-point "
            "movement.")) {
        return false;
    }

    context.presentation_action = 8;
    for (std::int32_t counter = 1;
         counter <= 90;
         ++counter) {
        update = controller.update(context);
        if (!check(
                update.event_number == -1,
                "Enemy action eleven completed before counter 91.")) {
            return false;
        }
    }
    update = controller.update(context);
    if (!check(
            update.event_number == 0 &&
                controller.actionCounter() == 92,
            "Enemy action eleven did not complete after counter 90.")) {
        return false;
    }

    controller.select(action(11, 0));
    context.presentation_action = 8;
    update = controller.update(context);
    context.presentation_action = 7;
    update = controller.update(context);
    return check(
        update.event_number == 0 &&
            update.movement.mode ==
                osf::MovementDestinationMode::none,
        "A stopped walk-point action did not return to event zero.");
}

bool testZeroDurationPatrolRequest() {
    osf::AiActionData patrol = action(1, 10);
    patrol.parameters[3] = 25;
    patrol.parameters[4] = 0;
    patrol.parameters[5] = 0;
    patrol.parameters[6] = 0;
    osf::EnemyAiActionController controller;
    controller.select(patrol);
    osf::EnemyAiActionContext context;
    context.spawn_position = {300, 400};
    context.patrol_bounds = {-20, -20, 19, 19};
    context.movement_speed_scale = 3000;
    context.presentation_action = 7;

    const osf::EnemyAiActionUpdate update =
        controller.update(context);
    return check(
            update.handled &&
            update.movement.mode ==
                osf::MovementDestinationMode::patrol &&
            update.movement.destination_bounds.left == 280 &&
            update.movement.destination_bounds.top == 380 &&
            update.movement.destination_bounds.right == 319 &&
            update.movement.destination_bounds.bottom == 419 &&
            update.movement.duration == 0 &&
            update.requested_presentation_action == 8,
        "A zero-duration retail patrol did not preserve its "
        "movement-selector request.");
}

bool testRetailPresentationActionDispatch() {
    osf::EnemyAiActionController controller;
    osf::EnemyAiActionContext context;
    context.presentation_action = 7;

    for (std::int32_t action_number = 2;
         action_number <= 7;
         ++action_number) {
        controller.select(
            action(action_number, 100));
        const osf::EnemyAiActionUpdate update =
            controller.update(context);
        if (!check(
                update.handled &&
                    update.event_number == -1 &&
                    update.clear_current_presentation &&
                    update.requested_presentation_action ==
                        action_number - 1 &&
                    controller.currentAction() ==
                        action_number &&
                    controller.actionCounter() == 1 &&
                    controller.eventNumber() == -1,
                "An enemy presentation action did not reproduce "
                "the native dispatcher mapping.")) {
            return false;
        }

        const osf::EnemyAiActionUpdate held =
            controller.update(context);
        if (!check(
                held.handled &&
                    held.event_number == -1 &&
                    !held.clear_current_presentation &&
                    held.requested_presentation_action == -1 &&
                    controller.actionCounter() == 2,
                "An active enemy presentation action restarted "
                "before its animation completed.")) {
            return false;
        }
    }

    controller.select(action(8, 100));
    const osf::EnemyAiActionUpdate retained =
        controller.update(context);
    return check(
        retained.handled &&
            retained.event_number == -1 &&
            !retained.clear_current_presentation &&
            retained.requested_presentation_action == -1 &&
            controller.currentAction() == 8 &&
            controller.actionCounter() == 1,
        "Enemy action eight did not retain the active native "
        "presentation.");
}

bool testRetailActionCatalog() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    osf::AiControlDatabase database;
    std::string error;
    if (!database.load(
            std::string(OPENSHADOWFLARE_SOURCE_DIR) +
                "/tmp/ShadowFlare/System/Game/Parameter/"
                "Control.aid",
            &error)) {
        std::cerr << error << '\n';
        return false;
    }

    std::size_t wait_count = 0;
    std::size_t wait_minus_one = 0;
    std::size_t wait_twenty = 0;
    std::size_t wait_sixty = 0;
    std::size_t patrol_count = 0;
    std::size_t zero_patrol = 0;
    std::size_t ordinary_patrol = 0;
    std::array<std::size_t, 12> action_counts{};
    std::size_t targeted_retreat = 0;
    std::size_t default_retreat = 0;
    std::size_t plain_approach = 0;
    std::size_t randomized_approach = 0;
    bool values_match = true;
    for (const osf::AiControlList& list :
         database.lists()) {
        for (const osf::AiEventData& event :
             list.events()) {
            for (const osf::AiActionData& candidate :
                 event.actions()) {
                if (candidate.action_number >= 0 &&
                    candidate.action_number <
                        static_cast<std::int32_t>(
                            action_counts.size())) {
                    ++action_counts[
                        static_cast<std::size_t>(
                            candidate.action_number)];
                }
                if (candidate.action_number == 0) {
                    ++wait_count;
                    switch (candidate.parameters[1]) {
                    case -1:
                        ++wait_minus_one;
                        break;
                    case 20:
                        ++wait_twenty;
                        break;
                    case 60:
                        ++wait_sixty;
                        break;
                    default:
                        values_match = false;
                        break;
                    }
                } else if (candidate.action_number == 1) {
                    ++patrol_count;
                    values_match =
                        values_match &&
                        candidate.parameters[6] == 0;
                    if (candidate.parameters[4] == 0 &&
                        candidate.parameters[5] == 0) {
                        ++zero_patrol;
                    } else if (
                        candidate.parameters[4] == 120 &&
                        candidate.parameters[5] == 20) {
                        ++ordinary_patrol;
                    } else {
                        values_match = false;
                    }
                } else if (candidate.action_number == 9) {
                    values_match =
                        values_match &&
                        candidate.parameters[7] == 0 &&
                        candidate.parameters[8] == 0;
                    if (candidate.conditions[3] == 1) {
                        ++targeted_retreat;
                    } else if (
                        candidate.conditions[3] == 0) {
                        ++default_retreat;
                    } else {
                        values_match = false;
                    }
                } else if (candidate.action_number == 10) {
                    values_match =
                        values_match &&
                        candidate.conditions[3] == 1;
                    if (candidate.parameters[7] == 0 &&
                        candidate.parameters[8] == 0) {
                        ++plain_approach;
                    } else if (
                        candidate.parameters[7] > 0 &&
                        candidate.parameters[8] > 0) {
                        ++randomized_approach;
                    } else {
                        values_match = false;
                    }
                }
            }
        }
    }
    const std::array<std::size_t, 12>
        expected_action_counts{
            61,
            92,
            450,
            158,
            0,
            178,
            91,
            42,
            0,
            61,
            205,
            0,
        };
    return check(
        wait_count == 61 &&
            wait_minus_one == 1 &&
            wait_twenty == 9 &&
            wait_sixty == 51 &&
            patrol_count == 92 &&
            zero_patrol == 6 &&
            ordinary_patrol == 86 &&
            targeted_retreat == 44 &&
            default_retreat == 17 &&
            plain_approach == 175 &&
            randomized_approach == 30 &&
            action_counts == expected_action_counts &&
            values_match,
        "The shipped action distribution or passive parameters no "
        "longer match the traced native handlers.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testRetailWaitAction() &&
                   testRetailPatrolAction() &&
                   testUnsupportedActionStaysDormant() &&
                   testRetailTargetMovementActions() &&
                   testRetailWalkPointAction() &&
                   testZeroDurationPatrolRequest() &&
                   testRetailPresentationActionDispatch() &&
                   testRetailActionCatalog()
               ? 0
               : 1;
}
