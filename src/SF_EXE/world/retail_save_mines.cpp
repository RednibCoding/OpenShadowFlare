#include "retail_save_mines.hpp"

#include "retail_save_extension.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace osf {
namespace {

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
        (static_cast<std::uint32_t>(payload[offset + 1]) << 8u) |
        (static_cast<std::uint32_t>(payload[offset + 2]) << 16u) |
        (static_cast<std::uint32_t>(payload[offset + 3]) << 24u));
    return true;
}

void writeI32(
    std::vector<std::uint8_t>& payload,
    std::size_t offset,
    std::int32_t value) {
    const std::uint32_t data = static_cast<std::uint32_t>(value);
    payload[offset] = static_cast<std::uint8_t>(data);
    payload[offset + 1] = static_cast<std::uint8_t>(data >> 8u);
    payload[offset + 2] = static_cast<std::uint8_t>(data >> 16u);
    payload[offset + 3] = static_cast<std::uint8_t>(data >> 24u);
}

}  // namespace

bool restoreRetailMineCount(
    const std::vector<std::uint8_t>& payload,
    std::size_t companion_progress_end,
    std::int32_t& mine_count,
    std::size_t* serialized_end,
    std::string* error) {
    const RetailSavePortableExtension extension =
        inspectRetailSavePortableExtension(payload);
    const std::size_t suffix_end =
        extension.present ? extension.start : payload.size();
    if (companion_progress_end > suffix_end) {
        setError(
            error,
            "The retail mine stream begins outside the save payload.");
        return false;
    }
    if (companion_progress_end == suffix_end) {
        if (extension.has_mine_count) {
            mine_count = std::max<std::int32_t>(extension.mine_count, 0);
        }
        if (serialized_end) {
            *serialized_end = companion_progress_end;
        }
        if (error) {
            error->clear();
        }
        return true;
    }
    std::int32_t restored = 0;
    if (suffix_end - companion_progress_end < 4u ||
        !readI32(
            payload,
            companion_progress_end,
            restored)) {
        setError(
            error,
            "The retail mine-count stream is truncated.");
        return false;
    }
    mine_count = std::max<std::int32_t>(restored, 0);
    if (serialized_end) {
        *serialized_end = companion_progress_end + 4u;
    }
    if (error) {
        error->clear();
    }
    return true;
}

bool replaceRetailMineCount(
    std::vector<std::uint8_t>& payload,
    std::size_t companion_progress_end,
    std::int32_t mine_count,
    std::size_t* serialized_end,
    std::string* error) {
    const RetailSavePortableExtension extension =
        inspectRetailSavePortableExtension(payload);
    const std::size_t suffix_end =
        extension.present ? extension.start : payload.size();
    if (companion_progress_end > suffix_end) {
        setError(
            error,
            "The retail mine stream begins outside the save payload.");
        return false;
    }
    mine_count = std::max<std::int32_t>(mine_count, 0);
    std::size_t end = companion_progress_end + 4u;
    if (companion_progress_end == suffix_end) {
        payload.insert(
            payload.begin() +
                static_cast<std::ptrdiff_t>(companion_progress_end),
            4u,
            0);
    } else if (suffix_end - companion_progress_end < 4u) {
        setError(
            error,
            "The existing retail mine-count stream is truncated.");
        return false;
    }
    writeI32(payload, companion_progress_end, mine_count);
    replaceRetailSavePortableExtension(
        payload,
        extension.present && extension.running,
        mine_count,
        true);
    if (serialized_end) {
        *serialized_end = end;
    }
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace osf
