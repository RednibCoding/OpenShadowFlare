#include "world_scene.hpp"
#include "enemy_death_rewards.hpp"
#include "core/retail_integer.hpp"
#include "items/item_audio.hpp"
#include "movement_controller.hpp"
#include "player_voice.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kRetailInteractionDistance = 0x9f;

}  // namespace

WorldScene::WorldScene()
    : scenario_script_({
          [this](
              const script::Operand& operand,
              std::int32_t& value) {
              return readScriptWorldOperand(operand, value);
          },
          [this](
              const script::Operand& operand,
              std::int32_t value) {
              return writeScriptWorldOperand(operand, value);
          },
          [this](std::int32_t character_number) {
              const NpcActor* npc =
                  findScriptNpc(character_number);
              return npc ? npc->id() : -1;
          },
          [this](
              std::int32_t opcode,
              const std::vector<std::int32_t>& arguments) {
              return executeScriptNativeCommand(
                  opcode, arguments);
          },
          [this](
              script::ValueQuery query,
              std::int32_t& value) {
              return queryScriptValue(query, value);
          },
      }) {}


void WorldScene::clear() {
    scenario_world_.clear();
    scenario_script_.clear();
    pointer_.reset();
    pending_interaction_ = {};
    player_attack_target_.cancel();
    player_visual_.clear();
    companion_visuals_.clear();
    companion_.clear();
    effect_visuals_.clear();
    player_powerup_visual_.clear();
    effect_pattern_resources_.clear();
    speech_patterns_.clear();
    player_appearance_.clear();
    pending_audio_samples_.clear();
    level_up_notice_ = {};
    pending_combat_effects_.clear();
    combat_effects_.clear();
    runtime_effects_.clear();
    miss_effects_.clear();
    quests_.clear();
    missions_.clear();
    transports_.clear();
    item_database_.clear();
    player_equipment_.clear();
    player_belt_.clear();
    player_inventory_.clear();
    player_special_items_.clear();
    item_inventory_patterns_.clear();
    parameter_tables_.clear();
    ai_control_database_.clear();
    script_persistent_values_.clear();
    script_state_flags_.clear();
    data_root_.clear();
    item_world_resources_.clear();
    item_random_.seed(1);
    player_data_.clear();
    player_magic_.clear();
    player_moon_spell_.clear();
    player_berserker_spell_.clear();
    player_energy_shield_.clear();
    player_magic_shield_.clear();
    player_counter_burst_.clear();
    player_life_rate_.clear();
    player_mana_rate_.clear();
    player_item_controller_.clear();
    player_transport_spell_.clear();
    player_.clear();
    has_player_ = false;
    pending_player_attack_impact_target_id_ = -1;
    next_ground_item_id_ = 0;
    camera_anchor_x_ = 320;
    camera_anchor_y_ = 240;
    camera_shake_counter_ = -1;
    camera_shake_duration_ = 0;
    camera_shake_magnitude_ = 0;
    gameplay_service_request_ = {};
    pending_script_travel_ = {};
    script_travel_pending_ = false;
    scenario_changed_ = false;
    player_identify_mode_active_ = false;
    player_infinite_life_ = false;
    player_infinite_mana_ = false;
}

std::int32_t WorldScene::playerExperienceThreshold() const {
    return player_data_.experienceThreshold(
        parameter_tables_);
}

const GroundMap& WorldScene::ground() const {
    return scenario_world_.ground();
}

const ObjectMap& WorldScene::objectMap() const {
    return scenario_world_.objectMap();
}

const std::vector<std::unique_ptr<gapi::NjpImage>>&
WorldScene::mapPatterns() const {
    return scenario_world_.mapPatterns();
}

const gapi::NjpImage& WorldScene::playerPatterns() const {
    return player_visual_.patterns();
}

const gapi::NjpImage& WorldScene::playerShadowPatterns() const {
    return player_visual_.shadowPatterns();
}

const gapi::CafAnimation& WorldScene::playerAnimation() const {
    return player_visual_.animation();
}

const std::vector<ScenarioObjectActor>&
WorldScene::scenarioObjects() const {
    return scenario_world_.objects();
}

