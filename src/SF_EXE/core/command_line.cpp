#include "command_line.hpp"

#include "game_config.hpp"

#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

bool isRetailShiftJisLeadByte(std::uint8_t value) {
    return (value >= 0x80U && value <= 0x9fU) || value >= 0xe0U;
}

std::uint8_t asciiLower(std::uint8_t value) {
    if (value >= static_cast<std::uint8_t>('A') &&
        value <= static_cast<std::uint8_t>('Z')) {
        return static_cast<std::uint8_t>(
            value + static_cast<std::uint8_t>('a' - 'A'));
    }
    return value;
}

}  // namespace

void applyRetailCommandLine(
    std::string_view command_line,
    GameConfig& config) {
    const std::size_t nul = command_line.find('\0');
    const std::size_t length =
        nul == std::string_view::npos ? command_line.size() : nul;

    if (length < 2) {
        return;
    }

    // Retail stops one byte before the terminator because every recognized
    // switch needs a following byte.
    for (std::size_t index = 0; index < length - 1; ++index) {
        const auto value =
            static_cast<std::uint8_t>(command_line[index]);
        if (isRetailShiftJisLeadByte(value)) {
            ++index;
            continue;
        }
        if (value != static_cast<std::uint8_t>('/')) {
            continue;
        }

        const auto option = asciiLower(
            static_cast<std::uint8_t>(command_line[index + 1]));
        if (option == static_cast<std::uint8_t>('w')) {
            config.windowed_at_start = true;
        } else if (option == static_cast<std::uint8_t>('f')) {
            config.windowed_at_start = false;
        }
    }
}

}  // namespace osf
