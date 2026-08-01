#include "retail_save_mines.hpp"

#include "retail_save_extension.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kMaximumArrayCount = 100000;

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

bool mineOffset(
    const std::vector<std::uint8_t>& payload,
    std::size_t magic_end,
    std::size_t suffix_end,
    std::size_t& result) {
    if (magic_end > suffix_end ||
        suffix_end - magic_end < 4u) {
        return false;
    }
    std::size_t offset = magic_end;
    std::int32_t count = 0;
    if (!readI32(payload, offset, count) ||
        count < 0 || count > kMaximumArrayCount) {
        return false;
    }
    offset += 4u;
    const std::size_t array_bytes =
        static_cast<std::size_t>(count) * 8u;
    if (offset > suffix_end ||
        suffix_end - offset < array_bytes + 4u) {
        return false;
    }
    result = offset + array_bytes;
    return true;
}

}  // namespace

bool restoreRetailMineCount(
    const std::vector<std::uint8_t>& payload,
    std::size_t magic_end,
    std::int32_t& mine_count,
    std::size_t* serialized_end,
    std::string* error) {
    const RetailSavePortableExtension extension =
        inspectRetailSavePortableExtension(payload);
    const std::size_t suffix_end =
        extension.present ? extension.start : payload.size();
    if (magic_end > suffix_end) {
        setError(
            error,
            "The retail mine stream begins outside the save payload.");
        return false;
    }
    if (magic_end == suffix_end) {
        if (extension.has_mine_count) {
            mine_count = std::max(extension.mine_count, 0);
        }
        if (serialized_end) {
            *serialized_end = magic_end;
        }
        if (error) {
            error->clear();
        }
        return true;
    }
    std::size_t offset = 0;
    std::int32_t restored = 0;
    if (!mineOffset(payload, magic_end, suffix_end, offset) ||
        !readI32(payload, offset, restored)) {
        setError(
            error,
            "The retail mine-count stream is truncated.");
        return false;
    }
    mine_count = std::max(restored, 0);
    if (serialized_end) {
        *serialized_end = offset + 4u;
    }
    if (error) {
        error->clear();
    }
    return true;
}

bool replaceRetailMineCount(
    std::vector<std::uint8_t>& payload,
    std::size_t magic_end,
    std::int32_t mine_count,
    std::size_t* serialized_end,
    std::string* error) {
    const RetailSavePortableExtension extension =
        inspectRetailSavePortableExtension(payload);
    const std::size_t suffix_end =
        extension.present ? extension.start : payload.size();
    if (magic_end > suffix_end) {
        setError(
            error,
            "The retail mine stream begins outside the save payload.");
        return false;
    }
    mine_count = std::max(mine_count, 0);
    std::size_t end = magic_end;
    if (magic_end != suffix_end) {
        std::size_t offset = 0;
        if (!mineOffset(payload, magic_end, suffix_end, offset)) {
            setError(
                error,
                "The existing retail mine-count stream is truncated.");
            return false;
        }
        writeI32(payload, offset, mine_count);
        end = offset + 4u;
    }
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
