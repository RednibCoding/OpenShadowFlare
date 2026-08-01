#include "world/movement_destination_selector.hpp"

#include <cstdint>
#include <iostream>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool testFixedPointAndPatrolModes() {
    osf::MovementDestinationSelector selector;
    osf::MovementDestinationRequest request;
    request.mode =
        osf::MovementDestinationMode::fixed_point;
    request.destination = {300, 400};
    selector.initialize(request, {10, 20});
    osf::MovementDestinationContext context;
    context.position = {10, 20};
    osf::RetailRandom random;

    osf::MovementDestinationResult result =
        selector.update(context, random);
    if (!check(
            result.active &&
                result.destination.x == 300 &&
                result.destination.y == 400 &&
                selector.counter() == 0,
            "Movement mode zero did not preserve its fixed "
            "destination.")) {
        return false;
    }

    request = {};
    request.mode =
        osf::MovementDestinationMode::patrol;
    request.destination_bounds = {90, 90, 110, 110};
    request.duration = 2;
    selector.initialize(request, {100, 100});
    context.position = {100, 100};
    const std::uint32_t initial_random =
        random.state();
    result = selector.update(context, random);
    if (!check(
            result.active &&
                result.destination.x == 110 &&
                result.destination.y == 98 &&
                selector.counter() == 1 &&
                random.state() != initial_random,
            "Movement mode three did not draw the retail inclusive "
            "patrol destination.")) {
        return false;
    }
    const std::uint32_t selected_random =
        random.state();
    result = selector.update(context, random);
    if (!check(
            result.active &&
                result.destination.x == 110 &&
                result.destination.y == 98 &&
                selector.counter() == 2 &&
                random.state() == selected_random,
            "Movement mode three redrew its patrol point after "
            "entry.")) {
        return false;
    }
    result = selector.update(context, random);
    if (!check(
            !result.active &&
                result.destination.x == 100 &&
                result.destination.y == 100 &&
                selector.counter() == 2 &&
                random.state() == selected_random,
            "Movement mode three did not stop at its exact duration "
            "boundary.")) {
        return false;
    }

    request.duration = 0;
    selector.initialize(request, {100, 100});
    const std::uint32_t zero_duration_random =
        random.state();
    result = selector.update(context, random);
    return check(
        !result.active &&
            selector.counter() == 0 &&
            random.state() == zero_duration_random,
        "A zero-duration patrol consumed random state.");
}

bool testApproachModesAndRefreshCadence() {
    osf::MovementDestinationSelector selector;
    osf::MovementDestinationRequest request;
    request.mode =
        osf::MovementDestinationMode::approach_player;
    request.target_identifier = 2;
    request.stop_distance = 0;
    request.target_refresh_interval = 3;
    osf::MovementDestinationContext context;
    context.position = {0, 0};
    context.resolve_target =
        [](osf::MovementTargetKind kind, std::int32_t identifier) {
            if (kind != osf::MovementTargetKind::player ||
                identifier != 2) {
                return osf::MovementTargetState{};
            }
            return osf::MovementTargetState{
                true, {100, 0}, {}};
        };
    osf::RetailRandom random;
    selector.initialize(request, context.position);

    osf::MovementDestinationResult result =
        selector.update(context, random);
    if (!check(
            result.active &&
                result.destination.x == 100 &&
                result.destination.y == 0 &&
                selector.counter() == 1,
            "Movement mode four did not acquire its player "
            "destination.")) {
        return false;
    }
    const std::uint32_t refreshed_random =
        random.state();
    context.position = {10, 0};
    result = selector.update(context, random);
    if (!check(
            result.active &&
                result.destination.x == 100 &&
                result.destination.y == 0 &&
                selector.counter() == 2 &&
                random.state() == refreshed_random,
            "Approach movement ignored its target refresh cadence.")) {
        return false;
    }

    context.position = {100, 0};
    result = selector.update(context, random);
    if (!check(
            !result.active &&
                result.destination.x == 100 &&
                result.destination.y == 0,
            "Approach movement did not stop at bounds contact.")) {
        return false;
    }

    request.mode =
        osf::MovementDestinationMode::
            approach_scenario_actor;
    request.target_identifier = 14000007;
    context.position = {0, 0};
    context.resolve_target =
        [](osf::MovementTargetKind kind, std::int32_t identifier) {
            return osf::MovementTargetState{
                kind ==
                        osf::MovementTargetKind::
                            scenario_actor &&
                    identifier == 14000007,
                {80, 20},
                {},
            };
        };
    selector.initialize(request, context.position);
    result = selector.update(context, random);
    return check(
        result.active &&
            result.destination.x == 80 &&
            result.destination.y == 20,
        "Movement mode one did not resolve a scenario-actor "
        "target.");
}

