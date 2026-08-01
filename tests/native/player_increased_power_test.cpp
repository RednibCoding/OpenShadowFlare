#include "core/retail_random.hpp"
#include "items/player_special_items.hpp"
#include "resources/effect_visual_resource.hpp"
#include "world/generic_effect_actor.hpp"
#include "world/player_increased_power.hpp"
#include "world/player_increased_power_attack.hpp"
#include "world/runtime_effect_actor.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

osf::InventoryItem earlyActivationItem() {
    osf::InventoryItem item;
    item.category = 4;
    item.definition_id = 98000001;
    return item;
}

osf::PlayerAttackImpactStats attackStats() {
    osf::PlayerAttackImpactStats stats;
    stats.source_character_number = 0;
    stats.level = 12;
    stats.physical_attack = 345;
    stats.physical_defense = 67;
    stats.hit_rate = 222;
    stats.reflection_chance = 25;
    stats.reflection_percent = 30;
    stats.weapon_identifier = 10000007;
    stats.weapon_subtype = 5;
    return stats;
}

bool testChargeActivationAndLifetime() {
    osf::PlayerIncreasedPower power;
    osf::PlayerSpecialItems special;
    for (std::int32_t kill = 0; kill < 29; ++kill) {
        power.accountDirectLocalKill();
    }
    if (!check(
            !power.ready(special) && !power.activate(special),
            "Increased Power became ready before either retail threshold.")) {
        return false;
    }
    power.accountDirectLocalKill();
    special.place(earlyActivationItem(), 0, 0);
    if (!check(
            power.ready(special) && power.activate(special) &&
                power.charge() == 0 &&
                power.remainingUpdates() == 900 &&
                power.movementSpeedTier(2) == 9 &&
                power.blocksSpell(7) &&
                power.blocksSpell(8) &&
                power.blocksSpell(9) &&
                !power.blocksSpell(10),
            "The special-item threshold or activation state differs.")) {
        return false;
    }

    power.updateAura(false);
    power.updateAura(true);
    if (!check(
            power.auraFrame() == 1,
            "The aura advanced while it was not displayed.")) {
        return false;
    }
    std::int32_t sounds = 0;
    for (std::int32_t update = 0; update < 900; ++update) {
        sounds += power.update() ? 1 : 0;
    }
    return check(
        sounds == 60 && !power.active() &&
            power.remainingUpdates() == 0 &&
            power.movementSpeedTier(2) == 2 &&
            !power.blocksSpell(7),
        "The 900-update lifetime or sample-76 cadence differs.");
}

bool testOrdinaryThresholdAndRedirect() {
    osf::PlayerIncreasedPower power;
    osf::PlayerSpecialItems special;
    for (std::int32_t kill = 0; kill < 49; ++kill) {
        power.accountDirectLocalKill();
    }
    if (!check(
            !power.ready(special),
            "The ordinary threshold triggered at 49 kills.")) {
        return false;
    }
    power.accountDirectLocalKill();
    if (!check(
            power.activate(special),
            "The ordinary threshold did not trigger at 50 kills.")) {
        return false;
    }

    osf::RetailRandom ineligible(1);
    const std::uint32_t initial_state = ineligible.state();
    if (!check(
            !power.redirectsRangedAttack(4, 20, ineligible) &&
                !power.redirectsRangedAttack(5, 19, ineligible) &&
                ineligible.state() == initial_state,
            "An ineligible action consumed the random stream.")) {
        return false;
    }
    osf::RetailRandom reference(1);
    const std::int32_t roll = reference.next() % 100;
    osf::RetailRandom tested(1);
    return check(
        power.redirectsRangedAttack(5, 20, tested) ==
                (roll < 33) &&
            tested.state() == reference.state(),
        "The Hunter redirect did not use one 33-percent retail roll.");
}

