#include "gameplay_overlay_renderer.hpp"

#include "ui/conversation_layout.hpp"
#include "gapi/gapi.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

namespace osf {
namespace {

const NpcActor* findNpc(
    const WorldScene& world,
    std::int32_t id) {
    const auto found = std::find_if(
        world.npcs().begin(),
        world.npcs().end(),
        [id](const NpcActor& npc) {
            return npc.id() == id;
        });
    return found == world.npcs().end() ? nullptr : &*found;
}

const EnemyActor* findEnemy(
    const WorldScene& world,
    std::int32_t id) {
    const auto found = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [id](const EnemyActor& enemy) {
            return enemy.id() == id;
        });
    return found == world.enemies().end()
        ? nullptr
        : &*found;
}

const ScenarioObjectActor* findScenarioObject(
    const WorldScene& world,
    std::int32_t id) {
    const auto found = std::find_if(
        world.scenarioObjects().begin(),
        world.scenarioObjects().end(),
        [id](const ScenarioObjectActor& object) {
            return object.id() == id;
        });
    return found == world.scenarioObjects().end()
        ? nullptr
        : &*found;
}

const GroundItem* findGroundItem(
    const WorldScene& world,
    std::int32_t id) {
    const auto found = std::find_if(
        world.groundItems().begin(),
        world.groundItems().end(),
        [id](const GroundItem& item) {
            return item.id == id;
        });
    return found == world.groundItems().end()
        ? nullptr
        : &*found;
}

gapi::Color npcNameColor(const NpcActor& npc) {
    const std::uint32_t color = npc.nameColor();
    return {
        static_cast<std::uint8_t>(color),
        static_cast<std::uint8_t>(color >> 8u),
        static_cast<std::uint8_t>(color >> 16u),
        255,
    };
}

gapi::Color enemyNameColor(const EnemyActor& enemy) {
    const std::uint32_t color = enemy.nameColor();
    return {
        static_cast<std::uint8_t>(color),
        static_cast<std::uint8_t>(color >> 8u),
        static_cast<std::uint8_t>(color >> 16u),
        255,
    };
}

gapi::Color scenarioObjectNameColor(
    const ScenarioObjectActor& object) {
    const std::uint32_t color = object.nameColor();
    return {
        static_cast<std::uint8_t>(color),
        static_cast<std::uint8_t>(color >> 8u),
        static_cast<std::uint8_t>(color >> 16u),
        255,
    };
}

void drawHoveredScenarioObjectLabel(
    gapi::Backend& renderer,
    const WorldScene& world,
    const gapi::NjpImage* font,
    std::int32_t camera_x,
    std::int32_t camera_y) {
    if (!font) {
        return;
    }
    const ScenarioObjectActor* object =
        findScenarioObject(
            world, world.hoveredScenarioObjectId());
    if (!object || object->name().empty()) {
        return;
    }
    const ScreenPosition anchor =
        calculateRealPosition(object->position());
    const std::int32_t center_x = anchor.x - camera_x;
    const std::int32_t label_y =
        anchor.y - camera_y - object->labelHeight();
    const std::int32_t half_width =
        bitmapTextPixelWidth(object->name(), 6) / 2;
    renderer.drawRectangle({
        center_x - half_width - 4,
        label_y - 2,
        half_width * 2 + 5,
        15,
        {0, 0, 0, 255},
        1000,
        500,
    });
    renderer.drawText(
        *font,
        object->name(),
        {
            center_x - half_width + 1,
            label_y + 1,
            {0, 0, 0, 255},
        });
    renderer.drawText(
        *font,
        object->name(),
        {
            center_x - half_width,
            label_y,
            scenarioObjectNameColor(*object),
        });
}

void drawHoveredNpcLabel(
    gapi::Backend& renderer,
    const WorldScene& world,
    const gapi::NjpImage* font,
    std::int32_t camera_x,
    std::int32_t camera_y,
    double interpolation) {
    if (!font) {
        return;
    }
    const NpcActor* npc =
        findNpc(world, world.hoveredNpcId());
    if (!npc || npc->name().empty()) {
        return;
    }
    const ScreenPosition anchor =
        calculateRealPosition(npc->renderPosition(interpolation));
    const std::int32_t center_x = anchor.x - camera_x;
    const std::int32_t label_y =
        anchor.y - camera_y - npc->labelHeight();
    const std::int32_t half_width =
        bitmapTextPixelWidth(npc->name(), 6) / 2;
    renderer.drawRectangle({
        center_x - half_width - 4,
        label_y - 2,
        half_width * 2 + 5,
        15,
        {0, 0, 0, 255},
        1000,
        500,
    });
    renderer.drawText(
        *font,
        npc->name(),
        {
            center_x - half_width + 1,
            label_y + 1,
            {0, 0, 0, 255},
        });
    renderer.drawText(
        *font,
        npc->name(),
        {
            center_x - half_width,
            label_y,
            npcNameColor(*npc),
        });
}

void drawHoveredEnemyLabel(
    gapi::Backend& renderer,
    const WorldScene& world,
    const gapi::NjpImage* font,
    std::int32_t camera_x,
    std::int32_t camera_y) {
    if (!font) {
        return;
    }
    const EnemyActor* enemy =
        findEnemy(world, world.hoveredEnemyId());
    if (!enemy || enemy->name().empty()) {
        return;
    }
    const ScreenPosition anchor =
        calculateRealPosition(enemy->position());
    const std::int32_t center_x = anchor.x - camera_x;
    const std::int32_t label_y =
        anchor.y - camera_y - enemy->labelHeight();
    const std::int32_t half_width =
        bitmapTextPixelWidth(enemy->name(), 6) / 2;
    renderer.drawRectangle({
        center_x - half_width - 4,
        label_y - 2,
        half_width * 2 + 5,
        15,
        {0, 0, 0, 255},
        1000,
        500,
    });
    renderer.drawText(
        *font,
        enemy->name(),
        {
            center_x - half_width + 1,
            label_y + 1,
            {0, 0, 0, 255},
        });
    renderer.drawText(
        *font,
        enemy->name(),
        {
            center_x - half_width,
            label_y,
            enemyNameColor(*enemy),
        });
}

void drawHoveredGroundItemLabel(
    gapi::Backend& renderer,
    const WorldScene& world,
    const gapi::NjpImage* font,
    std::int32_t camera_x,
    std::int32_t camera_y) {
    if (!font) {
        return;
    }
    const GroundItem* item =
        findGroundItem(
            world, world.hoveredGroundItemId());
    if (!item) {
        return;
    }
    const ItemDefinition* definition =
        world.itemDatabase().find(
            item->category, item->definition_id);
    if (!definition) {
        return;
    }
    const std::string label =
        item->category == 4 &&
                item->definition_id == 0
            ? std::to_string(item->quantity) + " Gold"
            : definition->name;
    if (label.empty()) {
        return;
    }

    const ScreenPosition anchor =
        calculateRealPosition(item->position);
    const std::int32_t center_x =
        anchor.x - camera_x + 2;
    const std::int32_t label_y =
        anchor.y - camera_y - 24;
    const std::int32_t half_width =
        bitmapTextPixelWidth(label, 6) / 2;
    renderer.drawRectangle({
        center_x - half_width - 4,
        label_y - 2,
        half_width * 2 + 5,
        15,
        {0, 0, 0, 255},
        1000,
        500,
    });
    renderer.drawText(
        *font,
        label,
        {
            center_x - half_width + 1,
            label_y + 1,
            {0, 0, 0, 255},
        });
    renderer.drawText(
        *font,
        label,
        {
            center_x - half_width,
            label_y,
            {224, 224, 224, 255},
        });
}

void drawPointerRange(
    gapi::Backend& renderer,
    const WorldScene& world) {
    const WorldPointerConfiguration& configuration =
        world.pointerConfiguration();
    if (!world.pointerActive() ||
        !configuration.range_enabled ||
        world.conversationActive() ||
        world.pointerScreenY() >= 408) {
        return;
    }
    const std::int32_t half_size =
        worldPointerHalfSize(configuration);
    if (half_size <= 0) {
        return;
    }

    gapi::Color color{255, 255, 255, 255};
    bool has_target = false;
    if (world.hoveredGroundItemId() >= 0) {
        color = {224, 224, 0, 255};
        has_target = true;
    } else if (
        world.hoveredScenarioObjectId() >= 0 ||
        world.hoveredNpcId() >= 0 ||
        world.hoveredEnemyId() >= 0) {
        has_target = true;
    }
    const std::int32_t opacity =
        has_target ? 300 : 100;
    const std::int32_t left =
        world.pointerScreenX() - half_size;
    const std::int32_t top =
        world.pointerScreenY() - half_size;
    const std::int32_t length = half_size * 2 + 1;
    renderer.drawRectangle({
        left, top, length, 1, color, 1000, opacity,
    });
    renderer.drawRectangle({
        left, top, 1, length, color, 1000, opacity,
    });
    renderer.drawRectangle({
        left + length - 1,
        top,
        1,
        length,
        color,
        1000,
        opacity,
    });
    renderer.drawRectangle({
        left,
        top + length - 1,
        length,
        1,
        color,
        1000,
        opacity,
    });
}

void drawConversation(
    gapi::Backend& renderer,
    const WorldScene& world,
    const gapi::NjpImage* font,
    std::int32_t camera_x,
    std::int32_t camera_y,
    double interpolation) {
    if (!font) {
        return;
    }
    ConversationLayout layout;
    if (!buildConversationLayout(
            world,
            *font,
            camera_x,
            camera_y,
            interpolation,
            layout)) {
        return;
    }
    const std::int32_t frame_x = layout.x - 9;
    const std::int32_t frame_y = layout.y - 9;
    const std::int32_t frame_width = layout.width + 18;
    const std::int32_t frame_height = layout.height + 18;

    // Hukidasi patterns 0-3 are the four 9x9 rounded corners. Their edge
    // pixels continue as black, black, 160, 224, then the 248 interior.
    // Keep the fills out of each transparent outer corner so the world
    // remains visible around the curve.
    renderer.drawRectangle({
        frame_x + 4,
        frame_y + 4,
        frame_width - 8,
        frame_height - 8,
        {255, 255, 255, 255},
    });
    renderer.drawRectangle({
        frame_x + 9,
        frame_y,
        frame_width - 18,
        2,
        {0, 0, 0, 255},
    });
    renderer.drawRectangle({
        frame_x + 9,
        frame_y + 2,
        frame_width - 18,
        1,
        {160, 160, 160, 255},
    });
    renderer.drawRectangle({
        frame_x + 9,
        frame_y + 3,
        frame_width - 18,
        1,
        {224, 224, 224, 255},
    });
    renderer.drawRectangle({
        frame_x + 9,
        frame_y + frame_height - 2,
        frame_width - 18,
        2,
        {0, 0, 0, 255},
    });
    renderer.drawRectangle({
        frame_x + 9,
        frame_y + frame_height - 3,
        frame_width - 18,
        1,
        {160, 160, 160, 255},
    });
    renderer.drawRectangle({
        frame_x + 9,
        frame_y + frame_height - 4,
        frame_width - 18,
        1,
        {224, 224, 224, 255},
    });
    renderer.drawRectangle({
        frame_x,
        frame_y + 9,
        2,
        frame_height - 18,
        {0, 0, 0, 255},
    });
    renderer.drawRectangle({
        frame_x + 2,
        frame_y + 9,
        1,
        frame_height - 18,
        {160, 160, 160, 255},
    });
    renderer.drawRectangle({
        frame_x + 3,
        frame_y + 9,
        1,
        frame_height - 18,
        {224, 224, 224, 255},
    });
    renderer.drawRectangle({
        frame_x + frame_width - 2,
        frame_y + 9,
        2,
        frame_height - 18,
        {0, 0, 0, 255},
    });
    renderer.drawRectangle({
        frame_x + frame_width - 3,
        frame_y + 9,
        1,
        frame_height - 18,
        {160, 160, 160, 255},
    });
    renderer.drawRectangle({
        frame_x + frame_width - 4,
        frame_y + 9,
        1,
        frame_height - 18,
        {224, 224, 224, 255},
    });
    const gapi::NjpImage& frame = world.speechPatterns();
    if (frame.patterns().size() >= 5) {
        renderer.drawPattern(
            frame, 0, {frame_x, frame_y});
        renderer.drawPattern(
            frame,
            2,
            {frame_x + frame_width - 9, frame_y});
        renderer.drawPattern(
            frame,
            1,
            {frame_x, frame_y + frame_height - 9});
        renderer.drawPattern(
            frame,
            3,
            {frame_x + frame_width - 9,
             frame_y + frame_height - 9});
        renderer.drawPattern(
            frame,
            4,
            {layout.x + layout.width / 2 - 5,
             layout.y + layout.height + 5});
    }
    renderer.drawText(
        *font,
        layout.text.text,
        {
            layout.x + 4,
            layout.y + 4,
            {0, 0, 0, 255},
        });
    for (const ConversationChoiceSpan& choice :
         layout.text.choices) {
        renderer.drawText(
            *font,
            layout.text.text.substr(
                choice.byte_offset,
                choice.byte_length),
            {
                layout.x + 4 +
                    choice.column * layout.cell_width,
                layout.y + 4 +
                    choice.line * layout.cell_height,
                choice.index ==
                        world.conversationSelectedOption()
                    ? gapi::Color{255, 0, 0, 255}
                    : gapi::Color{96, 96, 96, 255},
            });
    }
}

}  // namespace

void renderGameplayOverlay(
    gapi::Backend& renderer,
    const WorldScene& world,
    const gapi::NjpImage* font,
    std::int32_t camera_x,
    std::int32_t camera_y,
    double interpolation) {
    drawHoveredScenarioObjectLabel(
        renderer,
        world,
        font,
        camera_x,
        camera_y);
    drawHoveredNpcLabel(
        renderer,
        world,
        font,
        camera_x,
        camera_y,
        interpolation);
    drawHoveredEnemyLabel(
        renderer,
        world,
        font,
        camera_x,
        camera_y);
    drawHoveredGroundItemLabel(
        renderer,
        world,
        font,
        camera_x,
        camera_y);
    drawConversation(
        renderer,
        world,
        font,
        camera_x,
        camera_y,
        interpolation);
    drawPointerRange(renderer, world);
}

}  // namespace osf
