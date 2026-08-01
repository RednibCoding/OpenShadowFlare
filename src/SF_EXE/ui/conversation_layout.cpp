#include "conversation_layout.hpp"

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "world/npc_actor.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

bool shiftJisLead(std::uint8_t value) {
    return (value >= 0x80u && value <= 0x9fu) ||
           value >= 0xe0u;
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

}  // namespace

ConversationTextLayout layoutConversationText(
    std::string_view source,
    bool choices_enabled) {
    ConversationTextLayout result;
    result.text.reserve(source.size());
    std::int32_t line = 0;
    std::int32_t column = 0;
    std::int32_t choice_line = 0;
    std::int32_t choice_column = 0;
    std::size_t choice_byte_offset = 0;
    bool inside_choice = false;
    for (std::size_t index = 0; index < source.size(); ++index) {
        const std::uint8_t byte =
            static_cast<std::uint8_t>(source[index]);
        if (byte == '\r') {
            continue;
        }
        if (choices_enabled && byte == '~') {
            if (inside_choice) {
                result.choices.push_back({
                    static_cast<std::int32_t>(
                        result.choices.size()),
                    choice_line,
                    choice_column,
                    std::max(
                        column - choice_column,
                        std::int32_t{0}),
                    choice_byte_offset,
                    result.text.size() - choice_byte_offset,
                });
            } else {
                choice_line = line;
                choice_column = column;
                choice_byte_offset = result.text.size();
            }
            inside_choice = !inside_choice;
            continue;
        }
        result.text.push_back(static_cast<char>(byte));
        if (byte == '\n') {
            ++line;
            column = 0;
        } else if (
            shiftJisLead(byte) &&
            index + 1 < source.size()) {
            result.text.push_back(source[++index]);
            column += 2;
        } else {
            ++column;
        }
    }
    return result;
}

std::int32_t bitmapTextPixelWidth(
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

std::int32_t bitmapTextLineCount(std::string_view text) {
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
    layout.cell_width = font_pattern.width / 16;
    layout.cell_height = font_pattern.height / 16;
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
    const std::int32_t anchor_x = projected.x - camera_x;
    const std::int32_t anchor_y =
        projected.y - camera_y - actor->labelHeight();
    layout.x = anchor_x + 12 - layout.width / 2;
    layout.y = anchor_y - 16 - layout.height;
    return true;
}

std::int32_t conversationChoiceAtScreenPosition(
    const ConversationLayout& layout,
    std::int32_t screen_x,
    std::int32_t screen_y) {
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
    return conversationChoiceAtScreenPosition(
        layout, screen_x, screen_y);
}

}  // namespace osf
