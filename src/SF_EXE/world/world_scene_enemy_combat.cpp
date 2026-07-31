#include "world_scene.hpp"
#include "enemy_death_rewards.hpp"
#include "player_combat_defense.hpp"

namespace osf {

void WorldScene::accountEnemyKill(
    const EnemyDamageReceiverState& enemy,
    std::int32_t experience_reward,
    std::int32_t main_hand_subtype) {
    const EnemyKillAccountingResult accounting =
        accountRetailEnemyKill(
            player_data_,
            enemy,
            experience_reward,
            scenario_world_.localPlayerNumber(),
            main_hand_subtype,
            parameter_tables_);
    if (!accounting.level_gained) {
        return;
    }
    level_up_notice_ = {
        accounting.level_up_notice,
        accounting.level_up_notice_counter,
    };
    pending_audio_samples_.insert(
        pending_audio_samples_.end(),
        accounting.audio_samples.begin(),
        accounting.audio_samples.end());
}

PlayerDamageReceiverState
WorldScene::playerDamageReceiverState() const {
    PlayerDamageReceiverState state;
    state.defense.character_number =
        scenario_world_.localPlayerNumber();
    state.defense.attack =
        player_data_.basePhysicalAttack() +
        player_equipment_.derivedParameterBonus(
            0, item_database_);
    state.defense.physical_defense =
        player_data_.basePhysicalDefense() +
        player_equipment_.derivedParameterBonus(
            2, item_database_);
    state.defense.magical_defense =
        player_data_.baseMagicalDefense() +
        player_equipment_.derivedParameterBonus(
            6, item_database_);
    state.defense.element_x = player_data_.elementX();
    state.defense.element_y = player_data_.elementY();
    state.position = player_.position();
    state.judgement = player_.judgement();
    state.effect_owner_identifier =
        scenario_world_.localPlayerNumber();
    state.level = player_data_.level();
    state.maximum_life =
        player_data_.baseMaximumLife();
    state.current_life = player_data_.currentLife();
    state.maximum_mana =
        player_data_.baseMaximumMana();
    state.current_mana = player_data_.currentMana();
    state.equipment = player_equipment_;
    state.inventory = player_inventory_;
    state.special_items = player_special_items_;

    const PlayerDamagePresentation presentation =
        player_.damagePresentation();
    state.presentation_action = presentation.action;
    state.presentation_counter = presentation.counter;
    state.action_lock = presentation.action_lock;
    state.reaction_duration =
        presentation.reaction_duration;
    state.reaction_stage = presentation.reaction_stage;
    state.reaction_motion =
        presentation.suppress_displacement;
    state.reaction_additive =
        presentation.reaction_additive;
    state.reaction_angle = presentation.reaction_angle;
    state.direction = presentation.direction;
    state.event_number = presentation.event_number;
    return state;
}

void WorldScene::applyPlayerDamageReceiverState(
    const PlayerDamageReceiverState& state) {
    player_data_.setCurrentLife(state.current_life);
    player_data_.setCurrentMana(state.current_mana);
    player_equipment_ = state.equipment;
    player_inventory_ = state.inventory;
    player_special_items_ = state.special_items;
    player_.applyDamagePresentation({
        state.presentation_action,
        state.presentation_counter,
        state.action_lock,
        state.reaction_duration,
        state.reaction_stage,
        state.reaction_motion,
        state.reaction_additive,
        state.reaction_angle,
        state.direction,
        state.event_number,
    });
}

void WorldScene::applyEnemyDirectImpact(
    EnemyActor& enemy,
    const EnemyDirectImpactResult& impact) {
    if (!impact.valid ||
        !impact.apply_damage ||
        impact.target.kind !=
            MovementTargetKind::player ||
        impact.target.identifier !=
            scenario_world_.localPlayerNumber()) {
        return;
    }
    if (!applyPlayerDamagePacket(
            impact.packet,
            impact.damage_origin,
            enemy.characterNumber())) {
        return;
    }
    if (impact.post_hit_audio_sample >= 0) {
        pending_audio_samples_.push_back(
            impact.post_hit_audio_sample);
    }
}

bool WorldScene::applyPlayerDamagePacket(
    const CombatPacket& packet,
    WorldPosition impact_origin,
    std::int32_t source_character_number) {
    EnemyActor* source_enemy =
        findScriptEnemy(source_character_number);
    PlayerDamageReceiverContext context;
    context.local_player_character_number =
        scenario_world_.localPlayerNumber();
    context.reflection_target = {
        source_enemy && source_enemy->currentLife() > 0,
        source_character_number,
        2,
        source_enemy && source_enemy->currentLife() > 0
            ? 1
            : 0,
        0,
        source_enemy
            ? source_enemy->position()
            : WorldPosition{},
    };
    const PlayerDamageReceiverResult receiver =
        resolvePlayerDamage(
            playerDamageReceiverState(),
            packet,
            impact_origin,
            context,
            item_database_,
            parameter_tables_,
            item_random_);
    if (!receiver.valid || !receiver.accepted) {
        return false;
    }

    applyPlayerDamageReceiverState(receiver.state);
    pending_audio_samples_.insert(
        pending_audio_samples_.end(),
        receiver.audio_samples.begin(),
        receiver.audio_samples.end());
    for (const CombatEffectSpawnRequest& effect :
         receiver.effects) {
        queueCombatEffect(effect);
    }
    if (receiver.equipment_sync_requested ||
        receiver.derived_values_refresh_requested) {
        refreshPlayerAppearance();
    }

    if (source_enemy &&
        receiver.reflection.valid &&
        receiver.reflection.target_character_number ==
            source_enemy->characterNumber()) {
        EnemyDamageReceiverContext enemy_context;
        enemy_context.local_player_slot =
            scenario_world_.localPlayerNumber();
        enemy_context.local_player_available = true;
        enemy_context.source_player_available = true;
        enemy_context.source_player_position =
            player_.position();
        const EnemyDamageReceiverResult reflected =
            resolveEnemyDamage(
                source_enemy->damageReceiverState(
                    scenario_world_.id()),
                receiver.reflection.packet,
                receiver.reflection.impact_origin,
                enemy_context,
                parameter_tables_,
                item_random_);
        if (reflected.valid && reflected.accepted) {
            source_enemy->applyDamageReceiverState(
                reflected.state);
            if (reflected.kill_requested) {
                accountEnemyKill(
                    reflected.state,
                    source_enemy->experienceReward(),
                    -1);
            }
            pending_audio_samples_.insert(
                pending_audio_samples_.end(),
                reflected.audio_samples.begin(),
                reflected.audio_samples.end());
            for (const CombatEffectSpawnRequest& effect :
                 reflected.effects) {
                queueCombatEffect(effect);
            }
        }
    }
    return true;
}

EnemyActorUpdate WorldScene::updateEnemyActor(
    EnemyActor& enemy,
    const std::vector<MovementBlocker>& blockers) {
    EnemyTargetSearchContext targets;
    targets.scenario_id = scenario_world_.id();
    targets.position = enemy.position();
    targets.bounds = enemy.judgement();
    if (has_player_) {
        const std::int32_t slot =
            scenario_world_.localPlayerNumber();
        if (slot >= 0 &&
            static_cast<std::size_t>(slot) <
                targets.players.size()) {
            EnemyPlayerTargetState& target =
                targets.players[
                    static_cast<std::size_t>(slot)];
            target.present = true;
            target.active_state = 1;
            target.scenario_id = scenario_world_.id();
            target.current_life =
                player_data_.currentLife();
            target.combat_defense =
                player_data_.baseEvasionRate() +
                player_equipment_.derivedParameterBonus(
                    3, item_database_);
            target.position = player_.position();
            target.bounds = player_.judgement();
        }
    }

    const EnemyTargetSearch target_in_range =
        [&targets](
            std::int32_t minimum,
            std::int32_t maximum) {
            return findEnemyTargetInRange(
                targets,
                minimum,
                maximum,
                EnemyTargetLifeRequirement::living);
        };
    const bool ai_active =
        findEnemyTargetInRange(
            targets,
            0,
            kRetailEnemyActivationDistance,
            EnemyTargetLifeRequirement::living)
            .found;
    const EnemyDefaultTargetSearch default_target =
        [&targets] {
            return findDefaultEnemyTarget(
                targets,
                EnemyTargetLifeRequirement::living);
        };
    const EnemyDirectImpactTargetSearch direct_target =
        [&targets](
            std::int32_t maximum,
            std::int32_t direction) {
            return findEnemyDirectImpactTarget(
                targets,
                maximum,
                direction,
                EnemyTargetLifeRequirement::living);
        };
    const MovementTargetResolver movement_target =
        [this](
            MovementTargetKind kind,
            std::int32_t identifier) {
            if (kind != MovementTargetKind::player ||
                !has_player_ ||
                identifier !=
                    scenario_world_.localPlayerNumber()) {
                return MovementTargetState{};
            }
            return MovementTargetState{
                player_data_.currentLife() > 0,
                player_.position(),
                player_.judgement(),
            };
        };

    return enemy.update({
        &scenario_world_.ground(),
        &scenario_world_.objectMap(),
        &blockers,
        &parameter_tables_,
        &item_random_,
        ai_active,
        target_in_range,
        default_target,
        direct_target,
        movement_target,
    });
}

}  // namespace osf
