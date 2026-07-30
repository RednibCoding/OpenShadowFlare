#include "retail_save_progress.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kMaximumFlagCount = 100000;
constexpr std::array<std::uint8_t, 8> kExtensionSignature{{
    'O', 'S', 'F', 'S', 'T', '0', '1', '\0',
}};
constexpr std::uint32_t kExtensionSize = 20;
constexpr std::uint32_t kExtensionVersion = 1;

// The three flag arrays below are retail fields. The executable does not
// serialize its live walk/run word in FUN_0044b580, so that one setting uses
// a versioned tail which the retail reader safely leaves unread.

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

bool readFlagArray(
    const std::vector<std::uint8_t>& payload,
    std::size_t& offset,
    std::vector<std::int32_t>& values) {
    std::int32_t count = 0;
    if (!readI32(payload, offset, count)) {
        return false;
    }
    if (count < 0 || count > kMaximumFlagCount) {
        return false;
    }
    std::vector<std::int32_t> restored(
        static_cast<std::size_t>(count));
    for (std::int32_t& value : restored) {
        if (!readI32(payload, offset, value)) {
            return false;
        }
    }
    values = std::move(restored);
    return true;
}

void appendU32(
    std::vector<std::uint8_t>& payload,
    std::uint32_t value) {
    payload.push_back(static_cast<std::uint8_t>(value));
    payload.push_back(static_cast<std::uint8_t>(value >> 8u));
    payload.push_back(static_cast<std::uint8_t>(value >> 16u));
    payload.push_back(static_cast<std::uint8_t>(value >> 24u));
}

bool appendFlagArray(
    std::vector<std::uint8_t>& payload,
    const std::vector<std::int32_t>& values) {
    if (values.size() >
        static_cast<std::size_t>(
            std::numeric_limits<std::int32_t>::max())) {
        return false;
    }
    appendU32(
        payload,
        static_cast<std::uint32_t>(values.size()));
    for (std::int32_t value : values) {
        appendU32(payload, static_cast<std::uint32_t>(value));
    }
    return true;
}

bool hasPortableExtension(
    const std::vector<std::uint8_t>& payload) {
    if (payload.size() < kExtensionSize) {
        return false;
    }
    const std::size_t start =
        payload.size() - kExtensionSize;
    if (!std::equal(
            kExtensionSignature.begin(),
            kExtensionSignature.end(),
            payload.begin() +
                static_cast<std::ptrdiff_t>(start))) {
        return false;
    }
    std::size_t offset = start + kExtensionSignature.size();
    std::int32_t size = 0;
    std::int32_t version = 0;
    return readI32(payload, offset, size) &&
           readI32(payload, offset, version) &&
           size == static_cast<std::int32_t>(kExtensionSize) &&
           version ==
               static_cast<std::int32_t>(kExtensionVersion);
}

void appendPortableExtension(
    std::vector<std::uint8_t>& payload,
    bool running) {
    payload.insert(
        payload.end(),
        kExtensionSignature.begin(),
        kExtensionSignature.end());
    appendU32(payload, kExtensionSize);
    appendU32(payload, kExtensionVersion);
    appendU32(payload, running ? 1u : 0u);
}

}  // namespace

bool restoreRetailProgress(
    const std::vector<std::uint8_t>& payload,
    std::size_t owned_items_end,
    RetailSaveProgress& progress,
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
    RetailSaveProgress restored = progress;
    if (!readFlagArray(
            payload, offset, restored.scenario_flags)) {
        setError(
            error,
            "The retail scenario-flag stream is truncated.");
        return false;
    }
    if (!readFlagArray(
            payload, offset, restored.transport_flags)) {
        setError(
            error,
            "The retail transport-flag stream is truncated.");
        return false;
    }
    if (!readFlagArray(
            payload, offset, restored.quest_flags)) {
        setError(
            error,
            "The retail quest/conversation-flag stream is truncated.");
        return false;
    }
    if (hasPortableExtension(payload)) {
        const std::size_t start =
            payload.size() - kExtensionSize;
        restored.running = payload[start + 16] != 0;
    }
    progress = std::move(restored);
    if (error) {
        error->clear();
    }
    return true;
}

bool replaceRetailProgress(
    std::vector<std::uint8_t>& payload,
    std::size_t owned_items_end,
    const RetailSaveProgress& progress,
    std::string* error) {
    if (owned_items_end > payload.size()) {
        setError(
            error,
            "The retail progress stream begins outside the save payload.");
        return false;
    }

    std::size_t old_progress_end = owned_items_end;
    if (owned_items_end != payload.size()) {
        std::vector<std::int32_t> ignored;
        if (!readFlagArray(payload, old_progress_end, ignored) ||
            !readFlagArray(payload, old_progress_end, ignored) ||
            !readFlagArray(payload, old_progress_end, ignored)) {
            setError(
                error,
                "The existing retail progress stream is truncated.");
            return false;
        }
    }

    std::size_t suffix_end = payload.size();
    if (hasPortableExtension(payload)) {
        suffix_end -= kExtensionSize;
    }
    if (old_progress_end > suffix_end) {
        setError(
            error,
            "The existing portable save extension overlaps retail state.");
        return false;
    }

    std::vector<std::uint8_t> replacement;
    replacement.reserve(
        payload.size() +
        (progress.scenario_flags.size() +
         progress.transport_flags.size() +
         progress.quest_flags.size()) *
            4u +
        32u);
    replacement.insert(
        replacement.end(),
        payload.begin(),
        payload.begin() +
            static_cast<std::ptrdiff_t>(owned_items_end));
    if (!appendFlagArray(replacement, progress.scenario_flags) ||
        !appendFlagArray(replacement, progress.transport_flags) ||
        !appendFlagArray(replacement, progress.quest_flags)) {
        setError(error, "The retail progress stream is too large.");
        return false;
    }
    replacement.insert(
        replacement.end(),
        payload.begin() +
            static_cast<std::ptrdiff_t>(old_progress_end),
        payload.begin() +
            static_cast<std::ptrdiff_t>(suffix_end));
    appendPortableExtension(replacement, progress.running);
    payload = std::move(replacement);
    if (error) {
        error->clear();
    }
    return true;
}

bool restoreRetailTransportFlags(
    const std::vector<std::uint8_t>& payload,
    std::size_t owned_items_end,
    std::vector<std::int32_t>& flags,
    std::string* error) {
    RetailSaveProgress progress;
    progress.transport_flags = flags;
    if (!restoreRetailProgress(
            payload, owned_items_end, progress, error)) {
        return false;
    }
    if (progress.transport_flags.size() != flags.size()) {
        setError(
            error,
            "The retail transport-flag count does not match Table 40.");
        return false;
    }
    flags = std::move(progress.transport_flags);
    return true;
}

}  // namespace osf
