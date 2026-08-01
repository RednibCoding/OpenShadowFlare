#include "world/enemy_ai_evaluator.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct ActionFixture {
    std::int32_t action_number = 0;
    std::array<std::int32_t, 9> parameters{};
    std::array<std::int32_t, 6> conditions{};
};

using EventFixture = std::vector<ActionFixture>;
using ControlFixture =
    std::array<
        EventFixture,
        osf::kAiControlEventCount>;

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

void appendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value) {
    const std::uint32_t data =
        static_cast<std::uint32_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(data));
    bytes.push_back(
        static_cast<std::uint8_t>(data >> 8u));
    bytes.push_back(
        static_cast<std::uint8_t>(data >> 16u));
    bytes.push_back(
        static_cast<std::uint8_t>(data >> 24u));
}

const osf::AiControlList* decodeFixture(
    const ControlFixture& fixture,
    osf::AiControlDatabase& database) {
    std::vector<std::uint8_t> bytes{
        'R', 'K', 'C', '_', 'A', 'I', 'D', 'A',
        'T', 'A', ' ', 'v', '0', '0', '1', 0x1a,
    };
    appendI32(bytes, 1);
    appendI32(
        bytes,
        static_cast<std::int32_t>(
            osf::kAiControlEventCount));
    appendI32(bytes, 4);
    bytes.insert(bytes.end(), {'T', 'e', 's', 't'});
    appendI32(bytes, 10);
    for (const EventFixture& event : fixture) {
        appendI32(
            bytes,
            static_cast<std::int32_t>(event.size()));
        for (const ActionFixture& action : event) {
            appendI32(bytes, action.action_number);
            for (const std::int32_t value :
                 action.parameters) {
                appendI32(bytes, value);
            }
            for (const std::int32_t value :
                 action.conditions) {
                appendI32(bytes, value);
            }
        }
    }
    std::string error;
    if (!database.decode(
            bytes.data(), bytes.size(), &error)) {
        std::cerr << error << '\n';
        return nullptr;
    }
    return database.list(0);
}

ActionFixture action(
    std::int32_t number,
    std::int32_t priority,
    std::int32_t weight) {
    ActionFixture result;
    result.action_number = number;
    result.parameters[0] = priority;
    result.parameters[2] = weight;
    return result;
}

bool testRetailCandidateOrdering() {
    ControlFixture fixture;
    fixture[0].push_back(action(1, 0, 100));
    fixture[0].push_back(action(2, 100, 100));
    fixture[0].push_back(action(3, 0, 100));
    osf::AiControlDatabase database;
    const osf::AiControlList* control =
        decodeFixture(fixture, database);
    if (!control) {
        return false;
    }

    osf::EnemyAiEvaluationContext context;
    osf::RetailRandom first_random;
    const osf::EnemyAiSelection first =
        osf::evaluateEnemyAiEvent(
            *control, 0, context, first_random);
    if (!check(
            first.selected &&
                first.action.action_number == 3 &&
                first_random.state() != 1,
            "The evaluator did not preserve retail's reverse insertion "
            "and later lower-priority candidate behavior.")) {
        return false;
    }

    osf::RetailRandom second_random;
    second_random.next();
    second_random.next();
    const osf::EnemyAiSelection second =
        osf::evaluateEnemyAiEvent(
            *control, 0, context, second_random);
    return check(
        second.selected &&
            second.action.action_number == 2,
        "The weighted draw did not traverse retained candidates in "
        "retail order.");
}

bool testRetailConditions() {
    ControlFixture fixture;
    ActionFixture target = action(10, 100, 100);
    target.conditions = {0, 0, 0, 1, 10, 20};
    ActionFixture life = action(11, 100, 100);
    life.conditions = {1, 25, 75, 0, 0, 0};
    fixture[11] = {target, life};

    osf::AiControlDatabase database;
    const osf::AiControlList* control =
        decodeFixture(fixture, database);
    if (!control) {
        return false;
    }

    std::int32_t queried_minimum = 0;
    std::int32_t queried_maximum = 0;
    osf::EnemyAiEvaluationContext context;
    context.current_life = 50;
    context.maximum_life = 100;
    context.target_in_range =
        [&queried_minimum, &queried_maximum](
            std::int32_t minimum,
            std::int32_t maximum) {
            queried_minimum = minimum;
            queried_maximum = maximum;
            return osf::EnemyAiTarget{};
        };
    osf::RetailRandom random;
    osf::EnemyAiSelection selection =
        osf::evaluateEnemyAiEvent(
            *control, 11, context, random);
    if (!check(
            selection.selected &&
                selection.action.action_number == 11 &&
                queried_minimum == 10 &&
                queried_maximum == 20,
            "Target-distance rejection or life eligibility differs "
            "from retail.")) {
        return false;
    }

    context.current_life = 20;
    context.target_in_range =
        [](std::int32_t, std::int32_t) {
            return osf::EnemyAiTarget{
                true,
                osf::MovementTargetKind::player,
                0,
                0};
        };
    selection = osf::evaluateEnemyAiEvent(
        *control, 11, context, random);
    if (!check(
            selection.selected &&
                selection.action.action_number == 10,
            "The minimum life-percent boundary was not enforced.")) {
        return false;
    }

    context.current_life = 25;
    context.target_in_range =
        [](std::int32_t, std::int32_t) {
            return osf::EnemyAiTarget{};
        };
    selection = osf::evaluateEnemyAiEvent(
        *control, 11, context, random);
    if (!check(
            selection.selected &&
                selection.action.action_number == 11,
            "The life-percent minimum was not inclusive.")) {
        return false;
    }
    context.current_life = 75;
    selection = osf::evaluateEnemyAiEvent(
        *control, 11, context, random);
    return check(
        selection.selected &&
            selection.action.action_number == 11,
        "The life-percent maximum was not inclusive.");
}

