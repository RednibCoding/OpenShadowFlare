#include "retail_save_magic.hpp"

#include "player_magic.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace osf {
namespace {

constexpr std::array<std::uint8_t, 8> kExtensionSignature{{
    'O', 'S', 'F', 'S', 'T', '0', '1', '\0',
}};
constexpr std::size_t kExtensionSize = 20;

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

bool readI32(
    const std::vector<std::uint8_t>& payload,
    std::size_t& offset,
    std::int32_t& value) {
    if (offset > payload.size() ||
        payload.size() - offset < 4) {
        return false;
    }
    const std::uint32_t data =
        static_cast<std::uint32_t>(payload[offset]) |
        (static_cast<std::uint32_t>(payload[offset + 1]) << 8u) |
        (static_cast<std::uint32_t>(payload[offset + 2]) << 16u) |
        (static_cast<std::uint32_t>(payload[offset + 3]) << 24u);
    value = static_cast<std::int32_t>(data);
    offset += 4;
    return true;
}

void appendI32(
    std::vector<std::uint8_t>& payload,
    std::int32_t value) {
    const std::uint32_t data =
        static_cast<std::uint32_t>(value);
    payload.push_back(static_cast<std::uint8_t>(data));
    payload.push_back(static_cast<std::uint8_t>(data >> 8u));
    payload.push_back(static_cast<std::uint8_t>(data >> 16u));
    payload.push_back(static_cast<std::uint8_t>(data >> 24u));
}

bool beginsPortableExtension(
    const std::vector<std::uint8_t>& payload,
    std::size_t offset) {
    return offset <= payload.size() &&
           payload.size() - offset == kExtensionSize &&
           std::equal(
               kExtensionSignature.begin(),
               kExtensionSignature.end(),
               payload.begin() +
                   static_cast<std::ptrdiff_t>(offset));
}

bool parseMagic(
    const std::vector<std::uint8_t>& payload,
    std::size_t progress_end,
    PlayerMagicState* state,
    std::size_t& magic_end,
    std::string* error) {
    if (progress_end > payload.size()) {
        setError(
            error,
            "The retail magic stream begins outside the save payload.");
        return false;
    }
    if (progress_end == payload.size() ||
        beginsPortableExtension(payload, progress_end)) {
        magic_end = progress_end;
        return true;
    }

    std::size_t offset = progress_end;
    std::int32_t count = 0;
    if (!readI32(payload, offset, count) ||
        count !=
            static_cast<std::int32_t>(
                PlayerMagic::spell_count)) {
        setError(
            error,
            "The retail magic stream has an invalid spell count.");
        return false;
    }

    PlayerMagicState restored;
    for (std::int32_t& value : restored.availability) {
        if (!readI32(payload, offset, value)) {
            setError(
                error,
                "The retail spell-availability stream is truncated.");
            return false;
        }
    }
    for (std::int32_t& value : restored.levels) {
        if (!readI32(payload, offset, value)) {
            setError(
                error,
                "The retail spell-level stream is truncated.");
            return false;
        }
    }
    for (std::int32_t& value : restored.experience) {
        if (!readI32(payload, offset, value)) {
            setError(
                error,
                "The retail spell-experience stream is truncated.");
            return false;
        }
    }
    for (std::int32_t& value : restored.bar_slots) {
        if (!readI32(payload, offset, value)) {
            setError(
                error,
                "The retail magic-bar stream is truncated.");
            return false;
        }
    }
    if (state) {
        *state = restored;
    }
    magic_end = offset;
    return true;
}

}  // namespace

bool restoreRetailMagic(
    const std::vector<std::uint8_t>& payload,
    std::size_t progress_end,
    PlayerMagic& magic,
    std::size_t* serialized_end,
    std::string* error) {
    PlayerMagicState restored = magic.state();
    std::size_t magic_end = progress_end;
    if (!parseMagic(
            payload,
            progress_end,
            &restored,
            magic_end,
            error)) {
        return false;
    }
    magic.restore(restored);
    if (serialized_end) {
        *serialized_end = magic_end;
    }
    if (error) {
        error->clear();
    }
    return true;
}

bool replaceRetailMagic(
    std::vector<std::uint8_t>& payload,
    std::size_t progress_end,
    const PlayerMagic& magic,
    std::size_t* serialized_end,
    std::string* error) {
    std::size_t old_magic_end = progress_end;
    if (!parseMagic(
            payload,
            progress_end,
            nullptr,
            old_magic_end,
            error)) {
        return false;
    }

    std::vector<std::uint8_t> replacement;
    replacement.reserve(
        payload.size() +
        4u +
        PlayerMagic::spell_count * 12u +
        PlayerMagic::bar_slot_count * 4u);
    replacement.insert(
        replacement.end(),
        payload.begin(),
        payload.begin() +
            static_cast<std::ptrdiff_t>(progress_end));
    appendI32(
        replacement,
        static_cast<std::int32_t>(
            PlayerMagic::spell_count));
    const PlayerMagicState& state = magic.state();
    for (std::int32_t value : state.availability) {
        appendI32(replacement, value);
    }
    for (std::int32_t value : state.levels) {
        appendI32(replacement, value);
    }
    for (std::int32_t value : state.experience) {
        appendI32(replacement, value);
    }
    for (std::int32_t value : state.bar_slots) {
        appendI32(replacement, value);
    }
    const std::size_t new_magic_end = replacement.size();
    replacement.insert(
        replacement.end(),
        payload.begin() +
            static_cast<std::ptrdiff_t>(old_magic_end),
        payload.end());
    payload = std::move(replacement);
    if (serialized_end) {
        *serialized_end = new_magic_end;
    }
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace osf
