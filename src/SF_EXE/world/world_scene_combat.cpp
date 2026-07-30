#include "world_scene.hpp"

#include <string>
#include <utility>

namespace osf {

void WorldScene::queueCombatEffect(
    const CombatEffectSpawnRequest& request) {
    if (request.valid) {
        pending_combat_effects_.push_back(request);
    }
}

WorldPosition WorldScene::combatEffectOrigin(
    const CombatEffectSpawnRequest& request) const {
    if (request.has_explicit_origin) {
        return request.origin;
    }
    if (request.owner_kind == 1 && has_player_) {
        return player_.position();
    }
    if (const EnemyActor* enemy =
            findScriptEnemy(
                request.source_character_number)) {
        return enemy->position();
    }
    if (const NpcActor* npc =
            findScriptNpc(
                request.source_character_number)) {
        return npc->position();
    }
    if (const ScenarioObjectActor* object =
            findScriptObject(
                request.source_character_number)) {
        return object->position();
    }
    return {};
}

ObjectBounds WorldScene::combatEffectJudgement(
    const CombatEffectSpawnRequest& request) const {
    ObjectBounds judgement =
        request.has_source_judgement
            ? request.source_judgement
            : ObjectBounds{};
    ++judgement.left;
    ++judgement.top;
    ++judgement.right;
    ++judgement.bottom;
    return judgement;
}

void WorldScene::spawnPendingCombatEffects() {
    for (const CombatEffectSpawnRequest& request :
         pending_combat_effects_) {
        const std::int32_t resource_id =
            retailCombatEffectResourceId(
                request.effect_number);
        if (resource_id < 0) {
            continue;
        }
        std::string error;
        const EffectVisualResource* visual =
            effect_visuals_.load(
                data_root_, resource_id, &error);
        if (!visual) {
            continue;
        }
        CombatEffectActor effect;
        if (effect.initialize(
                request,
                combatEffectOrigin(request),
                combatEffectJudgement(request),
                *visual)) {
            combat_effects_.push_back(
                std::move(effect));
        }
    }
    pending_combat_effects_.clear();
}

}  // namespace osf
