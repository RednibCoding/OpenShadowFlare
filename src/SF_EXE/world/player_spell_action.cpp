#include "player_spell_action.hpp"

#include "player_attack_action.hpp"

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

SpellCharts chartsForAction(
    PlayerSpellAction action,
    PlayerSpellAnimationVariant variant) {
    if (action == PlayerSpellAction::sonic_blade) {
        switch (variant) {
        case PlayerSpellAnimationVariant::sonic_blade_subtype_0:
            return {5, 6};
        case PlayerSpellAnimationVariant::sonic_blade_subtype_3:
            return {15, 16};
        case PlayerSpellAnimationVariant::sonic_blade_subtype_1:
            return {19, 20};
        case PlayerSpellAnimationVariant::standard:
            return {};
        }
    }
    switch (action) {
    case PlayerSpellAction::transport:
    case PlayerSpellAction::plasma:
    case PlayerSpellAction::ice_blast:
    case PlayerSpellAction::heal:
    case PlayerSpellAction::moon:
    case PlayerSpellAction::berserker:
    case PlayerSpellAction::energy_shield:
    case PlayerSpellAction::earth_spear:
    case PlayerSpellAction::lightning_storm:
    case PlayerSpellAction::identify:
    case PlayerSpellAction::magic_shield:
    case PlayerSpellAction::counter_burst:
    case PlayerSpellAction::explosion:
        return {11, 12};
    case PlayerSpellAction::fire_ball:
    case PlayerSpellAction::ice_bolt:
    case PlayerSpellAction::hell_fire:
    case PlayerSpellAction::flame_strike:
    case PlayerSpellAction::dread_deathscythe:
    case PlayerSpellAction::medusa:
    case PlayerSpellAction::mud_javelin:
    case PlayerSpellAction::elemental_strike:
        return {13, 14};
    case PlayerSpellAction::sonic_blade:
        return {};
    }
    return {};
}

bool dispatchesAtEffectMarker(
    PlayerSpellAction action) {
    return action == PlayerSpellAction::heal ||
           action == PlayerSpellAction::moon ||
           action == PlayerSpellAction::berserker ||
           action == PlayerSpellAction::energy_shield ||
           action == PlayerSpellAction::identify ||
           action == PlayerSpellAction::magic_shield ||
           action == PlayerSpellAction::counter_burst ||
           action == PlayerSpellAction::explosion ||
           action == PlayerSpellAction::sonic_blade;
}

bool allowsRepeatedEffectMarkers(
    PlayerSpellAction action) {
    return action == PlayerSpellAction::sonic_blade;
}

bool frameHasEffectStatus(
    const PlayerSpellAnimationTiming& timing,
    std::int32_t frame) {
    return frame >= 0 &&
           static_cast<std::size_t>(frame) <
               timing.first_frame_statuses.size() &&
           (timing.first_frame_statuses[
                static_cast<std::size_t>(frame)] &
            kEffectStatus) != 0;
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
    case 0:
        action = PlayerSpellAction::transport;
        return true;
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
    case 6:
        action = PlayerSpellAction::heal;
        return true;
    case 7:
        action = PlayerSpellAction::moon;
        return true;
    case 8:
        action = PlayerSpellAction::berserker;
        return true;
    case 9:
        action = PlayerSpellAction::energy_shield;
        return true;
    case 10:
        action = PlayerSpellAction::earth_spear;
        return true;
    case 11:
        action = PlayerSpellAction::flame_strike;
        return true;
    case 12:
        action = PlayerSpellAction::dread_deathscythe;
        return true;
    case 13:
        action = PlayerSpellAction::lightning_storm;
        return true;
    case 14:
        action = PlayerSpellAction::medusa;
        return true;
    case 15:
        action = PlayerSpellAction::sonic_blade;
        return true;
    case 16:
        action = PlayerSpellAction::mud_javelin;
        return true;
    case 17:
        action = PlayerSpellAction::identify;
        return true;
    case 18:
        action = PlayerSpellAction::magic_shield;
        return true;
    case 19:
        action = PlayerSpellAction::counter_burst;
        return true;
    case 20:
        action = PlayerSpellAction::explosion;
        return true;
    case 21:
        action = PlayerSpellAction::elemental_strike;
        return true;
    default:
        return false;
    }
}

bool playerSonicBladeAnimationVariant(
    std::int32_t weapon_subtype,
    PlayerSpellAnimationVariant& variant) {
    switch (weapon_subtype) {
    case 0:
        variant = PlayerSpellAnimationVariant::sonic_blade_subtype_0;
        return true;
    case 3:
        variant = PlayerSpellAnimationVariant::sonic_blade_subtype_3;
        return true;
    case 1:
        variant = PlayerSpellAnimationVariant::sonic_blade_subtype_1;
        return true;
    default:
        variant = PlayerSpellAnimationVariant::standard;
        return false;
    }
}

bool buildPlayerSpellAnimationTiming(
    const gapi::CafAnimation& animation,
    PlayerSpellAction action,
    std::int32_t direction,
    PlayerSpellAnimationTiming& timing) {
    return buildPlayerSpellAnimationTiming(
        animation,
        action,
        PlayerSpellAnimationVariant::standard,
        direction,
        timing);
}

