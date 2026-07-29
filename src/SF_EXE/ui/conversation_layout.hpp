#ifndef OPENSHADOWFLARE_CONVERSATION_LAYOUT_HPP
#define OPENSHADOWFLARE_CONVERSATION_LAYOUT_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace osf {

class WorldScene;

namespace gapi {
class NjpImage;
}

struct ConversationChoiceSpan {
    std::int32_t index = -1;
    std::int32_t line = 0;
    std::int32_t column = 0;
    std::int32_t length = 0;
    std::size_t byte_offset = 0;
    std::size_t byte_length = 0;
};

struct ConversationTextLayout {
    std::string text;
    std::vector<ConversationChoiceSpan> choices;
};

struct ConversationLayout {
    ConversationTextLayout text;
    std::int32_t cell_width = 0;
    std::int32_t cell_height = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t x = 0;
    std::int32_t y = 0;
};

ConversationTextLayout layoutConversationText(
    std::string_view source,
    bool choices_enabled);

std::int32_t bitmapTextPixelWidth(
    std::string_view text,
    std::int32_t cell_width);

std::int32_t bitmapTextLineCount(std::string_view text);

bool buildConversationLayout(
    const WorldScene& world,
    const gapi::NjpImage& font,
    std::int32_t camera_x,
    std::int32_t camera_y,
    double interpolation,
    ConversationLayout& layout);

std::int32_t conversationChoiceAtScreenPosition(
    const ConversationLayout& layout,
    std::int32_t screen_x,
    std::int32_t screen_y);

std::int32_t conversationChoiceAtScreenPosition(
    const WorldScene& world,
    const gapi::NjpImage& font,
    std::int32_t camera_x,
    std::int32_t camera_y,
    std::int32_t screen_x,
    std::int32_t screen_y);

}  // namespace osf

#endif
