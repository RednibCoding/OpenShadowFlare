#include "gameplay_help_renderer.hpp"

#include "character_renderer.hpp"
#include "gapi/gapi.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "world/world_scene.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace osf {
namespace {

constexpr gapi::Color kHeadingColor{
    224, 224, 64, 255,
};
constexpr gapi::Color kKeyColor{
    139, 169, 201, 255,
};
constexpr gapi::Color kActionColor{
    192, 192, 192, 255,
};

struct HelpRow {
    std::string_view key;
    std::string_view action;
};

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

void drawPreviewFrame(gapi::Backend& renderer) {
    constexpr gapi::Color color{255, 255, 255, 255};
    renderer.drawRectangle({63, 69, 232, 1, color, 1000, 500});
    renderer.drawRectangle({63, 198, 232, 1, color, 1000, 500});
    renderer.drawRectangle({63, 69, 1, 130, color, 1000, 500});
    renderer.drawRectangle({294, 69, 1, 130, color, 1000, 500});
}

void drawPlayerPreview(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t animation_counter) {
    if (!world.hasPlayer()) {
        return;
    }

    const WorldPosition position =
        world.playerRenderPosition(1.0);
    const ScreenPosition screen =
        calculateRealPosition(position);
    renderCharacterAnimationPass(
        renderer,
        world.playerAnimation(),
        world.playerPatterns(),
        world.playerShadowPatterns(),
        position,
        7,
        world.playerDirection(),
        animation_counter,
        [&world](std::size_t part) {
            return world.playerPartEnabled(part);
        },
        [](std::size_t) {
            return CharacterColorStrength{};
        },
        screen.x - 152,
        screen.y - 148,
        false,
        1000);
}

void drawCompanionPreview(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t animation_counter) {
    if (!world.hasCompanion()) {
        return;
    }

    const CompanionActor& companion = world.companion();
    const WorldPosition position =
        companion.renderPosition(1.0);
    const ScreenPosition screen =
        calculateRealPosition(position);
    renderCharacterAnimationPass(
        renderer,
        companion.animation(),
        companion.patterns(),
        companion.shadowPatterns(),
        position,
        7,
        companion.direction(),
        animation_counter,
        [&companion](std::size_t part) {
            return companion.partEnabled(part);
        },
        [&companion](std::size_t part) {
            return CharacterColorStrength{
                companion.partRedStrength(part),
                companion.partGreenStrength(part),
                companion.partBlueStrength(part),
            };
        },
        screen.x - 212,
        screen.y - 158,
        false,
        1000);
}

void drawCloseControl(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    std::int32_t animation_counter) {
    if (animation_counter < 20) {
        renderer.drawPattern(
            status_patterns,
            27,
            {301, 413 - animation_counter});
        return;
    }

    constexpr std::array<std::size_t, 8> patterns{{
        27, 28, 29, 30, 30, 29, 28, 27,
    }};
    const std::size_t phase =
        static_cast<std::size_t>(
            (animation_counter - 20) / 4) %
        patterns.size();
    renderer.drawPattern(
        status_patterns, patterns[phase], {301, 393});
}

void drawKeyboardColumn(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const std::array<HelpRow, 10>& rows,
    std::int32_t key_x,
    std::int32_t action_x) {
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const std::int32_t y =
            232 + static_cast<std::int32_t>(index) * 14;
        drawText(
            renderer,
            font,
            rows[index].key,
            key_x,
            y,
            kKeyColor);
        drawText(
            renderer,
            font,
            rows[index].action,
            action_x,
            y,
            kActionColor);
    }
}

}  // namespace

void renderGameplayHelp(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const WorldScene& world,
    std::int32_t animation_counter,
    bool close_visible,
    std::int32_t close_animation_counter) {
    // FUN_0040e710 uses the authored full-screen help frame and preview
    // ground from Status.njp. The lower gameplay HUD remains visible.
    renderer.drawPattern(status_patterns, 10);
    renderer.drawPattern(status_patterns, 66, {64, 70});
    drawPreviewFrame(renderer);
    drawPlayerPreview(renderer, world, animation_counter);
    drawCompanionPreview(renderer, world, animation_counter);

    drawText(
        renderer,
        font,
        "SHADOW FLARE  \" MOUSE ACTION HELP \"",
        42,
        48,
        kHeadingColor);
    drawText(
        renderer,
        font,
        "SHADOW FLARE  \" KEYBOARD HELP \"",
        42,
        211,
        kHeadingColor);

    constexpr std::array<HelpRow, 7> mouse_rows{{
        {"Attack While Moving", "L-click on Enemy"},
        {"Attack When Not Moving", "SHIFT+L-click"},
        {"Use Magic", "Magic Not Selected"},
        {"Check Targets", "L-click Near Object"},
        {"Companions's Attack", "TAB+L-click on Enemy"},
        {"Companion's Dash", "TAB+R-click on Enemy"},
        {"Let Companion Get Items", "TAB+L-click on Items"},
    }};
    for (std::size_t index = 0;
         index < mouse_rows.size();
         ++index) {
        const std::int32_t y =
            68 + static_cast<std::int32_t>(index) * 20;
        drawText(
            renderer,
            font,
            mouse_rows[index].key,
            310,
            y,
            kKeyColor);
        drawText(
            renderer,
            font,
            mouse_rows[index].action,
            460,
            y,
            kActionColor);
    }

    constexpr std::array<HelpRow, 10> left_rows{{
        {"[F1]~[F9]", "Actions with R-clicking"},
        {"[1]~[8]", "Use Medicine in Belt Pocket"},
        {"[N]", "Open the Navigation Window"},
        {"[B]", "Land Mines"},
        {"[P]", "Increased-Power Mode On"},
        {"[CTRL]", "Run"},
        {"[SHIFT]", "Action w/o Movement"},
        {"[SPACE]", "Companion Active/Inactive"},
        {"[H]", "Open Help Window"},
        {"[ENTER]", "Chat(w/ Shift, Chat w/ All)"},
    }};
    constexpr std::array<HelpRow, 10> right_rows{{
        {"[S]", "Open Status Window"},
        {"[I]", "Open Item Window"},
        {"[M]", "Open Magic Window"},
        {"[X]", "Open Special Item Window"},
        {"[Q]", "Open Mission List Window"},
        {"[R]", "Switch Walk/Run"},
        {"[PrintScreen]", "Get Screen Shots"},
        {"[ScrollLock]", "Producers"},
        {"[Pause]", "Save Image for Load Screen"},
        {"[ESC]", "Open the Settings Menu"},
    }};
    drawKeyboardColumn(renderer, font, left_rows, 58, 124);
    drawKeyboardColumn(renderer, font, right_rows, 322, 406);
    if (close_visible) {
        drawCloseControl(
            renderer,
            status_patterns,
            close_animation_counter);
    }
}

}  // namespace osf