bool buildPlayerSpellAnimationTiming(
    const gapi::CafAnimation& animation,
    PlayerSpellAction action,
    PlayerSpellAnimationVariant variant,
    std::int32_t direction,
    PlayerSpellAnimationTiming& timing) {
    timing = {};
    const SpellCharts charts = chartsForAction(action, variant);
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
    timing.recovery_frame_count =
        recovery_direction.frame_count;
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
                   std::clamp<std::int32_t>(speed_tier, 0, 9))] *
           0.001;
}

double retailPlayerSonicBladeAnimationSpeed(
    std::int32_t speed_tier) {
    return retailPlayerMeleeAttackAnimationSpeed(
        speed_tier);
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
        timing.recovery_chart < 0 ||
        (action == PlayerSpellAction::sonic_blade &&
         timing.recovery_frame_count <= 0)) {
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
    if (action_ == PlayerSpellAction::sonic_blade) {
        // FUN_0043e5e0 constructs effect 10015 directly from the CAF
        // marker and passes one as the effect delay.
        effect_delay_ = 1;
    }
    action_counter_ = 0;
    displayed_frame_ = 0;
    last_effect_scan_frame_ = 0;
    cast_dispatched_ =
        !dispatchesAtEffectMarker(action_);
    const bool initial_marker =
        !cast_dispatched_ &&
        frameHasEffectStatus(timing_, 0);
    cast_dispatched_ =
        cast_dispatched_ || initial_marker;
    active_ = true;
    selectRenderedFrame();

    if (event) {
        *event = {};
        event->action = action_;
        event->spell = spell_;
        event->target_character_number =
            target_character_number_;
        event->aim_world_x = aim_world_x_;
        event->aim_world_y = aim_world_y_;
        event->effect_delay = effect_delay_;
        event->entry_visual_effect_number =
            action_ == PlayerSpellAction::sonic_blade
                ? 21025
                : action_ == PlayerSpellAction::identify
                      ? 21028
                      : -1;
        event->cast_due = initial_marker ||
            (!dispatchesAtEffectMarker(action_));
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

    const std::int32_t previous_displayed_frame =
        displayed_frame_;
    if (action_ == PlayerSpellAction::sonic_blade) {
        ++action_counter_;
        displayed_frame_ = static_cast<std::int32_t>(
            std::trunc(
                static_cast<double>(action_counter_) *
                animation_speed_));
    } else {
        displayed_frame_ = static_cast<std::int32_t>(
            std::trunc(
                static_cast<double>(action_counter_) *
                animation_speed_));
        ++action_counter_;
    }
    selectRenderedFrame();

    PlayerSpellActionEvent event;
    event.action = action_;
    event.spell = spell_;
    event.target_character_number =
        target_character_number_;
    event.aim_world_x = aim_world_x_;
    event.aim_world_y = aim_world_y_;
    event.effect_delay = effect_delay_;
    if ((!cast_dispatched_ ||
         allowsRepeatedEffectMarkers(action_)) &&
        dispatchesAtEffectMarker(action_) &&
        displayed_frame_ < timing_.first_frame_count) {
        const std::int32_t first_frame =
            std::max<std::int32_t>(
                last_effect_scan_frame_ + 1,
                previous_displayed_frame + 1);
        for (std::int32_t frame = first_frame;
             frame <= displayed_frame_;
             ++frame) {
            if (frameHasEffectStatus(timing_, frame)) {
                event.cast_due = true;
                cast_dispatched_ = true;
                break;
            }
        }
        last_effect_scan_frame_ =
            std::max<std::int32_t>(
                last_effect_scan_frame_,
                displayed_frame_);
    }
    event.swing_sound_due =
        action_ == PlayerSpellAction::sonic_blade &&
        action_counter_ == 6;
    const bool sonic_complete =
        action_ == PlayerSpellAction::sonic_blade &&
        animation_chart_ == timing_.recovery_chart &&
        animation_frame_ ==
            timing_.recovery_frame_count - 1;
    if (sonic_complete ||
        (action_ != PlayerSpellAction::sonic_blade &&
         displayed_frame_ >=
             timing_.first_frame_count - 1 +
                 completion_increment_)) {
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
    last_effect_scan_frame_ = -1;
    cast_dispatched_ = false;
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
    speed_tier_ = std::clamp<std::int32_t>(speed_tier, 0, 9);
    animation_speed_ =
        action_ == PlayerSpellAction::sonic_blade
            ? retailPlayerSonicBladeAnimationSpeed(
                  speed_tier_)
            : retailPlayerSpellAnimationSpeed(
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
        animation_frame_ = std::max<std::int32_t>(
            displayed_frame_, 0);
        return;
    }
    if (action_ == PlayerSpellAction::sonic_blade) {
        animation_chart_ = timing_.recovery_chart;
        animation_frame_ = std::min(
            displayed_frame_ - timing_.first_frame_count,
            timing_.recovery_frame_count - 1);
        return;
    }
    // FUN_00439730 selects chart fourteen after the casting chart, but
    // submits frame zero until the action's completion threshold is met.
    animation_chart_ = timing_.recovery_chart;
    animation_frame_ = 0;
}

}  // namespace osf
