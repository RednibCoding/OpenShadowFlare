#include "gameplay_equipment_color_renderer.hpp"

#include "character_renderer.hpp"
#include "gapi/gapi.hpp"
#include "items/item_appearance.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "states/gameplay_equipment_color.hpp"
#include "world/world_scene.hpp"

#include <cstddef>

namespace osf {

void renderGameplayEquipmentColor(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const GameplayEquipmentColor& equipment_color,
    const WorldScene& world,
    std::uint32_t gameplay_counter) {
    if (!equipment_color.active() || !world.hasPlayer()) {
        return;
    }

    renderer.drawPattern(status_patterns, 102);
    renderer.drawPattern(status_patterns, 105);
    if (equipment_color.acceptHovered()) {
        renderer.drawPattern(status_patterns, 106);
    }
    if (equipment_color.cancelHovered()) {
        renderer.drawPattern(status_patterns, 104);
    }

    const std::size_t target =
        static_cast<std::size_t>(equipment_color.selectedTarget());
    constexpr std::size_t target_patterns[] = {108, 107, 103};
    if (target < GameplayEquipmentColor::target_count) {
        renderer.drawPattern(status_patterns, target_patterns[target]);
    }

    for (std::int32_t color = 0;
         color < retail_item_color_count;
         ++color) {
        const ItemAppearanceStrength strength =
            retailItemColorStrength(color);
        renderer.drawPattern(
            status_patterns,
            109,
            {
                (color % 4) * 32,
                (color / 4) * 24,
                1000,
                1000,
                1000,
                1000,
                strength.red,
                strength.green,
                strength.blue,
            });
    }
    const std::int32_t selected_color =
        equipment_color.selectedColor();
    if (selected_color >= 0) {
        renderer.drawRectangle({
            332 + (selected_color % 4) * 32,
            187 + (selected_color / 4) * 24,
            26,
            18,
            {255, 255, 255, 255},
            1000,
            500,
        });
    }

    const WorldPosition position = world.playerRenderPosition(1.0);
    const ScreenPosition screen = calculateRealPosition(position);
    renderCharacterAnimationPass(
        renderer,
        world.playerAnimation(),
        world.playerPatterns(),
        world.playerShadowPatterns(),
        position,
        equipment_color.previewChart(),
        8,
        static_cast<std::int32_t>(gameplay_counter),
        [&world](std::size_t part) {
            return world.playerPartEnabled(part);
        },
        [&world](std::size_t part) {
            return CharacterColorStrength{
                world.playerPartRedStrength(part),
                world.playerPartGreenStrength(part),
                world.playerPartBlueStrength(part),
            };
        },
        screen.x - 520,
        screen.y - 244,
        false,
        1000);
}

}  // namespace osf
