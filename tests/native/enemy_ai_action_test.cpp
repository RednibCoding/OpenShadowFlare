#include "world/enemy_ai_action.hpp"

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
    osf::RetailRandom random;
    const std::uint32_t initial_random =
        random.state();

    osf::EnemyAiActionUpdate update =
        controller.update(context, random);
    if (!check(
            update.handled &&
                update.event_number == 11 &&
                update.requested_presentation_action == 7 &&
                !update.begin_patrol &&
                controller.currentAction() == 0 &&
                controller.actionCounter() == 1 &&
                random.state() == initial_random,
            "Enemy AI action zero did not enter retail's timed idle "
            "state.")) {
        return false;
    }

    context.presentation_action = 7;
    update = controller.update(context, random);
    if (!check(
            update.event_number == 11 &&
                update.requested_presentation_action == -1 &&
                controller.actionCounter() == 2,
            "Enemy AI action zero left its holding event too early.")) {
        return false;
    }
    update = controller.update(context, random);
    if (!check(
            update.event_number == 0 &&
                controller.eventNumber() == 0 &&
                controller.actionCounter() == 3,
            "Enemy AI action zero did not return to event zero on "
            "its inclusive duration boundary.")) {
        return false;
    }

    controller.select(action(0, -1));
    update = controller.update(context, random);
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
    osf::RetailRandom random;

    osf::EnemyAiActionUpdate update =
        controller.update(context, random);
    if (!check(
            update.handled &&
                update.event_number == 12 &&
                update.requested_presentation_action == 8 &&
                update.begin_patrol &&
                update.patrol_destination.x == 110 &&
                update.patrol_destination.y == 98 &&
                update.movement_speed == 45 &&
                update.movement_duration == 3 &&
                update.movement_animation_chart == 4 &&
                controller.actionCounter() == 1 &&
                controller.patrolCounter() == 1,
            "Enemy AI action one did not start the authored retail "
            "patrol cycle.")) {
        return false;
    }

    context.presentation_action = 8;
    update = controller.update(context, random);
    if (!check(
            !update.begin_patrol &&
                controller.patrolCounter() == 2,
            "An active patrol restarted before its movement phase "
            "completed.")) {
        return false;
    }

    context.presentation_action = 7;
    update = controller.update(context, random);
    if (!check(
            !update.begin_patrol &&
                controller.patrolCounter() == 4,
            "An early patrol arrival did not clamp to the authored "
            "movement duration.")) {
        return false;
    }
    update = controller.update(context, random);
    if (!check(
            !update.begin_patrol &&
                controller.patrolCounter() == 5,
            "The patrol idle phase did not retain retail cadence.")) {
        return false;
    }
    update = controller.update(context, random);
    if (!check(
            update.begin_patrol &&
                controller.actionCounter() == 5 &&
                controller.patrolCounter() == 1,
            "The next patrol target was not chosen at the complete "
            "movement-plus-idle boundary.")) {
        return false;
    }
    update = controller.update(context, random);
    return check(
        update.event_number == 1 &&
            controller.eventNumber() == 1 &&
            controller.actionCounter() == 6,
        "Enemy AI action one did not return to event one on its "
        "inclusive action-duration boundary.");
}

bool testUnsupportedActionStaysDormant() {
    osf::EnemyAiActionController controller;
    controller.select(action(2, 0));
    osf::EnemyAiActionContext context;
    osf::RetailRandom random;
    const std::uint32_t initial_random =
        random.state();
    const osf::EnemyAiActionUpdate update =
        controller.update(context, random);
    return check(
        !update.handled &&
            controller.currentAction() == -1 &&
            controller.eventNumber() == -1 &&
            controller.actionCounter() == 0 &&
            random.state() == initial_random,
        "An unreconstructed enemy action was partially executed.");
}

bool testZeroDurationPatrolDoesNotDrawRandom() {
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
    osf::RetailRandom random;
    const std::uint32_t initial_random =
        random.state();

    const osf::EnemyAiActionUpdate update =
        controller.update(context, random);
    return check(
        update.handled &&
            update.begin_patrol &&
            update.patrol_destination.x == 300 &&
            update.patrol_destination.y == 400 &&
            update.movement_duration == 0 &&
            update.requested_presentation_action == 8 &&
            random.state() == initial_random,
        "A zero-duration retail patrol selected a destination or "
        "consumed random state.");
}

bool testRetailPassiveActionCatalog() {
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
    bool values_match = true;
    for (const osf::AiControlList& list :
         database.lists()) {
        for (const osf::AiEventData& event :
             list.events()) {
            for (const osf::AiActionData& candidate :
                 event.actions()) {
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
                }
            }
        }
    }
    return check(
        wait_count == 61 &&
            wait_minus_one == 1 &&
            wait_twenty == 9 &&
            wait_sixty == 51 &&
            patrol_count == 92 &&
            zero_patrol == 6 &&
            ordinary_patrol == 86 &&
            values_match,
        "The shipped passive-action parameters no longer match the "
        "traced native handlers.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testRetailWaitAction() &&
                   testRetailPatrolAction() &&
                   testUnsupportedActionStaysDormant() &&
                   testZeroDurationPatrolDoesNotDrawRandom() &&
                   testRetailPassiveActionCatalog()
               ? 0
               : 1;
}
