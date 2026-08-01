#include "gameplay_debug_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "states/gameplay_debug_menu.hpp"

#include <string>
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

bool hovered(
    const GameplayDebugMenu& menu,
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
    const GameplayDebugMenu& menu,
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

}  // namespace

void renderGameplayDebugMenu(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayDebugMenu& menu) {
    if (!menu.active()) {
        return;
    }
    renderer.drawPattern(
        status_patterns,
        59,
        {0, 0, 1000, 1000, 1000, 500});
    renderer.drawPattern(status_patterns, 58);

    drawText(
        renderer,
        font,
        "DEBUG MENU",
        290,
        86,
        kLabelColor);
    drawBooleanRow(
        renderer,
        font,
        menu,
        "FPS Counter",
        118,
        menu.fpsCounterEnabled());
    drawBooleanRow(
        renderer,
        font,
        menu,
        "All Spells",
        134,
        menu.allSpellsEnabled());
    drawBooleanRow(
        renderer,
        font,
        menu,
        "Infinite HP",
        150,
        menu.infiniteLifeEnabled());
    drawBooleanRow(
        renderer,
        font,
        menu,
        "Infinite MP",
        166,
        menu.infiniteManaEnabled());

    const gapi::Color close_color =
        hovered(menu, 176, 198, 464, 210)
            ? kHoverColor
            : kLabelColor;
    drawText(
        renderer,
        font,
        "CLOSE",
        305,
        198,
        close_color);
}

void renderGameplayDebugFps(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    std::int32_t frames_per_second) {
    const std::string text =
        "FPS " + std::to_string(frames_per_second);
    drawText(
        renderer,
        font,
        text,
        636 - static_cast<std::int32_t>(text.size()) * 6,
        4,
        kSelectedColor);
}

}  // namespace osf
