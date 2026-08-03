#include "retail_save_world_state.hpp"

#include "retail_save_extension.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>

namespace osf {
namespace {

constexpr std::size_t kSerializedSize = 12u;

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

bool readI32(
    const std::vector<std::uint8_t>& payload,
    std::size_t offset,
    std::int32_t& value) {
    if (offset > payload.size() || payload.size() - offset < 4u) {
        return false;
    }
    value = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(payload[offset]) |
        (static_cast<std::uint32_t>(payload[offset + 1u]) << 8u) |
        (static_cast<std::uint32_t>(payload[offset + 2u]) << 16u) |
        (static_cast<std::uint32_t>(payload[offset + 3u]) << 24u));
    return true;
}

void writeI32(
    std::vector<std::uint8_t>& payload,
    std::size_t offset,
    std::int32_t value) {
    const std::uint32_t data = static_cast<std::uint32_t>(value);
    payload[offset] = static_cast<std::uint8_t>(data);
    payload[offset + 1u] = static_cast<std::uint8_t>(data >> 8u);
    payload[offset + 2u] = static_cast<std::uint8_t>(data >> 16u);
    payload[offset + 3u] = static_cast<std::uint8_t>(data >> 24u);
}

}  // namespace

bool restoreRetailWorldState(
    const std::vector<std::uint8_t>& payload,
    std::size_t mine_end,
    RetailSaveWorldState& state,
    std::size_t* serialized_end,
    std::string* error) {
    const RetailSavePortableExtension extension =
        inspectRetailSavePortableExtension(payload);
    const std::size_t suffix_end =
        extension.present ? extension.start : payload.size();
    if (mine_end > suffix_end) {
        setError(
            error,
            "The retail world-state stream begins outside the save payload.");
        return false;
    }
    if (mine_end == suffix_end) {
        if (extension.present) {
            state.running = extension.running;
        }
        if (serialized_end) {
            *serialized_end = mine_end;
        }
        if (error) {
            error->clear();
        }
        return true;
    }
    RetailSaveWorldState restored = state;
    std::int32_t running = 0;
    if (suffix_end - mine_end < kSerializedSize ||
        !readI32(payload, mine_end, running) ||
        !readI32(payload, mine_end + 4u, restored.scenario_id) ||
        !readI32(payload, mine_end + 8u, restored.entry_value)) {
        setError(error, "The retail world-state stream is truncated.");
        return false;
    }
    restored.running = running != 0;
    state = restored;
    if (serialized_end) {
        *serialized_end = mine_end + kSerializedSize;
    }
    if (error) {
        error->clear();
    }
    return true;
}

bool replaceRetailWorldState(
    std::vector<std::uint8_t>& payload,
    std::size_t mine_end,
    const RetailSaveWorldState& state,
    std::size_t* serialized_end,
    std::string* error) {
    const RetailSavePortableExtension extension =
        inspectRetailSavePortableExtension(payload);
    const std::size_t suffix_end =
        extension.present ? extension.start : payload.size();
    if (mine_end > suffix_end) {
        setError(
            error,
            "The retail world-state stream begins outside the save payload.");
        return false;
    }
    if (mine_end == suffix_end) {
        payload.insert(
            payload.begin() + static_cast<std::ptrdiff_t>(mine_end),
            kSerializedSize,
            0);
    } else if (suffix_end - mine_end < kSerializedSize) {
        setError(
            error,
            "The existing retail world-state stream is truncated.");
        return false;
    }
    writeI32(payload, mine_end, state.running ? 1 : 0);
    writeI32(payload, mine_end + 4u, state.scenario_id);
    writeI32(payload, mine_end + 8u, state.entry_value);
    const RetailSavePortableExtension updated =
        inspectRetailSavePortableExtension(payload);
    if (!updated.additional_state.empty()) {
        replaceRetailSavePortableExtensionState(
            payload,
            state.running,
            updated.mine_count,
            updated.additional_state);
    } else if (updated.present) {
        replaceRetailSavePortableExtension(
            payload,
            state.running,
            updated.mine_count,
            updated.has_mine_count);
    }
    if (serialized_end) {
        *serialized_end = mine_end + kSerializedSize;
    }
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace osf
