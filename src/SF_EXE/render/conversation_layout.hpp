#ifndef OPENSHADOWFLARE_CONVERSATION_LAYOUT_HPP
#define OPENSHADOWFLARE_CONVERSATION_LAYOUT_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace osf {

struct ConversationChoiceSpan {
    std::int32_t index = -1;
    std::int32_t line = 0;
    std::int32_t column = 0;
    std::int32_t length = 0;
};

struct ConversationTextLayout {
    std::string text;
    std::vector<ConversationChoiceSpan> choices;
};

ConversationTextLayout layoutConversationText(
    std::string_view source,
    bool choices_enabled);

std::int32_t bitmapTextPixelWidth(
    std::string_view text,
    std::int32_t cell_width);

std::int32_t bitmapTextLineCount(std::string_view text);

}  // namespace osf

#endif
