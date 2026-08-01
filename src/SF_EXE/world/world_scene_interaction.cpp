#include "world_scene.hpp"
#include "enemy_death_rewards.hpp"

#include "actor_direction.hpp"
#include "generic_effect_actor.hpp"
#include "player_attack_impact.hpp"
#include "player_combat_defense.hpp"
#include "player_heal_spell.hpp"
#include "player_increased_power_attack.hpp"
#include "player_ranged_attack.hpp"
#include "player_spell_cast.hpp"
#include "player_spell_parameters.hpp"
#include "items/item_audio.hpp"
#include "items/item_condition.hpp"
#include "movement_controller.hpp"
#include "player_voice.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kRetailInteractionDistance = 0x9f;
constexpr std::int32_t kRetailHeightScale = 20;

}  // namespace

void WorldScene::commandPlayerMovement(
    std::int32_t screen_x,
    std::int32_t screen_y) {
    if (!has_player_ || player_.actionLocked()) {
        return;
    }
    pending_interaction_ = {};
    player_attack_target_.cancel();
    player_.moveTo(
        calculateWorldPosition({
            cameraScreenX() + screen_x,
            cameraScreenY() + screen_y,
        }));
}

void WorldScene::cancelPlayerMovement() {
    if (player_.actionLocked()) {
        return;
    }
    pending_interaction_ = {};
    player_attack_target_.cancel();
    player_.cancelMovement();
}

void WorldScene::updatePointerHover(
    std::int32_t screen_x,
    std::int32_t screen_y) {
    if (!has_player_ || scenario_script_.messageActive()) {
        pointer_.clearSelection();
        return;
    }
    pointer_.update(
        screen_x,
        screen_y,
        pointerCandidatesAtScreenPosition(
            screen_x, screen_y));
}

void WorldScene::clearPointerHover() {
    pointer_.clearSelection();
}

void WorldScene::configurePointer(
    const WorldPointerConfiguration& configuration) {
    pointer_.configure(configuration);
}

bool WorldScene::commandWorldInteraction(
    std::int32_t screen_x,
    std::int32_t screen_y) {
    if (!has_player_ ||
        player_.actionLocked() ||
        scenario_script_.messageActive()) {
        return false;
    }
    const WorldPointerTarget target =
        pointerTargetAtScreenPosition(screen_x, screen_y);
    if (target.kind == WorldPointerTargetKind::none) {
        return false;
    }
    player_attack_target_.cancel();
    if (target.kind ==
        WorldPointerTargetKind::ground_item) {
        pending_interaction_ = target;
        GroundItem* item = findGroundItem(target.id);
        if (!item) {
            pending_interaction_ = {};
            return false;
        }
        if (distanceBetweenBounds(
                player_.position(),
                player_.judgement(),
                item->position,
                {}) > kRetailInteractionDistance) {
            player_.followTo(item->position);
            return true;
        }
        return startGroundItemInteraction(item->id);
    }

    if (target.kind ==
        WorldPointerTargetKind::scenario_object) {
        ScenarioObjectActor* selected =
            findScenarioObject(target.id);
        if (!selected) {
            return false;
        }
        pending_interaction_ = target;
        if (distanceBetweenBounds(
                player_.position(),
                player_.judgement(),
                selected->position(),
                selected->judgement()) >
            kRetailInteractionDistance) {
            player_.followTo(selected->position());
            return true;
        }
        return startScenarioObjectInteraction(*selected);
    }

    if (target.kind == WorldPointerTargetKind::enemy) {
        EnemyActor* selected = findEnemy(target.id);
        return selected && commandPlayerAttack(*selected);
    }

    NpcActor* selected = findNpc(target.id);
    if (!selected) {
        return false;
    }
    pending_interaction_ = target;
    if (distanceBetweenBounds(
            player_.position(),
            player_.judgement(),
            selected->position(),
            selected->judgement()) >
            kRetailInteractionDistance) {
        player_.followTo(selected->position());
        return true;
    }
    return startNpcInteraction(*selected);
}

bool WorldScene::commandPlayerMagic(
    std::int32_t screen_x,
    std::int32_t screen_y) {
    if (!has_player_ ||
        player_.actionLocked() ||
        scenario_script_.messageActive()) {
        return false;
    }
    if (player_magic_.targeting()) {
        return commandPlayerSecondaryAttack(
            screen_x, screen_y);
    }
    const std::int32_t spell =
        player_magic_.selectedSpell();
    PlayerSpellAction action;
    if (!player_magic_.learned(spell) ||
        !playerSpellActionForSpell(spell, action)) {
        return false;
    }

    if (spell == 17 &&
        player_identify_mode_active_) {
        pointer_.clearSelection();
        return true;
    }

    EnemyActor* enemy = nullptr;
    const bool requires_target =
        playerSpellRequiresCharacterTarget(spell);
    if (requires_target) {
        const WorldPointerTarget target =
            pointerTargetAtScreenPosition(
                screen_x, screen_y);
        if (target.kind !=
            WorldPointerTargetKind::enemy) {
            return false;
        }
        enemy = findEnemy(target.id);
        if (!enemy ||
            classifyPlayerAttackTarget(
                player_.position(),
                player_.judgement(),
                attackTargetSnapshot(*enemy)) ==
                PlayerAttackTargetDisposition::rejected) {
            return false;
        }
    }

    // Both retail command paths consume the secondary click before their
    // mana check. Pointed spells retain the selected character, while
    // ground/self spells enter the action with character -1.
    pointer_.clearSelection();
    pending_interaction_ = {};
    player_attack_target_.cancel();
    if (player_increased_power_.blocksSpell(spell)) {
        return true;
    }
    const PlayerSpellParameters parameters =
        playerSpellParameters(
            player_magic_,
            spell,
            player_equipment_,
            item_database_,
            parameter_tables_,
            0,
            player_increased_power_.active());
    if (!player_infinite_mana_ &&
        player_data_.currentMana() <
        parameters.mana_cost) {
        return true;
    }

    PlayerSpellAnimationVariant animation_variant =
        PlayerSpellAnimationVariant::standard;
    if (action == PlayerSpellAction::sonic_blade) {
        // The retail pointed-spell switch performs this gate after its MP
        // check and before deducting the cost or entering action 37.
        const InventoryItem* main_hand =
            player_equipment_.item(
                EquipmentSlot::main_hand);
        const ItemDefinition* definition =
            main_hand
                ? item_database_.find(
                      main_hand->category,
                      main_hand->definition_id)
                : nullptr;
        if (!definition ||
            !playerSonicBladeAnimationVariant(
                definition->subtype,
                animation_variant)) {
            return true;
        }
    }

    player_.cancelMovement();
    const WorldPosition facing_position =
        enemy
            ? enemy->position()
            : calculateWorldPosition({
                  cameraScreenX() + screen_x,
                  cameraScreenY() + screen_y,
              });
    player_.faceToward(facing_position);
    if (!player_.beginSpellCast(
            action,
            spell,
            enemy ? enemy->characterNumber() : -1,
            facing_position,
            animation_variant,
            playerAttackSpeedTier(),
            parameter_tables_.find(20),
            player_visual_.animation())) {
        return true;
    }
    if (!player_infinite_mana_) {
        player_data_.setCurrentMana(
            player_data_.currentMana() -
            parameters.mana_cost);
    }
    handlePlayerSpellEvent(player_.takeSpellEvent());
    return true;
}

