#include "world_scene.hpp"
#include "enemy_death_rewards.hpp"

#include <string>
#include <utility>

namespace osf {

void WorldScene::handleEnemyDeathStart(
    EnemyActor& enemy,
    CombatEffectSpawnRequest effect) {
    constexpr std::int32_t kEpisodeOneMask = 1;
    const std::vector<EnemyDeathDrop> drops =
        createRetailEnemyDrops(
            enemy.lootTableRow(),
            enemy.goldDropChance(),
            enemy.goldMinimum(),
            enemy.goldMaximum(),
            enemy.position(),
            enemy.judgement(),
            player_equipment_.instanceParameterBonus(
                26, item_database_),
            kEpisodeOneMask,
            1,
            parameter_tables_,
            item_database_,
            item_random_);

    std::vector<GroundItem>& ground_items =
        scenario_world_.groundItems();
    const std::size_t first_item =
        ground_items.size();
    const std::int32_t first_id =
        next_ground_item_id_;
    bool created = true;
    for (const EnemyDeathDrop& drop : drops) {
        if (!createGroundItem(
                ground_items,
                drop.item,
                drop.position)) {
            created = false;
            break;
        }
    }
    if (!created || !prepareGroundItems(first_item)) {
        ground_items.resize(first_item);
        next_ground_item_id_ = first_id;
    }

    // Retail creates all item and Gold instances first. Its death-effect
    // direction is the next PRNG draw after those constructors finish.
    if (effect.valid) {
        effect.packet_kind = item_random_.next() % 8;
        queueCombatEffect(effect);
    }
}

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
