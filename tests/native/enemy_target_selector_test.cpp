#include "world/enemy_target_selector.hpp"

#include <cstdint>
#include <iostream>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

osf::EnemyPlayerTargetState player(
    std::int32_t x,
    std::int32_t scenario = 6,
    std::int32_t active_state = 1,
    std::int32_t life = 100) {
    osf::EnemyPlayerTargetState result;
    result.present = true;
    result.active_state = active_state;
    result.scenario_id = scenario;
    result.current_life = life;
    result.combat_defense = x + 1000;
    result.position = {x, 0};
    return result;
}

osf::EnemyCompanionTargetState companion(
    std::int32_t character_number,
    std::int32_t x,
    std::int32_t scenario = 6,
    bool script_active = true,
    std::int32_t life = 100,
    std::int32_t owner_mode = 0) {
    osf::EnemyCompanionTargetState result;
    result.present = true;
    result.character_number = character_number;
    result.scenario_id = scenario;
    result.script_active = script_active;
    result.attack_target_enabled = true;
    result.current_life = life;
    result.combat_defense = x + 2000;
    result.owner_mode = owner_mode;
    result.position = {x, 0};
    return result;
}

osf::EnemyTargetSearchContext context() {
    osf::EnemyTargetSearchContext result;
    result.scenario_id = 6;
    result.position = {0, 0};
    return result;
}

bool testPlayerPriorityAndInclusiveRange() {
    osf::EnemyTargetSearchContext search = context();
    search.players[0] = player(1001);
    search.players[1] = player(501);
    search.companions.push_back(
        companion(
            osf::kFirstCompanionCharacterNumber,
            101));

    osf::EnemyAiTarget target =
        osf::findEnemyTargetInRange(
            search,
            500,
            1000,
            osf::EnemyTargetLifeRequirement::living);
    if (!check(
            target.found &&
                target.kind ==
                    osf::MovementTargetKind::player &&
                target.identifier == 1 &&
                target.distance == 500 &&
                target.position.x == 501 &&
                target.position.y == 0,
            "Range selection did not preserve inclusive bounds, "
            "nearest-player choice, or player priority.")) {
        return false;
    }

    search.players[0] = player(501);
    target = osf::findEnemyTargetInRange(
        search,
        -1,
        -1,
        osf::EnemyTargetLifeRequirement::living);
    return check(
        target.found &&
            target.identifier == 0 &&
            target.distance == 500,
        "Equal-distance players did not preserve lower-slot "
        "priority.");
}

bool testRetailPlayerEligibility() {
    osf::EnemyTargetSearchContext search = context();
    search.players[0] = player(101, 6, 2);
    search.players[1] = player(201, 6, 1, 0);
    search.players[2] = player(301, 7);
    search.players[3].present = false;

    osf::EnemyAiTarget target =
        osf::findEnemyTargetInRange(
            search,
            -1,
            -1,
            osf::EnemyTargetLifeRequirement::living);
    if (!check(
            !target.found,
            "Range selection accepted an absent, non-state-one, "
            "dead, or different-scenario player.")) {
        return false;
    }

    target = osf::findEnemyTargetInRange(
        search,
        -1,
        -1,
        osf::EnemyTargetLifeRequirement::ignore);
    return check(
        target.found &&
            target.kind ==
                osf::MovementTargetKind::player &&
            target.identifier == 1 &&
            target.distance == 200,
        "The no-life-filter path did not preserve the other retail "
        "player gates.");
}

bool testCompanionFallbackAndEligibility() {
    osf::EnemyTargetSearchContext search = context();
    search.companions = {
        companion(
            osf::kFirstCompanionCharacterNumber,
            401),
        companion(
            osf::kFirstCompanionCharacterNumber + 1,
            101,
            6,
            false),
        companion(
            osf::kFirstCompanionCharacterNumber + 2,
            201,
            6,
            true,
            100,
            1),
        companion(
            osf::kFirstCompanionCharacterNumber + 3,
            301,
            7),
        companion(16000004, 2),
    };

    osf::EnemyAiTarget target =
        osf::findEnemyTargetInRange(
            search,
            -1,
            -1,
            osf::EnemyTargetLifeRequirement::living);
    if (!check(
            target.found &&
                target.kind ==
                    osf::MovementTargetKind::
                        scenario_actor &&
                target.identifier ==
                    osf::kFirstCompanionCharacterNumber &&
                target.distance == 400,
            "Companion fallback did not preserve the four exact "
            "IDs and their activity, owner, and scenario gates.")) {
        return false;
    }

    search.companions[0].current_life = 0;
    target = osf::findEnemyTargetInRange(
        search,
        -1,
        -1,
        osf::EnemyTargetLifeRequirement::living);
    if (!check(
            !target.found,
            "A dead companion passed the living-target query.")) {
        return false;
    }
    target = osf::findEnemyTargetInRange(
        search,
        400,
        400,
        osf::EnemyTargetLifeRequirement::ignore);
    return check(
        target.found &&
            target.identifier ==
                osf::kFirstCompanionCharacterNumber,
        "The companion distance boundary was not inclusive.");
}