bool WorldScene::commandPlayerSecondaryAttack(
    std::int32_t screen_x,
    std::int32_t screen_y) {
    if (!has_player_ || player_.actionLocked()) {
        return false;
    }
    const InventoryItem* main_hand =
        player_equipment_.item(EquipmentSlot::main_hand);
    const ItemDefinition* definition =
        main_hand
            ? item_database_.find(
                  main_hand->category,
                  main_hand->definition_id)
            : nullptr;
    const PlayerAttackAction ordinary_action =
        retailPlayerAttackAction(definition);
    const WorldPosition aim = calculateWorldPosition({
        cameraScreenX() + screen_x,
        cameraScreenY() + screen_y,
    });

    pointer_.clearSelection();
    pending_interaction_ = {};
    player_attack_target_.cancel();
    player_.cancelMovement();
    player_.faceToward(aim);

    PlayerComboAttackKind combo_kind;
    const bool started =
        retailPlayerComboAttackKind(
            ordinary_action, combo_kind)
            ? player_.beginComboAttack(
                  combo_kind,
                  playerAttackSpeedTier(),
                  player_visual_.animation())
            : player_.beginAttack(
                  ordinary_action,
                  -1,
                  playerAttackSpeedTier(),
                  player_visual_.animation());
    if (started) {
        handlePlayerAttackEvent(player_.takeAttackEvent());
    }
    return started;
}

bool WorldScene::dropInventoryItem(
    const InventoryItem& item,
    std::int32_t screen_x,
    std::int32_t screen_y) {
    if (!has_player_ || player_.actionLocked()) {
        return false;
    }

    const ItemDefinition* definition =
        item_database_.find(
            item.category, item.definition_id);
    if (!definition ||
        !ensureItemWorldResource(
            definition->ground_resource_id)) {
        return false;
    }

    const WorldPosition pointer_world =
        calculateWorldPosition({
            cameraScreenX() + screen_x,
            cameraScreenY() + screen_y,
        });
    const WorldPosition player_position =
        player_.position();
    const std::int32_t direction =
        retailDirectionForVector(
            pointer_world.x - player_position.x,
            pointer_world.y - player_position.y);

    WorldPosition drop_position = player_position;
    constexpr std::int32_t kRetailDropDistance = 200;
    switch (direction) {
    case 0:
        drop_position.x += kRetailDropDistance;
        drop_position.y += kRetailDropDistance;
        break;
    case 1:
        drop_position.x += kRetailDropDistance;
        break;
    case 2:
        drop_position.x += kRetailDropDistance;
        drop_position.y -= kRetailDropDistance;
        break;
    case 3:
        drop_position.y -= kRetailDropDistance;
        break;
    case 4:
        drop_position.x -= kRetailDropDistance;
        drop_position.y -= kRetailDropDistance;
        break;
    case 5:
        drop_position.x -= kRetailDropDistance;
        break;
    case 6:
        drop_position.x -= kRetailDropDistance;
        drop_position.y += kRetailDropDistance;
        break;
    case 7:
        drop_position.y += kRetailDropDistance;
        break;
    default:
        return false;
    }

    const std::size_t first_item =
        scenario_world_.groundItems().size();
    const std::int32_t first_id =
        next_ground_item_id_;
    if (!createGroundItem(
            scenario_world_.groundItems(),
            item,
            drop_position) ||
        !prepareGroundItems(first_item)) {
        scenario_world_.groundItems().resize(first_item);
        next_ground_item_id_ = first_id;
        return false;
    }
    pending_interaction_ = {};
    player_attack_target_.cancel();
    player_.cancelMovement();
    pointer_.clearSelection();
    return true;
}

bool WorldScene::interactionPending() const {
    return pending_interaction_.kind !=
               WorldPointerTargetKind::none ||
           player_attack_target_.approachTargetId() >= 0 ||
           player_attack_target_.readyTargetId() >= 0 ||
           player_.actionLocked();
}

GameplayServiceRequest
WorldScene::takeGameplayServiceRequest() {
    GameplayServiceRequest request =
        gameplay_service_request_;
    gameplay_service_request_ = {};
    return request;
}

void WorldScene::completeBlackjack(std::int32_t result) {
    blackjack_result_ = std::clamp(result, 0, 2);
    scenario_script_.runStatusKind(8);
}

ScenarioTravelResult
WorldScene::activateTransportDestination(
    std::int32_t row,
    std::string* error) {
    const TransportDestination* destination =
        transports_.find(row);
    if (!destination ||
        !transports_.enabled(row)) {
        if (error) {
            *error = "The transport destination is not available.";
        }
        return ScenarioTravelResult::failed;
    }
    return transitionScenario(
        {
            destination->scenario,
            destination->entry,
            0,
        },
        error);
}

bool WorldScene::startNpcInteraction(NpcActor& selected) {
    pending_interaction_ = {};
    player_attack_target_.cancel();
    player_.cancelMovement();
    player_.faceToward(selected.position());
    const std::int32_t script_character_number =
        12000000 + selected.id();
    const script::StepResult result =
        scenario_script_.startStatus(
            0, script_character_number);
    if (result == script::StepResult::waiting_for_message ||
        result == script::StepResult::complete) {
        pointer_.clearSelection();
    }
    return result == script::StepResult::waiting_for_message ||
           result == script::StepResult::complete;
}

bool WorldScene::startScenarioObjectInteraction(
    ScenarioObjectActor& selected) {
    pending_interaction_ = {};
    player_attack_target_.cancel();
    player_.cancelMovement();
    player_.faceToward(selected.position());
    const script::StepResult result =
        scenario_script_.startStatus(
            0, selected.characterNumber());
    if (result == script::StepResult::waiting_for_message ||
        result == script::StepResult::complete) {
        pointer_.clearSelection();
    }
    return result == script::StepResult::waiting_for_message ||
           result == script::StepResult::complete;
}

