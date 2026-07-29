#include "gameplay_overlay_renderer.hpp"

#include "conversation_layout.hpp"
#include "gapi/gapi.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace osf {
namespace {

struct ConversationLayout {
    ConversationTextLayout text;
    std::int32_t cell_width = 0;
    std::int32_t cell_height = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t x = 0;
    std::int32_t y = 0;
};

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
    constexpr std::array<std::int32_t, 5> half_sizes{{
        0, 12, 16, 24, 48,
    }};
    if (!world.pointerActive() ||
        !configuration.range_enabled ||
        world.conversationActive() ||
        configuration.range < 0 ||
        static_cast<std::size_t>(configuration.range) >=
            half_sizes.size() ||
        world.pointerScreenY() >= 408) {
        return;
    }
    const std::int32_t half_size =
        half_sizes[static_cast<std::size_t>(
            configuration.range)];
    if (half_size <= 0) {
        return;
    }

    gapi::Color color{255, 255, 255, 255};
    bool has_target = false;
    if (world.hoveredGroundItemId() >= 0) {
        color = {224, 224, 0, 255};
        has_target = true;
    } else if (world.hoveredNpcId() >= 0) {
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

bool buildConversationLayout(
    const WorldScene& world,
    const gapi::NjpImage& font,
    std::int32_t camera_x,
    std::int32_t camera_y,
    double interpolation,
    ConversationLayout& layout) {
    if (!world.conversationActive() ||
        font.patterns().empty()) {
        return false;
    }
    const NpcActor* actor =
        findNpc(world, world.conversationActorId());
    if (!actor) {
        return false;
    }
    const gapi::NjpPattern& font_pattern =
        font.patterns().front();
    layout.cell_width =
        font_pattern.width / 16;
    layout.cell_height =
        font_pattern.height / 16;
    if (layout.cell_width <= 0 || layout.cell_height <= 0) {
        return false;
    }
    layout.text = layoutConversationText(
        world.conversationText(),
        world.conversationRequiresSelection());
    layout.width =
        bitmapTextPixelWidth(
            layout.text.text, layout.cell_width) + 8;
    layout.height =
        bitmapTextLineCount(layout.text.text) *
            layout.cell_height +
        8;
    const ScreenPosition projected =
        calculateRealPosition(actor->renderPosition(interpolation));
    const std::int32_t anchor_x =
        projected.x - camera_x;
    const std::int32_t anchor_y =
        projected.y - camera_y - actor->labelHeight();
    layout.x =
        anchor_x + 12 - layout.width / 2;
    layout.y =
        anchor_y - 16 - layout.height;
    return true;
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
    drawHoveredNpcLabel(
        renderer,
        world,
        font,
        camera_x,
        camera_y,
        interpolation);
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

std::int32_t conversationChoiceAtScreenPosition(
    const WorldScene& world,
    const gapi::NjpImage& font,
    std::int32_t camera_x,
    std::int32_t camera_y,
    std::int32_t screen_x,
    std::int32_t screen_y) {
    ConversationLayout layout;
    if (!world.conversationRequiresSelection() ||
        !buildConversationLayout(
            world,
            font,
            camera_x,
            camera_y,
            1.0,
            layout)) {
        return -1;
    }
    const std::int32_t text_x = layout.x + 4;
    const std::int32_t text_y = layout.y + 4;
    for (const ConversationChoiceSpan& choice :
         layout.text.choices) {
        const std::int32_t left =
            text_x + choice.column * layout.cell_width;
        const std::int32_t top =
            text_y + choice.line * layout.cell_height;
        if (screen_x >= left &&
            screen_x < left + choice.length * layout.cell_width &&
            screen_y >= top &&
            screen_y < top + layout.cell_height) {
            return choice.index;
        }
    }
    return -1;
}

}  // namespace osf
