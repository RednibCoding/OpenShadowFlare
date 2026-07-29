#include "gameplay_mission_list_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "states/gameplay_mission_list.hpp"
#include "world/mission_catalog.hpp"
#include "world/quest_state.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace osf {
namespace {

constexpr std::int32_t kMissionsPerPage = 24;
constexpr std::int32_t kMissionsPerColumn = 12;

void drawText(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    std::string_view text,
    std::int32_t x,
    std::int32_t y,
    gapi::Color color) {
    renderer.drawText(
        font,
        text,
        {x + 1, y + 1, {0, 0, 0, 255}});
    renderer.drawText(font, text, {x, y, color});
}

void drawMissionEntries(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayMissionList& mission_list,
    const MissionCatalog& catalog,
    const QuestState& quests) {
    const std::int32_t first =
        mission_list.page() * kMissionsPerPage;
    for (std::int32_t offset = 0;
         offset < kMissionsPerPage;
         ++offset) {
        const std::int32_t mission_id = first + offset;
        const std::int32_t state = quests.state(mission_id);
        const MissionDefinition* mission =
            catalog.find(mission_id);
        if (!mission || state == 0) {
            continue;
        }

        const std::int32_t column =
            offset / kMissionsPerColumn;
        const std::int32_t row =
            offset % kMissionsPerColumn;
        const std::int32_t x = 52 + column * 294;
        const std::int32_t y = 48 + row * 27;
        renderer.drawPattern(
            status_patterns,
            state == 1 ? 25 : 26,
            {x, y});
        const std::uint8_t strength =
            state == 1 ? 224 : 128;
        drawText(
            renderer,
            font,
            mission->title,
            x + 27,
            y + 6,
            {strength, strength, strength, 255});
    }
}

void drawMissionDetails(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const MissionDefinition& mission) {
    renderer.drawPattern(
        status_patterns,
        59,
        {
            0,
            0,
            1000,
            1000,
            1000,
            1000,
            800,
            1000,
            1200,
        });
    renderer.drawPattern(status_patterns, 58);

    const std::string title =
        "[" + mission.title + "]";
    drawText(
        renderer,
        font,
        title,
        190,
        96,
        {224, 192, 0, 255});
    for (std::size_t index = 0;
         index < mission.description.size();
         ++index) {
        drawText(
            renderer,
            font,
            mission.description[index],
            190,
            120 + static_cast<std::int32_t>(index) * 16,
            {224, 224, 224, 255});
    }
}

}  // namespace

void renderGameplayMissionList(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayMissionList& mission_list,
    const MissionCatalog& catalog,
    const QuestState& quests) {
    if (!mission_list.active()) {
        return;
    }

    // FUN_0040cea0 draws the authored mission frame, two 24-entry
    // page tabs, and script-owned mission states from Table.Tbd.
    renderer.drawPattern(status_patterns, 10);
    renderer.drawPattern(
        status_patterns,
        mission_list.page() == 0 ? 112 : 110);
    renderer.drawPattern(
        status_patterns,
        mission_list.page() == 1 ? 113 : 111);
    drawMissionEntries(
        renderer,
        status_patterns,
        font,
        mission_list,
        catalog,
        quests);

    const MissionDefinition* selected =
        catalog.find(mission_list.selectedMission());
    if (selected) {
        drawMissionDetails(
            renderer,
            status_patterns,
            font,
            *selected);
    }
}

}  // namespace osf
