#include "world_scene.hpp"
#include "movement_controller.hpp"

#include <algorithm>
#include <cctype>
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

bool isPersistentScriptOperand(
    const script::Operand& operand) {
    return operand.type >= 10 && operand.type <= 13;
}

std::uint64_t scriptOperandKey(
    const script::Operand& operand) {
    return
        (static_cast<std::uint64_t>(
             static_cast<std::uint32_t>(operand.type))
         << 32u) |
        static_cast<std::uint32_t>(operand.value);
}

}  // namespace

bool WorldScene::readScriptWorldOperand(
    const script::Operand& operand,
    std::int32_t& value) const {
    if (isPersistentScriptOperand(operand)) {
        const auto found =
            script_persistent_values_.find(
                scriptOperandKey(operand));
        value = found == script_persistent_values_.end()
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
            return false;
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
    return false;
}

bool WorldScene::writeScriptWorldOperand(
    const script::Operand& operand,
    std::int32_t value) {
    if (isPersistentScriptOperand(operand)) {
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
        return false;
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
    return false;
}

bool WorldScene::executeScriptNativeCommand(
    std::int32_t opcode,
    const std::vector<std::int32_t>& arguments) {
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
        return quests_.applyScriptUpdate(
            arguments[0], arguments[1]);
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
    }
    return false;
}


}  // namespace osf
