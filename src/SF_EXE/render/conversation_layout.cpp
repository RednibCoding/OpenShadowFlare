#include "conversation_layout.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

bool shiftJisLead(std::uint8_t value) {
    return (value >= 0x80u && value <= 0x9fu) ||
           value >= 0xe0u;
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
                    std::max(column - choice_column, 0),
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

}  // namespace osf
