#ifndef OPENSHADOWFLARE_QUEST_STATE_HPP
#define OPENSHADOWFLARE_QUEST_STATE_HPP

#include <cstdint>
#include <unordered_map>

namespace osf {

enum class QuestCue {
    none,
    updated,
    completed,
};

struct QuestNotice {
    std::int32_t quest_id = -1;
    std::int32_t counter = 0;
};

class QuestState {
public:
    void clear();

    bool applyScriptUpdate(
        std::int32_t quest_id,
        std::int32_t state);
    void selectNotice(std::int32_t quest_id);

    std::int32_t state(std::int32_t quest_id) const;
    bool completionLatched(std::int32_t quest_id) const;
    QuestCue lastCue() const;
    const QuestNotice& notice() const;

private:
    std::unordered_map<std::int32_t, std::int32_t> states_;
    std::unordered_map<std::int32_t, bool> completion_latches_;
    QuestCue last_cue_ = QuestCue::none;
    QuestNotice notice_;
};

}  // namespace osf

#endif
