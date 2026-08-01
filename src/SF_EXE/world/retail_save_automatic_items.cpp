#include "retail_save_automatic_items.hpp"

#include "items/player_automatic_items.hpp"
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
constexpr std::int32_t kGiantOnlyVersion = 1;
constexpr std::int32_t kAutomaticItemsVersion = 2;

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

void writeI32(
    std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::int32_t value) {
    const std::uint32_t encoded = static_cast<std::uint32_t>(value);
    bytes[offset] = static_cast<std::uint8_t>(encoded);
    bytes[offset + 1] = static_cast<std::uint8_t>(encoded >> 8u);
    bytes[offset + 2] = static_cast<std::uint8_t>(encoded >> 16u);
    bytes[offset + 3] = static_cast<std::uint8_t>(encoded >> 24u);
}

bool restorePages(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    const ItemDatabase& item_database,
    PlayerAutomaticItems& items,
    std::size_t& end,
    std::string* error) {
    PlayerAutomaticItems restored;
    for (std::size_t page = 0;
         page < PlayerAutomaticItems::page_count;
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
    items = std::move(restored);
    end = offset;
    return true;
}

bool appendPages(
    std::vector<std::uint8_t>& bytes,
    const ItemDatabase& item_database,
    const PlayerAutomaticItems& items,
    std::string* error) {
    for (std::size_t page = 0;
         page < PlayerAutomaticItems::page_count;
         ++page) {
        if (!appendRetailSpecialItemContainer(
                bytes, item_database, items.page(page), error)) {
            return false;
        }
    }
    return true;
}

bool locatePortablePages(
    const std::vector<std::uint8_t>& state,
    const ItemDatabase& item_database,
    std::int32_t& version,
    std::size_t& automatic_begin,
    std::size_t& automatic_end,
    std::string* error) {
    std::int32_t giant_page_count = 0;
    if (state.size() < 16u ||
        !std::equal(
            kPortableSignature.begin(),
            kPortableSignature.end(),
            state.begin()) ||
        !readI32(state, 8u, version) ||
        !readI32(state, 12u, giant_page_count) ||
        (version != kGiantOnlyVersion &&
         version != kAutomaticItemsVersion) ||
        giant_page_count != static_cast<std::int32_t>(
                                PlayerGiantWarehouse::page_count)) {
        setError(error, "The portable late-item header is invalid.");
        return false;
    }

    std::size_t offset = 16u +
        PlayerGiantWarehouse::page_count * 4u;
    for (std::size_t page = 0;
         page < PlayerGiantWarehouse::page_count;
         ++page) {
        PlayerSpecialItems ignored;
        if (!restoreRetailSpecialItemContainer(
                state,
                offset,
                item_database,
                ignored,
                &offset,
                error)) {
            return false;
        }
    }
    automatic_begin = offset;
    automatic_end = offset;
    if (version == kAutomaticItemsVersion) {
        PlayerAutomaticItems ignored_items;
        if (!restorePages(
                state,
                offset,
                item_database,
                ignored_items,
                automatic_end,
                error)) {
            return false;
        }
    }
    return automatic_end <= state.size();
}

}  // namespace

bool restoreRetailAutomaticItems(
    const std::vector<std::uint8_t>& payload,
    std::size_t giant_warehouse_end,
    const ItemDatabase& item_database,
    PlayerAutomaticItems& items,
    std::size_t* serialized_end,
    std::string* error) {
    items.clear();
    const RetailSavePortableExtension extension =
        inspectRetailSavePortableExtension(payload);
    const std::size_t suffix_end =
        extension.present ? extension.start : payload.size();
    std::size_t end = giant_warehouse_end;
    if (giant_warehouse_end < suffix_end) {
        if (!restorePages(
                payload,
                giant_warehouse_end,
                item_database,
                items,
                end,
                error) ||
            end > suffix_end) {
            return false;
        }
    } else if (!extension.additional_state.empty()) {
        std::int32_t version = 0;
        std::size_t begin = 0;
        std::size_t portable_end = 0;
        if (!locatePortablePages(
                extension.additional_state,
                item_database,
                version,
                begin,
                portable_end,
                error)) {
            return false;
        }
        if (version == kAutomaticItemsVersion &&
            (!restorePages(
                 extension.additional_state,
                 begin,
                 item_database,
                 items,
                 portable_end,
                 error) ||
             portable_end != extension.additional_state.size())) {
            return false;
        }
    }
    if (serialized_end) {
        *serialized_end = end;
    }
    if (error) {
        error->clear();
    }
    return true;
}

bool replaceRetailAutomaticItems(
    std::vector<std::uint8_t>& payload,
    std::size_t giant_warehouse_end,
    const ItemDatabase& item_database,
    const PlayerAutomaticItems& items,
    std::size_t* serialized_end,
    std::string* error) {
    const RetailSavePortableExtension extension =
        inspectRetailSavePortableExtension(payload);
    const std::size_t suffix_end =
        extension.present ? extension.start : payload.size();
    std::size_t end = giant_warehouse_end;
    if (giant_warehouse_end < suffix_end) {
        PlayerAutomaticItems ignored;
        std::size_t old_end = 0;
        if (!restorePages(
                payload,
                giant_warehouse_end,
                item_database,
                ignored,
                old_end,
                error) ||
            old_end > suffix_end) {
            return false;
        }
        std::vector<std::uint8_t> replacement;
        replacement.insert(
            replacement.end(),
            payload.begin(),
            payload.begin() + static_cast<std::ptrdiff_t>(
                                  giant_warehouse_end));
        if (!appendPages(replacement, item_database, items, error)) {
            return false;
        }
        end = replacement.size();
        replacement.insert(
            replacement.end(),
            payload.begin() + static_cast<std::ptrdiff_t>(old_end),
            payload.end());
        payload = std::move(replacement);
    } else {
        std::int32_t version = 0;
        std::size_t automatic_begin = 0;
        std::size_t old_end = 0;
        if (extension.additional_state.empty() ||
            !locatePortablePages(
                extension.additional_state,
                item_database,
                version,
                automatic_begin,
                old_end,
                error)) {
            return false;
        }
        std::vector<std::uint8_t> state;
        state.insert(
            state.end(),
            extension.additional_state.begin(),
            extension.additional_state.begin() +
                static_cast<std::ptrdiff_t>(automatic_begin));
        writeI32(state, 8u, kAutomaticItemsVersion);
        if (!appendPages(state, item_database, items, error)) {
            return false;
        }
        state.insert(
            state.end(),
            extension.additional_state.begin() +
                static_cast<std::ptrdiff_t>(old_end),
            extension.additional_state.end());
        replaceRetailSavePortableExtensionState(
            payload,
            extension.running,
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
