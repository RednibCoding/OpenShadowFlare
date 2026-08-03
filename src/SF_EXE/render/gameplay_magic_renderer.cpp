#include "gameplay_magic_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "states/gameplay_magic.hpp"
#include "world/player_magic.hpp"
#include "world/player_spell_parameters.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cinttypes>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace osf {
namespace {

constexpr std::array<std::string_view, 22> kSpellNames{{
    "Transport",
    "Fire Ball",
    "Ice Bolt",
    "Plasma",
    "Hell Fire",
    "Ice Blast",
    "Heal",
    "Moon",
    "Berserker",
    "Energy Shield",
    "Earth Spear",
    "Flame Strike",
    "Dread Deathscythe",
    "Lightning Storm",
    "Medusa",
    "Sonic Blade",
    "Mud Javelin",
    "Identify",
    "Magic Shield",
    "Counter Burst",
    "Explosion",
    "Elemental Strike",
}};

constexpr std::array<std::string_view, 22> kEffectLabels{{
    "",
    "Attack.",
    "Attack.",
    "Attack.",
    "Attack.",
    "Attack.",
    "Heal.",
    "",
    "",
    "Def.",
    "Attack.",
    "Attack.",
    "Attack.",
    "Attack.",
    "Attack.",
    "Attack.",
    "Attack.",
    "",
    "Shield.",
    "RefPer.",
    "Attack.",
    "Attack.",
}};

void drawShadowedText(
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
    renderer.drawText(
        font,
        text,
        {x, y, color});
}

std::string number(std::int32_t value) {
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%" PRId32, value);
    return buffer;
}

std::vector<std::string_view> descriptionLines(
    const WorldScene& world,
    std::int32_t spell) {
    std::vector<std::string_view> result;
    const TableData* description =
        world.parameterTables().find(600 + spell);
    if (!description) {
        return result;
    }
    for (std::int32_t row = 0;
         row < description->rowCount();
         ++row) {
        const std::string_view line =
            description->text(row, 0);
        // Retail's English table uses this Shift-JIS full-width space as a
        // deliberate blank line.
        if (line.size() == 2 &&
            static_cast<unsigned char>(line[0]) == 0x81 &&
            static_cast<unsigned char>(line[1]) == 0x40) {
            result.emplace_back();
        } else {
            result.push_back(line);
        }
    }
    return result;
}

GameplayMagicModel gameplayMagicModel(
    const PlayerMagic& magic) {
    GameplayMagicModel model;
    for (std::size_t spell = 0;
         spell < model.availability.size();
         ++spell) {
        model.availability[spell] =
            magic.availability(
                static_cast<std::int32_t>(spell));
    }
    for (std::size_t slot = 0;
         slot < model.bar_slots.size();
         ++slot) {
        model.bar_slots[slot] =
            magic.barSlot(
                static_cast<std::int32_t>(slot));
    }
    model.selected_spell =
        magic.selectedSpell();
    model.targeting = magic.targeting();
    return model;
}

void renderDescription(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const GameplayMagic& panel,
    const WorldScene& world) {
    const std::int32_t spell =
        panel.hoveredSpell();
    if (spell < 0 ||
        !world.playerMagic().learned(spell)) {
        return;
    }
    const auto lines = descriptionLines(world, spell);
    if (lines.empty()) {
        return;
    }

    std::size_t longest = 0;
    for (const std::string_view line : lines) {
        longest = std::max(longest, line.size());
    }
    const std::int32_t width =
        static_cast<std::int32_t>(longest) * 6 + 8;
    const std::int32_t height =
        static_cast<std::int32_t>(lines.size()) * 12 + 8;
    const std::int32_t x = std::clamp(
        panel.pointerX() - width / 2,
        std::int32_t{1},
        639 - width);
    const std::int32_t y = std::clamp(
        panel.pointerY() + 8,
        std::int32_t{1},
        479 - height);
    renderer.drawRectangle({
        x,
        y,
        width,
        height,
        {0, 0, 0, 255},
        1000,
        600,
    });
    constexpr gapi::Color border{
        255, 255, 255, 255,
    };
    renderer.drawRectangle(
        {x - 1, y - 1, width + 2, 1, border, 1000, 500});
    renderer.drawRectangle(
        {x - 1, y + height, width + 2, 1, border, 1000, 500});
    renderer.drawRectangle(
        {x - 1, y - 1, 1, height + 2, border, 1000, 500});
    renderer.drawRectangle(
        {x + width, y - 1, 1, height + 2, border, 1000, 500});
    std::int32_t line_y = y + 4;
    for (const std::string_view line : lines) {
        renderer.drawText(
            font,
            line,
            {x + 4, line_y, {224, 224, 224, 255}});
        line_y += 12;
    }
}

}  // namespace

