#include "player_attack_target.hpp"

#include "movement_controller.hpp"

namespace osf {

PlayerAttackTargetDisposition classifyPlayerAttackTarget(
    WorldPosition player_position,
    const ObjectBounds& player_judgement,
    const PlayerAttackTargetSnapshot& target,
    std::int32_t attack_range) {
    if (target.id < 0 ||
        target.life <= 0 ||
        !target.visible ||
        !target.pointer_enabled) {
        return PlayerAttackTargetDisposition::rejected;
    }
    return distanceBetweenBounds(
               player_position,
               player_judgement,
               target.position,
               target.judgement) <= attack_range
        ? PlayerAttackTargetDisposition::ready
        : PlayerAttackTargetDisposition::approach;
}

PlayerAttackTargetDisposition PlayerAttackTargetController::command(
    WorldPosition player_position,
    const ObjectBounds& player_judgement,
    const PlayerAttackTargetSnapshot& target) {
    cancel();
    const PlayerAttackTargetDisposition disposition =
        classifyPlayerAttackTarget(
            player_position,
            player_judgement,
            target);
    if (disposition ==
        PlayerAttackTargetDisposition::approach) {
        approach_target_id_ = target.id;
    } else if (
        disposition ==
        PlayerAttackTargetDisposition::ready) {
        ready_target_id_ = target.id;
    }
    return disposition;
}

PlayerAttackTargetDisposition PlayerAttackTargetController::refresh(
    WorldPosition player_position,
    const ObjectBounds& player_judgement,
    const PlayerAttackTargetSnapshot* target) {
    if (approach_target_id_ < 0 ||
        !target ||
        target->id != approach_target_id_) {
        cancel();
        return PlayerAttackTargetDisposition::rejected;
    }
    const PlayerAttackTargetDisposition disposition =
        classifyPlayerAttackTarget(
            player_position,
            player_judgement,
            *target);
    if (disposition ==
        PlayerAttackTargetDisposition::ready) {
        ready_target_id_ = approach_target_id_;
        approach_target_id_ = -1;
    } else if (
        disposition ==
        PlayerAttackTargetDisposition::rejected) {
        cancel();
    }
    return disposition;
}

bool PlayerAttackTargetController::validateReady(
    const PlayerAttackTargetSnapshot* target) {
    if (ready_target_id_ < 0 ||
        !target ||
        target->id != ready_target_id_ ||
        target->life <= 0 ||
        !target->visible ||
        !target->pointer_enabled) {
        cancel();
        return false;
    }
    return true;
}

void PlayerAttackTargetController::cancel() {
    approach_target_id_ = -1;
    ready_target_id_ = -1;
}

std::int32_t
PlayerAttackTargetController::approachTargetId() const {
    return approach_target_id_;
}

std::int32_t PlayerAttackTargetController::readyTargetId() const {
    return ready_target_id_;
}

}  // namespace osf