bool testAttackPacketAndDescriptors() {
    osf::RetailRandom random(7);
    const std::vector<std::int32_t> targets{101, 202, 303};
    const auto attack = osf::resolvePlayerIncreasedPowerAttack(
        {0, attackStats(), targets}, random);
    if (!check(
            attack.valid && attack.consume_durability &&
                attack.projectiles.size() == targets.size(),
            "Action 21 did not create one request per target.")) {
        return false;
    }
    for (std::size_t index = 0;
         index < attack.projectiles.size(); ++index) {
        const auto& request = attack.projectiles[index];
        if (!check(
                request.effect_number == 9000 &&
                    request.owner_kind == 1 &&
                    request.source_character_number == 0 &&
                    request.target_kind == 20 &&
                    request.target_identifier == targets[index] &&
                    request.constructor_value_6 == 400 &&
                    request.constructor_value_7 == 350 &&
                    request.packet_kind == 8 &&
                    request.instance_identifier == -1 &&
                    request.constructor_value_21 == 200 &&
                    request.constructor_value_22 == 0 &&
                    request.packet[2] == 0 &&
                    request.packet[4] == 345 &&
                    request.packet[34] == 20006,
                "An action-21 constructor or packet field differs.")) {
            return false;
        }
    }
    osf::RetailRandom invalid_random(1);
    return check(
        !osf::resolvePlayerIncreasedPowerAttack(
             {0, attackStats(), {}}, invalid_random).valid,
        "Action 21 accepted an empty target list.");
}

bool testDelayedDiagonalEffect() {
    osf::RetailRandom packet_random(3);
    const auto attack = osf::resolvePlayerIncreasedPowerAttack(
        {0, attackStats(), {77}}, packet_random);
    osf::RuntimeEffectActorSpawnRequest spawn;
    if (!check(
            attack.valid && osf::buildGenericEffectActor(
                attack.projectiles.front(), {1000, 2000}, spawn) &&
                spawn.resource_id == 9000 &&
                spawn.fixed_target_diagonal_approach &&
                spawn.random_start_delay == 15 &&
                spawn.target_approach_updates == 10,
            "Effect 9000 did not select its delayed controller.")) {
        return false;
    }

    osf::EffectVisualResource visual;
    std::string error;
    const std::filesystem::path directory =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Character" /
        "OPTION" / "00009000";
    if (!check(visual.load(directory, &error), error.c_str())) {
        return false;
    }
    osf::RuntimeEffectActor actor;
    if (!check(
            actor.initialize(spawn, visual),
            "The shipped effect-9000 visual did not initialize.")) {
        return false;
    }
    osf::RuntimeEffectTargetSnapshot target;
    target.kind = osf::RuntimeEffectTargetKind::enemy;
    target.character_number = 90000077;
    target.identifier = 77;
    target.position = {5000, 7000};
    target.judgement = {-80, -80, 79, 79};
    osf::RetailRandom actor_random(1);
    const osf::GroundMap ground;
    const osf::ObjectMap objects;
    std::int32_t hidden_updates = 0;
    while (!actor.visible() && hidden_updates < 15) {
        actor.update(ground, objects, {target}, actor_random);
        ++hidden_updates;
    }
    if (!check(
            actor.visible() && hidden_updates >= 1 &&
                hidden_updates <= 15 &&
                actor.position().x == 6001 &&
                actor.position().y == 6001,
            "Effect 9000 appeared at the wrong approach point.")) {
        return false;
    }
    osf::RuntimeEffectActorUpdate final_update;
    for (std::int32_t update = 0; update < 10; ++update) {
        final_update = actor.update(
            ground, objects, {target}, actor_random);
    }
    return check(
        actor.position().x == 5001 &&
            actor.position().y == 7001 &&
            final_update.target_collision_active &&
            final_update.target_contacts.size() == 1 &&
            final_update.expired,
        "Effect 9000 did not dispatch after ten approach updates.");
}

}  // namespace

int main() {
    return testChargeActivationAndLifetime() &&
            testOrdinaryThresholdAndRedirect() &&
            testAttackPacketAndDescriptors() &&
            testDelayedDiagonalEffect()
        ? 0 : 1;
}