bool testDefaultTargetRules() {
    osf::EnemyTargetSearchContext search = context();
    search.players[0] = player(801, 6, 2);
    search.players[1] = player(501);
    search.companions.push_back(
        companion(
            osf::kFirstCompanionCharacterNumber,
            101));

    osf::EnemyAiTarget target =
        osf::findDefaultEnemyTarget(
            search,
            osf::EnemyTargetLifeRequirement::living);
    if (!check(
            target.found &&
                target.kind ==
                    osf::MovementTargetKind::player &&
                target.identifier == 1 &&
                target.distance == 500,
            "Default selection did not choose the nearest nonzero-"
            "state player before a closer companion.")) {
        return false;
    }

    search.players = {};
    search.companions = {
        companion(
            osf::kFirstCompanionCharacterNumber + 1,
            301,
            6,
            false),
        companion(
            osf::kFirstCompanionCharacterNumber,
            301),
    };
    target = osf::findDefaultEnemyTarget(
        search,
        osf::EnemyTargetLifeRequirement::living);
    if (!check(
            target.found &&
                target.kind ==
                    osf::MovementTargetKind::
                        scenario_actor &&
                target.identifier ==
                    osf::kFirstCompanionCharacterNumber + 1 &&
                target.distance == 300,
            "Default companion selection incorrectly applied the "
            "range query's script-active gate or changed tie order.")) {
        return false;
    }

    search.companions[0].owner_mode = 1;
    target = osf::findDefaultEnemyTarget(
        search,
        osf::EnemyTargetLifeRequirement::living);
    return check(
        target.found &&
            target.identifier ==
                osf::kFirstCompanionCharacterNumber,
        "Default companion selection accepted retail owner mode "
        "one.");
}

bool testJudgementBoundsDistance() {
    osf::EnemyTargetSearchContext search = context();
    search.bounds = {-10, -10, 10, 10};
    search.players[0] = player(121);
    search.players[0].bounds = {-10, -10, 10, 10};

    const osf::EnemyAiTarget target =
        osf::findEnemyTargetInRange(
            search,
            100,
            100,
            osf::EnemyTargetLifeRequirement::living);
    return check(
        target.found &&
            target.distance == 100,
        "Enemy targeting measured actor origins instead of retail "
        "judgement bounds.");
}

bool testDirectImpactPlayerRules() {
    osf::EnemyTargetSearchContext search = context();
    search.players[0] = player(301, 6, 2);
    search.players[1] = player(101);
    search.companions.push_back(
        companion(16000009, 2));

    osf::EnemyAiTarget target =
        osf::findEnemyDirectImpactTarget(
            search,
            100,
            1,
            osf::EnemyTargetLifeRequirement::living);
    if (!check(
            target.found &&
                target.kind ==
                    osf::MovementTargetKind::player &&
                target.identifier == 1 &&
                target.distance == 100 &&
                target.combat_defense == 1101,
            "Direct impact did not accept a nonzero-state player "
            "at its inclusive range or preserve player priority "
            "and combat defense.")) {
        return false;
    }

    search.players[1].position = {100, -100};
    target = osf::findEnemyDirectImpactTarget(
        search,
        200,
        1,
        osf::EnemyTargetLifeRequirement::living);
    if (!check(
            target.found &&
                target.identifier == 1,
            "Direct impact rejected a player one direction sector "
            "beside the attacker's facing.")) {
        return false;
    }
    search.players[1].position = {0, -100};
    target = osf::findEnemyDirectImpactTarget(
        search,
        200,
        1,
        osf::EnemyTargetLifeRequirement::living);
    return check(
        target.found &&
            target.kind ==
                osf::MovementTargetKind::scenario_actor &&
            target.identifier == 16000009,
        "Direct impact accepted a player more than one direction "
        "sector outside the attacker's facing instead of using "
        "its companion fallback.");
}

bool testDirectImpactCompanionRules() {
    osf::EnemyTargetSearchContext search = context();
    search.companions = {
        companion(16000009, 101, 6, false, 100, 2),
        companion(16000010, 51),
    };
    search.companions[1].attack_target_enabled = false;

    osf::EnemyAiTarget target =
        osf::findEnemyDirectImpactTarget(
            search,
            100,
            1,
            osf::EnemyTargetLifeRequirement::living);
    if (!check(
            target.found &&
                target.kind ==
                    osf::MovementTargetKind::
                        scenario_actor &&
                target.identifier == 16000009 &&
                target.combat_defense == 2101,
            "Direct impact incorrectly required the entry "
            "selector's fixed companion IDs, script flag, or "
            "owner mode zero.")) {
        return false;
    }

    search.companions[0].position = {100, -100};
    target = osf::findEnemyDirectImpactTarget(
        search,
        200,
        1,
        osf::EnemyTargetLifeRequirement::living);
    if (!check(
            !target.found,
            "Direct impact gave a companion the player's adjacent-"
            "direction tolerance.")) {
        return false;
    }

    search.companions[0].position = {101, 0};
    search.companions[0].owner_mode = 1;
    target = osf::findEnemyDirectImpactTarget(
        search,
        100,
        1,
        osf::EnemyTargetLifeRequirement::living);
    return check(
        !target.found,
        "Direct impact accepted owner mode one or an actor without "
        "the active-status high bit.");
}

}  // namespace

int main() {
    return testPlayerPriorityAndInclusiveRange() &&
                   testRetailPlayerEligibility() &&
                   testCompanionFallbackAndEligibility() &&
                   testDefaultTargetRules() &&
                   testJudgementBoundsDistance() &&
                   testDirectImpactPlayerRules() &&
                   testDirectImpactCompanionRules()
               ? 0
               : 1;
}
