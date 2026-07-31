#include "quest_notice_layout.hpp"

#include "world/mission_catalog.hpp"
#include "world/quest_state.hpp"

namespace osf {
namespace {

constexpr std::int32_t kCellWidth = 6;
constexpr std::int32_t kTextRight = 612;
constexpr std::int32_t kTextY = 368;
constexpr std::int32_t kTextHeight = 12;

}  // namespace

bool buildQuestNoticeLayout(
    const QuestState& quests,
    const MissionCatalog& missions,
    QuestNoticeLayout& layout) {
    layout = {};
    const QuestNotice& notice = quests.notice();
    const MissionDefinition* mission =
        notice.counter > 0
            ? missions.find(notice.quest_id)
            : nullptr;
    if (!mission || mission->title.empty()) {
        return false;
    }

    // FUN_004050f0 wraps the Table 41 title in the two Shift-JIS corner
    // brackets at 0x47d530. Retail measures their encoded bytes as two
    // six-pixel cells apiece.
    layout.text.append("\x81\x75", 2);
    layout.text.append(mission->title);
    layout.text.append("\x81\x76", 2);
    layout.width =
        static_cast<std::int32_t>(layout.text.size()) *
        kCellWidth;
    layout.height = kTextHeight;
    layout.x = kTextRight - layout.width;
    layout.y = kTextY;
    return true;
}

bool questNoticeContains(
    const QuestNoticeLayout& layout,
    std::int32_t screen_x,
    std::int32_t screen_y) {
    return !layout.text.empty() &&
           screen_x >= layout.x &&
           screen_x < layout.x + layout.width &&
           screen_y >= layout.y &&
           screen_y < layout.y + layout.height;
}

}  // namespace osf
