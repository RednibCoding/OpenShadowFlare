#include "game_config.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <istream>
#include <ostream>

namespace osf {
namespace {

bool readInt32(std::istream& input, std::int32_t& value) {
    std::array<unsigned char, 4> bytes{};
    if (!input.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()))) {
        return false;
    }

    const std::uint32_t raw =
        static_cast<std::uint32_t>(bytes[0]) |
        (static_cast<std::uint32_t>(bytes[1]) << 8U) |
        (static_cast<std::uint32_t>(bytes[2]) << 16U) |
        (static_cast<std::uint32_t>(bytes[3]) << 24U);
    value = static_cast<std::int32_t>(raw);
    return true;
}

bool writeInt32(std::ostream& output, std::int32_t value) {
    const auto raw = static_cast<std::uint32_t>(value);
    const std::array<unsigned char, 4> bytes{{
        static_cast<unsigned char>(raw & 0xffU),
        static_cast<unsigned char>((raw >> 8U) & 0xffU),
        static_cast<unsigned char>((raw >> 16U) & 0xffU),
        static_cast<unsigned char>((raw >> 24U) & 0xffU),
    }};
    return static_cast<bool>(output.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())));
}

bool readRetailBoolean(std::istream& input, bool& destination) {
    std::int32_t value = 0;
    if (!readInt32(input, value)) {
        return false;
    }
    // Invalid values are normalized to one by the retail loader.
    destination = value != 0;
    return true;
}

bool isPriorityPermutation(
    const std::array<std::int32_t, 5>& priority) {
    std::array<bool, 5> present{};
    for (const std::int32_t value : priority) {
        if (value < 0 || value > 4) {
            return false;
        }
        present[static_cast<std::size_t>(value)] = true;
    }
    return std::all_of(
        present.begin(), present.end(), [](bool value) { return value; });
}

std::int32_t asInt(bool value) {
    return value ? 1 : 0;
}

}  // namespace

bool loadGameConfig(std::istream& input, GameConfig& config) {
    std::int32_t value = 0;

    if (!readInt32(input, value)) {
        return false;
    }
    config.windowed_at_start = value != 0;

    if (!readRetailBoolean(input, config.semi_transparent_shadow) ||
        !readRetailBoolean(input, config.semi_transparent_objects) ||
        !readRetailBoolean(input, config.display_darkness) ||
        !readRetailBoolean(input, config.unknown_48d528)) {
        return false;
    }

    // Retail reads this field but unconditionally enables Attack While
    // Moving rather than copying or validating the stored value.
    if (!readInt32(input, value)) {
        return false;
    }
    config.attack_while_moving = true;

    if (!readRetailBoolean(input, config.save_image_at_game_end)) {
        return false;
    }

    if (!readInt32(input, value)) {
        return false;
    }
    config.click_range = value < 0 || value > 4 ? 2 : value;

    if (!readRetailBoolean(input, config.click_range_enabled)) {
        return false;
    }

    std::array<std::int32_t, 5> priority{};
    for (std::int32_t& item : priority) {
        if (!readInt32(input, item)) {
            return false;
        }
    }
    if (!isPriorityPermutation(priority)) {
        return false;
    }
    config.click_priority = priority;

    if (!readInt32(input, config.effect_volume)) {
        return false;
    }
    config.effect_volume =
        std::max<std::int32_t>(-10000, std::min<std::int32_t>(
            0, config.effect_volume));

    if (!readInt32(input, config.bgm_volume)) {
        return false;
    }
    config.bgm_volume =
        std::max<std::int32_t>(-10000, std::min<std::int32_t>(
            0, config.bgm_volume));

    return true;
}

bool loadGameConfigFile(const std::string& path, GameConfig& config) {
    std::ifstream input(path, std::ios::binary);
    return input.is_open() && loadGameConfig(input, config);
}

bool saveGameConfig(std::ostream& output, const GameConfig& config) {
    if (!writeInt32(output, asInt(config.windowed_at_start)) ||
        !writeInt32(output, asInt(config.semi_transparent_shadow)) ||
        !writeInt32(output, asInt(config.semi_transparent_objects)) ||
        !writeInt32(output, asInt(config.display_darkness)) ||
        !writeInt32(output, asInt(config.unknown_48d528)) ||
        !writeInt32(output, asInt(config.attack_while_moving)) ||
        !writeInt32(output, asInt(config.save_image_at_game_end)) ||
        !writeInt32(output, config.click_range) ||
        !writeInt32(output, asInt(config.click_range_enabled))) {
        return false;
    }
    for (const std::int32_t item : config.click_priority) {
        if (!writeInt32(output, item)) {
            return false;
        }
    }
    return writeInt32(output, config.effect_volume) &&
           writeInt32(output, config.bgm_volume);
}

bool saveGameConfigFile(const std::string& path, const GameConfig& config) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    return output.is_open() && saveGameConfig(output, config);
}

}  // namespace osf