std::int32_t WorldScene::hoveredScenarioObjectId() const {
    return pointer_.target().kind ==
                   WorldPointerTargetKind::scenario_object
               ? pointer_.target().id
               : -1;
}

std::int32_t WorldScene::hoveredNpcId() const {
    return pointer_.target().kind ==
                   WorldPointerTargetKind::npc
               ? pointer_.target().id
               : -1;
}

std::int32_t WorldScene::hoveredEnemyId() const {
    return pointer_.target().kind ==
                   WorldPointerTargetKind::enemy
               ? pointer_.target().id
               : -1;
}

std::int32_t WorldScene::hoveredGroundItemId() const {
    return pointer_.target().kind ==
                   WorldPointerTargetKind::ground_item
               ? pointer_.target().id
               : -1;
}

std::int32_t WorldScene::pointerScreenX() const {
    return pointer_.screenX();
}

std::int32_t WorldScene::pointerScreenY() const {
    return pointer_.screenY();
}

bool WorldScene::pointerActive() const {
    return pointer_.active();
}

const WorldPointerConfiguration&
WorldScene::pointerConfiguration() const {
    return pointer_.configuration();
}

bool WorldScene::conversationActive() const {
    return scenario_script_.messageActive();
}

std::int32_t WorldScene::conversationActorId() const {
    return scenario_script_.actorId();
}

std::int32_t WorldScene::conversationMessageId() const {
    return scenario_script_.message().id;
}

const std::string& WorldScene::conversationText() const {
    return scenario_script_.message().text;
}

bool WorldScene::conversationRequiresSelection() const {
    return scenario_script_.message().selection_required;
}

std::int32_t WorldScene::conversationInitialSelection() const {
    return scenario_script_.message().initial_selection;
}

std::int32_t WorldScene::conversationSelectedOption() const {
    return scenario_script_.selectedOption();
}

void WorldScene::selectConversationOption(
    std::int32_t option) {
    scenario_script_.selectOption(option);
}

const gapi::NjpImage& WorldScene::speechPatterns() const {
    return speech_patterns_;
}

const gapi::NjpImage&
WorldScene::mapOverviewPatterns() const {
    return scenario_world_.mapOverviewPatterns();
}

const MapExploration& WorldScene::mapExploration() const {
    return scenario_world_.mapExploration();
}

void WorldScene::advanceConversation() {
    if (!scenario_script_.messageActive()) {
        return;
    }
    const script::StepResult result =
        scenario_script_.resume();
    if (result != script::StepResult::waiting_for_message) {
        for (NpcActor& npc : scenario_world_.people()) {
            npc.endInteraction();
        }
    }
}

void WorldScene::chooseConversationOption(
    std::int32_t option) {
    if (!scenario_script_.messageActive() ||
        !scenario_script_.message().selection_required ||
        option < 0) {
        return;
    }
    const script::StepResult result =
        scenario_script_.resume(option);
    if (result != script::StepResult::waiting_for_message) {
        for (NpcActor& npc : scenario_world_.people()) {
            npc.endInteraction();
        }
    }
}

WorldPointerTarget WorldScene::pointerTargetAtScreenPosition(
    std::int32_t screen_x,
    std::int32_t screen_y) const {
    WorldPointer resolver;
    resolver.configure(pointer_.configuration());
    resolver.update(
        screen_x,
        screen_y,
        pointerCandidatesAtScreenPosition(
            screen_x, screen_y));
    return resolver.target();
}