const std::vector<NpcActor>& WorldScene::npcs() const {
    return scenario_world_.people();
}

const std::vector<EnemyActor>&
WorldScene::enemies() const {
    return scenario_world_.enemies();
}

bool WorldScene::hasCompanion() const {
    return companion_.valid();
}

const CompanionActor& WorldScene::companion() const {
    return companion_;
}

const std::vector<CombatEffectActor>&
WorldScene::combatEffects() const {
    return combat_effects_;
}

const std::vector<RuntimeEffectActor>&
WorldScene::runtimeEffects() const {
    return runtime_effects_.actors();
}

const PlayerTransportSpell&
WorldScene::playerTransportSpell() const {
    return player_transport_spell_;
}

const gapi::NjpImage* WorldScene::playerTransportPatterns() const {
    return effect_pattern_resources_.find(10000020);
}

const EffectVisualResource*
WorldScene::playerTransportVisual() const {
    return effect_visuals_.find(10000020);
}

const std::vector<MissEffectActor>&
WorldScene::missEffects() const {
    return miss_effects_;
}

bool WorldScene::companionMoonAuraVisible() const {
    if (!player_moon_spell_.active() ||
        !hasCompanion() || companion_.currentLife() <= 0) {
        return false;
    }
    const std::int32_t action =
        companion_.presentationAction();
    return action != 7 && action != 8 && action != 10;
}

const EffectVisualResource*
WorldScene::companionMoonAuraVisual() const {
    return effect_visuals_.find(11000040);
}

std::int32_t WorldScene::companionMoonAuraFrame() const {
    return player_moon_spell_.auraFrame();
}

bool WorldScene::playerMoonActive() const {
    return player_moon_spell_.active();
}

bool WorldScene::playerBerserkerActive() const {
    return player_berserker_spell_.active();
}

const EffectVisualResource*
WorldScene::playerBerserkerVisual() const {
    return player_powerup_visual_.animation().charts().empty()
        ? nullptr
        : &player_powerup_visual_;
}

std::int32_t WorldScene::playerBerserkerFrame() const {
    return player_berserker_spell_.auraFrame();
}

bool WorldScene::playerEnergyShieldActive() const {
    return player_energy_shield_.active();
}

const EffectVisualResource*
WorldScene::playerEnergyShieldVisual() const {
    return player_powerup_visual_.animation().charts().empty()
        ? nullptr
        : &player_powerup_visual_;
}

std::int32_t WorldScene::playerEnergyShieldFrame() const {
    return player_energy_shield_.auraFrame();
}

bool WorldScene::playerMagicShieldActive() const {
    return player_magic_shield_.active();
}

const EffectVisualResource*
WorldScene::playerMagicShieldVisual() const {
    return effect_visuals_.find(11000240);
}

std::int32_t WorldScene::playerMagicShieldFrame() const {
    return player_magic_shield_.auraFrame();
}

bool WorldScene::playerCounterBurstActive() const {
    return player_counter_burst_.active();
}

const EffectVisualResource*
WorldScene::playerCounterBurstVisual() const {
    return effect_visuals_.find(11000250);
}

std::int32_t WorldScene::playerCounterBurstFrame() const {
    return player_counter_burst_.auraFrame();
}

std::size_t
WorldScene::runtimeEffectControllerCount() const {
    return runtime_effects_.controllerCount();
}

const TransportCatalog& WorldScene::transports() const {
    return transports_;
}

const std::vector<GroundItem>& WorldScene::groundItems() const {
    return scenario_world_.groundItems();
}

const QuestState& WorldScene::quests() const {
    return quests_;
}

const MissionCatalog& WorldScene::missions() const {
    return missions_;
}

const ItemDatabase& WorldScene::itemDatabase() const {
    return item_database_;
}

const AiControlDatabase&
WorldScene::aiControlDatabase() const {
    return ai_control_database_;
}

RetailSaveProgress WorldScene::retailSaveProgress() const {
    return {
        quests_.states(),
        transports_.enabledFlags(),
        script_state_flags_,
        player_.movementPace() == MovementPace::run,
    };
}

