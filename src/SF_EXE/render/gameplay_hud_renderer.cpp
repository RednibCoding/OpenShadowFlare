#include "gameplay_hud_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "world/player_actor.hpp"
#include "world/player_data.hpp"

#include <algorithm>
#include <array>

namespace osf {
namespace {

constexpr std::int32_t kRetailBarWidth = 206;

void drawClippedBar(
    gapi::Backend& renderer,
    const gapi::NjpImage& patterns,
    std::size_t pattern,
    std::int32_t x,
    std::int32_t y,
    std::int32_t width,
    std::int32_t height = 12) {
    if (width <= 0) {
        return;
    }
    gapi::PatternDraw draw;
    draw.clip = {x, y, width, height};
    renderer.drawPattern(patterns, pattern, draw);
}

void drawLevel(
    gapi::Backend& renderer,
    const gapi::NjpImage& patterns,
    std::int32_t level) {
    level = std::clamp(level, 0, 999);
    const std::int32_t digits =
        level > 99 ? 3 : (level > 9 ? 2 : 1);
    constexpr std::array<std::array<std::int32_t, 3>, 3>
        positions{{
            {{60, 0, 0}},
            {{65, 56, 0}},
            {{69, 60, 51}},
        }};
    std::int32_t divisor = 1;
    for (std::int32_t index = 0;
         index < digits;
         ++index) {
        const std::int32_t digit = (level / divisor) % 10;
        renderer.drawPattern(
            patterns,
            static_cast<std::size_t>(19 + digit),
            {positions[
                 static_cast<std::size_t>(digits - 1)]
                 [static_cast<std::size_t>(index)],
             465});
        divisor *= 10;
    }
}

}  // namespace

GameplayHudValues gameplayHudValues(
    const PlayerData& player,
    MovementPace movement_pace,
    std::int32_t experience_threshold) {
    return {
        player.level(),
        player.currentLife(),
        player.baseMaximumLife(),
        player.currentMana(),
        player.baseMaximumMana(),
        player.experience(),
        experience_threshold,
        movement_pace == MovementPace::run,
    };
}

std::int32_t gameplayHudExperienceBarWidth(
    std::int32_t experience,
    std::int32_t threshold) {
    constexpr std::int32_t kRetailExperienceWidth = 109;
    if (experience <= 0 || threshold <= 0) {
        return 0;
    }
    if (experience >= threshold) {
        return kRetailExperienceWidth;
    }
    return std::max(
        static_cast<std::int32_t>(
            static_cast<std::int64_t>(experience) *
            kRetailExperienceWidth / threshold),
        1);
}

std::int32_t gameplayHudBarWidth(
    std::int32_t current,
    std::int32_t maximum) {
    if (current <= 0 || maximum <= 0) {
        return 0;
    }
    if (current >= maximum) {
        return kRetailBarWidth;
    }
    return std::max(
        static_cast<std::int32_t>(
            static_cast<std::int64_t>(current) *
            kRetailBarWidth / maximum),
        1);
}

void renderGameplayHud(
    gapi::Backend& renderer,
    const gapi::NjpImage& bar_patterns,
    const GameplayHudValues& values) {
    // The bottom 68 pixels are a reserved UI surface in retail. Bar.njp has
    // transparent openings for its gauges and quick slots, so scenery must
    // not remain underneath them.
    renderer.drawRectangle({
        0,
        412,
        640,
        68,
        {0, 0, 0, 255},
    });

    // FUN_004039f0 draws the two fixed pieces before the live values.
    renderer.drawPattern(bar_patterns, 7);
    renderer.drawPattern(bar_patterns, 8);

    // The persistent movement mode selects one of the two small indicators.
    renderer.drawPattern(
        bar_patterns,
        values.running ? 10 : 11);

    drawLevel(renderer, bar_patterns, values.level);
    drawClippedBar(
        renderer,
        bar_patterns,
        0,
        81,
        425,
        gameplayHudBarWidth(
            values.current_life,
            values.maximum_life));
    drawClippedBar(
        renderer,
        bar_patterns,
        3,
        106,
        452,
        gameplayHudBarWidth(
            values.current_mana,
            values.maximum_mana));

    renderer.drawPattern(bar_patterns, 15);
    drawClippedBar(
        renderer,
        bar_patterns,
        14,
        530,
        395,
        gameplayHudExperienceBarWidth(
            values.experience,
            values.experience_threshold),
        9);
}

}  // namespace osf
