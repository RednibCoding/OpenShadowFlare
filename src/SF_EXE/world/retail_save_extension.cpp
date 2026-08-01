#include "retail_save_extension.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace osf {
namespace {

constexpr std::array<std::uint8_t, 8> kSignature{{
    'O', 'S', 'F', 'S', 'T', '0', '1', '\0',
}};
constexpr std::uint32_t kLegacySize = 20;
constexpr std::uint32_t kMineSize = 24;
constexpr std::uint32_t kCurrentVersion = 3;
constexpr std::uint32_t kVariableVersion = 4;
constexpr std::uint32_t kVariableMinimumSize = 28;

std::uint32_t readU32(
    const std::vector<std::uint8_t>& payload,
    std::size_t offset) {
    return static_cast<std::uint32_t>(payload[offset]) |
           (static_cast<std::uint32_t>(payload[offset + 1]) << 8u) |
           (static_cast<std::uint32_t>(payload[offset + 2]) << 16u) |
           (static_cast<std::uint32_t>(payload[offset + 3]) << 24u);
}

void appendU32(
    std::vector<std::uint8_t>& payload,
    std::uint32_t value) {
    payload.push_back(static_cast<std::uint8_t>(value));
    payload.push_back(static_cast<std::uint8_t>(value >> 8u));
    payload.push_back(static_cast<std::uint8_t>(value >> 16u));
    payload.push_back(static_cast<std::uint8_t>(value >> 24u));
}

RetailSavePortableExtension inspectAtSize(
    const std::vector<std::uint8_t>& payload,
    std::uint32_t size) {
    RetailSavePortableExtension result;
    if (payload.size() < size) {
        return result;
    }
    const std::size_t start = payload.size() - size;
    if (!std::equal(
            kSignature.begin(),
            kSignature.end(),
            payload.begin() + static_cast<std::ptrdiff_t>(start)) ||
        readU32(payload, start + 8u) != size) {
        return result;
    }
    const std::uint32_t version = readU32(payload, start + 12u);
    if ((size == kLegacySize &&
         version != 1u && version != 2u) ||
        (size == kMineSize && version != kCurrentVersion)) {
        return result;
    }
    result.present = true;
    result.start = start;
    result.size = size;
    result.version = version;
    result.running = readU32(payload, start + 16u) != 0;
    if (size == kMineSize) {
        result.has_mine_count = true;
        result.mine_count = static_cast<std::int32_t>(
            readU32(payload, start + 20u));
    }
    return result;
}

RetailSavePortableExtension inspectVariable(
    const std::vector<std::uint8_t>& payload) {
    RetailSavePortableExtension result;
    if (payload.size() < kVariableMinimumSize) {
        return result;
    }
    const std::uint32_t size =
        readU32(payload, payload.size() - 4u);
    if (size < kVariableMinimumSize ||
        payload.size() < size) {
        return result;
    }
    const std::size_t start = payload.size() - size;
    if (!std::equal(
            kSignature.begin(),
            kSignature.end(),
            payload.begin() + static_cast<std::ptrdiff_t>(start)) ||
        readU32(payload, start + 8u) != size ||
        readU32(payload, start + 12u) != kVariableVersion) {
        return result;
    }
    result.present = true;
    result.start = start;
    result.size = size;
    result.version = kVariableVersion;
    result.running = readU32(payload, start + 16u) != 0;
    result.has_mine_count = true;
    result.mine_count = static_cast<std::int32_t>(
        readU32(payload, start + 20u));
    result.additional_state.assign(
        payload.begin() + static_cast<std::ptrdiff_t>(start + 24u),
        payload.end() - 4);
    return result;
}

}  // namespace

RetailSavePortableExtension inspectRetailSavePortableExtension(
    const std::vector<std::uint8_t>& payload) {
    RetailSavePortableExtension variable =
        inspectVariable(payload);
    if (variable.present) {
        return variable;
    }
    RetailSavePortableExtension result =
        inspectAtSize(payload, kMineSize);
    return result.present
        ? result
        : inspectAtSize(payload, kLegacySize);
}

void replaceRetailSavePortableExtension(
    std::vector<std::uint8_t>& payload,
    bool running,
    std::int32_t mine_count,
    bool include_mine_count) {
    const RetailSavePortableExtension old =
        inspectRetailSavePortableExtension(payload);
    if (!old.additional_state.empty()) {
        replaceRetailSavePortableExtensionState(
            payload,
            running,
            mine_count,
            old.additional_state);
        return;
    }
    if (old.present) {
        payload.resize(old.start);
    }
    payload.insert(payload.end(), kSignature.begin(), kSignature.end());
    appendU32(payload, include_mine_count ? kMineSize : kLegacySize);
    appendU32(payload, include_mine_count ? kCurrentVersion : 2u);
    appendU32(payload, running ? 1u : 0u);
    if (include_mine_count) {
        appendU32(payload, static_cast<std::uint32_t>(mine_count));
    }
}

void replaceRetailSavePortableExtensionState(
    std::vector<std::uint8_t>& payload,
    bool running,
    std::int32_t mine_count,
    const std::vector<std::uint8_t>& additional_state) {
    const RetailSavePortableExtension old =
        inspectRetailSavePortableExtension(payload);
    if (old.present) {
        payload.resize(old.start);
    }
    const std::uint64_t full_size =
        static_cast<std::uint64_t>(kVariableMinimumSize) +
        additional_state.size();
    if (full_size > std::numeric_limits<std::uint32_t>::max()) {
        return;
    }
    const std::uint32_t size =
        static_cast<std::uint32_t>(full_size);
    payload.insert(payload.end(), kSignature.begin(), kSignature.end());
    appendU32(payload, size);
    appendU32(payload, kVariableVersion);
    appendU32(payload, running ? 1u : 0u);
    appendU32(payload, static_cast<std::uint32_t>(mine_count));
    payload.insert(
        payload.end(), additional_state.begin(), additional_state.end());
    appendU32(payload, size);
}

}  // namespace osf
