#include "quest_state.hpp"

namespace osf {

void QuestState::clear() {
    states_.clear();
    completion_latches_.clear();
    last_cue_ = QuestCue::none;
    notice_ = {};
}

bool QuestState::applyScriptUpdate(
    std::int32_t quest_id,
    std::int32_t state_value) {
    if (quest_id < 0) {
        return false;
    }

    if (state_value != 2) {
        states_.insert_or_assign(quest_id, state_value);
        last_cue_ = QuestCue::updated;
        return true;
    }

    if (completionLatched(quest_id)) {
        return false;
    }
    completion_latches_.insert_or_assign(quest_id, true);
    if (state(quest_id) != 1) {
        return false;
    }
    states_.insert_or_assign(quest_id, 2);
    last_cue_ = QuestCue::completed;
    return true;
}

void QuestState::selectNotice(std::int32_t quest_id) {
    notice_.quest_id = quest_id;
    notice_.counter = 600;
}

std::int32_t QuestState::state(std::int32_t quest_id) const {
    const auto found = states_.find(quest_id);
    return found == states_.end() ? 0 : found->second;
}

bool QuestState::completionLatched(
    std::int32_t quest_id) const {
    const auto found = completion_latches_.find(quest_id);
    return found != completion_latches_.end() && found->second;
}

QuestCue QuestState::lastCue() const {
    return last_cue_;
}

const QuestNotice& QuestState::notice() const {
    return notice_;
}

}  // namespace osf
