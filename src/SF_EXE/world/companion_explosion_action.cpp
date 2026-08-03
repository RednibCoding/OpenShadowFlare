#include "companion_explosion_action.hpp"

#include "core/retail_random.hpp"
#include "enemy_effect_impact.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kDepartureChart = 6;
constexpr std::int32_t kArrivalChart = 7;
constexpr std::int32_t kSpecialDirection = 8;
constexpr std::int16_t kImpactStatus = 0x40;
constexpr std::int32_t kExplosionSpell = 20;
constexpr std::int32_t kHitEffectBase = 21000;

const gapi::CafDirection* directionFor(
    const gapi::CafAnimation& animation,
    std::int32_t chart) {
    if (chart < 0 ||
        static_cast<std::size_t>(chart) >=
            animation.charts().size()) {
        return nullptr;
    }
    return &animation.charts()[
                static_cast<std::size_t>(chart)]
                .directions[
                    static_cast<std::size_t>(
                        kSpecialDirection)];
}

std::int32_t tableValue(
    const TableDatabase& tables,
    std::int32_t table_number,
    std::int32_t row,
    std::int32_t column) {
    const TableData* table = tables.find(table_number);
    return table && table->contains(row, column)
        ? table->value(row, column)
        : -1;
}

void initializePacketDefaults(CombatPacket& packet) {
    packet.write(0, 0);
    packet.write(1, 3);
    packet.write(3, 1);
    packet.write(5, 0);
    packet.write(33, 0);
    packet.write(35, 8);
    packet.write(37, 0);
    packet.write(38, 0);
    packet.write(39, 0);
    packet.write(42, 0);
    packet.write(44, 0);
    packet.write(72, 0);
    packet.write(73, kExplosionSpell);
    packet.write(74, -1);
    packet.write(75, 8);
    packet.write(76, 0);
}

}  // namespace

bool buildCompanionExplosionAnimationTiming(
    const gapi::CafAnimation& animation,
    CompanionExplosionAnimationTiming& timing) {
    timing = {};
    const gapi::CafDirection* departure =
        directionFor(animation, kDepartureChart);
    const gapi::CafDirection* arrival =
        directionFor(animation, kArrivalChart);
    if (!departure || !arrival ||
        departure->frame_count < 1 ||
        arrival->frame_count < 1) {
        return false;
    }

    timing.departure_frame_count =
        departure->frame_count;
    timing.arrival_frame_count = arrival->frame_count;
    if (!arrival->parts.empty()) {
        const std::vector<gapi::CafCell>& cells =
            arrival->parts.front();
        timing.arrival_frame_statuses.reserve(cells.size());
        for (const gapi::CafCell& cell : cells) {
            timing.arrival_frame_statuses.push_back(
                cell.status);
        }
    }
    return true;
}

bool CompanionExplosionActionController::start(
    WorldPosition destination,
    CompanionExplosionAnimationTiming timing) {
    cancel();
    if (timing.departure_frame_count < 1 ||
        timing.arrival_frame_count < 1) {
        return false;
    }
    destination_ = destination;
    timing_ = std::move(timing);
    active_ = true;
    return true;
}

CompanionExplosionActionEvent
CompanionExplosionActionController::update() {
    CompanionExplosionActionEvent event;
    if (!active_) {
        return event;
    }

    if (counter_ < timing_.departure_frame_count) {
        animation_chart_ = kDepartureChart;
        animation_frame_ = counter_;
    } else {
        animation_chart_ = kArrivalChart;
        animation_frame_ =
            counter_ - timing_.departure_frame_count;
        event.relocate_due =
            animation_frame_ == 0;
        if (animation_frame_ >= 0 &&
            static_cast<std::size_t>(animation_frame_) <
                timing_.arrival_frame_statuses.size()) {
            event.impact_due =
                (timing_.arrival_frame_statuses[
                     static_cast<std::size_t>(
                         animation_frame_)] &
                 kImpactStatus) != 0;
        }
    }

    if (counter_ >=
        timing_.departure_frame_count +
            timing_.arrival_frame_count - 1) {
        event.completed = true;
        active_ = false;
    }
    ++counter_;
    return event;
}

void CompanionExplosionActionController::cancel() {
    timing_ = {};
    destination_ = {};
    counter_ = 0;
    animation_chart_ = kDepartureChart;
    animation_frame_ = 0;
    active_ = false;
}

bool CompanionExplosionActionController::active() const {
    return active_;
}

WorldPosition
CompanionExplosionActionController::destination() const {
    return destination_;
}

std::int32_t
CompanionExplosionActionController::animationChart() const {
    return animation_chart_;
}

std::int32_t
CompanionExplosionActionController::animationFrame() const {
    return animation_frame_;
}

CombatPacket buildCompanionExplosionPacket(
    const CompanionExplosionPacketInput& input,
    const TableDatabase& tables,
    RetailRandom& random) {
    CombatPacket packet;
    if (input.source_character_number < 0 ||
        input.scaling_level < 1) {
        return packet;
    }

    initializePacketDefaults(packet);
    packet.write(2, input.source_character_number);
    // FUN_00461c40 deliberately uses the owner's magical-defense field as
    // Explosion's base damage and leaves packet defense at zero.
    packet.write(4, std::max<std::int32_t>(input.damage_value, 1));
    for (std::size_t index = 0;
         index < input.element_affinities.size();
         ++index) {
        packet.write(6 + index, input.element_affinities[index]);
    }
    for (std::size_t index = 0;
         index < input.state_words.size();
         ++index) {
        packet.write(14 + index, input.state_words[index]);
    }
    packet.write(31, input.source_level);
    const std::int32_t type_value =
        retailEffectParameter(
            tables,
            kExplosionSpell,
            input.scaling_level,
            5);
    packet.write(32, type_value);
    packet.write(34, random.next() % 4 + kHitEffectBase);
    packet.write(
        36,
        retailEffectParameter(
            tables,
            kExplosionSpell,
            input.scaling_level,
            1) +
            input.magical_hit_rate);
    packet.write(40, type_value);
    packet.write(41, type_value);
    packet.write(43, type_value);

    const std::int32_t first_column =
        input.scaling_level * 3 - 3;
    for (std::size_t table_index = 0;
         table_index < 9;
         ++table_index) {
        const std::int32_t table_number =
            70 + static_cast<std::int32_t>(table_index);
        packet.write(
            54 + table_index,
            tableValue(
                tables,
                table_number,
                kExplosionSpell,
                first_column));
        packet.write(
            63 + table_index,
            tableValue(
                tables,
                table_number,
                kExplosionSpell,
                first_column + 1));
        packet.write(
            45 + table_index,
            tableValue(
                tables,
                table_number,
                kExplosionSpell,
                first_column + 2));
    }
    return packet;
}

}  // namespace osf
