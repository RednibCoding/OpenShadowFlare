#include "world_scene.hpp"

#include "enemy_death_rewards.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace osf {
namespace {

constexpr std::int32_t kPositionalAudioRange = 3000;

std::int32_t positionDistance(
    WorldPosition first,
    WorldPosition second) {
    const double x =
        static_cast<double>(first.x) - second.x;
    const double y =
        static_cast<double>(first.y) - second.y;
    return static_cast<std::int32_t>(
        std::trunc(std::hypot(x, y)));
}

}  // namespace

EnemyEffectControllerSource WorldScene::runtimeEffectSource(
    std::int32_t owner_kind,
    std::int32_t source_character_number) const {
    if (owner_kind == 1) {
        return {
            has_player_ &&
                source_character_number ==
                    scenario_world_.localPlayerNumber(),
            has_player_
                ? player_.position()
                : WorldPosition{},
        };
    }
    if (const EnemyActor* enemy =
            findScriptEnemy(source_character_number)) {
        return {true, enemy->position()};
    }
    if (const NpcActor* npc =
            findScriptNpc(source_character_number)) {
        return {true, npc->position()};
    }
    if (const ScenarioObjectActor* object =
            findScriptObject(source_character_number)) {
        return {true, object->position()};
    }
    return {};
}

std::vector<RuntimeEffectTargetSnapshot>
WorldScene::runtimeEffectTargets() const {
    std::vector<RuntimeEffectTargetSnapshot> targets;
    targets.reserve(
        (has_player_ ? 1u : 0u) +
        scenario_world_.objects().size() +
        scenario_world_.people().size() +
        scenario_world_.enemies().size());

    if (has_player_) {
        RuntimeEffectTargetSnapshot target;
        target.kind = RuntimeEffectTargetKind::player;
        target.character_number =
            scenario_world_.localPlayerNumber();
        target.identifier =
            scenario_world_.localPlayerNumber();
        target.position = player_.position();
        target.judgement = player_.judgement();
        target.current_life = player_data_.currentLife();
        target.physical_evasion =
            player_data_.baseEvasionRate() +
            player_equipment_.derivedParameterBonus(
                3, item_database_);
        target.magical_evasion =
            player_data_.baseMagicalEvasionRate() +
            player_equipment_.derivedParameterBonus(
                7, item_database_);
        targets.push_back(target);
    }

    for (const ScenarioObjectActor& object :
         scenario_world_.objects()) {
        RuntimeEffectTargetSnapshot target;
        target.kind =
            RuntimeEffectTargetKind::scenario_object;
        target.character_number = object.characterNumber();
        target.identifier = object.characterNumber();
        target.position = object.position();
        target.judgement = object.judgement();
        target.present = object.judgementEnabled();
        target.displayed =
            (object.displayStatus() & 1) != 0;
        targets.push_back(target);
    }
    for (const NpcActor& npc : scenario_world_.people()) {
        RuntimeEffectTargetSnapshot target;
        target.kind = RuntimeEffectTargetKind::npc;
        target.character_number = npc.characterNumber();
        target.identifier = npc.characterNumber();
        target.position = npc.position();
        target.judgement = npc.judgement();
        target.present = npc.judgementEnabled();
        targets.push_back(target);
    }
    for (const EnemyActor& enemy :
         scenario_world_.enemies()) {
        RuntimeEffectTargetSnapshot target;
        target.kind = RuntimeEffectTargetKind::enemy;
        target.character_number = enemy.characterNumber();
        target.identifier = enemy.characterNumber();
        target.position = enemy.position();
        target.judgement = enemy.judgement();
        target.present = enemy.judgementEnabled();
        target.current_life = enemy.currentLife();
        target.active = !enemy.expired();
        target.physical_evasion =
            enemy.physicalEvasion();
        target.magical_evasion =
            enemy.magicalEvasion();
        targets.push_back(target);
    }
    return targets;
}

