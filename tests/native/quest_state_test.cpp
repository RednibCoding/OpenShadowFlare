#include "world/quest_state.hpp"

#include <iostream>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool testQuestUpdates() {
    osf::QuestState quests;
    if (!check(
            quests.state(1) == 0 &&
                !quests.completionLatched(1) &&
                quests.lastCue() == osf::QuestCue::none,
            "A new quest state did not start empty.")) {
        return false;
    }
    if (!check(
            quests.applyScriptUpdate(1, 1) &&
                quests.state(1) == 1 &&
                quests.lastCue() == osf::QuestCue::updated,
            "A retail quest-start update was not stored.")) {
        return false;
    }

    quests.selectNotice(1);
    if (!check(
            quests.notice().quest_id == 1 &&
                quests.notice().counter == 600,
            "The retail quest notice values were not stored.")) {
        return false;
    }
    if (!check(
            quests.applyScriptUpdate(1, 2) &&
                quests.state(1) == 2 &&
                quests.completionLatched(1) &&
                quests.lastCue() == osf::QuestCue::completed,
            "A valid retail quest-completion update failed.")) {
        return false;
    }
    if (!check(
            !quests.applyScriptUpdate(1, 2) &&
                quests.state(1) == 2,
            "A repeated retail quest completion was accepted.")) {
        return false;
    }

    quests.clear();
    return check(
        quests.state(1) == 0 &&
            !quests.completionLatched(1) &&
            quests.lastCue() == osf::QuestCue::none &&
            quests.notice().quest_id == -1 &&
            quests.notice().counter == 0,
        "Clearing quest state left scenario-owned values behind.");
}

}  // namespace

int main() {
    return testQuestUpdates() ? 0 : 1;
}