PlayerEquipment& WorldScene::playerEquipment() {
    return player_equipment_;
}

const PlayerEquipment& WorldScene::playerEquipment() const {
    return player_equipment_;
}

PlayerBelt& WorldScene::playerBelt() {
    return player_belt_;
}

const PlayerBelt& WorldScene::playerBelt() const {
    return player_belt_;
}

PlayerInventory& WorldScene::playerInventory() {
    return player_inventory_;
}

const PlayerInventory& WorldScene::playerInventory() const {
    return player_inventory_;
}

PlayerSpecialItems& WorldScene::playerSpecialItems() {
    return player_special_items_;
}

const PlayerSpecialItems& WorldScene::playerSpecialItems() const {
    return player_special_items_;
}

const ItemInventoryResource&
WorldScene::itemInventoryPatterns() const {
    return item_inventory_patterns_;
}

const PlayerData& WorldScene::playerData() const {
    return player_data_;
}

void WorldScene::configurePlayerDebugResources(
    bool infinite_life,
    bool infinite_mana) {
    player_infinite_life_ = infinite_life;
    player_infinite_mana_ = infinite_mana;
}

bool WorldScene::playerInfiniteLife() const {
    return player_infinite_life_;
}

bool WorldScene::playerInfiniteMana() const {
    return player_infinite_mana_;
}

std::int32_t WorldScene::playerCurrentLife() const {
    return player_infinite_life_
        ? playerRuntimeProfile().maximum_life
        : player_data_.currentLife();
}

std::int32_t WorldScene::playerCurrentMana() const {
    return player_infinite_mana_
        ? playerRuntimeProfile().maximum_mana
        : player_data_.currentMana();
}

PlayerMagic& WorldScene::playerMagic() {
    return player_magic_;
}

const PlayerMagic& WorldScene::playerMagic() const {
    return player_magic_;
}

const TableDatabase& WorldScene::parameterTables() const {
    return parameter_tables_;
}

PlayerItemUseResult WorldScene::usePlayerBeltPocket(
    std::int32_t pocket) {
    return player_item_controller_.useBeltPocket(
        pocket,
        player_belt_,
        item_database_,
        player_data_);
}

PlayerItemUseResult WorldScene::usePlayerInventoryItem(
    std::int32_t item_index) {
    return player_item_controller_.useInventoryItem(
        item_index,
        player_inventory_,
        item_database_,
        player_data_);
}

std::int32_t WorldScene::playerMineCount() const {
    return player_item_controller_.mineCount();
}

const ItemWorldResource* WorldScene::itemWorldResource(
    std::int32_t resource_id) const {
    if (resource_id < 0 ||
        static_cast<std::size_t>(resource_id) >=
            item_world_resources_.size()) {
        return nullptr;
    }
    return item_world_resources_[
        static_cast<std::size_t>(resource_id)].get();
}

bool WorldScene::playerPartEnabled(std::size_t part) const {
    return player_appearance_.partEnabled(part);
}

std::int32_t WorldScene::playerPartRedStrength(
    std::size_t part) const {
    return player_appearance_.redStrength(part);
}

std::int32_t WorldScene::playerPartGreenStrength(
    std::size_t part) const {
    return player_appearance_.greenStrength(part);
}

std::int32_t WorldScene::playerPartBlueStrength(
    std::size_t part) const {
    return player_appearance_.blueStrength(part);
}

void WorldScene::refreshPlayerAppearance() {
    player_appearance_.refresh(
        player_visual_.animation().maxPartCount(),
        player_equipment_,
        item_database_);
}

bool WorldScene::hasPlayer() const {
    return has_player_;
}

void WorldScene::togglePlayerRun() {
    if (has_player_) {
        player_.toggleMovementPace();
    }
}

