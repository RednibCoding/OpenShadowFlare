#include "gameplay_options_renderer.hpp"

#include "core/game_config.hpp"
#include "gapi/gapi.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "states/gameplay_options_menu.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace osf {
namespace {

constexpr gapi::Color kLabelColor{
    224, 224, 224, 255,
};
constexpr gapi::Color kSelectedColor{
    255, 255, 255, 255,
};
constexpr gapi::Color kUnselectedColor{
    128, 128, 128, 255,
};
constexpr gapi::Color kHoverColor{
    64, 64, 224, 255,
};
constexpr gapi::Color kPriorityHoverColor{
    64, 64, 255, 255,
};

bool hovered(
    const GameplayOptionsMenu& menu,
    std::int32_t left,
    std::int32_t top,
    std::int32_t right,
    std::int32_t bottom) {
    return menu.pointerX() >= left &&
           menu.pointerX() < right &&
           menu.pointerY() >= top &&
           menu.pointerY() < bottom;
}

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

void drawBooleanRow(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const GameplayOptionsMenu& menu,
    std::string_view label,
    std::int32_t y,
    bool value) {
    drawText(renderer, font, label, 184, y, kLabelColor);
    drawText(
        renderer,
        font,
        "  ON",
        376,
        y,
        hovered(menu, 376, y, 426, y + 12)
            ? kHoverColor
            : (value ? kSelectedColor : kUnselectedColor));
    drawText(
        renderer,
        font,
        " OFF",
        426,
        y,
        hovered(menu, 426, y, 464, y + 12)
            ? kHoverColor
            : (!value ? kSelectedColor : kUnselectedColor));
}

void drawCenteredAction(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const GameplayOptionsMenu& menu,
    std::string_view text,
    std::int32_t y) {
    constexpr std::int32_t panel_x = 176;
    constexpr std::int32_t panel_width = 288;
    const std::int32_t x =
        panel_x +
        (panel_width -
         static_cast<std::int32_t>(text.size()) * 6) /
            2;
    const gapi::Color color =
        hovered(menu, 176, y, 464, y + 12)
            ? kHoverColor
            : kLabelColor;
    drawText(renderer, font, text, x, y, color);
}

std::int32_t knobOpacity(std::int32_t counter) {
    const std::int32_t phase = counter % 100;
    return phase * 10 < 500
        ? 1000 - phase * 10
        : phase * 10;
}

}  // namespace

void renderGameplayOptions(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayOptionsMenu& menu,
    const GameConfig& config) {
    if (!menu.active()) {
        return;
    }

    // OptionsMenu (0x004103c0) uses the two Status.njp panel patterns at
    // their authored coordinates. Pattern 59 is the half-transparent fill;
    // pattern 58 supplies the opaque frame.
    renderer.drawPattern(
        status_patterns,
        59,
        {0, 0, 1000, 1000, 1000, 500});
    renderer.drawPattern(status_patterns, 58);

    // The retail screen-mode row occupies y=86. It is deliberately omitted
    // in the portable game, but all following rows retain their retail y.
    drawBooleanRow(
        renderer,
        font,
        menu,
        "Semi-transparent Objects",
        102,
        config.semi_transparent_objects);
    drawBooleanRow(
        renderer,
        font,
        menu,
        "Semi-transparent Shadow",
        118,
        config.semi_transparent_shadow);
    drawBooleanRow(
        renderer,
        font,
        menu,
        "Display Darkness",
        134,
        config.display_darkness);
    drawBooleanRow(
        renderer,
        font,
        menu,
        "Save Image at Game End",
        150,
        config.save_image_at_game_end);
    drawBooleanRow(
        renderer,
        font,
        menu,
        "Click Range",
        166,
        config.click_range_enabled);

    drawText(
        renderer, font, "Click Range", 184, 182, kLabelColor);
    constexpr std::array<std::string_view, 5> ranges{{
        "MINI", "SMAL", "NORM", "LARG", "MAX.",
    }};
    for (std::int32_t index = 0; index < 5; ++index) {
        const std::int32_t x = 316 + index * 30;
        const gapi::Color color =
            hovered(menu, x + 1, 182, x + 25, 194)
                ? kHoverColor
                : (config.click_range == index
                       ? kSelectedColor
                       : kUnselectedColor);
        drawText(
            renderer,
            font,
            ranges[static_cast<std::size_t>(index)],
            x,
            182,
            color);
    }

    drawText(
        renderer,
        font,
        "Click Priority",
        184,
        198,
        kLabelColor);
    constexpr std::array<std::string_view, 5> target_types{{
        "ENEM", "OBJ.", "ITEM", "PEOP", "COMP",
    }};
    for (std::int32_t display_index = 0;
         display_index < 5;
         ++display_index) {
        const std::int32_t priority = 4 - display_index;
        std::size_t type_index = 0;
        while (type_index < config.click_priority.size() &&
               config.click_priority[type_index] != priority) {
            ++type_index;
        }
        if (type_index == config.click_priority.size()) {
            continue;
        }
        const std::int32_t x =
            316 + display_index * 30;
        drawText(
            renderer,
            font,
            target_types[type_index],
            x,
            198,
            hovered(menu, x + 1, 198, x + 25, 210)
                ? kPriorityHoverColor
                : kLabelColor);
    }

    drawText(
        renderer, font, "EFF.VOLUME", 184, 218, kLabelColor);
    drawText(
        renderer, font, "BGM VOLUME", 184, 238, kLabelColor);
    renderer.drawPattern(
        status_patterns, 120, {246, 223});
    renderer.drawPattern(
        status_patterns, 120, {246, 243});
    const std::int32_t opacity =
        knobOpacity(menu.animationCounter());
    renderer.drawPattern(
        status_patterns,
        68,
        {
            246 + gameplayOptionsVolumeSliderOffset(
                      config.effect_volume),
            223,
            1000,
            1000,
            1000,
            opacity,
        });
    renderer.drawPattern(
        status_patterns,
        68,
        {
            246 + gameplayOptionsVolumeSliderOffset(
                      config.bgm_volume),
            243,
            1000,
            1000,
            1000,
            opacity,
        });

    drawCenteredAction(
        renderer, font, menu, "Mission List", 254);
    drawCenteredAction(
        renderer, font, menu, "MAP", 270);
    drawCenteredAction(
        renderer, font, menu, "HELP", 286);
    drawCenteredAction(
        renderer,
        font,
        menu,
        "Save and Return to Title",
        302);
    drawCenteredAction(
        renderer, font, menu, "Save and Exit", 318);
}

}  // namespace osf