void renderGameplayMagicPanel(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& magic_icons,
    const gapi::NjpImage& font,
    const GameplayMagic& panel,
    const WorldScene& world) {
    if (!panel.active()) {
        return;
    }

    // MagicWindowDisplay (0x00407a60) uses Status.njp pattern 6, six
    // 48-pixel rows, and the spell-number-plus-two MagicIcon mapping.
    renderer.drawPattern(status_patterns, 6);
    const PlayerMagic& magic = world.playerMagic();
    for (std::int32_t row = 0;
         row < GameplayMagic::spells_per_page;
         ++row) {
        const std::int32_t spell =
            panel.page() *
                GameplayMagic::spells_per_page +
            row;
        if (spell >=
            static_cast<std::int32_t>(
                PlayerMagic::spell_count)) {
            break;
        }
        const std::int32_t icon_y = 59 + row * 48;
        renderer.drawPattern(
            status_patterns,
            32,
            {24, icon_y - 3});

        const std::int32_t availability =
            magic.availability(spell);
        if (availability == 3) {
            renderer.drawPattern(
                magic_icons,
                static_cast<std::size_t>(spell + 2),
                {27, icon_y});
        } else if (availability == 1) {
            renderer.drawPattern(
                magic_icons,
                static_cast<std::size_t>(spell + 2),
                {
                    27,
                    icon_y,
                    1000,
                    1000,
                    300,
                });
        }
        if ((availability & 1) == 0) {
            continue;
        }

        const PlayerSpellParameters parameters =
            playerSpellParameters(
                magic,
                spell,
                world.playerEquipment(),
                world.itemDatabase(),
                world.parameterTables(),
                0,
                world.playerIncreasedPowerActive());
        constexpr gapi::Color name_color{
            224, 192, 128, 255,
        };
        constexpr gapi::Color label_color{
            160, 160, 64, 255,
        };
        constexpr gapi::Color value_color{
            224, 224, 224, 255,
        };
        const std::int32_t first_y = icon_y + 7;
        const std::int32_t second_y = first_y + 12;
        drawShadowedText(
            renderer,
            font,
            kSpellNames[
                static_cast<std::size_t>(spell)],
            59,
            first_y,
            name_color);
        drawShadowedText(
            renderer,
            font,
            "Lv.",
            192,
            first_y,
            label_color);
        drawShadowedText(
            renderer,
            font,
            number(parameters.effective_level),
            216,
            first_y,
            value_color);
        drawShadowedText(
            renderer,
            font,
            "Exp.",
            234,
            first_y,
            label_color);
        const std::string experience =
            parameters.maximum_level
                ? "Max"
                : number(magic.experience(spell)) +
                      "/" +
                      number(
                          parameters
                              .experience_threshold);
        drawShadowedText(
            renderer,
            font,
            experience,
            264,
            first_y,
            value_color);
        drawShadowedText(
            renderer,
            font,
            "MP.",
            192,
            second_y,
            label_color);
        drawShadowedText(
            renderer,
            font,
            number(parameters.mana_cost),
            216,
            second_y,
            value_color);
        const std::string_view effect_label =
            kEffectLabels[
                static_cast<std::size_t>(spell)];
        if (!effect_label.empty()) {
            drawShadowedText(
                renderer,
                font,
                effect_label,
                240,
                second_y,
                label_color);
            drawShadowedText(
                renderer,
                font,
                number(parameters.effect_value),
                294,
                second_y,
                value_color);
        }
    }

    if (panel.page() > 0) {
        renderer.drawPattern(status_patterns, 69);
    }
    if (panel.page() + 1 <
        GameplayMagic::page_count) {
        renderer.drawPattern(status_patterns, 70);
    }
    for (std::int32_t slot = 0;
         slot <
             static_cast<std::int32_t>(
                 PlayerMagic::bar_slot_count);
         ++slot) {
        const std::int32_t spell =
            magic.barSlot(slot);
        if (spell >= 0 && magic.learned(spell)) {
            renderer.drawPattern(
                magic_icons,
                static_cast<std::size_t>(spell + 2),
                {32 + slot * 32, 359});
        }
    }
    renderDescription(
        renderer, font, panel, world);
}

void renderGameplayMagicBar(
    gapi::Backend& renderer,
    const gapi::NjpImage& magic_icons,
    const gapi::NjpImage& magic_bar_icons,
    bool left_panel_active,
    bool right_panel_active,
    const WorldScene& world) {
    if (left_panel_active &&
        right_panel_active) {
        return;
    }

    const PlayerMagic& magic = world.playerMagic();
    const GameplayMagicModel model =
        gameplayMagicModel(magic);
    const auto slots =
        GameplayMagic::persistentBarSlots(
            model,
            left_panel_active,
            right_panel_active);
    for (std::int32_t slot = 0;
         slot <
             static_cast<std::int32_t>(
                 slots.size());
         ++slot) {
        const MagicBarSlotRegion& region =
            slots[static_cast<std::size_t>(slot)];
        const std::int32_t spell =
            magic.barSlot(slot);
        if (spell < 0 || !magic.learned(spell)) {
            renderer.drawPattern(
                magic_bar_icons,
                3,
                {region.x, 392});
        } else if (
            spell == magic.selectedSpell()) {
            renderer.drawPattern(
                magic_icons,
                static_cast<std::size_t>(spell + 2),
                {region.x, 382});
        } else {
            renderer.drawPattern(
                magic_bar_icons,
                static_cast<std::size_t>(spell + 4),
                {region.x, 392});
        }
    }
    const MagicBarSlotRegion target =
        GameplayMagic::persistentTargetRegion(
            model,
            left_panel_active,
            right_panel_active);
    renderer.drawPattern(
        magic.targeting()
            ? magic_icons
            : magic_bar_icons,
        magic.targeting() ? 0u : 2u,
        {
            target.x,
            magic.targeting() ? 382 : 392,
        });
}

void renderHeldMagic(
    gapi::Backend& renderer,
    const gapi::NjpImage& magic_icons,
    const GameplayMagic& panel) {
    if (panel.heldSpell() < 0) {
        return;
    }
    renderer.drawPattern(
        magic_icons,
        static_cast<std::size_t>(
            panel.heldSpell() + 2),
        {
            panel.pointerX() - 13,
            panel.pointerY() - 13,
        });
}

}  // namespace osf