std::vector<WorldPointerCandidate>
WorldScene::pointerCandidatesAtScreenPosition(
    std::int32_t screen_x,
    std::int32_t screen_y) const {
    const std::int32_t camera_x = cameraScreenX();
    const std::int32_t camera_y = cameraScreenY();
    const ScreenPosition point{screen_x, screen_y};
    const std::int32_t half_size =
        worldPointerHalfSize(pointer_.configuration());
    const DisplayHitRectangle hit_rectangle{
        screen_x - half_size,
        screen_y - half_size,
        screen_x + half_size,
        screen_y + half_size,
    };
    const WorldPosition pointer_world =
        calculateWorldPosition({
            camera_x + screen_x,
            camera_y + screen_y,
        });
    const auto pointerDistanceSquared =
        [pointer_world](WorldPosition position) {
            const std::int64_t delta_x =
                static_cast<std::int64_t>(position.x) -
                pointer_world.x;
            const std::int64_t delta_y =
                static_cast<std::int64_t>(position.y) -
                pointer_world.y;
            return delta_x * delta_x + delta_y * delta_y;
        };
    std::vector<WorldPointerCandidate> candidates;
    candidates.reserve(
        scenario_world_.objects().size() +
        scenario_world_.people().size() +
        scenario_world_.enemies().size() +
        scenario_world_.groundItems().size());
    for (const ScenarioObjectActor& object :
         scenario_world_.objects()) {
        if (!object.visible() ||
            !object.pointerEnabled() ||
            !object.drawEnabled()) {
            continue;
        }
        bool intersects = false;
        bool exact_hit = false;
        if (object.hasStaticVisual()) {
            const ScreenPosition projected =
                calculateRealPosition(object.position());
            const ScreenPosition anchor{
                projected.x - camera_x,
                projected.y - camera_y,
            };
            intersects =
                displayPatternIntersectsRectangle(
                    object.staticPatterns(),
                    static_cast<std::size_t>(
                        object.staticPattern()),
                    anchor,
                    hit_rectangle,
                    object.displayHeight());
            exact_hit =
                displayPatternContainsPoint(
                    object.staticPatterns(),
                    static_cast<std::size_t>(
                        object.staticPattern()),
                    anchor,
                    point,
                    object.displayHeight());
        } else if (object.hasAnimatedVisual()) {
            const auto part_enabled =
                [&object](std::size_t part) {
                    return object.partEnabled(part);
                };
            intersects =
                displayAnimationIntersectsRectangle(
                    object.animation(),
                    object.animationPatterns(),
                    object.position(),
                    object.animationChart(),
                    object.direction(),
                    object.animationFrame(),
                    part_enabled,
                    camera_x,
                    camera_y,
                    hit_rectangle,
                    object.displayHeight());
            exact_hit =
                displayAnimationContainsPoint(
                    object.animation(),
                    object.animationPatterns(),
                    object.position(),
                    object.animationChart(),
                    object.direction(),
                    object.animationFrame(),
                    part_enabled,
                    camera_x,
                    camera_y,
                    point,
                    object.displayHeight());
        }
        if (!intersects) {
            continue;
        }
        candidates.push_back({
            {
                WorldPointerTargetKind::scenario_object,
                object.id(),
            },
            {
                0,
                object.position(),
                object.judgement(),
                static_cast<std::int16_t>(
                    object.displayStatus()),
            },
            0,
            exact_hit,
            pointerDistanceSquared(object.position()),
        });
    }
    for (const NpcActor& npc : scenario_world_.people()) {
        if (!npc.visible() || !npc.pointerEnabled()) {
            continue;
        }
        const auto part_enabled =
            [&npc](std::size_t part) {
                return npc.partEnabled(part);
            };
        if (!displayAnimationIntersectsRectangle(
                npc.animation(),
                npc.patterns(),
                npc.position(),
                npc.animationChart(),
                npc.direction(),
                npc.animationFrame(),
                part_enabled,
                camera_x,
                camera_y,
                hit_rectangle)) {
            continue;
        }
        candidates.push_back({
            {WorldPointerTargetKind::npc, npc.id()},
            {
                0,
                npc.position(),
                npc.judgement(),
                0,
            },
            0,
            displayAnimationContainsPoint(
                npc.animation(),
                npc.patterns(),
                npc.position(),
                npc.animationChart(),
                npc.direction(),
                npc.animationFrame(),
                part_enabled,
                camera_x,
                camera_y,
                point),
            pointerDistanceSquared(npc.position()),
        });
    }
    for (const EnemyActor& enemy :
         scenario_world_.enemies()) {
        if (classifyPlayerAttackTarget(
                player_.position(),
                player_.judgement(),
                attackTargetSnapshot(enemy)) ==
            PlayerAttackTargetDisposition::rejected) {
            continue;
        }
        const auto part_enabled =
            [&enemy](std::size_t part) {
                return enemy.partEnabled(part);
            };
        if (!displayAnimationIntersectsRectangle(
                enemy.animation(),
                enemy.patterns(),
                enemy.position(),
                enemy.animationChart(),
                enemy.direction(),
                enemy.animationFrame(),
                part_enabled,
                camera_x,
                camera_y,
                hit_rectangle)) {
            continue;
        }
        candidates.push_back({
            {WorldPointerTargetKind::enemy, enemy.id()},
            {
                0,
                enemy.position(),
                enemy.judgement(),
                0,
            },
            2,
            displayAnimationContainsPoint(
                enemy.animation(),
                enemy.patterns(),
                enemy.position(),
                enemy.animationChart(),
                enemy.direction(),
                enemy.animationFrame(),
                part_enabled,
                camera_x,
                camera_y,
                point),
            pointerDistanceSquared(enemy.position()),
        });
    }
    for (const GroundItem& item :
         scenario_world_.groundItems()) {
        if (!item.visible() || !item.pointerEnabled()) {
            continue;
        }
        const ItemWorldResource* resource =
            itemWorldResource(item.resource_id);
        const auto part_enabled = [](std::size_t) {
            return true;
        };
        const std::int32_t display_height =
            item.height * kRetailHeightScale / 100;
        if (!resource) {
            continue;
        }
        const ScreenPosition projected =
            calculateRealPosition(item.position);
        const ScreenPosition anchor{
            projected.x - camera_x,
            projected.y - camera_y,
        };
        const bool intersects =
            resource->animated()
                ? displayAnimationIntersectsRectangle(
                      resource->animation(),
                      resource->patterns(),
                      item.position,
                      item.animation_chart,
                      8,
                      0,
                      part_enabled,
                      camera_x,
                      camera_y,
                      hit_rectangle,
                      display_height)
                : item.animation_chart >= 0 &&
                      displayPatternIntersectsRectangle(
                          resource->patterns(),
                          static_cast<std::size_t>(
                              item.animation_chart),
                          anchor,
                          hit_rectangle,
                          display_height);
        if (!intersects) {
            continue;
        }
        const bool exact_hit =
            resource->animated()
                ? displayAnimationContainsPoint(
                      resource->animation(),
                      resource->patterns(),
                      item.position,
                      item.animation_chart,
                      8,
                      0,
                      part_enabled,
                      camera_x,
                      camera_y,
                      point,
                      display_height)
                : item.animation_chart >= 0 &&
                      displayPatternContainsPoint(
                          resource->patterns(),
                          static_cast<std::size_t>(
                              item.animation_chart),
                          anchor,
                          point,
                          display_height);
        candidates.push_back({
            {WorldPointerTargetKind::ground_item, item.id},
            {
                0,
                item.position,
                item.judgement,
                0,
            },
            3,
            exact_hit,
            pointerDistanceSquared(item.position),
        });
    }
    return candidates;
}

NpcActor* WorldScene::findScriptNpc(
    std::int32_t character_number) {
    std::vector<NpcActor>& people =
        scenario_world_.people();
    const auto found = std::find_if(
        people.begin(),
        people.end(),
        [character_number](const NpcActor& npc) {
            return 12000000 + npc.id() ==
                   character_number;
        });
    return found == people.end() ? nullptr : &*found;
}

const NpcActor* WorldScene::findScriptNpc(
    std::int32_t character_number) const {
    const std::vector<NpcActor>& people =
        scenario_world_.people();
    const auto found = std::find_if(
        people.begin(),
        people.end(),
        [character_number](const NpcActor& npc) {
            return 12000000 + npc.id() ==
                   character_number;
        });
    return found == people.end() ? nullptr : &*found;
}

ScenarioObjectActor* WorldScene::findScriptObject(
    std::int32_t character_number) {
    std::vector<ScenarioObjectActor>& objects =
        scenario_world_.objects();
    const auto found = std::find_if(
        objects.begin(),
        objects.end(),
        [character_number](const ScenarioObjectActor& object) {
            return object.characterNumber() ==
                   character_number;
        });
    return found == objects.end()
               ? nullptr
               : &*found;
}

const ScenarioObjectActor* WorldScene::findScriptObject(
    std::int32_t character_number) const {
    const std::vector<ScenarioObjectActor>& objects =
        scenario_world_.objects();
    const auto found = std::find_if(
        objects.begin(),
        objects.end(),
        [character_number](const ScenarioObjectActor& object) {
            return object.characterNumber() ==
                   character_number;
        });
    return found == objects.end()
               ? nullptr
               : &*found;
}

EnemyActor* WorldScene::findScriptEnemy(
    std::int32_t character_number) {
    std::vector<EnemyActor>& enemies =
        scenario_world_.enemies();
    const auto found = std::find_if(
        enemies.begin(),
        enemies.end(),
        [character_number](const EnemyActor& enemy) {
            return enemy.characterNumber() ==
                   character_number;
        });
    return found == enemies.end() ? nullptr : &*found;
}

