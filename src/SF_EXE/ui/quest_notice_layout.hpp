#ifndef OPENSHADOWFLARE_QUEST_NOTICE_LAYOUT_HPP
#define OPENSHADOWFLARE_QUEST_NOTICE_LAYOUT_HPP

#include <cstdint>
#include <string>

namespace osf {

class MissionCatalog;
class QuestState;

struct QuestNoticeLayout {
    std::string text;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
};

struct ActiveQuestShortcutLayout {
    std::int32_t pattern_x = 616;
    std::int32_t pattern_y = 360;
    std::int32_t hit_x = 616;
    std::int32_t hit_y = 368;
    std::int32_t hit_width = 24;
    std::int32_t hit_height = 16;
};

bool buildQuestNoticeLayout(
    const QuestState& quests,
    const MissionCatalog& missions,
    QuestNoticeLayout& layout);

bool questNoticeContains(
    const QuestNoticeLayout& layout,
    std::int32_t screen_x,
    std::int32_t screen_y);

bool activeQuestShortcutVisible(const QuestState& quests);
bool activeQuestShortcutContains(
    const ActiveQuestShortcutLayout& layout,
    std::int32_t screen_x,
    std::int32_t screen_y);

}  // namespace osf

#endif