void WorldScene::update() {
    // FUN_00443490 drops Magic Shield and Counter Burst at the start of the
    // next player update when no mana remains. Keeping this before the cast
    // action lets an exact-cost activation show its marker frame once, as in
    // retail.
    if (playerCurrentMana() == 0) {
        player_magic_shield_.deactivate();
        player_counter_burst_.deactivate();
    }
    player_moon_spell_.updateAura(
        companionMoonAuraVisible());
    player_berserker_spell_.updateAura(
        has_player_);
    player_energy_shield_.updateAura(has_player_);
    player_magic_shield_.updateAura(has_player_);
    player_counter_burst_.updateAura(has_player_);
    if (camera_shake_counter_ >= 0) {
        camera_shake_counter_ =
            retailAdd(camera_shake_counter_, 1);
        if (camera_shake_counter_ >=
            camera_shake_duration_) {
            camera_shake_counter_ = -1;
        }
    }
    pending_player_attack_impact_target_id_ = -1;
    level_up_notice_.update();
    quests_.updateNotice();
    std::vector<EnemyActor>& live_enemies =
        scenario_world_.enemies();
    live_enemies.erase(
        std::remove_if(
            live_enemies.begin(),
            live_enemies.end(),
            [](const EnemyActor& enemy) {
                return enemy.expired();
            }),
        live_enemies.end());
    for (CombatEffectActor& effect : combat_effects_) {
        effect.update();
    }
    combat_effects_.erase(
        std::remove_if(
            combat_effects_.begin(),
            combat_effects_.end(),
            [](const CombatEffectActor& effect) {
                return effect.expired();
            }),
        combat_effects_.end());
    if (!scenario_script_.messageActive()) {
        scenario_script_.runStatusKind(5);
    }
    NpcActor* interaction_npc = nullptr;
    ScenarioObjectActor* interaction_object = nullptr;
    EnemyActor* interaction_enemy = nullptr;
    GroundItem* interaction_item = nullptr;
    if (pending_interaction_.kind ==
        WorldPointerTargetKind::scenario_object) {
        interaction_object =
            findScenarioObject(pending_interaction_.id);
        if (interaction_object) {
            player_.followTo(interaction_object->position());
        }
    } else if (pending_interaction_.kind ==
        WorldPointerTargetKind::npc) {
        interaction_npc =
            findNpc(pending_interaction_.id);
        if (interaction_npc) {
            player_.followTo(interaction_npc->position());
        }
    } else if (
        pending_interaction_.kind ==
        WorldPointerTargetKind::ground_item) {
        interaction_item =
            findGroundItem(pending_interaction_.id);
        if (interaction_item) {
            player_.followTo(interaction_item->position);
        }
    }
    if (pending_interaction_.kind !=
            WorldPointerTargetKind::none &&
        !interaction_object &&
        !interaction_npc &&
        !interaction_item) {
        pending_interaction_ = {};
    }
    const std::int32_t approach_target_id =
        player_attack_target_.approachTargetId();
    if (approach_target_id >= 0) {
        interaction_enemy = findEnemy(approach_target_id);
        const PlayerAttackTargetSnapshot snapshot =
            interaction_enemy
                ? attackTargetSnapshot(*interaction_enemy)
                : PlayerAttackTargetSnapshot{};
        const PlayerAttackTargetDisposition disposition =
            interaction_enemy
                ? classifyPlayerAttackTarget(
                player_.position(),
                player_.judgement(),
                snapshot)
                : PlayerAttackTargetDisposition::rejected;
        if (disposition ==
            PlayerAttackTargetDisposition::approach) {
            player_.followTo(interaction_enemy->position());
        } else if (
            disposition ==
            PlayerAttackTargetDisposition::rejected) {
            player_attack_target_.cancel();
            player_.cancelMovement();
            interaction_enemy = nullptr;
        }
    }
    const std::int32_t ready_target_id =
        player_attack_target_.readyTargetId();
    if (ready_target_id >= 0) {
        const EnemyActor* ready_enemy =
            findEnemy(ready_target_id);
        const PlayerAttackTargetSnapshot snapshot =
            ready_enemy
                ? attackTargetSnapshot(*ready_enemy)
                : PlayerAttackTargetSnapshot{};
        player_attack_target_.validateReady(
            ready_enemy ? &snapshot : nullptr);
    }
    constexpr std::int32_t player_blocker_id =
        kNoMovementBlockerId + 1;
    std::vector<MovementBlocker> actor_blockers;
    actor_blockers.reserve(
        scenario_world_.objects().size() +
        scenario_world_.people().size() +
        scenario_world_.enemies().size() +
        (has_player_ ? 1u : 0u) +
        (hasCompanion() ? 1u : 0u));
    for (const ScenarioObjectActor& object :
         scenario_world_.objects()) {
        if (!object.judgementEnabled()) {
            continue;
        }
        actor_blockers.push_back({
            object.movementBlockerId(),
            object.position(),
            object.judgement(),
        });
    }
    std::vector<std::size_t> npc_blocker_indices(
        scenario_world_.people().size(),
        actor_blockers.size());
    constexpr std::size_t no_blocker =
        static_cast<std::size_t>(-1);
    for (const NpcActor& npc : scenario_world_.people()) {
        const std::size_t index =
            static_cast<std::size_t>(
                &npc - scenario_world_.people().data());
        if (!npc.judgementEnabled()) {
            npc_blocker_indices[index] = no_blocker;
            continue;
        }
        npc_blocker_indices[index] = actor_blockers.size();
        actor_blockers.push_back({
            npc.movementBlockerId(),
            npc.position(),
            npc.judgement(),
        });
    }
    std::vector<std::size_t> enemy_blocker_indices(
        scenario_world_.enemies().size(),
        actor_blockers.size());
    for (const EnemyActor& enemy :
         scenario_world_.enemies()) {
        const std::size_t index =
            static_cast<std::size_t>(
                &enemy -
                scenario_world_.enemies().data());
        if (!enemyBlocksMovement(enemy)) {
            enemy_blocker_indices[index] = no_blocker;
            continue;
        }
        enemy_blocker_indices[index] =
            actor_blockers.size();
        actor_blockers.push_back({
            enemy.movementBlockerId(),
            enemy.position(),
            enemy.judgement(),
        });
    }
    std::size_t companion_blocker_index = no_blocker;
    if (has_player_) {
        player_.setWalkingSpeedTier(
            playerRuntimeProfile().walkingSpeedTier());
        player_.update(
            scenario_world_.ground(),
            scenario_world_.objectMap(),
            &actor_blockers,
            playerAttackSpeedTier(),
            &player_visual_.animation(),
            parameter_tables_.find(20));
        handlePlayerAttackEvent(player_.takeAttackEvent());
        handlePlayerSpellEvent(player_.takeSpellEvent());
        if (updatePlayerTransportContact()) {
            return;
        }
        updatePlayerTransportPresentation();
        updatePlayerResourceRates();
        const std::int32_t footstep_sample =
            player_.takeFootstepSample();
        if (footstep_sample >= 0) {
            pending_audio_samples_.push_back(
                footstep_sample);
        }
        if (player_.takeDeathVoiceRequest()) {
            pending_audio_samples_.push_back(
                retailPlayerDeathVoiceSample(
                    player_data_.gender()));
        }
        if (player_.takeRespawnRequest()) {
            deactivatePlayerPowerupsForRespawn();
            player_data_.restoreForRespawn();
            const ScenarioTravelResult respawn =
                transitionScenario({
                    scenario_world_.id(),
                    scenario_world_.entryValue(),
                    scenario_world_.localPlayerNumber(),
                });
            if (respawn != ScenarioTravelResult::failed) {
                return;
            }
        }
        scenario_world_.mapExploration().reveal(
            player_.position());
        actor_blockers.push_back({
            player_blocker_id,
            player_.position(),
            player_.judgement(),
        });
    }
    // The retail owner is allowed to step out of its companion's
    // overlapping spawn position. Other actors still treat the companion
    // as a normal category-five judgement object.
    if (hasCompanion() &&
        companion_.judgementEnabled()) {
        companion_blocker_index = actor_blockers.size();
        actor_blockers.push_back({
            companion_.movementBlockerId(),
            companion_.position(),
            companion_.judgement(),
        });
    }
    for (ScenarioObjectActor& object :
         scenario_world_.objects()) {
        object.update();
    }
    for (std::size_t index = 0;
         index < scenario_world_.people().size();
         ++index) {
        NpcActor& npc = scenario_world_.people()[index];
        npc.update(
            scenario_world_.ground(),
            scenario_world_.objectMap(),
            &actor_blockers);
        if (npc_blocker_indices[index] != no_blocker) {
            actor_blockers[
                npc_blocker_indices[index]].position =
                npc.position();
        }
    }
    if (hasCompanion() && has_player_) {
        updateCompanionActor(actor_blockers);
        if (companion_blocker_index != no_blocker) {
            actor_blockers[
                companion_blocker_index].position =
                companion_.position();
        }
    }
    for (std::size_t index = 0;
         index < scenario_world_.enemies().size();
         ++index) {
        EnemyActor& enemy =
            scenario_world_.enemies()[index];
        const EnemyActorUpdate update =
            updateEnemyActor(
                enemy, actor_blockers);
        pending_audio_samples_.insert(
            pending_audio_samples_.end(),
            update.audio_samples.begin(),
            update.audio_samples.end());
        applyEnemyDirectImpact(
            enemy, update.direct_impact);
        if (update.death_started) {
            handleEnemyDeathStart(
                enemy, update.effect_spawn);
        } else {
            queueCombatEffect(update.effect_spawn);
        }
        if (update.death_finished &&
            scenario_script_.data().findStatus(
                4, enemy.characterNumber())) {
            // The retail enemy owner calls FUN_004309a0 after the death
            // animation and fade have completed. Scenario status kind four
            // owns authored consequences such as completing the first Red
            // Goblin quest; ordinary enemies simply have no matching row.
            scenario_script_.startStatus(
                4, enemy.characterNumber());
        }
        if (enemy_blocker_indices[index] != no_blocker) {
            actor_blockers[
                enemy_blocker_indices[index]].position =
                enemy.position();
        }
    }
    updateRuntimeEffects();
    spawnPendingCombatEffects();
    for (GroundItem& item : scenario_world_.groundItems()) {
        if (updateGroundItem(item) !=
            GroundItemUpdateEvent::first_impact) {
            continue;
        }
        const ItemDefinition* definition =
            item_database_.find(
                item.item.category,
                item.item.definition_id);
        if (definition) {
            pending_audio_samples_.push_back(
                retailItemLandingSound(*definition));
        }
    }
    runScenarioContactTriggers();
    if (processPendingScriptTravel()) {
        return;
    }
    interaction_npc =
        pending_interaction_.kind ==
                WorldPointerTargetKind::npc
            ? findNpc(pending_interaction_.id)
            : nullptr;
    if (interaction_npc &&
        distanceBetweenBounds(
            player_.position(),
            player_.judgement(),
            interaction_npc->position(),
            interaction_npc->judgement()) <=
            kRetailInteractionDistance) {
        startNpcInteraction(*interaction_npc);
        return;
    }
    interaction_object =
        pending_interaction_.kind ==
                WorldPointerTargetKind::scenario_object
            ? findScenarioObject(pending_interaction_.id)
            : nullptr;
    if (interaction_object &&
        distanceBetweenBounds(
            player_.position(),
            player_.judgement(),
            interaction_object->position(),
            interaction_object->judgement()) <=
            kRetailInteractionDistance) {
        startScenarioObjectInteraction(*interaction_object);
        return;
    }
    if (interaction_enemy &&
        player_attack_target_.approachTargetId() >= 0) {
        const PlayerAttackTargetSnapshot snapshot =
            attackTargetSnapshot(*interaction_enemy);
        const PlayerAttackTargetDisposition disposition =
            player_attack_target_.refresh(
                player_.position(),
                player_.judgement(),
                &snapshot);
        if (disposition ==
            PlayerAttackTargetDisposition::ready) {
            readyPlayerAttack(*interaction_enemy);
            return;
        }
        if (disposition ==
            PlayerAttackTargetDisposition::rejected) {
            player_.cancelMovement();
        }
    }
    interaction_item =
        pending_interaction_.kind ==
                WorldPointerTargetKind::ground_item
            ? findGroundItem(pending_interaction_.id)
            : nullptr;
    if (interaction_item &&
        distanceBetweenBounds(
            player_.position(),
            player_.judgement(),
            interaction_item->position,
            interaction_item->judgement) <=
            kRetailInteractionDistance) {
        startGroundItemInteraction(interaction_item->id);
    }
}

