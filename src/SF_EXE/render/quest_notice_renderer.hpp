#ifndef OPENSHADOWFLARE_QUEST_NOTICE_RENDERER_HPP
#define OPENSHADOWFLARE_QUEST_NOTICE_RENDERER_HPP

namespace osf {

class MissionCatalog;
class QuestState;

namespace gapi {
class Backend;
class NjpImage;
}

void renderQuestNotice(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const gapi::NjpImage* status_icons,
    const QuestState& quests,
    const MissionCatalog& missions);

}  // namespace osf

#endif