void WorldScene::applyRuntimeEffectDispatch(
    const RuntimeEffectReceiverDispatch& dispatch) {
    if (dispatch.contact.receiver_action ==
        RuntimeEffectReceiverAction::show_miss) {
        spawnRuntimeMiss(dispatch.contact);
        return;
    }
    if (dispatch.contact.receiver_action !=
        RuntimeEffectReceiverAction::apply_packet) {
        return;
    }

    if (dispatch.contact.kind ==
        RuntimeEffectTargetKind::player) {
        if (has_player_ &&
            dispatch.contact.identifier ==
                scenario_world_.localPlayerNumber()) {
            applyPlayerDamagePacket(
                dispatch.packet,
                dispatch.contact.impact_origin,
                dispatch.source_character_number);
        }
        return;
    }
    if (dispatch.contact.kind !=
        RuntimeEffectTargetKind::enemy) {
        return;
    }

    EnemyActor* enemy =
        findScriptEnemy(dispatch.contact.identifier);
    if (!enemy) {
        return;
    }
    EnemyDamageReceiverContext context;
    context.local_player_slot =
        scenario_world_.localPlayerNumber();
    context.local_player_available = has_player_;
    context.source_player_available =
        has_player_ &&
        ((dispatch.owner_kind == 1 &&
          dispatch.source_character_number ==
              scenario_world_.localPlayerNumber()) ||
         (dispatch.packet.written_words.test(0) &&
          dispatch.packet.written_words.test(2) &&
          dispatch.packet[0] == 0 &&
          dispatch.packet[2] ==
              scenario_world_.localPlayerNumber()));
    context.source_player_position =
        has_player_
            ? player_.position()
            : WorldPosition{};
    const EnemyDamageReceiverResult receiver =
        resolveEnemyDamage(
            enemy->damageReceiverState(
                scenario_world_.id()),
            dispatch.packet,
            dispatch.contact.impact_origin,
            context,
            parameter_tables_,
            item_random_);
    if (!receiver.valid || !receiver.accepted) {
        return;
    }
    enemy->applyDamageReceiverState(receiver.state);
    if (receiver.kill_requested &&
        context.source_player_available) {
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
}

void WorldScene::spawnRuntimeMiss(
    const RuntimeEffectTargetContact& contact) {
    WorldPosition position;
    ObjectBounds judgement;
    if (contact.kind == RuntimeEffectTargetKind::player) {
        if (!has_player_ ||
            contact.identifier !=
                scenario_world_.localPlayerNumber()) {
            return;
        }
        position = player_.position();
        judgement = player_.judgement();
    } else if (
        contact.kind == RuntimeEffectTargetKind::enemy) {
        const EnemyActor* enemy =
            findScriptEnemy(contact.identifier);
        if (!enemy) {
            return;
        }
        position = enemy->position();
        judgement = enemy->judgement();
    } else {
        return;
    }

    constexpr std::int32_t miss_resource_id = 11000011;
    std::string error;
    const gapi::NjpImage* patterns =
        effect_pattern_resources_.load(
            data_root_, miss_resource_id, &error);
    if (!patterns) {
        return;
    }
    MissEffectActor effect;
    if (effect.initialize(
            position, judgement, *patterns)) {
        miss_effects_.push_back(std::move(effect));
    }
}

void WorldScene::queueRuntimeEffectAudio(
    const RuntimeEffectAudioRequest& request) {
    if (request.sound.bank != 0 ||
        request.sound.sample < 0 ||
        (!request.npc_spatial_mode &&
         has_player_ &&
         positionDistance(
             player_.position(),
             request.position) >
             kPositionalAudioRange)) {
        return;
    }
    pending_audio_samples_.push_back(
        request.sound.sample);
}

void WorldScene::updateRuntimeEffects() {
    miss_effects_.erase(
        std::remove_if(
            miss_effects_.begin(),
            miss_effects_.end(),
            [](const MissEffectActor& effect) {
                return effect.expired();
            }),
        miss_effects_.end());
    for (MissEffectActor& effect : miss_effects_) {
        effect.update();
    }

    const RuntimeEffectSystemUpdate update =
        runtime_effects_.update({
            &scenario_world_.ground(),
            &scenario_world_.objectMap(),
            &item_random_,
            [this](
                std::int32_t owner_kind,
                std::int32_t source_character_number) {
                return runtimeEffectSource(
                    owner_kind,
                    source_character_number);
            },
            [this] {
                return runtimeEffectTargets();
            },
            [this](std::int32_t resource_id) {
                std::string error;
                return effect_visuals_.load(
                    data_root_, resource_id, &error);
            },
            [this](
                const RuntimeEffectReceiverDispatch&
                    dispatch) {
                applyRuntimeEffectDispatch(dispatch);
            },
            [this](
                WorldPosition position,
                const ObjectBounds& judgement) {
                if (!positionIsWalkable(
                        scenario_world_.ground(),
                        scenario_world_.objectMap(),
                        position,
                        judgement)) {
                    return false;
                }
                for (const ScenarioObjectActor& object :
                     scenario_world_.objects()) {
                    if (!object.judgementEnabled()) {
                        continue;
                    }
                    const WorldPosition other =
                        object.position();
                    const ObjectBounds bounds =
                        object.judgement();
                    if (position.x + judgement.left <=
                            other.x + bounds.right &&
                        other.x + bounds.left <=
                            position.x + judgement.right &&
                        position.y + judgement.top <=
                            other.y + bounds.bottom &&
                        other.y + bounds.top <=
                            position.y + judgement.bottom) {
                        return false;
                    }
                }
                return true;
            },
            [this] {
                return EnemyEffectControllerSource{
                    has_player_,
                    has_player_
                        ? player_.position()
                        : WorldPosition{},
                };
            },
        });
    if (update.camera_shake) {
        camera_shake_counter_ = 0;
        camera_shake_duration_ =
            update.camera_shake_duration;
        camera_shake_magnitude_ =
            update.camera_shake_magnitude;
    }
    for (const RuntimeEffectAudioRequest& audio :
         update.audio) {
        queueRuntimeEffectAudio(audio);
    }
}

}  // namespace osf
