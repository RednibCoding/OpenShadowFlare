#include "retail_save_progress.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kMaximumFlagCount = 100000;

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

bool skipFlagArray(
    const std::vector<std::uint8_t>& payload,
    std::size_t& offset,
    std::int32_t count) {
    if (count < 0 || count > kMaximumFlagCount) {
        return false;
    }
    const std::size_t byte_count =
        static_cast<std::size_t>(count) * 4u;
    if (offset > payload.size() ||
        byte_count > payload.size() - offset) {
        return false;
    }
    offset += byte_count;
    return true;
}

}  // namespace

bool restoreRetailTransportFlags(
    const std::vector<std::uint8_t>& payload,
    std::size_t owned_items_end,
    std::vector<std::int32_t>& flags,
    std::string* error) {
    if (owned_items_end > payload.size()) {
        setError(
            error,
            "The retail progress stream begins outside the save payload.");
        return false;
    }
    if (owned_items_end == payload.size()) {
        if (error) {
            error->clear();
        }
        return true;
    }

    std::size_t offset = owned_items_end;
    std::int32_t mission_flag_count = 0;
    if (!readI32(payload, offset, mission_flag_count) ||
        !skipFlagArray(
            payload, offset, mission_flag_count)) {
        setError(
            error,
            "The retail mission-flag stream is truncated.");
        return false;
    }

    std::int32_t transport_flag_count = 0;
    if (!readI32(payload, offset, transport_flag_count) ||
        transport_flag_count < 0 ||
        static_cast<std::size_t>(transport_flag_count) !=
            flags.size()) {
        setError(
            error,
            "The retail transport-flag count does not match Table 40.");
        return false;
    }
    std::vector<std::int32_t> restored(flags.size());
    for (std::int32_t& flag : restored) {
        if (!readI32(payload, offset, flag)) {
            setError(
                error,
                "The retail transport-flag stream is truncated.");
            return false;
        }
    }
    flags = std::move(restored);
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace osf