const EnemyActor* WorldScene::findScriptEnemy(
    std::int32_t character_number) const {
    const std::vector<EnemyActor>& enemies =
        scenario_world_.enemies();
    const auto found = std::find_if(
        enemies.begin(),
        enemies.end(),
        [character_number](const EnemyActor& enemy) {
            return enemy.characterNumber() ==
                   character_number;
        });
    return found == enemies.end() ? nullptr : &*found;
}

EnemyActor* WorldScene::findEnemy(std::int32_t id) {
    std::vector<EnemyActor>& enemies =
        scenario_world_.enemies();
    const auto found = std::find_if(
        enemies.begin(),
        enemies.end(),
        [id](const EnemyActor& enemy) {
            return enemy.id() == id;
        });
    return found == enemies.end() ? nullptr : &*found;
}

const EnemyActor* WorldScene::findEnemy(
    std::int32_t id) const {
    const std::vector<EnemyActor>& enemies =
        scenario_world_.enemies();
    const auto found = std::find_if(
        enemies.begin(),
        enemies.end(),
        [id](const EnemyActor& enemy) {
            return enemy.id() == id;
        });
    return found == enemies.end() ? nullptr : &*found;
}

PlayerAttackTargetSnapshot WorldScene::attackTargetSnapshot(
    const EnemyActor& enemy) const {
    return {
        enemy.id(),
        enemy.position(),
        enemy.judgement(),
        enemy.currentLife(),
        enemy.visible(),
        enemy.pointerEnabled(),
    };
}

bool WorldScene::commandPlayerAttack(EnemyActor& enemy) {
    if (player_.actionLocked()) {
        return false;
    }
    const InventoryItem* main_hand =
        player_equipment_.item(
            EquipmentSlot::main_hand);
    const ItemDefinition* main_hand_definition =
        main_hand
            ? item_database_.find(
                  main_hand->category,
                  main_hand->definition_id)
            : nullptr;
    const PlayerAttackAction action =
        retailPlayerAttackAction(main_hand_definition);
    const std::int32_t attack_range =
        playerAttackActionIsRanged(action)
            ? std::numeric_limits<std::int32_t>::max()
            : kRetailPlayerAttackRange;
    const PlayerAttackTargetDisposition disposition =
        player_attack_target_.command(
            player_.position(),
            player_.judgement(),
            attackTargetSnapshot(enemy),
            attack_range);
    if (disposition ==
        PlayerAttackTargetDisposition::rejected) {
        return false;
    }
    pending_interaction_ = {};
    if (disposition ==
        PlayerAttackTargetDisposition::approach) {
        player_.followTo(enemy.position());
        return true;
    }
    return readyPlayerAttack(enemy);
}

void WorldScene::handlePlayerSpellEvent(
    const PlayerSpellActionEvent& event) {
    if (event.spell < 0) {
        return;
    }
    if (event.entry_visual_effect_number >= 0) {
        queueCombatEffect(
            buildPlayerSpellEntryVisual(
                event.entry_visual_effect_number,
                scenario_world_.localPlayerNumber(),
                player_.judgement()));
    }
    if (event.swing_sound_due &&
        event.spell == 15) {
        const InventoryItem* main_hand =
            player_equipment_.item(
                EquipmentSlot::main_hand);
        const ItemDefinition* definition =
            main_hand
                ? item_database_.find(
                      main_hand->category,
                      main_hand->definition_id)
                : nullptr;
        const std::int32_t sample =
            retailItemAttackSound(definition);
        if (sample >= 0) {
            pending_audio_samples_.push_back(sample);
        }
    }
    if (!event.cast_due) {
        return;
    }
    if (event.spell == 0) {
        createPlayerTransport({
            event.aim_world_x,
            event.aim_world_y,
        });
        return;
    }
    if (event.spell == 17) {
        player_identify_mode_active_ = true;
        gameplay_service_request_ = {
            GameplayServiceKind::identify_item,
            0,
        };
        return;
    }
    if (event.spell == 6) {
        const PlayerSpellParameters parameters =
            playerSpellParameters(
                player_magic_,
                event.spell,
                player_equipment_,
                item_database_,
                parameter_tables_,
                0,
                player_increased_power_.active());
        const PlayerHealSpellResolution resolution =
            resolvePlayerHealSpell({
                scenario_world_.localPlayerNumber(),
                player_data_.currentLife(),
                playerRuntimeProfile().maximum_life,
                parameters.effect_value,
                player_.judgement(),
            });
        if (!resolution.valid) {
            return;
        }
        queueCombatEffect(resolution.visual);
        if (resolution.award_practice) {
            player_data_.setCurrentLife(
                resolution.restored_life,
                playerRuntimeProfile().maximum_life);
            player_magic_.train(
                event.spell,
                false,
                parameter_tables_);
            pending_audio_samples_.push_back(
                resolution.audio_sample);
        }
        return;
    }
    if (event.spell == 7) {
        const PlayerSpellParameters parameters =
            playerSpellParameters(
                player_magic_,
                event.spell,
                player_equipment_,
                item_database_,
                parameter_tables_,
                0,
                player_increased_power_.active());
        player_moon_spell_.toggle(
            200,
            parameters.effective_level,
            parameter_tables_);
        effect_visuals_.load(
            data_root_, 11000040, nullptr);
        refreshCompanionRuntimeProfile();
        return;
    }
    if (event.spell == 8) {
        const PlayerSpellParameters parameters =
            playerSpellParameters(
                player_magic_,
                event.spell,
                player_equipment_,
                item_database_,
                parameter_tables_,
                0,
                player_increased_power_.active());
        player_berserker_spell_.toggle(
            201,
            parameters.effective_level,
            parameter_tables_);
        player_powerup_visual_.load(
            data_root_ / "Player" / "Common",
            "Powerup",
            nullptr);
        refreshPlayerRuntimeProfile();
        return;
    }
    if (event.spell == 9) {
        player_energy_shield_.toggle(
            playerCurrentMana());
        player_powerup_visual_.load(
            data_root_ / "Player" / "Common",
            "Powerup",
            nullptr);
        return;
    }
    if (event.spell == 18) {
        player_magic_shield_.toggle();
        player_counter_burst_.deactivate();
        effect_visuals_.load(
            data_root_, 11000240, nullptr);
        refreshPlayerRuntimeProfile();
        return;
    }
    if (event.spell == 19) {
        player_counter_burst_.toggle();
        player_magic_shield_.deactivate();
        effect_visuals_.load(
            data_root_, 11000250, nullptr);
        refreshPlayerRuntimeProfile();
        return;
    }
    if (event.spell == 20) {
        const WorldPosition destination{
            event.aim_world_x,
            event.aim_world_y,
        };
        if (hasCompanion() &&
            positionIsWalkable(
                scenario_world_.ground(),
                scenario_world_.objectMap(),
                destination,
                companion_.judgement())) {
            companion_.beginExplosion(destination);
        }
        return;
    }
    const bool requires_target =
        playerSpellRequiresCharacterTarget(
            event.spell);
    const EnemyActor* target =
        requires_target
            ? findScriptEnemy(
                  event.target_character_number)
            : nullptr;
    if (requires_target && !target) {
        return;
    }

    const PlayerRuntimeProfile profile =
        playerRuntimeProfile();
    PlayerCombatDefenseSnapshot affinity_source;
    affinity_source.character_number =
        scenario_world_.localPlayerNumber();
    affinity_source.attack =
        profile.physical_attack;
    affinity_source.physical_defense =
        profile.physical_defense;
    affinity_source.magical_defense =
        profile.magical_defense;
    affinity_source.element_x = player_data_.elementX();
    affinity_source.element_y = player_data_.elementY();

    PlayerSpellCastStats stats;
    stats.source_character_number =
        scenario_world_.localPlayerNumber();
    stats.player_level = player_data_.level();
    stats.physical_attack = profile.physical_attack;
    stats.magical_attack =
        profile.magical_attack;
    stats.physical_defense =
        affinity_source.physical_defense;
    stats.magical_defense =
        affinity_source.magical_defense;
    stats.magical_hit_rate =
        profile.magical_hit_rate;
    stats.element_affinities =
        buildPlayerElementAffinities(
            affinity_source,
            player_equipment_,
            player_inventory_,
            item_database_);
    stats.state_words =
        player_data_.combatPacketStateWords();

    const CombatEffectSpawnRequest request =
        buildPlayerSpellCast(
            event.spell,
            {
                stats,
                playerSpellParameters(
                    player_magic_,
                    event.spell,
                    player_equipment_,
                    item_database_,
                    parameter_tables_,
                    0,
                    player_increased_power_.active()),
                target
                    ? target->characterNumber()
                    : -1,
                player_.position(),
                player_.judgement(),
                target
                    ? target->position()
                    : WorldPosition{
                          event.aim_world_x,
                          event.aim_world_y,
                      },
                event.effect_delay,
            },
            parameter_tables_,
            &item_random_);
    if (event.spell == 15) {
        const InventoryItem* main_hand =
            player_equipment_.item(
                EquipmentSlot::main_hand);
        if (!main_hand) {
            return;
        }
        RuntimeEffectActorSpawnRequest actor;
        if (buildGenericEffectActor(
                request, player_.position(), actor) &&
            runtime_effects_.queueActor(
                std::move(actor))) {
            pending_audio_samples_.push_back(154);
        }
        return;
    }
    queueCombatEffect(request);
}