std::vector<std::int32_t> WorldScene::takeAudioSamples() {
    std::vector<std::int32_t> samples;
    samples.swap(pending_audio_samples_);
    return samples;
}

const PlayerLevelUpNotice&
WorldScene::levelUpNotice() const {
    return level_up_notice_;
}

void WorldScene::dismissLevelUpNotice() {
    level_up_notice_.dismiss();
}

std::int32_t WorldScene::playerWorldX() const {
    return player_.position().x;
}

std::int32_t WorldScene::playerWorldY() const {
    return player_.position().y;
}

std::int32_t WorldScene::playerDirection() const {
    return player_.direction();
}

PlayerMotion WorldScene::playerMotion() const {
    return player_.motion();
}

bool WorldScene::playerSpellActive() const {
    return player_.spellActive();
}

std::int32_t
WorldScene::playerSpellTargetCharacterNumber() const {
    return player_.spellTargetCharacterNumber();
}

MovementPace WorldScene::playerMovementPace() const {
    return player_.movementPace();
}

std::int32_t WorldScene::playerAnimationChart() const {
    return player_.animationChart();
}

std::int32_t WorldScene::playerAnimationFrame() const {
    return player_.animationFrame();
}

std::int32_t WorldScene::playerAttackTargetId() const {
    const std::int32_t active_target =
        player_.attackTargetId();
    return active_target >= 0
        ? active_target
        : player_attack_target_.readyTargetId();
}

