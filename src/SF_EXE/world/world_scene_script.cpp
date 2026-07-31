#include "world_scene.hpp"
#include "enemy_death_rewards.hpp"
#include "movement_controller.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
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

}  // namespace

bool WorldScene::readScriptWorldOperand(
    const script::Operand& operand,
    std::int32_t& value) const {
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
        return true;
    }

    if (opcode == 41) {
        if (arguments.empty() ||
            arguments[0] != 0 ||
            gameplay_service_request_.kind !=
                GameplayServiceKind::none) {
            return false;
        }
        gameplay_service_request_ = {
            GameplayServiceKind::toggle_special_items,
            0,
        };
        return true;
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

    if ((opcode != 18 && opcode != 19 && opcode != 21) ||
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
        if (const ScenarioObjectActor* object =
                findScriptObject(status.character_number)) {
            position = object->position();
            judgement = &object->judgement();
        } else if (const NpcActor* npc =
                       findScriptNpc(status.character_number)) {
            position = npc->position();
            judgement = &npc->judgement();
        } else if (const EnemyActor* enemy =
                       findScriptEnemy(status.character_number)) {
            position = enemy->position();
            judgement = &enemy->judgement();
        } else if (const GroundItem* item =
                       findScriptGroundItem(status.character_number)) {
            position = item->position;
            judgement = &item->judgement;
        }
        if (!judgement ||
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