bool testDefaultEventFallback() {
    ControlFixture fixture;
    fixture[0].push_back(action(90, 0, 100));
    ActionFixture needs_target = action(20, 100, 100);
    needs_target.conditions = {0, 0, 0, 1, 0, 50};
    fixture[1].push_back(needs_target);
    fixture[11].push_back(needs_target);

    osf::AiControlDatabase database;
    const osf::AiControlList* control =
        decodeFixture(fixture, database);
    if (!control) {
        return false;
    }
    osf::EnemyAiEvaluationContext context;
    context.current_life = 100;
    context.maximum_life = 100;
    context.target_in_range =
        [](std::int32_t, std::int32_t) {
            return osf::EnemyAiTarget{};
        };
    osf::RetailRandom random;
    const osf::EnemyAiSelection fallback =
        osf::evaluateEnemyAiEvent(
            *control, 1, context, random);
    const osf::EnemyAiSelection no_fallback =
        osf::evaluateEnemyAiEvent(
            *control, 11, context, random);
    return check(
        fallback.selected &&
            fallback.action.action_number == 90 &&
            !no_fallback.selected,
        "The event-zero fallback set differs from retail.");
}

bool testNoSelectionDoesNotDrawRandom() {
    ControlFixture fixture;
    fixture[0].push_back(action(1, 0, 0));
    osf::AiControlDatabase database;
    const osf::AiControlList* control =
        decodeFixture(fixture, database);
    if (!control) {
        return false;
    }
    osf::EnemyAiEvaluationContext context;
    osf::RetailRandom random;
    const std::uint32_t initial_state = random.state();
    const osf::EnemyAiSelection zero_weight =
        osf::evaluateEnemyAiEvent(
            *control, 0, context, random);
    const osf::EnemyAiSelection disabled =
        osf::evaluateEnemyAiEvent(
            *control, -1, context, random);
    return check(
        !zero_weight.selected &&
            !disabled.selected &&
            random.state() == initial_state,
        "An empty, disabled, or zero-weight event consumed random "
        "state.");
}

bool testRetailDefaultEvent() {
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
    const osf::AiControlList* enemy_hole =
        database.list(0);
    if (!enemy_hole) {
        return false;
    }

    std::size_t priority_decreases = 0;
    std::size_t life_conditions = 0;
    std::size_t target_conditions = 0;
    bool positive_weights = true;
    for (const osf::AiControlList& list :
         database.lists()) {
        for (const osf::AiEventData& event :
             list.events()) {
            const std::vector<osf::AiActionData>& actions =
                event.actions();
            for (std::size_t index = 0;
                 index < actions.size();
                 ++index) {
                const osf::AiActionData& candidate =
                    actions[index];
                if (index > 0 &&
                    candidate.parameters[0] <
                        actions[index - 1].parameters[0]) {
                    ++priority_decreases;
                }
                life_conditions +=
                    candidate.conditions[0] == 1 ? 1 : 0;
                target_conditions +=
                    candidate.conditions[3] == 1 ? 1 : 0;
                positive_weights =
                    positive_weights &&
                    candidate.parameters[2] > 0;
            }
        }
    }
    if (!check(
            priority_decreases == 33 &&
                life_conditions == 38 &&
                target_conditions == 1168 &&
                positive_weights,
            "The retail Control.aid condition and ordering catalog "
            "no longer matches the traced evaluator assumptions.")) {
        return false;
    }

    osf::EnemyAiEvaluationContext context;
    context.current_life = 1;
    context.maximum_life = 1;
    context.target_in_range =
        [](std::int32_t, std::int32_t) {
            return osf::EnemyAiTarget{};
        };
    osf::RetailRandom random;
    const osf::EnemyAiSelection selection =
        osf::evaluateEnemyAiEvent(
            *enemy_hole, 0, context, random);
    return check(
        selection.selected &&
            selection.action.action_number == 0 &&
            selection.action.event_number == 0 &&
            selection.action.parameters[0] == 0 &&
            selection.action.parameters[2] == 100,
        "The retail Enemy Hole default event did not select its "
        "authored action.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testRetailCandidateOrdering() &&
                   testRetailConditions() &&
                   testDefaultEventFallback() &&
                   testNoSelectionDoesNotDrawRandom() &&
                   testRetailDefaultEvent()
               ? 0
               : 1;
}
