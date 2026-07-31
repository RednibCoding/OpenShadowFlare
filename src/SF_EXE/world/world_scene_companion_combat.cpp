#include "world_scene.hpp"

#include "companion_attack_impact.hpp"
#include "companion_target_selector.hpp"
#include "movement_controller.hpp"

#include <vector>

namespace osf {
namespace {

constexpr std::int32_t kEnemySearchDistance = 1200;
constexpr std::int32_t kOwnerDisengageDistance = 1500;
constexpr std::int32_t kCompanionAttackRange = 159;
constexpr std::int32_t kCompanionImpactRange = 150;
constexpr std::int32_t kCompanionSwingSample = 95;

std::vector<CompanionEnemyTargetState> enemyTargets(
    const std::vector<EnemyActor>& enemies) {
    std::vector<CompanionEnemyTargetState> targets;
    targets.reserve(enemies.size());
    for (const EnemyActor& enemy : enemies) {
        targets.push_back({
            enemy.characterNumber(),
            enemy.position(),
            enemy.judgement(),
            enemy.currentLife(),
            enemy.physicalEvasion(),
            !enemy.expired(),
        });
    }
    return targets;
}

}  // namespace

void WorldScene::updateCompanionActor(
    const std::vector<MovementBlocker>& blockers) {
    if (companion_.attackActive()) {
        const CompanionActorUpdate update =
            companion_.updateAttack();
        if (update.swing_sound_due) {
            pending_audio_samples_.push_back(
                kCompanionSwingSample);
        }
        if (update.impact_due) {
            applyCompanionAttackImpact();
        }
        return;
    }

    const std::vector<CompanionEnemyTargetState> targets =
        enemyTargets(scenario_world_.enemies());
    const std::int32_t owner_distance =
        distanceBetweenBounds(
            companion_.position(),
            companion_.judgement(),
            player_.position(),
            player_.judgement());

    if (companion_.combatTargetCharacterNumber() >= 0 &&
        owner_distance >= kOwnerDisengageDistance) {
        companion_.leaveCombat();
    }

    CompanionEnemyTarget target;
    const bool combat_mode =
        companion_.combatTargetCharacterNumber() >= 0;
    if ((!combat_mode &&
         owner_distance < kEnemySearchDistance) ||
        (combat_mode &&
         owner_distance < kOwnerDisengageDistance)) {
        target = findCompanionEnemyTarget(
            companion_.position(),
            companion_.judgement(),
            targets,
            kEnemySearchDistance);
    }
    if (!target.found) {
        if (companion_.combatTargetCharacterNumber() >= 0) {
            companion_.leaveCombat();
        }
        companion_.updateFollow(
            player_.position(),
            player_.judgement(),
            scenario_world_.ground(),
            scenario_world_.objectMap(),
            &blockers);
        return;
    }
    companion_.trackCombatTarget(
        target.character_number);

    if (target.distance <= kCompanionAttackRange) {
        companion_.beginAttack(
            target.character_number,
            target.position);
        return;
    }
    companion_.updateCombatApproach(
        target.position,
        scenario_world_.ground(),
        scenario_world_.objectMap(),
        &blockers);
}

void WorldScene::applyCompanionAttackImpact() {
    const std::vector<CompanionEnemyTargetState> targets =
        enemyTargets(scenario_world_.enemies());
    const CompanionEnemyTarget target =
        findCompanionForwardEnemyTarget(
            companion_.position(),
            companion_.judgement(),
            companion_.direction(),
            targets,
            kCompanionImpactRange);
    if (!target.found) {
        return;
    }

    const CompanionProfile& profile =
        companion_.profile();
    const CompanionAttackImpactResult impact =
        resolveCompanionAttackImpact(
            {
                companion_.characterNumber(),
                profile.level,
                profile.physical_attack,
                profile.hit_rate,
                profile.native_element,
                target.character_number,
                target.physical_evasion,
            },
            item_random_);
    if (!impact.valid) {
        return;
    }
    if (impact.show_miss) {
        RuntimeEffectTargetContact contact;
        contact.kind = RuntimeEffectTargetKind::enemy;
        contact.identifier = target.character_number;
        spawnRuntimeMiss(contact);
        return;
    }
    if (!impact.apply_damage) {
        return;
    }

    EnemyActor* enemy =
        findScriptEnemy(target.character_number);
    if (!enemy || enemy->currentLife() <= 0) {
        return;
    }
    EnemyDamageReceiverContext context;
    context.local_player_slot =
        scenario_world_.localPlayerNumber();
    context.local_player_available = has_player_;
    context.source_player_available = has_player_;
    context.source_player_position = player_.position();
    const EnemyDamageReceiverResult receiver =
        resolveEnemyDamage(
            enemy->damageReceiverState(
                scenario_world_.id()),
            impact.packet,
            companion_.position(),
            context,
            parameter_tables_,
            item_random_);
    if (!receiver.valid || !receiver.accepted) {
        return;
    }
    enemy->applyDamageReceiverState(receiver.state);
    if (receiver.kill_requested) {
        accountEnemyKill(
            receiver.state,
            enemy->experienceReward(),
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
    if (impact.post_hit_audio_sample >= 0) {
        pending_audio_samples_.push_back(
            impact.post_hit_audio_sample);
    }
}

}  // namespace osf
