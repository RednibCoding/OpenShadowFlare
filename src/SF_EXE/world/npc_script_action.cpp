#include "npc_script_action.hpp"

namespace osf {

bool NpcScriptActionController::start(
    std::int32_t action,
    std::int32_t repeat,
    std::int32_t restart_frame,
    std::int32_t end_frame) {
    if (action < 4 || action > 19) {
        return false;
    }
    action_ = action;
    frame_ = -1;
    restart_frame_ = restart_frame;
    end_frame_ = end_frame;
    repeat_ = repeat != -1;
    active_ = true;
    return true;
}

void NpcScriptActionController::cancel() {
    action_ = 1;
    frame_ = -1;
    restart_frame_ = -1;
    end_frame_ = -1;
    repeat_ = false;
    active_ = false;
}

NpcScriptActionUpdate NpcScriptActionController::update(
    std::int32_t frame_count) {
    if (!active_) {
        return {};
    }

    ++frame_;
    NpcScriptActionUpdate result{
        true,
        false,
        action_,
        frame_,
    };
    if (frame_count < 1) {
        result.frame = 0;
        result.completed = true;
        active_ = false;
        action_ = 1;
        return result;
    }

    const std::int32_t final_frame = frame_count - 1;
    if (!repeat_) {
        if (frame_ == final_frame) {
            result.completed = true;
            active_ = false;
            action_ = 1;
        }
        return result;
    }

    const std::int32_t loop_end =
        end_frame_ == -1 ? final_frame : end_frame_;
    if (frame_ == loop_end) {
        frame_ = restart_frame_ == -1
            ? -1
            : restart_frame_ - 1;
    }
    return result;
}

bool NpcScriptActionController::active() const {
    return active_;
}

std::int32_t NpcScriptActionController::action() const {
    return action_;
}

}  // namespace osf
