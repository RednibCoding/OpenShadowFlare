#include "world_scene.hpp"
#include "enemy_death_rewards.hpp"
#include "movement_controller.hpp"
#include "script/scenario_attached_effect_command.hpp"
#include "script/scenario_effect_command.hpp"
#include "script/scenario_numeric_label_command.hpp"
#include "script/scenario_placed_effect_command.hpp"
#include "vendor_stock_generator.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <optional>
#include <utility>

namespace osf {
namespace {

bool decodeEntityStateKey(
    std::int32_t key,
    std::int32_t& character_number,
    ScenarioEntityStateChannel& channel) {
    if (key >= 300000000 && key < 400000000) {
        character_number = key - 300000000;
        channel = ScenarioEntityStateChannel::pointer;
        return true;
    }
    if (key >= 200000000 && key < 300000000) {
        character_number = key - 200000000;
        channel = ScenarioEntityStateChannel::judgement;
        return true;
    }
    if (key >= 100000000 && key < 200000000) {
        character_number = key - 100000000;
        channel = ScenarioEntityStateChannel::visible;
        return true;
    }
    return false;
}

std::uint64_t scriptOperandKey(
    const script::Operand& operand) {
    return
        (static_cast<std::uint64_t>(
             static_cast<std::uint32_t>(operand.type))
         << 32u) |
        static_cast<std::uint32_t>(operand.value);
}

bool boundsIntersect(
    WorldPosition first_position,
    const ObjectBounds& first_bounds,
    WorldPosition second_position,
    const ObjectBounds& second_bounds) {
    return
        first_position.x + first_bounds.left <=
            second_position.x + second_bounds.right &&
        second_position.x + second_bounds.left <=
            first_position.x + first_bounds.right &&
        first_position.y + first_bounds.top <=
            second_position.y + second_bounds.bottom &&
        second_position.y + second_bounds.top <=
            first_position.y + first_bounds.bottom;
}

std::int32_t scriptAudioDistance(
    WorldPosition first,
    WorldPosition second) {
    const double x =
        static_cast<double>(first.x) - second.x;
    const double y =
        static_cast<double>(first.y) - second.y;
    return static_cast<std::int32_t>(
        std::trunc(std::hypot(x, y)));
}

std::optional<EquipmentRepairGroup> equipmentRepairGroup(
    std::int32_t selector) {
    switch (selector) {
    case 0:
        return EquipmentRepairGroup::arms;
    case 1:
        return EquipmentRepairGroup::helmet;
    case 2:
        return EquipmentRepairGroup::body;
    case 3:
        return EquipmentRepairGroup::shields;
    case 4:
        return EquipmentRepairGroup::boots;
    default:
        return std::nullopt;
    }
}

}  // namespace

bool WorldScene::readScriptWorldOperand(
    const script::Operand& operand,
    std::int32_t& value) const {
    if (operand.type == 9) {
        value = 0;
        if (!has_player_ ||
            player_.damagePresentation().action != 1 ||
            !scriptCharacterDisplayed(operand.value)) {
            return true;
        }
        WorldPosition position;
        const ObjectBounds* judgement = nullptr;
        if (!scriptCharacterBounds(
                operand.value, position, judgement)) {
            return true;
        }
        // EvaluateScriptOperand (0x004346b0) uses the player's live
        // interaction range at +0x3f4 after finding the target in the
        // object-display registry. The normal retail value is 0x9f.
        constexpr std::int32_t kRetailInteractionDistance = 0x9f;
        value = distanceBetweenBounds(
                    player_.position(),
                    player_.judgement(),
                    position,
                    *judgement) <=
                kRetailInteractionDistance
            ? 1
            : 0;
        return true;
    }
    if (operand.type == 10) {
        value = transports_.enabled(operand.value) ? 1 : 0;
        return true;
    }
    if (operand.type == 12) {
        value = quests_.state(operand.value);
        return true;
    }
    if (operand.type == 11) {
        value =
            operand.value >= 0 &&
                    static_cast<std::size_t>(operand.value) <
                        script_state_flags_.size()
                ? script_state_flags_[
                      static_cast<std::size_t>(operand.value)]
                : 0;
        return true;
    }
    if (operand.type == 13) {
        const auto found =
            script_persistent_values_.find(
                scriptOperandKey(operand));
        value =
            found == script_persistent_values_.end()
                ? 0
                : found->second;
        return true;
    }
    if (operand.type == 5) {
        std::int32_t character_number = 0;
        ScenarioEntityStateChannel channel =
            ScenarioEntityStateChannel::visible;
        if (!decodeEntityStateKey(
                operand.value, character_number, channel)) {
            const auto found =
                script_persistent_values_.find(
                    scriptOperandKey(operand));
            value =
                found == script_persistent_values_.end()
                    ? 0
                    : found->second;
            return true;
        }
        if (const ScenarioObjectActor* object =
                findScriptObject(character_number)) {
            value = object->stateValue(channel);
            return true;
        }
        if (const NpcActor* npc =
                findScriptNpc(character_number)) {
            value = npc->stateValue(channel);
            return true;
        }
        if (const GroundItem* item =
                findScriptGroundItem(character_number)) {
            value = item->state.value(channel);
            return true;
        }
        if (const EnemyActor* enemy =
                findScriptEnemy(character_number)) {
            value = enemy->stateValue(channel);
            return true;
        }
        return false;
    }
    if (operand.type != 6 && operand.type != 7) {
        return false;
    }
    if (const ScenarioObjectActor* object =
            findScriptObject(operand.value)) {
        value = operand.type == 6
                    ? object->position().x
                    : object->position().y;
        return true;
    }
    if (const NpcActor* npc =
            findScriptNpc(operand.value)) {
        value = operand.type == 6
                    ? npc->position().x
                    : npc->position().y;
        return true;
    }
    if (const GroundItem* item =
            findScriptGroundItem(operand.value)) {
        value = operand.type == 6
                    ? item->position.x
                    : item->position.y;
        return true;
    }
    if (const EnemyActor* enemy =
            findScriptEnemy(operand.value)) {
        value = operand.type == 6
                    ? enemy->position().x
                    : enemy->position().y;
        return true;
    }
    return false;
}

bool WorldScene::queryScriptEnemyLifecycleState(
    std::int32_t character_number,
    std::int32_t& state) const {
    const EnemyActor* enemy =
        findScriptEnemy(character_number);
    if (!enemy) {
        return false;
    }
    // Retail keeps the scenario registry active throughout the complete
    // death presentation, then clears it when that presentation expires.
    state = enemy->expired() ? 0 : 1;
    return true;
}

bool WorldScene::writeScriptWorldOperand(
    const script::Operand& operand,
    std::int32_t value) {
    if (operand.type == 10) {
        if (operand.value < 0 ||
            static_cast<std::size_t>(operand.value) >=
                transports_.enabledFlags().size()) {
            return false;
        }
        transports_.setEnabled(operand.value, value != 0);
        return true;
    }
    if (operand.type == 12) {
        return quests_.setScriptState(operand.value, value);
    }
    if (operand.type == 11) {
        if (operand.value < 0) {
            return false;
        }
        const std::size_t index =
            static_cast<std::size_t>(operand.value);
        if (index >= script_state_flags_.size()) {
            script_state_flags_.resize(index + 1u, 0);
        }
        script_state_flags_[index] = value;
        return true;
    }
    if (operand.type == 13) {
        script_persistent_values_.insert_or_assign(
            scriptOperandKey(operand), value);
        return true;
    }
    if (operand.type != 5) {
        return false;
    }
    std::int32_t character_number = 0;
    ScenarioEntityStateChannel channel =
        ScenarioEntityStateChannel::visible;
    if (!decodeEntityStateKey(
            operand.value, character_number, channel)) {
        script_persistent_values_.insert_or_assign(
            scriptOperandKey(operand), value);
        return true;
    }
    if (ScenarioObjectActor* object =
            findScriptObject(character_number)) {
        object->setStateValue(channel, value);
        return true;
    }
    if (NpcActor* npc = findScriptNpc(character_number)) {
        npc->setStateValue(channel, value);
        return true;
    }
    if (GroundItem* item =
            findScriptGroundItem(character_number)) {
        item->state.setValue(channel, value);
        if (!item->visible() || !item->pointerEnabled()) {
            if (pointer_.target().kind ==
                    WorldPointerTargetKind::ground_item &&
                pointer_.target().id == item->id) {
                pointer_.clearSelection();
            }
            if (pending_interaction_.kind ==
                    WorldPointerTargetKind::ground_item &&
                pending_interaction_.id == item->id) {
                pending_interaction_ = {};
                player_.cancelMovement();
            }
        }
        return true;
    }
    if (EnemyActor* enemy =
            findScriptEnemy(character_number)) {
        enemy->setStateValue(channel, value);
        return true;
    }
    return false;
}

bool WorldScene::executeScriptNativeCommand(
    std::int32_t opcode,
    const std::vector<std::int32_t>& arguments) {
    if (opcode == 64) {
        if (arguments.size() != 1) {
            return false;
        }
        beginScenarioVisual(arguments[0]);
        return true;
    }

    if (opcode == 65) {
        if (arguments.size() != 4) {
            return false;
        }
        scenario_screen_particles_.request(
            arguments[0],
            arguments[1],
            arguments[2],
            arguments[3]);
        return true;
    }

    if (opcode == 26) {
        if (arguments.size() != 7) {
            return false;
        }
        WorldPosition position;
        if (arguments[0] >= 0 && arguments[0] < 4) {
            if (arguments[0] !=
                    scenario_world_.localPlayerNumber() ||
                !has_player_) {
                // Other player slots are absent in portable single-player.
                return true;
            }
            position = player_.position();
        } else {
            const ObjectBounds* judgement = nullptr;
            if (!scriptCharacterBounds(
                    arguments[0], position, judgement)) {
                return true;
            }
        }
        ScenarioTextLabel label;
        if (!makeScenarioNumericLabel(
                arguments, position, label)) {
            return false;
        }
        scenario_text_labels_.push_back(std::move(label));
        return true;
    }

    if (opcode == 29) {
        // Retail only reaches this branch as a network client and sends
        // packet 0x22. Play mode zero takes opcode 28 instead, so there is
        // intentionally no local state mutation in the portable game.
        return arguments.size() == 1;
    }

    if (opcode == 60) {
        if (arguments.size() != 1) {
            return false;
        }
        player_unlock_switch_active_ =
            has_player_ && arguments[0] != 0;
        return true;
    }

    if (opcode == 25) {
        if (arguments.size() != 4) {
            return false;
        }
        EnemyActor* enemy =
            findScriptEnemy(arguments[0]);
        if (!enemy) {
            return false;
        }
        enemy->activate(
            {arguments[1], arguments[2]},
            arguments[3]);
        return true;
    }

    if (opcode == 27) {
        if (arguments.size() != 8) {
            return false;
        }
        WorldPosition position;
        const ObjectBounds* judgement = nullptr;
        if (!scriptCharacterBounds(
                arguments[0], position, judgement)) {
            if (arguments[0] !=
                    scenario_world_.localPlayerNumber() ||
                !has_player_) {
                return false;
            }
            position = player_.position();
        }
        const script::Message* message =
            scenario_script_.data().findMessage(arguments[3]);
        if (!message) {
            return false;
        }
        scenario_text_labels_.push_back({
            position,
            arguments[1],
            arguments[2],
            message->text,
            arguments[4],
            arguments[5],
            arguments[6],
            arguments[7],
        });
        return true;
    }

    if (opcode == 46) {
        if (arguments.size() != 2) {
            return false;
        }
        ScenarioObjectActor* object =
            findScriptObject(arguments[0]);
        if (object) {
            object->setDrawStrength(arguments[1]);
        }
        return true;
    }

    if (opcode == 7) {
        if (!arguments.empty() || !has_player_) {
            return false;
        }
        const PlayerRuntimeProfile profile =
            playerRuntimeProfile();
        player_data_.setCurrentLife(
            profile.maximum_life, profile.maximum_life);
        if (hasCompanion() && companion_.currentLife() > 0) {
            companion_.restoreLife(0, 100);
        }
        return true;
    }

    if (opcode == 8) {
        if (!arguments.empty() || !has_player_) {
            return false;
        }
        const std::int32_t maximum_mana =
            playerRuntimeProfile().maximum_mana;
        player_data_.setCurrentMana(
            maximum_mana, maximum_mana);
        return true;
    }

    if (opcode == 30) {
        CombatEffectSpawnRequest request;
        if (!makeScenarioEffectRequest(
                arguments, item_random_.next(), request)) {
            return false;
        }
        queueCombatEffect(request);
        return true;
    }

    if (opcode == 36) {
        CombatEffectSpawnRequest request;
        if (!makeScenarioPlacedEffectRequest(
                arguments, request)) {
            return false;
        }
        queueCombatEffect(request);
        return true;
    }

    if (opcode == 40) {
        if (arguments.size() != 2) {
            return false;
        }
        constexpr std::int32_t kPlayerOwner = 1;
        constexpr std::int32_t kScenarioActorOwner = 4;
        const std::int32_t source_character_number = arguments[1];
        std::int32_t owner_kind = kScenarioActorOwner;
        ObjectBounds source_judgement;
        if (source_character_number >= 0 &&
            source_character_number < 4) {
            if (!has_player_ ||
                source_character_number !=
                    scenario_world_.localPlayerNumber()) {
                return true;
            }
            owner_kind = kPlayerOwner;
            source_judgement = player_.judgement();
        } else {
            WorldPosition source_position;
            const ObjectBounds* judgement = nullptr;
            if (!scriptCharacterBounds(
                    source_character_number,
                    source_position,
                    judgement)) {
                return true;
            }
            source_judgement = *judgement;
        }

        CombatEffectSpawnRequest request;
        if (!makeScenarioAttachedEffectRequest(
                arguments,
                owner_kind,
                source_judgement,
                request)) {
            return false;
        }
        queueCombatEffect(request);
        return true;
    }

    if (opcode == 59) {
        return arguments.size() == 2 &&
               removeScriptItem(arguments[0], arguments[1]);
    }

    if (opcode == 75) {
        return arguments.size() == 2 &&
               addScriptItem(arguments[0], arguments[1]);
    }

    if (opcode == 4) {
        player_equipment_.identifyAll();
        player_inventory_.identifyAll();
        player_belt_.identifyAll();
        refreshPlayerRuntimeProfile();
        return true;
    }

    if (opcode == 9) {
        if (arguments.size() != 1) {
            return false;
        }
        if (arguments[0] == -1) {
            player_inventory_.repairAll(item_database_);
        } else {
            const std::optional<EquipmentRepairGroup> group =
                equipmentRepairGroup(arguments[0]);
            if (!group) {
                return false;
            }
            player_equipment_.repair(*group, item_database_);
        }
        refreshPlayerRuntimeProfile();
        return true;
    }

    if (opcode == 54) {
        return arguments.size() == 1 &&
               player_inventory_.spendGold(arguments[0]);
    }

    if (opcode == 56) {
        if (arguments.size() != 4) {
            return false;
        }
        if (ScenarioObjectActor* object =
                findScriptObject(arguments[0])) {
            object->setStateOverride(
                arguments[1], arguments[2], arguments[3]);
        } else if (NpcActor* npc =
                       findScriptNpc(arguments[0])) {
            npc->setStateOverride(
                arguments[1], arguments[2], arguments[3]);
        } else if (GroundItem* item =
                       findScriptGroundItem(arguments[0])) {
            item->state.setOverride(
                arguments[1], arguments[2], arguments[3]);
        } else if (EnemyActor* enemy =
                       findScriptEnemy(arguments[0])) {
            enemy->setStateOverride(
                arguments[1], arguments[2], arguments[3]);
        }
        // The retail handler treats an absent target as a successful no-op.
        return true;
    }

    if (opcode == 16) {
        if (arguments.empty() || arguments[0] < 0) {
            return false;
        }
        constexpr std::int32_t kAudibleRange = 3000;
        if (arguments.size() >= 4 &&
            arguments[1] == 0 &&
            scriptAudioDistance(
                player_.position(),
                {arguments[2], arguments[3]}) >
                kAudibleRange) {
            return true;
        }
        pending_audio_samples_.push_back(arguments[0]);
        return true;
    }

    if (opcode == 17) {
        if (arguments.size() < 2 ||
            script_travel_pending_) {
            return false;
        }
        pending_script_travel_ = {
            arguments[0],
            arguments[1],
            0,
        };
        script_travel_pending_ = true;
        return true;
    }

    if (opcode == 10) {
        if (arguments.size() < 6) {
            return false;
        }
        std::vector<GroundItem>& ground_items =
            scenario_world_.groundItems();
        const std::size_t first_item = ground_items.size();
        if (!createGroundItems(
                ground_items,
                item_random_,
                arguments[0],
                arguments[1],
                {arguments[2], arguments[3]},
                arguments[4],
                arguments[5])) {
            return false;
        }
        return prepareGroundItems(first_item);
    }

    if (opcode == 6) {
        if (arguments.size() < 2 || arguments[0] < 0) {
            return false;
        }
        const std::size_t inventory_index =
            static_cast<std::size_t>(arguments[0]);
        if (vendor_inventories_.size() <= inventory_index) {
            vendor_inventories_.resize(inventory_index + 1);
        }
        return generateRetailVendorStock(
            vendor_inventories_[inventory_index],
            arguments[1],
            parameter_tables_,
            item_database_,
            item_random_);
    }

    if (opcode == 5) {
        if (arguments.empty() ||
            !vendorInventory(arguments[0]) ||
            gameplay_service_request_.kind !=
                GameplayServiceKind::none) {
            return false;
        }
        gameplay_service_request_ = {
            GameplayServiceKind::vendor,
            arguments[0],
        };
        return true;
    }

    if (opcode == 24) {
        if (arguments.size() < 3) {
            return false;
        }
        constexpr std::int32_t kEpisodeOneMask = 1;
        const std::vector<EnemyDeathDrop> drops =
            createRetailEnemyDrops(
                arguments[0],
                0,
                0,
                0,
                {arguments[1], arguments[2]},
                {},
                0,
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
        for (const EnemyDeathDrop& drop : drops) {
            if (!createGroundItem(
                    ground_items,
                    drop.item,
                    drop.position)) {
                ground_items.resize(first_item);
                next_ground_item_id_ = first_id;
                return false;
            }
        }
        if (!prepareGroundItems(first_item)) {
            ground_items.resize(first_item);
            next_ground_item_id_ = first_id;
            return false;
        }
        return true;
    }

    if (opcode == 48) {
        if (arguments.empty()) {
            return false;
        }
        quests_.selectNotice(arguments[0]);
        return true;
    }

    if (opcode == 37) {
        if (arguments.empty() ||
            gameplay_service_request_.kind !=
                GameplayServiceKind::none) {
            return false;
        }
        gameplay_service_request_ = {
            GameplayServiceKind::transport,
            arguments[0],
        };
        script_transport_service_ = arguments[0];
        return true;
    }

    if (opcode == 38) {
        if (arguments.size() != 1) {
            return false;
        }
        if (script_transport_service_ == arguments[0] &&
            (gameplay_service_request_.kind ==
                 GameplayServiceKind::none ||
             gameplay_service_request_.kind ==
                 GameplayServiceKind::transport)) {
            gameplay_service_request_ = {
                GameplayServiceKind::close_transport,
                arguments[0],
            };
            script_transport_service_ = -1;
        }
        return true;
    }

    if (opcode == 41) {
        if (arguments.empty() ||
            gameplay_service_request_.kind !=
                GameplayServiceKind::none) {
            return false;
        }
        gameplay_service_request_ = {
            GameplayServiceKind::toggle_special_items,
            arguments[0],
        };
        return true;
    }

    if (opcode == 45) {
        return arguments.size() == 1 &&
               switchOwnedCompanion(arguments[0]);
    }

    if (opcode == 62) {
        if (arguments.size() < 3) {
            return false;
        }
        // Argument two requests the retail server broadcast when a quest is
        // completed. The initial scenario is strictly single-player, but the
        // interpreter still evaluates and preserves that argument.
        if (!quests_.applyScriptUpdate(
                arguments[0], arguments[1])) {
            return false;
        }
        const QuestCue cue = quests_.lastCue();
        if (cue == QuestCue::updated) {
            pending_audio_samples_.push_back(65);
        } else if (cue == QuestCue::completed) {
            pending_audio_samples_.push_back(66);
        }
        return true;
    }

    if (opcode == 68) {
        if (arguments.size() != 1) {
            return false;
        }
        const PlayerExperienceAwardResult award =
            awardRetailPlayerExperiencePercentage(
                player_data_,
                arguments[0],
                parameter_tables_);
        presentPlayerLevelUp(award.level_up);
        return true;
    }

    if (opcode == 67) {
        return arguments.size() == 1 &&
               player_magic_.learnPermanently(arguments[0]);
    }

    if (opcode == 70) {
        if (arguments.size() != 1) {
            return false;
        }
        const std::optional<PlayerJob> job =
            retailJobForScriptSelection(arguments[0]);
        if (job && has_player_) {
            player_data_.setJob(*job);
        }
        return true;
    }

    if (opcode == 72) {
        if (!arguments.empty() ||
            gameplay_service_request_.kind !=
                GameplayServiceKind::none) {
            return false;
        }
        gameplay_service_request_ = {
            GameplayServiceKind::equipment_color,
            0,
        };
        return true;
    }

    if (opcode == 73) {
        if (!arguments.empty() ||
            gameplay_service_request_.kind !=
                GameplayServiceKind::none) {
            return false;
        }
        gameplay_service_request_ = {
            GameplayServiceKind::blackjack,
            0,
        };
        return true;
    }

    if ((opcode != 18 && opcode != 19 && opcode != 20 &&
         opcode != 21) ||
        arguments.empty()) {
        return false;
    }
    NpcActor* npc = findScriptNpc(arguments.front());
    if (!npc) {
        return false;
    }
    if (opcode == 19) {
        npc->endInteraction();
        return true;
    }
    if (opcode == 20) {
        return arguments.size() == 6 &&
               npc->startScriptAction(
                   arguments[1],
                   arguments[2],
                   arguments[3],
                   arguments[4]);
    }
    if (opcode == 18) {
        npc->beginInteraction();
        scenario_script_.setActorId(npc->id());
        pointer_.clearSelection();
        return true;
    }
    if (arguments.size() < 2) {
        return false;
    }
    if (arguments[1] == 0) {
        npc->faceToward(player_.position());
        return true;
    }
    const NpcActor* target =
        findScriptNpc(arguments[1]);
    if (!target) {
        return false;
    }
    npc->faceToward(target->position());
    return true;
}

bool WorldScene::queryScriptValue(
    script::ValueQuery query,
    std::int32_t& value) const {
    if (!has_player_) {
        return false;
    }
    switch (query) {
    case script::ValueQuery::local_player_number:
        value = scenario_world_.localPlayerNumber();
        return true;
    case script::ValueQuery::local_player_gender:
        value = player_data_.gender();
        return true;
    case script::ValueQuery::local_player_level:
        value = player_data_.level();
        return true;
    case script::ValueQuery::local_player_companion_type:
        value = player_data_.companionType();
        return true;
    case script::ValueQuery::play_mode:
        value = 0;
        return true;
    case script::ValueQuery::local_player_current_life:
        value = playerCurrentLife();
        return true;
    case script::ValueQuery::local_player_maximum_life:
        value = playerRuntimeProfile().maximum_life;
        return true;
    case script::ValueQuery::local_player_current_mana:
        value = playerCurrentMana();
        return true;
    case script::ValueQuery::local_player_maximum_mana:
        value = playerRuntimeProfile().maximum_mana;
        return true;
    case script::ValueQuery::local_player_condition_current:
    case script::ValueQuery::local_player_condition_maximum:
        // The initial portable combat slice does not yet model the optional
        // player condition at runtime offsets 0xf8/0xfc. Retail returns two
        // equal -1 sentinels while it is inactive, which is the normal state
        // exercised by Syria's repeat conversation.
        value = -1;
        return true;
    case script::ValueQuery::local_player_gold:
        value = player_inventory_.gold();
        return true;
    case script::ValueQuery::local_player_has_unidentified_items:
        value =
            player_equipment_.hasUnidentifiedItems() ||
                    player_inventory_.hasUnidentifiedItems() ||
                    player_belt_.hasUnidentifiedItems()
                ? 1
                : 0;
        return true;
    case script::ValueQuery::local_player_repair_price:
    case script::ValueQuery::local_player_spell_learned:
        return false;
    case script::ValueQuery::local_player_job_selection:
        value = retailScriptJobSelection(player_data_.job());
        return true;
    case script::ValueQuery::scenario_entry_value:
        value = scenario_world_.entryValue();
        return true;
    case script::ValueQuery::blackjack_result:
        value = blackjack_result_;
        return true;
    }
    return false;
}

bool WorldScene::queryScriptIndexedValue(
    script::ValueQuery query,
    std::int32_t index,
    std::int32_t& value) const {
    if (!has_player_) {
        return false;
    }
    if (query ==
        script::ValueQuery::local_player_spell_learned) {
        if (index < 0 ||
            static_cast<std::size_t>(index) >=
                PlayerMagic::spell_count) {
            return false;
        }
        value = player_magic_.permanentlyLearned(index) ? 1 : 0;
        return true;
    }
    if (query !=
        script::ValueQuery::local_player_repair_price) {
        return false;
    }
    const TableData* value_parameters =
        parameter_tables_.find(34);
    if (!value_parameters) {
        return false;
    }
    if (index == -1) {
        value = player_inventory_.repairPrice(
            item_database_, *value_parameters);
        return true;
    }
    const std::optional<EquipmentRepairGroup> group =
        equipmentRepairGroup(index);
    if (!group) {
        return false;
    }
    value = player_equipment_.repairPrice(
        *group, item_database_, *value_parameters);
    return true;
}

bool WorldScene::measureScriptCharacterDistance(
    std::int32_t character_number,
    std::int32_t& distance) const {
    if (!has_player_) {
        return false;
    }

    WorldPosition position;
    const ObjectBounds* judgement = nullptr;
    if (!scriptCharacterBounds(
            character_number, position, judgement)) {
        return false;
    }

    distance = distanceBetweenBounds(
        player_.position(),
        player_.judgement(),
        position,
        *judgement);
    return true;
}

bool WorldScene::queryScriptLocalPlayerTarget(
    std::int32_t character_number,
    std::int32_t lower_distance,
    std::int32_t upper_distance,
    script::LocalPlayerTarget& target) const {
    target = {};
    WorldPosition source_position;
    const ObjectBounds* source_judgement = nullptr;
    if (!scriptCharacterBounds(
            character_number,
            source_position,
            source_judgement)) {
        return true;
    }
    target.source_found = true;
    if (!has_player_ || player_data_.currentLife() <= 0) {
        return true;
    }

    const std::int32_t distance = distanceBetweenBounds(
        source_position,
        *source_judgement,
        player_.position(),
        player_.judgement());
    if ((lower_distance != -1 && distance < lower_distance) ||
        (upper_distance != -1 && distance > upper_distance)) {
        return true;
    }
    target.player_number = scenario_world_.localPlayerNumber();
    target.world_x = player_.position().x;
    target.world_y = player_.position().y;
    return true;
}

bool WorldScene::scriptCharacterBounds(
    std::int32_t character_number,
    WorldPosition& position,
    const ObjectBounds*& judgement) const {
    judgement = nullptr;
    if (const ScenarioObjectActor* object =
            findScriptObject(character_number)) {
        position = object->position();
        judgement = &object->judgement();
    } else if (const NpcActor* npc =
                   findScriptNpc(character_number)) {
        position = npc->position();
        judgement = &npc->judgement();
    } else if (const EnemyActor* enemy =
                   findScriptEnemy(character_number)) {
        position = enemy->position();
        judgement = &enemy->judgement();
    } else if (const GroundItem* item =
                   findScriptGroundItem(character_number)) {
        position = item->position;
        judgement = &item->judgement;
    }
    return judgement != nullptr;
}

bool WorldScene::scriptCharacterDisplayed(
    std::int32_t character_number) const {
    if (const ScenarioObjectActor* object =
            findScriptObject(character_number)) {
        return object->visible();
    }
    if (const NpcActor* npc =
            findScriptNpc(character_number)) {
        return npc->visible();
    }
    if (const EnemyActor* enemy =
            findScriptEnemy(character_number)) {
        return enemy->visible() && !enemy->expired();
    }
    if (const GroundItem* item =
            findScriptGroundItem(character_number)) {
        return item->visible();
    }
    return false;
}

void WorldScene::runScenarioContactTriggers() {
    if (!has_player_ || scenario_script_.messageActive()) {
        return;
    }
    const WorldPosition player_position = player_.position();
    const ObjectBounds& player_judgement = player_.judgement();
    for (const script::Status& status :
         scenario_script_.data().statuses()) {
        if (status.kind != 3) {
            continue;
        }

        WorldPosition position;
        const ObjectBounds* judgement = nullptr;
        if (!scriptCharacterBounds(
                status.character_number,
                position,
                judgement) ||
            !boundsIntersect(
                player_position,
                player_judgement,
                position,
                *judgement)) {
            continue;
        }
        scenario_script_.startStatus(
            3, status.character_number);
    }
}

bool WorldScene::processPendingScriptTravel() {
    if (!script_travel_pending_) {
        return false;
    }
    const ScenarioStart start = pending_script_travel_;
    pending_script_travel_ = {};
    script_travel_pending_ = false;
    std::string error;
    return transitionScenario(start, &error) !=
           ScenarioTravelResult::failed;
}


}  // namespace osf
