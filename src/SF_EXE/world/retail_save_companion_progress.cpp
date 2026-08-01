#include "retail_save_companion_progress.hpp"

#include "player_data.hpp"
#include "retail_save_extension.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace osf {
namespace {

constexpr std::int32_t kMaximumCompanionCount = 100000;

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

bool readI32(
    const std::vector<std::uint8_t>& payload,
    std::size_t& offset,
    std::size_t end,
    std::int32_t& value) {
    if (offset > end || end - offset < 4u) {
        return false;
    }
    value = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(payload[offset]) |
        (static_cast<std::uint32_t>(payload[offset + 1]) << 8u) |
        (static_cast<std::uint32_t>(payload[offset + 2]) << 16u) |
        (static_cast<std::uint32_t>(payload[offset + 3]) << 24u));
    offset += 4u;
    return true;
}

void appendI32(
    std::vector<std::uint8_t>& payload,
    std::int32_t value) {
    const std::uint32_t data = static_cast<std::uint32_t>(value);
    payload.push_back(static_cast<std::uint8_t>(data));
    payload.push_back(static_cast<std::uint8_t>(data >> 8u));
    payload.push_back(static_cast<std::uint8_t>(data >> 16u));
    payload.push_back(static_cast<std::uint8_t>(data >> 24u));
}

bool readProgress(
    const std::vector<std::uint8_t>& payload,
    std::size_t begin,
    std::size_t end,
    std::vector<std::int32_t>& levels,
    std::vector<std::int32_t>& experiences,
    std::size_t& serialized_end) {
    std::size_t offset = begin;
    std::int32_t count = 0;
    if (!readI32(payload, offset, end, count) ||
        count <= 0 || count > kMaximumCompanionCount) {
        return false;
    }
    const std::size_t value_count =
        static_cast<std::size_t>(count);
    if (value_count >
        (std::numeric_limits<std::size_t>::max() - 4u) / 8u ||
        offset > end || end - offset < value_count * 8u) {
        return false;
    }
    levels.resize(value_count);
    experiences.resize(value_count);
    for (std::int32_t& level : levels) {
        if (!readI32(payload, offset, end, level)) {
            return false;
        }
    }
    for (std::int32_t& experience : experiences) {
        if (!readI32(payload, offset, end, experience)) {
            return false;
        }
    }
    serialized_end = offset;
    return true;
}

}  // namespace

bool restoreRetailCompanionProgress(
    const std::vector<std::uint8_t>& payload,
    std::size_t magic_end,
    PlayerData& player,
    std::size_t* serialized_end,
    std::string* error) {
    const RetailSavePortableExtension extension =
        inspectRetailSavePortableExtension(payload);
    const std::size_t suffix_end =
        extension.present ? extension.start : payload.size();
    if (magic_end > suffix_end) {
        setError(
            error,
            "The retail companion stream begins outside the save payload.");
        return false;
    }
    if (magic_end == suffix_end) {
        if (serialized_end) {
            *serialized_end = magic_end;
        }
        if (error) {
            error->clear();
        }
        return true;
    }

    std::vector<std::int32_t> levels;
    std::vector<std::int32_t> experiences;
    std::size_t end = magic_end;
    if (!readProgress(
            payload,
            magic_end,
            suffix_end,
            levels,
            experiences,
            end)) {
        setError(
            error,
            "The retail companion-progression stream is truncated.");
        return false;
    }
    if (!player.restoreCompanionProgress(
            std::move(levels),
            std::move(experiences))) {
        setError(
            error,
            "The retail companion-progression count does not match "
            "the player record.");
        return false;
    }
    if (serialized_end) {
        *serialized_end = end;
    }
    if (error) {
        error->clear();
    }
    return true;
}

bool replaceRetailCompanionProgress(
    std::vector<std::uint8_t>& payload,
    std::size_t magic_end,
    const PlayerData& player,
    std::size_t* serialized_end,
    std::string* error) {
    const RetailSavePortableExtension extension =
        inspectRetailSavePortableExtension(payload);
    const std::size_t suffix_end =
        extension.present ? extension.start : payload.size();
    if (magic_end > suffix_end ||
        player.companionCount() == 0 ||
        player.companionLevels().size() !=
            player.companionExperiences().size() ||
        player.companionCount() >
            static_cast<std::size_t>(
                std::numeric_limits<std::int32_t>::max())) {
        setError(
            error,
            "The player companion progression cannot be serialized.");
        return false;
    }

    std::size_t old_end = magic_end;
    if (magic_end != suffix_end) {
        std::vector<std::int32_t> ignored_levels;
        std::vector<std::int32_t> ignored_experiences;
        if (!readProgress(
                payload,
                magic_end,
                suffix_end,
                ignored_levels,
                ignored_experiences,
                old_end)) {
            setError(
                error,
                "The existing retail companion-progression stream is "
                "truncated.");
            return false;
        }
    }

    std::vector<std::uint8_t> replacement;
    replacement.reserve(
        payload.size() - (old_end - magic_end) +
        4u + player.companionCount() * 8u);
    replacement.insert(
        replacement.end(),
        payload.begin(),
        payload.begin() + static_cast<std::ptrdiff_t>(magic_end));
    appendI32(
        replacement,
        static_cast<std::int32_t>(player.companionCount()));
    for (std::int32_t level : player.companionLevels()) {
        appendI32(replacement, level);
    }
    for (std::int32_t experience : player.companionExperiences()) {
        appendI32(replacement, experience);
    }
    const std::size_t new_end = replacement.size();
    replacement.insert(
        replacement.end(),
        payload.begin() + static_cast<std::ptrdiff_t>(old_end),
        payload.end());
    payload = std::move(replacement);
    if (serialized_end) {
        *serialized_end = new_end;
    }
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace osf
