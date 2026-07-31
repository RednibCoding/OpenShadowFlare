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

bool buildQuestNoticeLayout(
    const QuestState& quests,
    const MissionCatalog& missions,
    QuestNoticeLayout& layout);

bool questNoticeContains(
    const QuestNoticeLayout& layout,
    std::int32_t screen_x,
    std::int32_t screen_y);

}  // namespace osf

#endif
