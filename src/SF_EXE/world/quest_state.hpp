#ifndef OPENSHADOWFLARE_QUEST_STATE_HPP
#define OPENSHADOWFLARE_QUEST_STATE_HPP

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

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
    bool setScriptState(
        std::int32_t quest_id,
        std::int32_t state);
    void initialize(std::size_t count);
    void restore(const std::vector<std::int32_t>& states);
    void selectNotice(std::int32_t quest_id);

    std::int32_t state(std::int32_t quest_id) const;
    bool completionLatched(std::int32_t quest_id) const;
    QuestCue lastCue() const;
    const QuestNotice& notice() const;
    const std::vector<std::int32_t>& states() const;

private:
    std::vector<std::int32_t> states_;
    std::unordered_map<std::int32_t, bool> completion_latches_;
    QuestCue last_cue_ = QuestCue::none;
    QuestNotice notice_;
};

}  // namespace osf

#endif