std::int32_t
WorldScene::takePlayerAttackImpactTargetId() {
    const std::int32_t target_id =
        pending_player_attack_impact_target_id_;
    pending_player_attack_impact_target_id_ = -1;
    return target_id;
}

std::int32_t WorldScene::cameraScreenX() const {
    const ScreenPosition position =
        calculateRealPosition(player_.position());
    return position.x - camera_anchor_x_;
}

std::int32_t WorldScene::cameraScreenY() const {
    const ScreenPosition position =
        calculateRealPosition(player_.position());
    const std::int32_t shake_y =
        camera_shake_counter_ >= 0 &&
                (camera_shake_counter_ & 1) != 0
            ? camera_shake_magnitude_
            : 0;
    return position.y - camera_anchor_y_ - shake_y;
}

std::int32_t WorldScene::renderCameraScreenX(
    double alpha) const {
    const ScreenPosition position =
        calculateRealPosition(player_.renderPosition(alpha));
    return position.x - camera_anchor_x_;
}

std::int32_t WorldScene::renderCameraScreenY(
    double alpha) const {
    const ScreenPosition position =
        calculateRealPosition(player_.renderPosition(alpha));
    const std::int32_t shake_y =
        camera_shake_counter_ >= 0 &&
                (camera_shake_counter_ & 1) != 0
            ? camera_shake_magnitude_
            : 0;
    return position.y - camera_anchor_y_ - shake_y;
}

void WorldScene::setCameraAnchor(
    std::int32_t screen_x,
    std::int32_t screen_y) {
    camera_anchor_x_ = screen_x;
    camera_anchor_y_ = screen_y;
}

WorldPosition WorldScene::playerRenderPosition(
    double alpha) const {
    return player_.renderPosition(alpha);
}

const ObjectBounds& WorldScene::playerJudgement() const {
    return player_.judgement();
}

std::int32_t WorldScene::musicTrack() const {
    return scenario_world_.musicTrack();
}

const ScenarioData& WorldScene::scenario() const {
    return scenario_world_.data();
}

std::int32_t WorldScene::scenarioId() const {
    return scenario_world_.id();
}

const script::ScriptData& WorldScene::scenarioScript() const {
    return scenario_script_.data();
}


}  // namespace osf
