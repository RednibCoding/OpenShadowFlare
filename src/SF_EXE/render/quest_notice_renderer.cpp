#include "quest_notice_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "ui/quest_notice_layout.hpp"

namespace osf {

void renderQuestNotice(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const gapi::NjpImage* status_icons,
    const QuestState& quests,
    const MissionCatalog& missions) {
    if (status_icons && activeQuestShortcutVisible(quests)) {
        const ActiveQuestShortcutLayout shortcut;
        renderer.drawPattern(
            *status_icons,
            0,
            {shortcut.pattern_x, shortcut.pattern_y});
    }

    QuestNoticeLayout layout;
    if (!buildQuestNoticeLayout(
            quests, missions, layout)) {
        return;
    }

    renderer.drawText(
        font,
        layout.text,
        {
            layout.x + 1,
            layout.y + 1,
            {0, 0, 0, 255},
        });
    renderer.drawText(
        font,
        layout.text,
        {
            layout.x,
            layout.y,
            {224, 224, 224, 255},
        });
}

}  // namespace osf
