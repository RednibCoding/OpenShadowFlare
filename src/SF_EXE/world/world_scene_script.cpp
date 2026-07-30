#include "world_scene.hpp"
#include "movement_controller.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <utility>

namespace osf {

bool WorldScene::readScriptWorldOperand(
    const script::Operand& operand,
    std::int32_t& value) const {
    if (operand.type != 6 && operand.type != 7) {
        return false;
    }
    const NpcActor* npc = findScriptNpc(operand.value);
    value = !npc
                ? 0
                : (operand.type == 6
                       ? npc->position().x
                       : npc->position().y);
    return true;
}

bool WorldScene::executeScriptNativeCommand(
    std::int32_t opcode,
    const std::vector<std::int32_t>& arguments) {
    if (opcode == 10) {
        if (arguments.size() < 6) {
            return false;
        }
        const std::size_t first_item = ground_items_.size();
        if (!createGroundItems(
                ground_items_,
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
    }
    return false;
}


}  // namespace osf
