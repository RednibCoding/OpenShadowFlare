#include "retail_save_giant_warehouse.hpp"

#include "items/player_giant_warehouse.hpp"
#include "retail_save_extension.hpp"
#include "retail_save_items.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace osf {
namespace {

constexpr std::array<std::uint8_t, 8> kPortableSignature{{
    'O', 'S', 'F', 'G', 'W', '0', '1', '\0',
}};
constexpr std::int32_t kPortableVersion = 1;
constexpr std::int32_t kPortableVersionWithAutomaticItems = 2;

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

bool readI32(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::int32_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < 4u) {
        return false;
    }
    value = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
        (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u));
    return true;
}

void appendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value) {
    const std::uint32_t data = static_cast<std::uint32_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(data));
    bytes.push_back(static_cast<std::uint8_t>(data >> 8u));
    bytes.push_back(static_cast<std::uint8_t>(data >> 16u));
    bytes.push_back(static_cast<std::uint8_t>(data >> 24u));
}

bool restorePages(
    const std::vector<std::uint8_t>& bytes,
    std::size_t flags_offset,
    const ItemDatabase& item_database,
    PlayerGiantWarehouse& warehouse,
    std::size_t& end,
    std::string* error) {
    PlayerGiantWarehouse restored;
    restored.initializeNew();
    PlayerGiantWarehouse::EnabledFlags flags{};
    std::size_t offset = flags_offset;
    for (std::size_t page = 0;
         page < PlayerGiantWarehouse::page_count;
         ++page) {
        if (!readI32(bytes, offset, flags[page])) {
            setError(
                error,
                "The Giant Warehouse page table is truncated.");
            return false;
        }
        offset += 4u;
    }
    for (std::size_t page = 0;
         page < PlayerGiantWarehouse::page_count;
         ++page) {
        if (!restoreRetailSpecialItemContainer(
                bytes,
                offset,
                item_database,
                restored.page(page),
                &offset,
                error)) {
            return false;
        }
    }
    restored.restoreEnabledFlags(flags);
    warehouse = std::move(restored);
    end = offset;
    return true;
}

bool appendPages(
    std::vector<std::uint8_t>& bytes,
    const ItemDatabase& item_database,
    const PlayerGiantWarehouse& warehouse,
    std::string* error) {
    for (std::int32_t flag : warehouse.enabledFlags()) {
        appendI32(bytes, flag);
    }
    for (std::size_t page = 0;
         page < PlayerGiantWarehouse::page_count;
         ++page) {
        if (!appendRetailSpecialItemContainer(
                bytes,
                item_database,
                warehouse.page(page),
                error)) {
            return false;
        }
    }
    return true;
}

bool retailHeaderEnd(
    const std::vector<std::uint8_t>& payload,
    std::size_t world_state_end,
    std::size_t suffix_end,
    std::size_t& flags_offset) {
    if (world_state_end > suffix_end ||
        suffix_end - world_state_end < 4u) {
        return false;
    }
    std::int32_t page_count = 0;
    if (!readI32(payload, world_state_end, page_count) ||
        page_count != static_cast<std::int32_t>(
                          PlayerGiantWarehouse::page_count)) {
        return false;
    }
    flags_offset = world_state_end + 4u;
    return true;
}

bool portableHeaderEnd(
    const std::vector<std::uint8_t>& bytes,
    std::size_t& flags_offset,
    std::int32_t& version) {
    if (bytes.size() < kPortableSignature.size() + 8u ||
        !std::equal(
            kPortableSignature.begin(),
            kPortableSignature.end(),
            bytes.begin())) {
        return false;
    }
    std::int32_t page_count = 0;
    if (!readI32(bytes, 8u, version) ||
        !readI32(bytes, 12u, page_count) ||
        (version != kPortableVersion &&
         version != kPortableVersionWithAutomaticItems) ||
        page_count != static_cast<std::int32_t>(
                          PlayerGiantWarehouse::page_count)) {
        return false;
    }
    flags_offset = 16u;
    return true;
}

}  // namespace

