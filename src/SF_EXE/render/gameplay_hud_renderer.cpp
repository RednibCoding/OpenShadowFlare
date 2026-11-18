#include "gameplay_hud_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "world/companion_actor.hpp"
#include "world/player_actor.hpp"
#include "world/player_data.hpp"
#include "world/player_runtime_profile.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <array>

namespace osf {
namespace {

constexpr std::int32_t kRetailBarWidth = 206;
constexpr std::int32_t kRetailCompanionBarWidth = 109;

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

void drawTownLegend(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const WorldScene& world) {
    if (!world.compassActive() ||
        world.compassPatterns().patterns().empty()) {
        return;
    }

    constexpr std::int32_t kCompassTicksPerFrame = 1;
    constexpr std::int32_t kCompassHoldTicks = 120;
    constexpr std::int32_t kCompassFadeTicks = 60;
    constexpr std::int32_t kCompassCenterX = 50;
    constexpr std::int32_t kCompassCenterY = 38;
    constexpr std::int32_t kCompassTextX = 106;
    constexpr std::int32_t kCompassTextY = 32;
    constexpr std::int32_t kCompassDirection = 8;

    const std::int32_t ticks_elapsed =
        static_cast<std::int32_t>(world.frameCounter()) -
        static_cast<std::int32_t>(world.compassActivatedFrame());
    const std::int32_t settle_ticks = 7 * kCompassTicksPerFrame;
    const std::int32_t hold_end_ticks =
        settle_ticks + kCompassHoldTicks;

    std::int32_t fade_alpha = 1000;
    if (ticks_elapsed > hold_end_ticks) {
        const std::int32_t fade_ticks = ticks_elapsed - hold_end_ticks;
        if (fade_ticks >= kCompassFadeTicks) {
            return;
        }
        fade_alpha = 1000 - fade_ticks * 1000 / kCompassFadeTicks;
        if (fade_alpha <= 0) {
            return;
        }
    }

    const gapi::CafAnimation& animation = world.compassAnimation();
    if (animation.charts().empty()) {
        return;
    }
    const gapi::CafChart& chart = animation.charts().front();
    const gapi::CafDirection& direction =
        chart.directions[static_cast<std::size_t>(kCompassDirection)];
    if (direction.frame_count <= 0 || direction.parts.empty()) {
        return;
    }

    const std::int32_t anim_frame =
        ticks_elapsed / kCompassTicksPerFrame;
    const std::int32_t frame_idx = std::min(
        anim_frame, direction.frame_count - 1);

    const gapi::NjpImage& patterns = world.compassPatterns();
    for (const std::vector<gapi::CafCell>& part : direction.parts) {
        if (static_cast<std::int32_t>(part.size()) <= frame_idx) {
            continue;
        }
        const gapi::CafCell& cell =
            part[static_cast<std::size_t>(frame_idx)];
        if (cell.pattern_index < 0 ||
            static_cast<std::size_t>(cell.pattern_index) >=
                patterns.patterns().size()) {
            continue;
        }
        const gapi::NjpPattern& pattern =
            patterns.patterns()[static_cast<std::size_t>(
                cell.pattern_index)];
        const std::int32_t cell_opacity =
            cell.transparency * fade_alpha / 1000;
        const std::int32_t draw_x =
            kCompassCenterX - pattern.x - pattern.width / 2;
        const std::int32_t draw_y =
            kCompassCenterY - pattern.y - pattern.height / 2;
        renderer.drawPattern(
            patterns,
            static_cast<std::size_t>(cell.pattern_index),
            {draw_x,
             draw_y,
             1000,
             1000,
             1000,
             cell_opacity,
             1000,
             1000,
             1000,
             -1,
             {},
             (cell.status & 0x10) != 0
                 ? gapi::PatternBlendMode::additive
                 : gapi::PatternBlendMode::normal});
    }

    if (!world.compassText().empty() && !font.patterns().empty()) {
        gapi::TextDraw text_draw;
        text_draw.x = kCompassTextX;
        text_draw.y = kCompassTextY;
        text_draw.color = {255, 255, 255,
                           static_cast<std::uint8_t>(
                               (255 * fade_alpha) / 1000)};
        renderer.drawText(font, world.compassText(), text_draw);
    }
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
    const PlayerRuntimeProfile& profile,
    MovementPace movement_pace,
    std::int32_t experience_threshold,
    std::int32_t current_life,
    std::int32_t current_mana,
    bool increased_power_ready,
    bool increased_power_activation_feedback,
    std::int32_t animation_counter,
    const CompanionActor* companion,
    bool companion_inactive) {
    GameplayHudValues values{
        player.level(),
        current_life,
        profile.maximum_life,
        current_mana,
        profile.maximum_mana,
        player.experience(),
        experience_threshold,
        movement_pace == MovementPace::run,
        increased_power_ready,
        increased_power_activation_feedback,
        animation_counter,
    };
    if (companion) {
        values.companion_present = true;
        values.companion_current_life =
            companion->currentLife();
        values.companion_maximum_life =
            companion->maximumLife();
        values.companion_inactive = companion_inactive;
    }
    return values;
}

std::int32_t gameplayHudCompanionBarWidth(
    std::int32_t current,
    std::int32_t maximum) {
    if (current <= 0 || maximum <= 0) {
        return 0;
    }
    if (current == maximum) {
        return kRetailCompanionBarWidth;
    }
    return static_cast<std::int32_t>(
        static_cast<std::int64_t>(current) *
        kRetailCompanionBarWidth / maximum);
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
    const GameplayHudValues& values,
    const gapi::NjpImage& font,
    const WorldScene& world) {
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

    if (values.companion_present) {
        renderer.drawPattern(bar_patterns, 30);
        const std::int32_t width =
            gameplayHudCompanionBarWidth(
                values.companion_current_life,
                values.companion_maximum_life);
        if (width > 0) {
            std::int32_t strength = 1000;
            if (width * 100 /
                    kRetailCompanionBarWidth < 30 &&
                values.animation_counter % 4 < 2) {
                strength = 1500;
            }
            gapi::PatternDraw draw;
            draw.clip = {
                110 - width,
                396,
                width,
                11,
            };
            draw.red_strength = strength;
            draw.green_strength = strength;
            draw.blue_strength = strength;
            renderer.drawPattern(bar_patterns, 29, draw);
        }
        renderer.drawPattern(
            bar_patterns,
            values.companion_inactive ? 32 : 31);
    }

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

    if (values.increased_power_ready) {
        const std::int32_t phase =
            values.animation_counter % 4;
        const std::int32_t strength =
            phase < 2 ? 1000 : 800;
        renderer.drawPattern(
            bar_patterns,
            12,
            {0, 0, 1000, 1000, 1000, 1000,
             strength, strength, strength});
    } else if (values.increased_power_activation_feedback) {
        renderer.drawPattern(bar_patterns, 13);
    }
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

    drawTownLegend(renderer, font, world);
}

}  // namespace osf