bool testRandomTurningAndRetreatModes() {
    osf::MovementDestinationRequest request;
    request.mode =
        osf::MovementDestinationMode::approach_player;
    request.target_identifier = 1;
    request.random_turn_chance = 50;
    osf::MovementDestinationContext context;
    context.position = {0, 0};
    context.resolve_target =
        [](osf::MovementTargetKind, std::int32_t) {
            return osf::MovementTargetState{
                true, {100, 0}, {}};
        };
    osf::MovementDestinationSelector selector;
    osf::RetailRandom random;
    selector.initialize(request, context.position);

    osf::MovementDestinationResult result =
        selector.update(context, random);
    if (!check(
            result.active &&
                result.destination.x == 84 &&
                result.destination.y == -53,
            "The retail random-turn angle or integer projection "
            "changed.")) {
        return false;
    }

    request.mode =
        osf::MovementDestinationMode::retreat_from_player;
    request.stop_distance = 100;
    request.random_turn_chance = 0;
    context.position = {10, 0};
    selector.initialize(request, context.position);
    result = selector.update(context, random);
    if (!check(
            result.active &&
                result.destination.x == -1 &&
                result.destination.y == 0,
            "Movement mode five did not choose a point beyond its "
            "retreat distance.")) {
        return false;
    }

    context.position = {-100, 0};
    result = selector.update(context, random);
    if (!check(
            !result.active,
            "Retreat movement did not stop at its distance "
            "boundary.")) {
        return false;
    }

    request.mode =
        osf::MovementDestinationMode::
            retreat_from_scenario_actor;
    request.random_turn_chance = 100;
    context.position = {10, 0};
    selector.initialize(request, context.position);
    result = selector.update(context, random);
    return check(
        !result.active &&
            selector.destination().x != context.position.x,
        "Movement mode two lost retail's random-turn no-step "
        "quirk.");
}

bool testRectangleEdgeMode() {
    osf::MovementDestinationRequest request;
    request.mode =
        osf::MovementDestinationMode::rectangle_edge;
    request.destination_bounds = {0, 0, 100, 200};
    osf::MovementDestinationSelector selector;
    osf::MovementDestinationContext context;
    context.position = {50, 100};
    osf::RetailRandom random;
    selector.initialize(request, context.position);

    osf::MovementDestinationResult result =
        selector.update(context, random);
    if (!check(
            result.active &&
                result.destination.x == 100 &&
                result.destination.y == 100 &&
                selector.counter() == 0,
            "Movement mode six did not choose the retail center "
            "fallback edge point.")) {
        return false;
    }

    request.destination_bounds = {-101, -201, 0, 0};
    context.position = {-51, -101};
    selector.initialize(request, context.position);
    result = selector.update(context, random);
    if (!check(
            result.active &&
                result.destination.x == -1 &&
                result.destination.y == -101,
            "Movement mode six changed retail's signed midpoint "
            "rounding.")) {
        return false;
    }

    request.destination_bounds = {0, 0, 100, 200};
    context.position = {100, 100};
    selector.initialize(request, context.position);
    result = selector.update(context, random);
    return check(
        result.active &&
            result.destination.x == 93 &&
            result.destination.y == 150,
        "Movement mode six did not rotate around its rectangle.");
}

}  // namespace

int main() {
    return testFixedPointAndPatrolModes() &&
                   testApproachModesAndRefreshCadence() &&
                   testRandomTurningAndRetreatModes() &&
                   testRectangleEdgeMode()
               ? 0
               : 1;
}
