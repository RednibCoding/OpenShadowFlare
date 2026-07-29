#include "gameplay_overlay_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace osf {
namespace {

ScreenPosition toScreen(
    std::int32_t world_x,
    std::int32_t world_y) {
    return calculateRealPosition({world_x, world_y});
}

std::string conversationDisplayText(
    const std::string& source) {
    std::string result;
    result.reserve(source.size());
    for (std::size_t index = 0; index < source.size(); ++index) {
        const unsigned char byte =
            static_cast<unsigned char>(source[index]);
        if (byte == '\r') {
            continue;
        }
        result.push_back(static_cast<char>(byte));
    }
    return result;
}

bool shiftJisLead(std::uint8_t value) {
    return (value >= 0x80u && value <= 0x9fu) ||
           value >= 0xe0u;
}

std::int32_t textPixelWidth(
    std::string_view text,
    std::int32_t cell_width) {
    std::int32_t width = 0;
    std::int32_t maximum = 0;
    for (std::size_t index = 0; index < text.size(); ++index) {
        const std::uint8_t byte =
            static_cast<std::uint8_t>(text[index]);
        if (byte == '\r') {
            continue;
        }
        if (byte == '\n') {
            maximum = std::max(maximum, width);
            width = 0;
            continue;
        }
        if (shiftJisLead(byte) && index + 1 < text.size()) {
            ++index;
            width += cell_width * 2;
        } else {
            width += cell_width;
        }
    }
    return std::max(maximum, width);
}

std::int32_t textLineCount(std::string_view text) {
    std::int32_t lines = 0;
    bool content_after_break = false;
    for (char character : text) {
        if (character == '\r') {
            continue;
        }
        if (character == '\n') {
            ++lines;
            content_after_break = false;
        } else {
            content_after_break = true;
        }
    }
    if (content_after_break || lines == 0) {
        ++lines;
    }
    return lines;
}

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
    std::int32_t camera_y) {
    if (!font) {
        return;
    }
    const NpcActor* npc =
        findNpc(world, world.hoveredNpcId());
    if (!npc || npc->name().empty()) {
        return;
    }
    const ScreenPosition anchor =
        toScreen(npc->position().x, npc->position().y);
    const std::int32_t center_x = anchor.x - camera_x;
    const std::int32_t label_y =
        anchor.y - camera_y - npc->labelHeight();
    const std::int32_t half_width =
        textPixelWidth(npc->name(), 6) / 2;
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

void drawConversation(
    gapi::Backend& renderer,
    const WorldScene& world,
    const gapi::NjpImage* font,
    std::int32_t camera_x,
    std::int32_t camera_y) {
    if (!world.conversationActive() || !font ||
        font->patterns().empty()) {
        return;
    }
    const NpcActor* actor =
        findNpc(world, world.conversationActorId());
    if (!actor) {
        return;
    }
    const gapi::NjpPattern& font_pattern =
        font->patterns().front();
    const std::int32_t cell_width =
        font_pattern.width / 16;
    const std::int32_t cell_height =
        font_pattern.height / 16;
    if (cell_width <= 0 || cell_height <= 0) {
        return;
    }
    const std::string text =
        conversationDisplayText(world.conversationText());
    const std::int32_t width =
        textPixelWidth(text, cell_width) + 8;
    const std::int32_t height =
        textLineCount(text) * cell_height + 8;
    const ScreenPosition projected =
        toScreen(actor->position().x, actor->position().y);
    const std::int32_t anchor_x =
        projected.x - camera_x;
    const std::int32_t anchor_y =
        projected.y - camera_y - actor->labelHeight();
    const std::int32_t x =
        anchor_x + 12 - width / 2;
    const std::int32_t y =
        anchor_y - 16 - height;
    const std::int32_t frame_x = x - 9;
    const std::int32_t frame_y = y - 9;
    const std::int32_t frame_width = width + 18;
    const std::int32_t frame_height = height + 18;

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
            {x + width / 2 - 5, y + height + 5});
    }
    renderer.drawText(
        *font,
        text,
        {
            x + 4,
            y + 4,
            {0, 0, 0, 255},
        });
}

}  // namespace

void renderGameplayOverlay(
    gapi::Backend& renderer,
    const WorldScene& world,
    const gapi::NjpImage* font,
    std::int32_t camera_x,
    std::int32_t camera_y) {
    drawHoveredNpcLabel(
        renderer, world, font, camera_x, camera_y);
    drawConversation(
        renderer, world, font, camera_x, camera_y);
}

}  // namespace osf

