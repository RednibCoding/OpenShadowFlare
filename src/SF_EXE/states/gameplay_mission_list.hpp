#ifndef OPENSHADOWFLARE_GAMEPLAY_MISSION_LIST_HPP
#define OPENSHADOWFLARE_GAMEPLAY_MISSION_LIST_HPP

#include <cstdint>

namespace osf {

class QuestState;

struct GameplayMissionListInput {
    bool toggle_pressed = false;
    bool close_pressed = false;
    bool pointer_primary_pressed = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

struct GameplayMissionListResult {
    bool play_move_sound = false;
};

class GameplayMissionList {
public:
    void open();
    void close();
    GameplayMissionListResult update(
        const GameplayMissionListInput& input,
        const QuestState& quests);

    bool active() const;
    std::int32_t page() const;
    std::int32_t selectedMission() const;

private:
    std::int32_t missionAt(
        std::int32_t pointer_x,
        std::int32_t pointer_y,
        const QuestState& quests) const;

    bool active_ = false;
    std::int32_t page_ = 0;
    std::int32_t selected_mission_ = -1;
};

}  // namespace osf

#endif