bool WorldScene::playerIdentifyModeActive() const {
    return player_identify_mode_active_;
}

bool WorldScene::identifyPlayerInventoryItem(
    std::int32_t item_index) {
    if (!player_identify_mode_active_ ||
        item_index < 0 ||
        !player_inventory_.identify(
            static_cast<std::size_t>(item_index))) {
        return false;
    }
    player_magic_.train(17, false, parameter_tables_);
    player_identify_mode_active_ = false;
    return true;
}

void WorldScene::cancelPlayerIdentifyMode() {
    player_identify_mode_active_ = false;
}

bool WorldScene::readyPlayerAttack(EnemyActor& enemy) {
    const InventoryItem* main_hand =
        player_equipment_.item(EquipmentSlot::main_hand);
    const ItemDefinition* main_hand_definition =
        main_hand
            ? item_database_.find(
                  main_hand->category,
                  main_hand->definition_id)
            : nullptr;
    PlayerAttackAction action =
        retailPlayerAttackAction(main_hand_definition);
    const std::int32_t attack_range =
        playerAttackActionIsRanged(action)
            ? std::numeric_limits<std::int32_t>::max()
            : kRetailPlayerAttackRange;
    if (classifyPlayerAttackTarget(
            player_.position(),
            player_.judgement(),
            attackTargetSnapshot(enemy),
            attack_range) !=
        PlayerAttackTargetDisposition::ready) {
        return false;
    }
    pending_interaction_ = {};
    player_.cancelMovement();
    player_.faceToward(enemy.position());
    if (!playerAttackActionIsSupported(action)) {
        player_attack_target_.cancel();
        return false;
    }
    player_increased_power_attack_targets_.clear();
    if (player_increased_power_.redirectsRangedAttack(
            player_data_.job(),
            static_cast<std::int32_t>(action),
            item_random_)) {
        player_increased_power_attack_targets_ =
            playerIncreasedPowerTargets();
        if (!player_increased_power_attack_targets_.empty()) {
            action = PlayerAttackAction::
                increased_power_ranged_21;
        }
    }
    if (!player_.beginAttack(
            action,
            enemy.id(),
            playerAttackSpeedTier(),
            player_visual_.animation())) {
        player_attack_target_.cancel();
        return false;
    }
    player_attack_target_.takeReadyTargetId();
    handlePlayerAttackEvent(player_.takeAttackEvent());
    pointer_.clearSelection();
    return true;
}

std::int32_t WorldScene::playerAttackSpeedTier() const {
    const PlayerRuntimeProfile profile =
        playerRuntimeProfile();
    return retailPlayerAttackSpeedTier(
        profile.attack_speed_raw,
        player_equipment_.totalWeight(item_database_),
        profile.weight_capacity,
        parameter_tables_.find(4));
}