bool restoreRetailGiantWarehouse(
    const std::vector<std::uint8_t>& payload,
    std::size_t world_state_end,
    const ItemDatabase& item_database,
    PlayerGiantWarehouse& warehouse,
    std::size_t* serialized_end,
    std::string* error) {
    const RetailSavePortableExtension extension =
        inspectRetailSavePortableExtension(payload);
    const std::size_t suffix_end =
        extension.present ? extension.start : payload.size();
    if (world_state_end > suffix_end) {
        setError(
            error,
            "The Giant Warehouse stream begins outside the save payload.");
        return false;
    }

    std::size_t end = world_state_end;
    if (world_state_end < suffix_end) {
        std::size_t flags_offset = 0;
        if (!retailHeaderEnd(
                payload, world_state_end, suffix_end, flags_offset) ||
            !restorePages(
                payload,
                flags_offset,
                item_database,
                warehouse,
                end,
                error) ||
            end > suffix_end) {
            if (error && error->empty()) {
                setError(error, "The retail Giant Warehouse stream is invalid.");
            }
            return false;
        }
    } else if (!extension.additional_state.empty()) {
        std::size_t flags_offset = 0;
        std::int32_t portable_version = 0;
        if (!portableHeaderEnd(
                extension.additional_state,
                flags_offset,
                portable_version) ||
            !restorePages(
                extension.additional_state,
                flags_offset,
                item_database,
                warehouse,
                end,
                error) ||
            end > extension.additional_state.size() ||
            (portable_version == kPortableVersion &&
             end != extension.additional_state.size())) {
            if (error && error->empty()) {
                setError(error, "The portable Giant Warehouse stream is invalid.");
            }
            return false;
        }
        end = world_state_end;
    }
    if (serialized_end) {
        *serialized_end = end;
    }
    if (error) {
        error->clear();
    }
    return true;
}

bool replaceRetailGiantWarehouse(
    std::vector<std::uint8_t>& payload,
    std::size_t world_state_end,
    const ItemDatabase& item_database,
    const PlayerGiantWarehouse& warehouse,
    std::size_t* serialized_end,
    std::string* error) {
    const RetailSavePortableExtension extension =
        inspectRetailSavePortableExtension(payload);
    const std::size_t suffix_end =
        extension.present ? extension.start : payload.size();
    if (world_state_end > suffix_end) {
        setError(
            error,
            "The Giant Warehouse stream begins outside the save payload.");
        return false;
    }

    std::size_t end = world_state_end;
    if (world_state_end < suffix_end) {
        std::size_t flags_offset = 0;
        PlayerGiantWarehouse ignored;
        std::size_t old_end = 0;
        if (!retailHeaderEnd(
                payload, world_state_end, suffix_end, flags_offset) ||
            !restorePages(
                payload,
                flags_offset,
                item_database,
                ignored,
                old_end,
                error) ||
            old_end > suffix_end) {
            if (error && error->empty()) {
                setError(error, "The existing Giant Warehouse stream is invalid.");
            }
            return false;
        }
        std::vector<std::uint8_t> replacement;
        replacement.insert(
            replacement.end(), payload.begin(),
            payload.begin() + static_cast<std::ptrdiff_t>(flags_offset));
        if (!appendPages(
                replacement, item_database, warehouse, error)) {
            return false;
        }
        end = replacement.size();
        replacement.insert(
            replacement.end(),
            payload.begin() + static_cast<std::ptrdiff_t>(old_end),
            payload.end());
        payload = std::move(replacement);
    } else {
        std::int32_t portable_version = kPortableVersion;
        std::vector<std::uint8_t> preserved_state;
        if (!extension.additional_state.empty()) {
            std::size_t flags_offset = 0;
            PlayerGiantWarehouse ignored;
            std::size_t old_end = 0;
            if (!portableHeaderEnd(
                    extension.additional_state,
                    flags_offset,
                    portable_version) ||
                !restorePages(
                    extension.additional_state,
                    flags_offset,
                    item_database,
                    ignored,
                    old_end,
                    error) ||
                old_end > extension.additional_state.size()) {
                if (error && error->empty()) {
                    setError(
                        error,
                        "The existing portable Giant Warehouse stream "
                        "is invalid.");
                }
                return false;
            }
            preserved_state.assign(
                extension.additional_state.begin() +
                    static_cast<std::ptrdiff_t>(old_end),
                extension.additional_state.end());
        }
        std::vector<std::uint8_t> state(
            kPortableSignature.begin(), kPortableSignature.end());
        appendI32(state, portable_version);
        appendI32(
            state,
            static_cast<std::int32_t>(
                PlayerGiantWarehouse::page_count));
        if (!appendPages(state, item_database, warehouse, error)) {
            return false;
        }
        state.insert(
            state.end(),
            preserved_state.begin(),
            preserved_state.end());
        replaceRetailSavePortableExtensionState(
            payload,
            extension.present && extension.running,
            extension.has_mine_count ? extension.mine_count : 0,
            state);
    }
    if (serialized_end) {
        *serialized_end = end;
    }
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace osf
