#ifndef OPENSHADOWFLARE_GAMEPLAY_MISSION_LIST_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_MISSION_LIST_RENDERER_HPP

namespace osf {

class GameplayMissionList;
class MissionCatalog;
class QuestState;

namespace gapi {
class Backend;
class NjpImage;
}  // namespace gapi

void renderGameplayMissionList(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayMissionList& mission_list,
    const MissionCatalog& catalog,
    const QuestState& quests);

}  // namespace osf

#endif
