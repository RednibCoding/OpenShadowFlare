#include "gameplay_mission_list.hpp"

#include "world/quest_state.hpp"

namespace osf {
namespace {

constexpr std::int32_t kMissionsPerPage = 24;
constexpr std::int32_t kMissionsPerColumn = 12;

bool inside(
    std::int32_t x,
    std::int32_t y,
    std::int32_t left,
    std::int32_t top,
    std::int32_t right,
    std::int32_t bottom) {
    return x >= left && x < right &&
           y >= top && y < bottom;
}

}  // namespace

void GameplayMissionList::open() {
    active_ = true;
    selected_mission_ = -1;
}

void GameplayMissionList::close() {
    active_ = false;
    selected_mission_ = -1;
}

GameplayMissionListResult GameplayMissionList::update(
    const GameplayMissionListInput& input,
    const QuestState& quests) {
    if (input.toggle_pressed) {
        if (active_) {
            close();
        } else {
            open();
        }
        return {};
    }
    if (!active_) {
        return {};
    }
    if (input.close_pressed) {
        close();
        return {};
    }
    if (!input.pointer_primary_pressed) {
        return {};
    }

    GameplayMissionListResult result;
    if (selected_mission_ >= 0) {
        if (input.pointer_y >= 0 &&
            input.pointer_y < 413) {
            selected_mission_ = -1;
        }
        return result;
    }

    if (inside(
            input.pointer_x,
            input.pointer_y,
            276,
            10,
            296,
            33)) {
        page_ = 0;
        result.play_move_sound = true;
        return result;
    }
    if (inside(
            input.pointer_x,
            input.pointer_y,
            344,
            10,
            364,
            33)) {
        page_ = 1;
        result.play_move_sound = true;
        return result;
    }

    const std::int32_t mission_id =
        missionAt(
            input.pointer_x,
            input.pointer_y,
            quests);
    if (mission_id >= 0) {
        selected_mission_ = mission_id;
        return result;
    }

    if (input.pointer_y < 412) {
        close();
    }
    return result;
}

bool GameplayMissionList::active() const {
    return active_;
}

std::int32_t GameplayMissionList::page() const {
    return page_;
}

std::int32_t GameplayMissionList::selectedMission() const {
    return selected_mission_;
}

std::int32_t GameplayMissionList::missionAt(
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    const QuestState& quests) const {
    if (pointer_x < 52 || pointer_x >= 640 ||
        pointer_y < 48 || pointer_y >= 372) {
        return -1;
    }

    const std::int32_t column =
        pointer_x < 346 ? 0 : 1;
    const std::int32_t row = (pointer_y - 48) / 27;
    if (row < 0 || row >= kMissionsPerColumn) {
        return -1;
    }
    const std::int32_t mission_id =
        page_ * kMissionsPerPage +
        column * kMissionsPerColumn +
        row;
    return quests.state(mission_id) != 0
        ? mission_id
        : -1;
}

}  // namespace osf
