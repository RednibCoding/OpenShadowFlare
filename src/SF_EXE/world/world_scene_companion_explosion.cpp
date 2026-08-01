#include "world_scene.hpp"

#include "combat_hit_chance.hpp"
#include "companion_explosion_action.hpp"
#include "player_combat_defense.hpp"
#include "player_spell_parameters.hpp"

#include <cmath>
#include <cstddef>

namespace osf {
namespace {

constexpr std::int32_t kExplosionSpell = 20;
constexpr std::int32_t kExplosionScalingSpell = 21;
constexpr std::int32_t kExplosionVisualResource = 10000000;
constexpr std::int32_t kExplosionRadius = 320;
constexpr std::int32_t kExplosionDisplayHeight = 200;
constexpr std::int32_t kExplosionFirstSample = 29;
constexpr std::int32_t kExplosionSecondSample = 23;
constexpr std::int32_t kExplosionShakeRange = 3001;
constexpr std::int32_t kExplosionShakeDuration = 8;
constexpr std::int32_t kExplosionShakeMagnitude = 6;
constexpr std::int32_t kExplosionRedStrength = 500;
constexpr std::int32_t kExplosionGreenStrength = 500;
constexpr std::int32_t kExplosionBlueStrength = 1200;

bool boundsOverlap(
    WorldPosition first_position,
    const ObjectBounds& first,
    WorldPosition second_position,
    const ObjectBounds& second) {
    return first_position.x + first.left <=
               second_position.x + second.right &&
           second_position.x + second.left <=
               first_position.x + first.right &&
           first_position.y + first.top <=
               second_position.y + second.bottom &&
           second_position.y + second.top <=
               first_position.y + first.bottom;
}

std::int32_t positionDistance(
    WorldPosition first,
    WorldPosition second) {
    return static_cast<std::int32_t>(
        std::trunc(
            std::hypot(
                static_cast<double>(first.x) - second.x,
                static_cast<double>(first.y) - second.y)));
}

RuntimeEffectActorSpawnRequest explosionVisual(
    const CompanionActor& companion,
    std::int32_t chart,
    std::int32_t point) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number = 21031;
    actor.resource_id = kExplosionVisualResource;
    actor.owner_kind = 0;
    actor.source_character_number =
        companion.characterNumber();
    actor.position = companion.position();
    actor.judgement = {point, point, point, point};
    actor.display_height = kExplosionDisplayHeight;
    actor.lifetime_from_animation = true;
    actor.lifetime_animation_chart = 1;
    actor.animation_chart = chart;
    actor.animation_direction = 8;
    actor.red_strength = kExplosionRedStrength;
    actor.green_strength = kExplosionGreenStrength;
    actor.blue_strength = kExplosionBlueStrength;
    return actor;
}

}  // namespace

void WorldScene::applyCompanionExplosionImpact() {
    if (!hasCompanion() ||
        companion_.currentLife() <= 0) {
        return;
    }

    runtime_effects_.queueActor(
        explosionVisual(companion_, 1, -2));
    runtime_effects_.queueActor(
        explosionVisual(companion_, 0, 2));
    pending_audio_samples_.push_back(
        kExplosionFirstSample);
    pending_audio_samples_.push_back(
        kExplosionSecondSample);
    if (has_player_ &&
        positionDistance(
            player_.position(), companion_.position()) <
            kExplosionShakeRange) {
        camera_shake_counter_ = 0;
        camera_shake_duration_ =
            kExplosionShakeDuration;
        camera_shake_magnitude_ =
            kExplosionShakeMagnitude;
    }

    const PlayerRuntimeProfile profile =
        playerRuntimeProfile();
    PlayerCombatDefenseSnapshot affinity_source;
    affinity_source.character_number =
        scenario_world_.localPlayerNumber();
    affinity_source.attack = profile.physical_attack;
    affinity_source.physical_defense =
        profile.physical_defense;
    affinity_source.magical_defense =
        profile.magical_defense;
    affinity_source.element_x = player_data_.elementX();
    affinity_source.element_y = player_data_.elementY();
    const PlayerSpellParameters scaling =
        playerSpellParameters(
            player_magic_,
            kExplosionScalingSpell,
            player_equipment_,
            item_database_,
            parameter_tables_);
    CompanionExplosionPacketInput input;
    input.source_character_number =
        companion_.characterNumber();
    input.source_level = player_data_.level();
    input.scaling_level = scaling.effective_level;
    input.damage_value = profile.magical_defense;
    input.magical_hit_rate = profile.magical_hit_rate;
    input.element_affinities =
        buildPlayerElementAffinities(
            affinity_source,
            player_equipment_,
            player_inventory_,
            item_database_);
    input.state_words =
        player_data_.combatPacketStateWords();
    const CombatPacket packet =
        buildCompanionExplosionPacket(
            input, parameter_tables_, item_random_);

    const ObjectBounds area{
        -kExplosionRadius,
        -kExplosionRadius,
        kExplosionRadius,
        kExplosionRadius,
    };
    for (EnemyActor& enemy : scenario_world_.enemies()) {
        if (enemy.currentLife() <= 0 ||
            enemy.expired() ||
            !boundsOverlap(
                companion_.position(),
                area,
                enemy.position(),
                enemy.judgement())) {
            continue;
        }
        const std::int32_t hit_chance =
            retailCombatHitChance(
                companion_.profile().hit_rate,
                enemy.physicalEvasion());
        if (item_random_.next() % 100 >= hit_chance) {
            RuntimeEffectTargetContact contact;
            contact.kind = RuntimeEffectTargetKind::enemy;
            contact.identifier = enemy.characterNumber();
            spawnRuntimeMiss(contact);
            continue;
        }

        EnemyDamageReceiverContext context;
        context.local_player_slot =
            scenario_world_.localPlayerNumber();
        context.local_player_available = has_player_;
        context.source_player_available = has_player_;
        context.source_player_position = player_.position();
        const EnemyDamageReceiverResult receiver =
            resolveEnemyDamage(
                enemy.damageReceiverState(
                    scenario_world_.id()),
                packet,
                companion_.position(),
                context,
                parameter_tables_,
                item_random_);
        if (!receiver.valid || !receiver.accepted) {
            continue;
        }
        player_magic_.train(
            kExplosionSpell,
            false,
            parameter_tables_);
        enemy.applyDamageReceiverState(receiver.state);
        if (receiver.kill_requested) {
            accountEnemyKill(
                receiver.state,
                enemy.experienceReward(),
                -1);
        }
        pending_audio_samples_.insert(
            pending_audio_samples_.end(),
            receiver.audio_samples.begin(),
            receiver.audio_samples.end());
        for (const CombatEffectSpawnRequest& effect :
             receiver.effects) {
            queueCombatEffect(effect);
        }
    }
}

}  // namespace osf
