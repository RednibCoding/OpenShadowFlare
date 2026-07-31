#include "player_spell_action.hpp"

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

namespace osf {
namespace {

constexpr std::int16_t kEffectStatus = 0x40;
constexpr double kRetailCompletionNumerator = 7.0;

constexpr std::array<double, 10> kSpellSpeedFactors{{
    0.6,
    0.7,
    0.8,
    1.0,
    1.15,
    1.3,
    1.45,
    1.6,
    1.75,
    1.9,
}};

struct SpellCharts {
    std::int32_t first = -1;
    std::int32_t recovery = -1;
};

SpellCharts chartsForAction(PlayerSpellAction action) {
    switch (action) {
    case PlayerSpellAction::plasma:
    case PlayerSpellAction::ice_blast:
        return {11, 12};
    case PlayerSpellAction::fire_ball:
    case PlayerSpellAction::ice_bolt:
    case PlayerSpellAction::hell_fire:
        return {13, 14};
    }
    return {};
}

std::int32_t firstEffectFrame(
    const PlayerSpellAnimationTiming& timing) {
    for (std::size_t frame = 0;
         frame < timing.first_frame_statuses.size();
         ++frame) {
        if ((timing.first_frame_statuses[frame] &
             kEffectStatus) != 0) {
            return static_cast<std::int32_t>(frame);
        }
    }
    return -1;
}

}  // namespace

bool playerSpellActionForSpell(
    std::int32_t spell,
    PlayerSpellAction& action) {
    switch (spell) {
    case 1:
        action = PlayerSpellAction::fire_ball;
        return true;
    case 2:
        action = PlayerSpellAction::ice_bolt;
        return true;
    case 3:
        action = PlayerSpellAction::plasma;
        return true;
    case 4:
        action = PlayerSpellAction::hell_fire;
        return true;
    case 5:
        action = PlayerSpellAction::ice_blast;
        return true;
    default:
        return false;
    }
}

bool buildPlayerSpellAnimationTiming(
    const gapi::CafAnimation& animation,
    PlayerSpellAction action,
    std::int32_t direction,
    PlayerSpellAnimationTiming& timing) {
    timing = {};
    const SpellCharts charts = chartsForAction(action);
    if (charts.first < 0 ||
        charts.recovery < 0 ||
        direction < 0 ||
        direction >= 9 ||
        static_cast<std::size_t>(charts.first) >=
            animation.charts().size() ||
        static_cast<std::size_t>(charts.recovery) >=
            animation.charts().size()) {
        return false;
    }

    const gapi::CafDirection& first_direction =
        animation.charts()[
            static_cast<std::size_t>(charts.first)]
            .directions[static_cast<std::size_t>(direction)];
    const gapi::CafDirection& recovery_direction =
        animation.charts()[
            static_cast<std::size_t>(charts.recovery)]
            .directions[static_cast<std::size_t>(direction)];
    if (first_direction.frame_count <= 0 ||
        recovery_direction.frame_count <= 0) {
        return false;
    }

    timing.first_chart = charts.first;
    timing.recovery_chart = charts.recovery;
    timing.first_frame_count = first_direction.frame_count;
    if (!first_direction.parts.empty()) {
        const std::vector<gapi::CafCell>& cells =
            first_direction.parts.front();
        timing.first_frame_statuses.reserve(cells.size());
        for (const gapi::CafCell& cell : cells) {
            timing.first_frame_statuses.push_back(cell.status);
        }
    }
    return true;
}

double retailPlayerSpellAnimationSpeed(
    std::int32_t spell,
    std::int32_t speed_tier,
    const TableData* speed_table) {
    const std::int32_t base =
        speed_table &&
                speed_table->contains(spell, 0)
            ? speed_table->value(spell, 0)
            : 0;
    return static_cast<double>(base) *
           kSpellSpeedFactors[
               static_cast<std::size_t>(
                   std::clamp(speed_tier, 0, 9))] *
           0.001;
}

bool PlayerSpellActionController::start(
    PlayerSpellAction action,
    std::int32_t spell,
    std::int32_t target_character_number,
    std::int32_t aim_world_x,
    std::int32_t aim_world_y,
    std::int32_t speed_tier,
    const TableData* speed_table,
    PlayerSpellAnimationTiming timing,
    PlayerSpellActionEvent* event) {
    cancel();
    if (spell < 0 ||
        timing.first_frame_count <= 0 ||
        timing.first_chart < 0 ||
        timing.recovery_chart < 0) {
        return false;
    }

    action_ = action;
    spell_ = spell;
    target_character_number_ =
        target_character_number;
    aim_world_x_ = aim_world_x;
    aim_world_y_ = aim_world_y;
    timing_ = std::move(timing);
    refreshSpeed(speed_tier, speed_table);
    if (animation_speed_ <= 0.0) {
        cancel();
        return false;
    }

    const std::int32_t effect_frame =
        firstEffectFrame(timing_);
    effect_delay_ = static_cast<std::int32_t>(
        std::trunc(
            static_cast<double>(effect_frame) /
            animation_speed_));
    action_counter_ = 0;
    displayed_frame_ = 0;
    active_ = true;
    selectRenderedFrame();

    if (event) {
        *event = {
            action_,
            spell_,
            target_character_number_,
            aim_world_x_,
            aim_world_y_,
            effect_delay_,
            true,
            false,
        };
    }
    return true;
}

PlayerSpellActionEvent
PlayerSpellActionController::update(
    std::int32_t speed_tier,
    const TableData* speed_table) {
    if (!active_) {
        return {};
    }
    if (speed_tier >= 0) {
        refreshSpeed(speed_tier, speed_table);
    }

    displayed_frame_ = static_cast<std::int32_t>(
        std::trunc(
            static_cast<double>(action_counter_) *
            animation_speed_));
    ++action_counter_;
    selectRenderedFrame();

    PlayerSpellActionEvent event;
    event.action = action_;
    event.spell = spell_;
    event.target_character_number =
        target_character_number_;
    event.aim_world_x = aim_world_x_;
    event.aim_world_y = aim_world_y_;
    event.effect_delay = effect_delay_;
    if (displayed_frame_ >=
        timing_.first_frame_count - 1 +
            completion_increment_) {
        event.completed = true;
        active_ = false;
    }
    return event;
}

void PlayerSpellActionController::cancel() {
    timing_ = {};
    action_ = PlayerSpellAction::fire_ball;
    spell_ = -1;
    target_character_number_ = -1;
    aim_world_x_ = 0;
    aim_world_y_ = 0;
    speed_tier_ = 0;
    animation_speed_ = 1.0;
    completion_increment_ = 1;
    effect_delay_ = 0;
    action_counter_ = 0;
    displayed_frame_ = 0;
    animation_chart_ = 0;
    animation_frame_ = 0;
    active_ = false;
}

bool PlayerSpellActionController::active() const {
    return active_;
}

PlayerSpellAction PlayerSpellActionController::action() const {
    return action_;
}

std::int32_t PlayerSpellActionController::spell() const {
    return spell_;
}

std::int32_t
PlayerSpellActionController::targetCharacterNumber() const {
    return target_character_number_;
}

std::int32_t
PlayerSpellActionController::animationChart() const {
    return animation_chart_;
}

std::int32_t
PlayerSpellActionController::animationFrame() const {
    return animation_frame_;
}

std::int32_t
PlayerSpellActionController::actionCounter() const {
    return action_counter_;
}

std::int32_t
PlayerSpellActionController::displayedFrame() const {
    return displayed_frame_;
}

std::int32_t
PlayerSpellActionController::effectDelay() const {
    return effect_delay_;
}

void PlayerSpellActionController::refreshSpeed(
    std::int32_t speed_tier,
    const TableData* speed_table) {
    speed_tier_ = std::clamp(speed_tier, 0, 9);
    animation_speed_ =
        retailPlayerSpellAnimationSpeed(
            spell_, speed_tier_, speed_table);
    completion_increment_ =
        animation_speed_ > 0.0
            ? std::max<std::int32_t>(
                  static_cast<std::int32_t>(
                      std::trunc(
                          kRetailCompletionNumerator /
                          animation_speed_)),
                  1)
            : 1;
}

void PlayerSpellActionController::selectRenderedFrame() {
    if (displayed_frame_ < timing_.first_frame_count) {
        animation_chart_ = timing_.first_chart;
        animation_frame_ = std::max(
            displayed_frame_, 0);
        return;
    }
    // FUN_00439730 selects chart fourteen after the casting chart, but
    // submits frame zero until the action's completion threshold is met.
    animation_chart_ = timing_.recovery_chart;
    animation_frame_ = 0;
}

}  // namespace osf