void WorldScene::handlePlayerAttackEvent(
    const PlayerAttackActionEvent& event) {
    if (event.swing_sound_due) {
        const InventoryItem* main_hand =
            player_equipment_.item(
                EquipmentSlot::main_hand);
        const ItemDefinition* definition =
            main_hand
                ? item_database_.find(
                      main_hand->category,
                      main_hand->definition_id)
                : nullptr;
        pending_audio_samples_.push_back(
            playerAttackActionIsRanged(event.action)
                ? 3
                : retailItemAttackSound(definition));
    }
    if (!event.impact_due) {
        return;
    }
    if (playerAttackActionIsRanged(event.action)) {
        if (event.action == PlayerAttackAction::
                increased_power_ranged_21) {
            launchPlayerIncreasedPowerAttack();
        } else {
            launchPlayerRangedAttack(event);
        }
        return;
    }
    if (event.combo_step >= 0) {
        pending_audio_samples_.push_back(
            retailPlayerComboVoiceSample(
                player_data_.gender(), event.combo_step));
    } else if (event.action != PlayerAttackAction::basic) {
        pending_audio_samples_.push_back(
            retailPlayerAttackVoiceSample(
                player_data_.gender()));
    }
    if (event.combo_step >= 0) {
        for (EnemyActor& candidate :
             scenario_world_.enemies()) {
            if (classifyPlayerAttackTarget(
                    player_.position(),
                    player_.judgement(),
                    attackTargetSnapshot(candidate)) !=
                PlayerAttackTargetDisposition::ready) {
                continue;
            }
            pending_player_attack_impact_target_id_ =
                candidate.id();
            applyPlayerAttackImpact(candidate);
        }
        return;
    }
    EnemyActor* enemy = findEnemy(event.target_id);
    if (!enemy && event.target_id == -1) {
        auto found = std::find_if(
            scenario_world_.enemies().begin(),
            scenario_world_.enemies().end(),
            [this](const EnemyActor& candidate) {
                return classifyPlayerAttackTarget(
                           player_.position(),
                           player_.judgement(),
                           attackTargetSnapshot(candidate)) ==
                       PlayerAttackTargetDisposition::ready;
            });
        enemy = found == scenario_world_.enemies().end()
            ? nullptr
            : &*found;
    }
    if (!enemy ||
        classifyPlayerAttackTarget(
            player_.position(),
            player_.judgement(),
            attackTargetSnapshot(*enemy)) !=
            PlayerAttackTargetDisposition::ready) {
        return;
    }
    pending_player_attack_impact_target_id_ = enemy->id();
    applyPlayerAttackImpact(*enemy);
}

std::vector<std::int32_t>
WorldScene::playerIncreasedPowerTargets() const {
    constexpr std::int32_t kHalfExtent = 2000;
    constexpr std::size_t kMaximumTargets = 100;
    const WorldPosition source = player_.position();
    const std::int32_t left = source.x - kHalfExtent;
    const std::int32_t top = source.y - kHalfExtent;
    const std::int32_t right = source.x + kHalfExtent;
    const std::int32_t bottom = source.y + kHalfExtent;
    std::vector<std::int32_t> targets;
    targets.reserve(std::min(
        scenario_world_.enemies().size(), kMaximumTargets));
    for (const EnemyActor& enemy :
         scenario_world_.enemies()) {
        if (enemy.currentLife() <= 0) {
            continue;
        }
        const WorldPosition position = enemy.position();
        const ObjectBounds& judgement = enemy.judgement();
        if (position.x + judgement.right < left ||
            position.x + judgement.left > right ||
            position.y + judgement.bottom < top ||
            position.y + judgement.top > bottom) {
            continue;
        }
        targets.push_back(enemy.characterNumber());
        if (targets.size() == kMaximumTargets) {
            break;
        }
    }
    return targets;
}

void WorldScene::launchPlayerIncreasedPowerAttack() {
    const InventoryItem* main_hand =
        player_equipment_.item(
            EquipmentSlot::main_hand);
    const ItemDefinition* definition =
        main_hand
            ? item_database_.find(
                  main_hand->category,
                  main_hand->definition_id)
            : nullptr;
    const PlayerIncreasedPowerAttackResult attack =
        resolvePlayerIncreasedPowerAttack(
            {
                scenario_world_.localPlayerNumber(),
                buildPlayerAttackImpactStats(
                    scenario_world_.localPlayerNumber(),
                    player_data_,
                    player_equipment_,
                    player_inventory_,
                    item_database_,
                    playerRuntimeProfile()),
                player_increased_power_attack_targets_,
            },
            item_random_);
    if (!attack.valid || !main_hand || !definition ||
        itemCurrentDurability(
            *main_hand, *definition) == 0) {
        return;
    }
    for (const CombatEffectSpawnRequest& projectile :
         attack.projectiles) {
        RuntimeEffectActorSpawnRequest actor;
        if (buildGenericEffectActor(
                projectile,
                player_.position(),
                actor)) {
            runtime_effects_.queueActor(
                std::move(actor));
        }
    }
    if (attack.consume_durability &&
        !player_equipment_.decreaseDurability(
            EquipmentSlot::main_hand, 1)) {
        refreshPlayerAppearance();
    }
}

void WorldScene::launchPlayerRangedAttack(
    const PlayerAttackActionEvent& event) {
    const InventoryItem* main_hand =
        player_equipment_.item(
            EquipmentSlot::main_hand);
    const ItemDefinition* definition =
        main_hand
            ? item_database_.find(
                  main_hand->category,
                  main_hand->definition_id)
            : nullptr;
    if (!main_hand || !definition ||
        itemCurrentDurability(
            *main_hand, *definition) == 0) {
        return;
    }

    const EnemyActor* target =
        findEnemy(event.target_id);
    WorldPosition target_position;
    if (target) {
        target_position = target->position();
        player_.faceToward(target_position);
    } else {
        const double direction =
            retailAngleForDirection(
                player_.direction());
        target_position = {
            player_.position().x +
                static_cast<std::int32_t>(
                    std::cos(direction) * 1000.0),
            player_.position().y -
                static_cast<std::int32_t>(
                    std::sin(direction) * 1000.0),
        };
    }

    const PlayerRangedAttackResult ranged =
        resolvePlayerRangedAttack(
            {
                scenario_world_.localPlayerNumber(),
                player_.position(),
                target_position,
                event.target_id,
                player_data_.job(),
                player_data_.jobLevel(5),
                buildPlayerAttackImpactStats(
                    scenario_world_.localPlayerNumber(),
                    player_data_,
                    player_equipment_,
                    player_inventory_,
                    item_database_,
                    playerRuntimeProfile()),
                definition,
            },
            item_random_);
    if (!ranged.valid) {
        return;
    }
    for (const CombatEffectSpawnRequest& projectile :
         ranged.projectiles) {
        RuntimeEffectActorSpawnRequest actor;
        if (buildGenericEffectActor(
                projectile,
                player_.position(),
                actor)) {
            runtime_effects_.queueActor(
                std::move(actor));
        }
    }
    if (ranged.consume_durability &&
        !player_equipment_.decreaseDurability(
            EquipmentSlot::main_hand, 1)) {
        refreshPlayerAppearance();
    }
}

