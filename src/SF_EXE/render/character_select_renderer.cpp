#include "character_select_renderer.hpp"

#include "world/player_job.hpp"

#include <algorithm>
#include <string>
#include <string_view>

namespace osf {
namespace {

void drawText(
    gapi::Backend& renderer,
    const gapi::NjpImage* font,
    std::string_view text,
    std::int32_t x,
    std::int32_t y,
    std::int32_t brightness = 1000) {
    if (!font) {
        return;
    }
    renderer.drawText(
        *font,
        text,
        {x + 1, y + 1, {0, 0, 0, 255}, brightness});
    renderer.drawText(
        *font,
        text,
        {x, y, {255, 255, 255, 255}, brightness});
}

std::int32_t textCellCount(std::string_view text) {
    std::int32_t cells = 0;
    for (std::size_t index = 0; index < text.size();) {
        const std::uint8_t first =
            static_cast<std::uint8_t>(text[index]);
        std::size_t length = 1;
        if ((first & 0xe0u) == 0xc0u) {
            length = 2;
        } else if ((first & 0xf0u) == 0xe0u) {
            length = 3;
        } else if ((first & 0xf8u) == 0xf0u) {
            length = 4;
        }
        if (index + length > text.size()) {
            break;
        }
        cells += length == 1 ? 1 : 2;
        index += length;
    }
    return cells;
}

void renderNameEditor(
    gapi::Backend& renderer,
    const gapi::NjpImage* font,
    const CharacterSelectStateData& data,
    std::int32_t brightness) {
    constexpr std::int32_t kEditorX = 190;
    constexpr std::int32_t kEditorY = 407;
    constexpr std::int32_t kEditorWidth = 130;
    constexpr std::int32_t kEditorHeight = 20;
    constexpr std::int32_t kFontWidth = 6;
    constexpr std::int32_t kFontHeight = 12;
    constexpr std::int32_t kTextInset = 4;

    renderer.drawRectangle(
        {kEditorX,
         kEditorY,
         kEditorWidth,
         kEditorHeight,
         {64, 64, 64, 255},
         brightness});
    const std::int32_t caretColumn =
        std::min(textCellCount(data.character_name), 20);
    renderer.drawRectangle(
        {kEditorX + kTextInset + caretColumn * kFontWidth,
         kEditorY + kTextInset,
         kFontWidth,
         kFontHeight,
         {128, 0, 0, 255},
         brightness});
    if (font) {
        renderer.drawText(
            *font,
            data.character_name,
            {kEditorX + kTextInset,
             kEditorY + kTextInset,
             {255, 255, 255, 255},
             brightness});
    }
}

void renderCharacterModeScreen(
    gapi::Backend& renderer,
    const gapi::NjpImage& select,
    const gapi::NjpImage* font,
    const CharacterSelectStateData& data,
    const CharacterSelectFrameResult& frame,
    std::int32_t brightness) {
    renderer.drawPattern(
        select, 38, {0, 0, 1000, 1000, brightness});
    if (data.screen == 10) {
        const std::size_t patterns[] = {
            data.dialog_selection == 0 ? 23u : 22u,
            data.dialog_selection == 1 ? 25u : 24u,
            data.dialog_selection == 2 ? 21u : 20u,
        };
        for (std::size_t pattern : patterns) {
            renderer.drawPattern(
                select,
                pattern,
                {0, 0, 1000, 1000, brightness});
        }
    } else if (data.screen == 11) {
        const std::size_t patterns[] = {
            data.dialog_selection == 0 ? 27u : 26u,
            data.dialog_selection == 1 ? 29u : 28u,
            data.dialog_selection == 2 ? 21u : 20u,
        };
        for (std::size_t pattern : patterns) {
            renderer.drawPattern(
                select,
                pattern,
                {0, 0, 1000, 1000, brightness});
        }
    } else if (data.screen == 12) {
        renderer.drawPattern(
            select,
            frame.host_connect_hovered ? 19 : 18,
            {0, 0, 1000, 1000, brightness});
        renderer.drawPattern(
            select,
            frame.host_back_hovered ? 21 : 20,
            {-51, -3, 1000, 1000, brightness});
        renderer.drawPattern(
            select,
            frame.host_paste_hovered ? 33 : 32,
            {0, 0, 1000, 1000, brightness});
        drawText(
            renderer,
            font,
            "Host IP Address",
            243,
            205,
            brightness);
        drawText(
            renderer,
            font,
            data.host_address,
            243,
            225,
            brightness);
    }
}

void renderNewCharacter(
    gapi::Backend& renderer,
    const gapi::NjpImage& select,
    const gapi::NjpImage* font,
    const CharacterSelectStateData& data,
    const CharacterSelectFrameResult& frame,
    std::int32_t brightness,
    std::int32_t overlayBrightness) {
    renderer.drawPattern(
        select, 41, {0, 0, 1000, 1000, brightness});
    renderer.drawPattern(
        select, 36, {0, 0, 1000, 1000, brightness});
    renderer.drawPattern(
        select,
        data.launch_counter >= 1000 &&
                data.launch_counter < 2000
            ? 3
            : 2,
        {0, 0, 1000, 1000, brightness});
    renderer.drawPattern(
        select,
        data.launch_counter >= 2000 ? 1 : 0,
        {0, 0, 1000, 1000, brightness});

    if (data.screen == 0) {
        std::int32_t maleX = 0;
        std::int32_t femaleX = 0;
        std::int32_t maleBrightness = brightness;
        std::int32_t femaleBrightness = brightness;
        const std::int32_t transition =
            frame.character_transition_counter;
        if (transition >= 2000 && transition <= 2020) {
            const std::int32_t phase = transition - 2000;
            if (data.character_gender == 1) {
                maleX = 97 - phase * 97 / 20;
                femaleBrightness = brightness * phase / 20;
            } else {
                femaleX = -97 + phase * 97 / 20;
                maleBrightness = brightness * phase / 20;
            }
        }

        renderer.drawPattern(
            select,
            data.dialog_selection == 0 ? 8 : 7,
            {maleX, 0, 1000, 1000, maleBrightness});
        renderer.drawPattern(
            select,
            11,
            {maleX, 0, 1000, 1000, maleBrightness / 2});
        renderer.drawPattern(
            select,
            data.dialog_selection == 1 ? 10 : 9,
            {femaleX, 0, 1000, 1000, femaleBrightness});
        renderer.drawPattern(
            select,
            12,
            {femaleX, 0, 1000, 1000, femaleBrightness / 2});
        renderer.drawPattern(
            select, 34, {0, 0, 1000, 1000, brightness});
        return;
    }
    if (data.screen == 1) {
        const std::int32_t transition =
            frame.character_transition_counter;
        const std::int32_t phase =
            transition >= 1000 && transition <= 1020
                ? transition - 1000
                : 20;
        const std::int32_t otherBrightness =
            brightness * (20 - phase) / 20;
        if (data.character_gender == 1) {
            renderer.drawPattern(
                select,
                9,
                {0, 0, 1000, 1000, otherBrightness});
            renderer.drawPattern(
                select,
                12,
                {0, 0, 1000, 1000, otherBrightness / 2});
            const std::int32_t x = phase * 97 / 20;
            renderer.drawPattern(
                select, 8, {x, 0, 1000, 1000, brightness});
            renderer.drawPattern(
                select,
                11,
                {x, 0, 1000, 1000, brightness / 2});
        } else {
            renderer.drawPattern(
                select,
                7,
                {0, 0, 1000, 1000, otherBrightness});
            renderer.drawPattern(
                select,
                11,
                {0, 0, 1000, 1000, otherBrightness / 2});
            const std::int32_t x = -phase * 97 / 20;
            renderer.drawPattern(
                select, 10, {x, 0, 1000, 1000, brightness});
            renderer.drawPattern(
                select,
                12,
                {x, 0, 1000, 1000, brightness / 2});
        }

        renderer.drawPattern(
            select, 35, {0, 0, 1000, 1000, brightness});
        renderNameEditor(renderer, font, data, brightness);
        if (!data.character_name.empty()) {
            const bool confirmHighlighted =
                data.launch_counter == 0 &&
                frame.name_confirm_hovered;
            renderer.drawPattern(
                select,
                confirmHighlighted ? 5 : 4,
                {0, 0, 1000, 1000, brightness});
        }
        return;
    }
    if (data.screen >= 10 && data.screen <= 12) {
        renderCharacterModeScreen(
            renderer,
            select,
            font,
            data,
            frame,
            overlayBrightness);
    }
}

void renderSavedGames(
    gapi::Backend& renderer,
    const gapi::NjpImage& select,
    const gapi::NjpImage* font,
    const CharacterSelectStateData& data,
    const CharacterSelectFrameResult& frame,
    const std::vector<RetailSaveSummary>& savedGames,
    const std::vector<gapi::BitmapImage>& savedPreviews,
    std::int32_t brightness,
    std::int32_t overlayBrightness) {
    renderer.drawPattern(
        select, 41, {0, 0, 1000, 1000, brightness});
    renderer.drawPattern(
        select, 37, {0, 0, 1000, 1000, brightness});

    const std::int32_t selected = data.saved_game_selection;
    if (selected >= 0 &&
        static_cast<std::size_t>(selected) < savedPreviews.size() &&
        savedPreviews[static_cast<std::size_t>(selected)].width() > 0) {
        renderer.drawBitmap(
            savedPreviews[static_cast<std::size_t>(selected)],
            {224, 60, 1000, 1000, brightness});
    }
    renderer.drawPattern(
        select, 39, {0, 0, 1000, 1000, brightness});

    constexpr std::int32_t hoverFrames[] = {
        3, 2, 1, 0, 0, 1, 2, 3,
    };
    for (std::size_t index = 0; index < 6; ++index) {
        const std::int32_t x =
            32 + static_cast<std::int32_t>(index % 2) * 304;
        const std::int32_t y =
            188 + static_cast<std::int32_t>(index / 2) * 88;
        const std::int32_t itemBrightness =
            index ==
                    static_cast<std::size_t>(
                        std::max(selected, 0))
                ? brightness
                : brightness / 2;
        renderer.drawPattern(
            select,
            40,
            {x, y, 1000, 1000, itemBrightness});
        const bool hovered =
            data.brightness_increasing != 0 &&
            data.launch_counter == 0 &&
            data.save_hover_animation > 28 &&
            frame.save_slot_hovered[index];
        std::int32_t numberFrame =
            static_cast<std::int32_t>(index) == selected ? 0 : 3;
        if (hovered) {
            const std::int32_t renderedCounter =
                std::max(data.save_hover_animation - 1, 0);
            numberFrame =
                hoverFrames[(renderedCounter / 4) & 7];
        }
        const std::size_t animation =
            42 + index * 4 +
            static_cast<std::size_t>(numberFrame);
        renderer.drawPattern(
            select,
            animation,
            {x, y + 28, 1000, 1000, itemBrightness});

        if (index < savedGames.size()) {
            const RetailSaveSummary& save = savedGames[index];
            drawText(
                renderer,
                font,
                "Level.",
                x + 39,
                y + 12,
                itemBrightness);
            drawText(
                renderer,
                font,
                "       " + std::to_string(save.level),
                x + 39,
                y + 12,
                itemBrightness);
            drawText(
                renderer,
                font,
                "Job.",
                x + 104,
                y + 12,
                itemBrightness);
            drawText(
                renderer,
                font,
                "     " +
                    std::string(retailPlayerJobName(
                        save.job, save.gender)),
                x + 104,
                y + 12,
                itemBrightness);
            drawText(
                renderer,
                font,
                "Sex.",
                x + 200,
                y + 12,
                itemBrightness);
            drawText(
                renderer,
                font,
                "     " +
                    std::string(
                        save.gender == 1 ? "Male" : "Female"),
                x + 200,
                y + 12,
                itemBrightness);
            drawText(
                renderer,
                font,
                "Name.",
                x + 39,
                y + 32,
                itemBrightness);
            drawText(
                renderer,
                font,
                "      " + save.name,
                x + 39,
                y + 32,
                itemBrightness);
            drawText(
                renderer,
                font,
                "HP.",
                x + 39,
                y + 52,
                itemBrightness);
            drawText(
                renderer,
                font,
                "    " + std::to_string(save.life),
                x + 39,
                y + 52,
                itemBrightness);
            drawText(
                renderer,
                font,
                "MP.",
                x + 100,
                y + 52,
                itemBrightness);
            drawText(
                renderer,
                font,
                "    " + std::to_string(save.mana),
                x + 100,
                y + 52,
                itemBrightness);
            drawText(
                renderer,
                font,
                "EXP.",
                x + 160,
                y + 52,
                itemBrightness);
            drawText(
                renderer,
                font,
                "     " +
                    std::to_string(save.experience),
                x + 160,
                y + 52,
                itemBrightness);
        } else {
            drawText(
                renderer,
                font,
                "No Data",
                x + 39,
                y + 12,
                itemBrightness);
        }
    }

    renderer.drawPattern(
        select,
        data.launch_counter >= 1000 &&
                data.launch_counter < 2000
            ? 3
            : 2,
        {0, 0, 1000, 1000, brightness});
    renderer.drawPattern(
        select,
        data.launch_counter >= 2000 ? 1 : 0,
        {0, 0, 1000, 1000, brightness});
    if (!savedGames.empty()) {
        renderer.drawPattern(
            select, 4, {0, 0, 1000, 1000, brightness});
        renderer.drawPattern(
            select, 6, {0, 0, 1000, 1000, brightness});
    }

    if (data.screen == 1) {
        renderer.drawPattern(
            select,
            38,
            {0, 0, 1000, 1000, overlayBrightness});
        const std::string name =
            selected >= 0 &&
                    static_cast<std::size_t>(selected) <
                        savedGames.size()
                ? savedGames[static_cast<std::size_t>(selected)].name
                : std::string{};
        drawText(
            renderer,
            font,
            "No. " + std::to_string(selected + 1) +
                "  " + name,
            240,
            190,
            overlayBrightness);
        drawText(
            renderer,
            font,
            "Are you sure you want to delete",
            215,
            210,
            overlayBrightness);
        drawText(
            renderer,
            font,
            "this saved data?",
            268,
            230,
            overlayBrightness);
        renderer.drawPattern(
            select,
            data.dialog_selection == 0 ? 15 : 14,
            {0, 0, 1000, 1000, overlayBrightness});
        renderer.drawPattern(
            select,
            data.dialog_selection == 1 ? 17 : 16,
            {0, 0, 1000, 1000, overlayBrightness});
    } else if (data.screen >= 10 && data.screen <= 12) {
        renderCharacterModeScreen(
            renderer,
            select,
            font,
            data,
            frame,
            overlayBrightness);
    }
}

}  // namespace

void renderCharacterSelect(
    gapi::Backend& renderer,
    const gapi::NjpImage& select,
    const gapi::NjpImage* font,
    const CharacterSelectStateData& data,
    const CharacterSelectFrameResult& frame,
    const std::vector<RetailSaveSummary>& saved_games,
    const std::vector<gapi::BitmapImage>& saved_previews) {
    const std::int32_t brightness =
        std::min(
            frame.background_brightness,
            frame.mode_brightness);
    constexpr std::int32_t overlayBrightness = 1000;
    if (data.mode == CharacterSelectMode::new_character) {
        renderNewCharacter(
            renderer,
            select,
            font,
            data,
            frame,
            brightness,
            overlayBrightness);
    } else {
        renderSavedGames(
            renderer,
            select,
            font,
            data,
            frame,
            saved_games,
            saved_previews,
            brightness,
            overlayBrightness);
    }
}

}  // namespace osf
