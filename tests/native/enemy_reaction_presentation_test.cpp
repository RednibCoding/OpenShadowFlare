#include "core/retail_random.hpp"
#include "libs/RKC_RPG_AICONTROL/rkc_rpg_aicontrol.hpp"
#include "resources/effect_visual_resource.hpp"
#include "world/combat_effect_actor.hpp"
#include "world/enemy_presentation_audio.hpp"
#include "world/scenario_world.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool testEffectResourceMappingAndLifetime(
    const std::filesystem::path& data_root) {
    constexpr std::int32_t expected[] = {
        11000000,
        11000001,
        11000002,
        11000009,
        11000017,
        11000018,
        11000019,
        11000020,
        11000021,
        11000022,
        11000023,
        11000024,
        11000025,
        11000026,
        11000027,
    };
    for (std::size_t index = 0;
         index < std::size(expected);
         ++index) {
        if (!check(
                osf::retailCombatEffectResourceId(
                    21000 +
                    static_cast<std::int32_t>(index)) ==
                    expected[index],
                "A retail hit-effect number mapped to the wrong "
                "OPTION resource.")) {
            return false;
        }
    }
    if (!check(
            osf::retailCombatEffectResourceId(20999) == -1 &&
                osf::retailCombatEffectResourceId(21015) == -1 &&
                osf::retailCombatEffectResourceId(21020) ==
                    11000060,
            "The simple effect owner accepted a specialized effect "
            "family or lost the Heal visual.")) {
        return false;
    }

    osf::EffectVisualResources resources;
    std::string error;
    const osf::EffectVisualResource* ordinary =
        resources.load(data_root, 11000000, &error);
    if (!check(
            ordinary != nullptr,
            "The retail ordinary hit-effect resource could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::CombatEffectSpawnRequest request;
    request.valid = true;
    request.effect_number = 21000;
    request.packet_kind = 8;
    request.constructor_value_7 = 25;
    osf::CombatEffectActor actor;
    if (!check(
            actor.initialize(
                request,
                {100, 200},
                {-5, -6, 7, 8},
                *ordinary) &&
                actor.animationFrame() == 0 &&
                actor.drawStrength() == 1000 &&
                actor.displayHeight() == 25,
            "The ordinary hit-effect actor did not preserve its "
            "retail construction fields.")) {
        return false;
    }
    const std::int32_t ordinary_frames =
        ordinary->animation()
            .charts()
            .front()
            .directions[8]
            .frame_count;
    std::int32_t ordinary_updates = 0;
    while (!actor.expired() &&
           ordinary_updates < 1000) {
        actor.update();
        ++ordinary_updates;
    }
    if (!check(
            ordinary_updates == ordinary_frames,
            "An ordinary effect did not live for exactly one CAF "
            "pass.")) {
        return false;
    }

    const osf::EffectVisualResource* timed =
        resources.load(data_root, 11000023, &error);
    if (!check(
            timed != nullptr,
            "The retail timed hit-effect resource could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    request.effect_number = 21010;
    request.packet_kind = 0;
    request.constructor_value_7 = 0;
    if (!check(
            actor.initialize(
                request, {0, 0}, {}, *timed) &&
                actor.drawStrength() == 500,
            "The timed hit-effect actor did not start at retail "
            "strength.")) {
        return false;
    }
    const std::int32_t timed_frames =
        timed->animation()
            .charts()
            .front()
            .directions[0]
            .frame_count;
    actor.update();
    if (!check(
            actor.animationFrame() == 1,
            "The timed death effect stretched its CAF over its "
            "120-update owner lifetime.")) {
        return false;
    }
    for (std::int32_t update = 1;
         update < 90;
         ++update) {
        actor.update();
    }
    if (!check(
            !actor.expired() &&
                actor.drawStrength() == 500 &&
                actor.animationFrame() ==
                    timed_frames - 1,
            "The timed hit effect faded before update 91.")) {
        return false;
    }
    actor.update();
    if (!check(
            actor.drawStrength() == 483,
            "The timed hit effect did not begin its exact 30-update "
            "fade.")) {
        return false;
    }
    for (std::int32_t update = 91;
         update < 120;
         ++update) {
        actor.update();
    }
    return check(
        actor.expired(),
        "The timed hit effect survived beyond update 120.");
}

bool testHitAndDeathActions(
    const std::filesystem::path& data_root) {
    osf::AiControlDatabase controls;
    std::string error;
    if (!check(
            controls.load(
                data_root / "System" / "Game" /
                    "Parameter" / "Control.aid",
                &error),
            "The enemy presentation fixture could not load "
            "Control.aid.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::RetailRandom random(1);
    osf::ScenarioWorld world;
    if (!check(
            world.load(
                data_root,
                {6, 4, 0},
                controls,
                random,
                &error),
            "The enemy presentation fixture scenario could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    auto found = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [](const osf::EnemyActor& enemy) {
            if (!enemy.hasVisual() ||
                enemy.animation().charts().size() <= 3 ||
                enemy.direction() < 0 ||
                enemy.direction() >= 8) {
                return false;
            }
            return enemy.animation()
                       .charts()[2]
                       .directions[
                           static_cast<std::size_t>(
                               enemy.direction())]
                       .frame_count > 0 &&
                   enemy.animation()
                       .charts()[3]
                       .directions[
                           static_cast<std::size_t>(
                               enemy.direction())]
                       .frame_count > 0;
        });
    if (!check(
            found != world.enemies().end(),
            "The retail scenario contains no suitable enemy "
            "presentation fixture.")) {
        return false;
    }
    osf::EnemyActor& enemy = *found;
    osf::GroundMap empty_ground;
    osf::ObjectMap empty_objects;

    osf::EnemyDamageReceiverState state =
        enemy.damageReceiverState(world.id());
    state.presentation_action = 10;
    state.presentation_counter = 0;
    state.action_lock = 1;
    state.reaction_duration = 4;
    state.reaction_stage = 0;
    state.reaction_displacement_suppressed = true;
    state.reaction_additive = 0;
    state.event_number = -1;
    enemy.applyDamageReceiverState(state);
    const osf::WorldPosition stationary =
        enemy.position();
    const std::int32_t hit_frames =
        enemy.animation()
            .charts()[2]
            .directions[
                static_cast<std::size_t>(
                    enemy.direction())]
            .frame_count;
    enemy.update(
        empty_ground, empty_objects, nullptr);
    if (!check(
            enemy.animationChart() == 2 &&
                enemy.animationFrame() == 0 &&
                enemy.position().x == stationary.x &&
                enemy.position().y == stationary.y,
            "Enemy action ten did not select chart two or honor "
            "the displacement-suppression flag.")) {
        return false;
    }
    enemy.update(
        empty_ground, empty_objects, nullptr);
    if (!check(
            enemy.animationFrame() ==
                hit_frames / 4,
            "Enemy action ten did not stretch the CAF over its "
            "authored duration.")) {
        return false;
    }
    enemy.update(
        empty_ground, empty_objects, nullptr);
    enemy.update(
        empty_ground, empty_objects, nullptr);
    state = enemy.damageReceiverState(world.id());
    if (!check(
            state.presentation_action == 7 &&
                state.presentation_counter == 4 &&
                state.action_lock == 0 &&
                state.event_number == 16 &&
                enemy.animationFrame() ==
                    hit_frames - 1,
            "Enemy action ten did not finish on the last frame and "
            "publish retail event 16.")) {
        return false;
    }

    state.presentation_action = 10;
    state.presentation_counter = 0;
    state.action_lock = 1;
    state.reaction_duration = 4;
    state.reaction_stage = 0;
    state.reaction_displacement_suppressed = false;
    state.reaction_additive = 0;
    state.reaction_angle = 0.0;
    enemy.applyDamageReceiverState(state);
    const osf::WorldPosition before_impulse =
        enemy.position();
    enemy.update(
        empty_ground, empty_objects, nullptr);
    if (!check(
            enemy.position().x ==
                    before_impulse.x + 120 &&
                enemy.position().y ==
                    before_impulse.y,
            "Enemy action ten did not apply the retail first "
            "reaction impulse.")) {
        return false;
    }

    state = enemy.damageReceiverState(world.id());
    state.presentation_action = 10;
    state.presentation_counter = 0;
    state.action_lock = 1;
    state.reaction_duration = 4;
    state.reaction_stage = 0;
    state.reaction_displacement_suppressed = false;
    state.reaction_additive = 0;
    state.reaction_angle = 0.0;
    enemy.applyDamageReceiverState(state);
    const osf::WorldPosition before_blocked_impulse =
        enemy.position();
    const std::vector<osf::MovementBlocker> blockers{{
        999,
        {
            before_blocked_impulse.x + 80,
            before_blocked_impulse.y,
        },
        enemy.judgement(),
    }};
    enemy.update(
        empty_ground,
        empty_objects,
        &blockers);
    if (!check(
            enemy.position().x <
                before_blocked_impulse.x + 120,
            "Enemy reaction displacement crossed a dynamic actor "
            "blocker.")) {
        return false;
    }

    state = enemy.damageReceiverState(world.id());
    state.current_life = 0;
    state.presentation_action = 11;
    state.presentation_counter = 0;
    state.action_lock = 1;
    enemy.applyDamageReceiverState(state);
    const osf::EnemyActorUpdate first_death =
        enemy.update(
            empty_ground,
            empty_objects,
            nullptr);
    if (!check(
            enemy.animationChart() == 3 &&
                first_death.effect_spawn.valid &&
                first_death.effect_spawn.effect_number ==
                    21010 &&
                first_death.effect_spawn.owner_kind == 4 &&
                first_death.effect_spawn
                        .source_character_number ==
                    enemy.characterNumber() &&
                first_death.death_started &&
                first_death.effect_spawn.packet_kind == 0,
            "Enemy action eleven did not create the retail "
            "owner-bound death effect request.")) {
        return false;
    }
    const osf::EnemyActorUpdate second_death =
        enemy.update(
            empty_ground,
            empty_objects,
            nullptr);
    const std::int32_t death_sample =
        osf::retailEnemyDeathSample(
            enemy.resourceId());
    if (!check(
            death_sample < 0 ||
                std::find(
                    second_death.audio_samples.begin(),
                    second_death.audio_samples.end(),
                    death_sample) !=
                    second_death.audio_samples.end(),
            "Enemy action eleven did not play its resource-specific "
            "death sample on update one.")) {
        return false;
    }

    bool faded = false;
    bool death_finished = false;
    for (std::int32_t update = 0;
         update < 1000 && !enemy.expired();
         ++update) {
        const osf::EnemyActorUpdate death_update =
            enemy.update(
                empty_ground,
                empty_objects,
                nullptr);
        death_finished =
            death_finished || death_update.death_finished;
        faded =
            faded || enemy.drawStrength() < 1000;
    }
    const osf::EnemyActorUpdate expired_update =
        enemy.update(
            empty_ground,
            empty_objects,
            nullptr);
    return check(
        faded && enemy.expired() && death_finished &&
            expired_update.expired &&
            !expired_update.death_finished,
        "Enemy action eleven did not publish exactly one completion "
        "after the retail chart-three fade.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(
            OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(data_root)) {
        return 0;
    }
    return testEffectResourceMappingAndLifetime(data_root) &&
                   testHitAndDeathActions(data_root)
               ? 0
               : 1;
#else
    return 0;
#endif
}