void WorldScene::applyPlayerAttackImpact(
    EnemyActor& enemy) {
    const PlayerAttackImpactStats stats =
        buildPlayerAttackImpactStats(
            scenario_world_.localPlayerNumber(),
            player_data_,
            player_equipment_,
            player_inventory_,
            item_database_,
            playerRuntimeProfile());
    EnemyDamageReceiverContext context;
    context.local_player_slot =
        scenario_world_.localPlayerNumber();
    context.local_player_available = true;
    context.source_player_available = true;
    context.source_player_position =
        player_.position();
    const InventoryItem* main_hand =
        player_equipment_.item(
            EquipmentSlot::main_hand);
    const ItemDefinition* definition =
        main_hand
            ? item_database_.find(
                  main_hand->category,
                  main_hand->definition_id)
            : nullptr;
    const PlayerAttackApplicationResult application =
        resolvePlayerAttackAgainstEnemy(
            {
                stats,
                enemy.id(),
                enemy.physicalEvasion(),
            },
            enemy.damageReceiverState(
                scenario_world_.id()),
            player_.position(),
            context,
            definition,
            parameter_tables_,
            item_random_);
    if (!application.impact.valid ||
        !application.impact.apply_damage) {
        return;
    }
    if (application.receiver.valid &&
        application.receiver.accepted) {
        enemy.applyDamageReceiverState(
            application.receiver.state);
        if (application.receiver.kill_requested) {
            accountEnemyKill(
                application.receiver.state,
                enemy.experienceReward(),
                definition
                    ? definition->subtype
                    : -1);
        }
        pending_audio_samples_.insert(
            pending_audio_samples_.end(),
            application.receiver.audio_samples.begin(),
            application.receiver.audio_samples.end());
        for (const CombatEffectSpawnRequest& effect :
             application.receiver.effects) {
            queueCombatEffect(effect);
        }
    }
    if (application.impact.post_hit_audio_sample >= 0) {
        pending_audio_samples_.push_back(
            application.impact.post_hit_audio_sample);
    }

    if (application.durability.lose_durability &&
        !player_equipment_.decreaseDurability(
            EquipmentSlot::main_hand, 1)) {
        refreshPlayerAppearance();
    }
}

GroundItem* WorldScene::findScriptGroundItem(
    std::int32_t character_number) {
    std::vector<GroundItem>& items =
        scenario_world_.groundItems();
    const auto found = std::find_if(
        items.begin(),
        items.end(),
        [character_number](const GroundItem& item) {
            return item.scenario_character_number ==
                   character_number;
        });
    return found == items.end() ? nullptr : &*found;
}

const GroundItem* WorldScene::findScriptGroundItem(
    std::int32_t character_number) const {
    const std::vector<GroundItem>& items =
        scenario_world_.groundItems();
    const auto found = std::find_if(
        items.begin(),
        items.end(),
        [character_number](const GroundItem& item) {
            return item.scenario_character_number ==
                   character_number;
        });
    return found == items.end() ? nullptr : &*found;
}

NpcActor* WorldScene::findNpc(std::int32_t id) {
    std::vector<NpcActor>& people =
        scenario_world_.people();
    const auto found = std::find_if(
        people.begin(),
        people.end(),
        [id](const NpcActor& npc) {
            return npc.id() == id;
        });
    return found == people.end() ? nullptr : &*found;
}

ScenarioObjectActor* WorldScene::findScenarioObject(
    std::int32_t id) {
    std::vector<ScenarioObjectActor>& objects =
        scenario_world_.objects();
    const auto found = std::find_if(
        objects.begin(),
        objects.end(),
        [id](const ScenarioObjectActor& object) {
            return object.id() == id;
        });
    return found == objects.end()
        ? nullptr
        : &*found;
}

GroundItem* WorldScene::findGroundItem(std::int32_t id) {
    std::vector<GroundItem>& ground_items =
        scenario_world_.groundItems();
    const auto found = std::find_if(
        ground_items.begin(),
        ground_items.end(),
        [id](const GroundItem& item) {
            return item.id == id;
        });
    return found == ground_items.end()
        ? nullptr
        : &*found;
}

bool WorldScene::startGroundItemInteraction(
    std::int32_t item_id) {
    std::vector<GroundItem>& ground_items =
        scenario_world_.groundItems();
    const auto found = std::find_if(
        ground_items.begin(),
        ground_items.end(),
        [item_id](const GroundItem& item) {
            return item.id == item_id;
        });
    if (found == ground_items.end()) {
        pending_interaction_ = {};
        return false;
    }
    if (!found->visible() ||
        !found->pointerEnabled()) {
        pending_interaction_ = {};
        return false;
    }

    pending_interaction_ = {};
    player_.cancelMovement();
    const ItemDefinition* definition =
        item_database_.find(
            found->item.category,
            found->item.definition_id);
    const bool mine_item =
        found->item.category == 4 &&
        found->item.definition_id == 1;
    if (definition && mine_item) {
        if (player_item_controller_.collectMine(
                playerMaximumMineCount())) {
            pending_audio_samples_.push_back(
                retailItemMoveSound(*definition));
            if (pointer_.target().kind ==
                    WorldPointerTargetKind::ground_item &&
                pointer_.target().id == item_id) {
                pointer_.clearSelection();
            }
            ground_items.erase(found);
            return true;
        }

        // InsertPickedUpItem (0x00449ef0) keeps category-four definition
        // one outside the 9-by-4 owner. At maximum capacity it returns the
        // still-live instance to the caller, whose single-player failure
        // tail recreates the ordinary drop response.
        restartGroundItemDrop(*found);
        if (pointer_.target().kind ==
                WorldPointerTargetKind::ground_item &&
            pointer_.target().id == item_id) {
            pointer_.clearSelection();
        }
        return true;
    }
    if (definition &&
        definition->automatic_inventory_page >= 0) {
        if (!player_automatic_items_.add(
                *definition, found->item)) {
            restartGroundItemDrop(*found);
            if (pointer_.target().kind ==
                    WorldPointerTargetKind::ground_item &&
                pointer_.target().id == item_id) {
                pointer_.clearSelection();
            }
            return true;
        }
        pending_audio_samples_.push_back(
            retailItemMoveSound(*definition));
        if (pointer_.target().kind ==
                WorldPointerTargetKind::ground_item &&
            pointer_.target().id == item_id) {
            pointer_.clearSelection();
        }
        ground_items.erase(found);
        return true;
    }
    if (!definition ||
        !player_inventory_.store(found->item)) {
        // The single-player failure tail of FUN_004526a0 recreates the
        // concrete item as a mode-zero world drop. Its ordinary first-impact
        // update supplies the audible feedback; there is no pickup sound.
        restartGroundItemDrop(*found);
        if (pointer_.target().kind ==
                WorldPointerTargetKind::ground_item &&
            pointer_.target().id == item_id) {
            pointer_.clearSelection();
        }
        return true;
    }

    pending_audio_samples_.push_back(
        retailItemMoveSound(*definition));
    if (pointer_.target().kind ==
            WorldPointerTargetKind::ground_item &&
        pointer_.target().id == item_id) {
        pointer_.clearSelection();
    }
    ground_items.erase(found);
    return true;
}


}  // namespace osf
