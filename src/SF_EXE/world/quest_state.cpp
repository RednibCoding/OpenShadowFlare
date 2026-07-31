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
        if (!setScriptState(quest_id, state_value)) {
            return false;
        }
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
    setScriptState(quest_id, 2);
    last_cue_ = QuestCue::completed;
    return true;
}

bool QuestState::setScriptState(
    std::int32_t quest_id,
    std::int32_t state_value) {
    if (quest_id < 0) {
        return false;
    }
    const std::size_t index =
        static_cast<std::size_t>(quest_id);
    if (index >= states_.size()) {
        states_.resize(index + 1u, 0);
    }
    states_[index] = state_value;
    return true;
}

void QuestState::initialize(std::size_t count) {
    states_.assign(count, 0);
    completion_latches_.clear();
    last_cue_ = QuestCue::none;
    notice_ = {};
}

void QuestState::restore(
    const std::vector<std::int32_t>& states) {
    states_ = states;
    completion_latches_.clear();
    for (std::size_t index = 0; index < states_.size(); ++index) {
        if (states_[index] == 2) {
            completion_latches_.insert_or_assign(
                static_cast<std::int32_t>(index), true);
        }
    }
    last_cue_ = QuestCue::none;
    notice_ = {};
}

void QuestState::selectNotice(std::int32_t quest_id) {
    notice_.quest_id = quest_id;
    notice_.counter = 600;
}

std::int32_t QuestState::state(std::int32_t quest_id) const {
    return quest_id >= 0 &&
                   static_cast<std::size_t>(quest_id) <
                       states_.size()
               ? states_[static_cast<std::size_t>(quest_id)]
               : 0;
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

const std::vector<std::int32_t>& QuestState::states() const {
    return states_;
}

}  // namespace osf
