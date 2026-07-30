#include "retail_save_items.hpp"

#include "items/item_database.hpp"
#include "items/player_belt.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "items/player_special_items.hpp"
#include "world/player_data.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace osf {
namespace {

constexpr std::size_t kItemPayloadOffset =
    PlayerData::retail_record_size;
constexpr std::int32_t kMaximumSerializedItems = 4096;

constexpr std::array<EquipmentSlot, 9> kRetailEquipmentOrder{{
    EquipmentSlot::main_hand,
    EquipmentSlot::helmet,
    EquipmentSlot::body,
    EquipmentSlot::off_hand,
    EquipmentSlot::boots,
    EquipmentSlot::accessory_1,
    EquipmentSlot::accessory_2,
    EquipmentSlot::accessory_3,
    EquipmentSlot::accessory_4,
}};

struct ParsedSections {
    bool has_item_stream = false;
    std::size_t extra_equipment_begin = kItemPayloadOffset;
    std::size_t extra_equipment_end = kItemPayloadOffset;
    std::size_t special_items_begin = kItemPayloadOffset;
    std::size_t special_items_end = kItemPayloadOffset;
    std::size_t end = kItemPayloadOffset;
};

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

std::size_t retailStateSize(std::int32_t category) {
    // FUN_00465ea0(-1) supplies the instance block sizes used by both
    // 0x0044b580 and 0x0044cac0.
    constexpr std::array<std::size_t, 5> sizes{{
        200,
        200,
        192,
        0,
        4,
    }};
    return category >= 0 &&
                   static_cast<std::size_t>(category) < sizes.size()
        ? sizes[static_cast<std::size_t>(category)]
        : std::numeric_limits<std::size_t>::max();
}

void appendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value) {
    const std::uint32_t data =
        static_cast<std::uint32_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(data));
    bytes.push_back(static_cast<std::uint8_t>(data >> 8u));
    bytes.push_back(static_cast<std::uint8_t>(data >> 16u));
    bytes.push_back(static_cast<std::uint8_t>(data >> 24u));
}

std::int32_t readStateI32(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset) {
    if (offset > bytes.size() ||
        bytes.size() - offset < 4) {
        return 0;
    }
    const std::uint32_t value =
        static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
        (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
    return static_cast<std::int32_t>(value);
}

void writeStateI32(
    std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::int32_t value) {
    if (offset > bytes.size() ||
        bytes.size() - offset < 4) {
        return;
    }
    const std::uint32_t data =
        static_cast<std::uint32_t>(value);
    bytes[offset] = static_cast<std::uint8_t>(data);
    bytes[offset + 1] =
        static_cast<std::uint8_t>(data >> 8u);
    bytes[offset + 2] =
        static_cast<std::uint8_t>(data >> 16u);
    bytes[offset + 3] =
        static_cast<std::uint8_t>(data >> 24u);
}

class PayloadCursor {
public:
    PayloadCursor(
        const std::vector<std::uint8_t>& payload,
        std::size_t offset)
        : payload_(payload), offset_(offset) {}

    bool readI32(std::int32_t& value) {
        if (remaining() < 4) {
            return false;
        }
        value = readStateI32(payload_, offset_);
        offset_ += 4;
        return true;
    }

    bool readBytes(
        std::size_t count,
        std::vector<std::uint8_t>& bytes) {
        if (remaining() < count) {
            return false;
        }
        bytes.assign(
            payload_.begin() +
                static_cast<std::ptrdiff_t>(offset_),
            payload_.begin() +
                static_cast<std::ptrdiff_t>(offset_ + count));
        offset_ += count;
        return true;
    }

    std::size_t offset() const {
        return offset_;
    }

private:
    std::size_t remaining() const {
        return offset_ <= payload_.size()
            ? payload_.size() - offset_
            : 0;
    }

    const std::vector<std::uint8_t>& payload_;
    std::size_t offset_ = 0;
};

bool decodeItem(
    PayloadCursor& cursor,
    const ItemDatabase* item_database,
    bool has_grid_position,
    InventoryItem* item,
    std::string* error) {
    std::int32_t category = -1;
    std::int32_t definition_id = -1;
    std::int32_t identified = 0;
    std::int32_t grid_x = 0;
    std::int32_t grid_y = 0;
    if (!cursor.readI32(category) ||
        !cursor.readI32(definition_id) ||
        !cursor.readI32(identified) ||
        (has_grid_position &&
         (!cursor.readI32(grid_x) ||
          !cursor.readI32(grid_y)))) {
        setError(error, "The retail item record is truncated.");
        return false;
    }

    const std::size_t state_size =
        retailStateSize(category);
    if (state_size ==
        std::numeric_limits<std::size_t>::max()) {
        setError(error, "The retail item category is invalid.");
        return false;
    }
    std::int32_t serialized_state_size = 0;
    if (!cursor.readI32(serialized_state_size) ||
        serialized_state_size < 0 ||
        static_cast<std::size_t>(serialized_state_size) !=
            state_size) {
        setError(
            error,
            "The retail item instance size is invalid.");
        return false;
    }
    std::vector<std::uint8_t> state;
    if (!cursor.readBytes(state_size, state)) {
        setError(
            error,
            "The retail item instance state is truncated.");
        return false;
    }
    if (!item) {
        return true;
    }

    const ItemDefinition* definition =
        item_database
            ? item_database->find(category, definition_id)
            : nullptr;
    if (!definition) {
        setError(
            error,
            "A saved item definition is unavailable.");
        return false;
    }

    std::int32_t quantity = 1;
    if (category == 4) {
        quantity = readStateI32(state, 0);
        if (definition_id != 0 ||
            quantity <= 0 ||
            quantity >
                PlayerInventory::maximum_gold_stack) {
            setError(error, "A saved Gold stack is invalid.");
            return false;
        }
    }
    *item = makeInventoryItem(*definition, quantity);
    item->grid_x = grid_x;
    item->grid_y = grid_y;
    item->identified = identified;
    item->retail_state = std::move(state);
    if (category == 0 || category == 1) {
        item->durability =
            readStateI32(item->retail_state, 47u * 4u);
    }
    return true;
}

bool skipOrRestoreEquipment(
    PayloadCursor& cursor,
    const ItemDatabase* item_database,
    std::int32_t player_level,
    PlayerEquipment* equipment,
    ParsedSections& sections,
    std::string* error) {
    for (std::size_t index = 0; index < 11; ++index) {
        if (index == kRetailEquipmentOrder.size()) {
            sections.extra_equipment_begin =
                cursor.offset();
        }
        std::int32_t present = 0;
        if (!cursor.readI32(present) ||
            (present != 0 && present != 1)) {
            setError(
                error,
                "The retail equipment presence table is invalid at "
                "slot " +
                    std::to_string(index) +
                    " (value " +
                    std::to_string(present) + ").");
            return false;
        }
        if (present == 0) {
            continue;
        }
        InventoryItem item;
        InventoryItem* destination =
            equipment && index < kRetailEquipmentOrder.size()
                ? &item
                : nullptr;
        if (!decodeItem(
                cursor,
                item_database,
                false,
                destination,
                error)) {
            return false;
        }
        if (!destination) {
            continue;
        }
        const ItemDefinition* definition =
            item_database->find(
                item.category,
                item.definition_id);
        const EquipmentPlacementResult placement =
            equipment->place(
                kRetailEquipmentOrder[index],
                std::move(item),
                *definition,
                player_level);
        if (!placement.accepted || placement.held_item) {
            setError(
                error,
                "A saved item does not fit its equipment slot.");
            return false;
        }
    }
    sections.extra_equipment_end = cursor.offset();
    return true;
}

enum class ContainerKind {
    inventory,
    belt,
    special_items,
    ignored,
};

bool skipOrRestoreContainer(
    PayloadCursor& cursor,
    ContainerKind kind,
    const ItemDatabase* item_database,
    PlayerInventory* inventory,
    PlayerBelt* belt,
    PlayerSpecialItems* special_items,
    std::string* error) {
    std::int32_t count = 0;
    if (!cursor.readI32(count) ||
        count < 0 ||
        count > kMaximumSerializedItems) {
        setError(error, "The retail item-list count is invalid.");
        return false;
    }
    for (std::int32_t index = 0; index < count; ++index) {
        InventoryItem item;
        InventoryItem* destination =
            (kind == ContainerKind::inventory && inventory) ||
                    (kind == ContainerKind::belt && belt) ||
                    (kind == ContainerKind::special_items &&
                     special_items)
                ? &item
                : nullptr;
        if (!decodeItem(
                cursor,
                item_database,
                true,
                destination,
                error)) {
            return false;
        }
        if (kind == ContainerKind::inventory && inventory) {
            const std::int32_t grid_x = item.grid_x;
            const std::int32_t grid_y = item.grid_y;
            const InventoryPlacementResult placement =
                inventory->place(
                    std::move(item),
                    grid_x,
                    grid_y);
            if (!placement.accepted || placement.held_item) {
                setError(
                    error,
                    "A saved item does not fit the backpack.");
                return false;
            }
        } else if (kind == ContainerKind::belt && belt) {
            const ItemDefinition* definition =
                item_database->find(
                    item.category,
                    item.definition_id);
            const std::int32_t grid_x = item.grid_x;
            const std::int32_t grid_y = item.grid_y;
            const InventoryPlacementResult placement =
                belt->place(
                    std::move(item),
                    grid_x,
                    grid_y,
                    *definition);
            if (!placement.accepted || placement.held_item) {
                setError(
                    error,
                    "A saved item does not fit the belt.");
                return false;
            }
        } else if (
            kind == ContainerKind::special_items &&
            special_items) {
            const std::int32_t grid_x = item.grid_x;
            const std::int32_t grid_y = item.grid_y;
            const InventoryPlacementResult placement =
                special_items->place(
                    std::move(item),
                    grid_x,
                    grid_y);
            if (!placement.accepted ||
                placement.held_item) {
                setError(
                    error,
                    "A saved item does not fit the special-item window.");
                return false;
            }
        }
    }
    return true;
}

bool parseOwnedItems(
    const std::vector<std::uint8_t>& payload,
    const ItemDatabase* item_database,
    std::int32_t player_level,
    PlayerInventory* inventory,
    PlayerEquipment* equipment,
    PlayerBelt* belt,
    PlayerSpecialItems* special_items,
    ParsedSections& sections,
    std::string* error) {
    if (payload.size() < kItemPayloadOffset) {
        setError(error, "The retail save payload is truncated.");
        return false;
    }
    if (payload.size() == kItemPayloadOffset) {
        sections = {};
        return true;
    }

    sections.has_item_stream = true;
    PayloadCursor cursor(payload, kItemPayloadOffset);
    if (!skipOrRestoreEquipment(
            cursor,
            item_database,
            player_level,
            equipment,
            sections,
            error) ||
        !skipOrRestoreContainer(
            cursor,
            ContainerKind::inventory,
            item_database,
            inventory,
            belt,
            special_items,
            error) ||
        !skipOrRestoreContainer(
            cursor,
            ContainerKind::belt,
            item_database,
            inventory,
            belt,
            special_items,
            error)) {
        return false;
    }
    sections.special_items_begin = cursor.offset();
    if (!skipOrRestoreContainer(
            cursor,
            ContainerKind::special_items,
            item_database,
            inventory,
            belt,
            special_items,
            error)) {
        return false;
    }
    sections.special_items_end = cursor.offset();
    sections.end = cursor.offset();
    return true;
}

std::vector<std::uint8_t> encodedState(
    const InventoryItem& item) {
    const std::size_t state_size =
        retailStateSize(item.category);
    std::vector<std::uint8_t> state =
        item.retail_state.size() == state_size
            ? item.retail_state
            : std::vector<std::uint8_t>(state_size);
    if (item.category == 0 || item.category == 1) {
        writeStateI32(state, 47u * 4u, item.durability);
        writeStateI32(state, 48u * 4u, item.identified);
        if (item.retail_state.size() != state_size) {
            writeStateI32(state, 49u * 4u, -1);
        }
    } else if (item.category == 2) {
        writeStateI32(state, 47u * 4u, item.identified);
    } else if (item.category == 4) {
        writeStateI32(state, 0, item.quantity);
    }
    return state;
}

bool encodeItem(
    std::vector<std::uint8_t>& bytes,
    const InventoryItem& item,
    const ItemDatabase& item_database,
    bool include_grid_position,
    std::string* error) {
    const ItemDefinition* definition =
        item_database.find(
            item.category,
            item.definition_id);
    if (!definition ||
        retailStateSize(item.category) ==
            std::numeric_limits<std::size_t>::max() ||
        item.quantity <= 0 ||
        (item.category != 4 && item.quantity != 1) ||
        (item.category == 4 &&
         (item.definition_id != 0 ||
          item.quantity >
              PlayerInventory::maximum_gold_stack))) {
        setError(error, "An owned item cannot be serialized.");
        return false;
    }
    appendI32(bytes, item.category);
    appendI32(bytes, item.definition_id);
    appendI32(bytes, item.identified);
    if (include_grid_position) {
        appendI32(bytes, item.grid_x);
        appendI32(bytes, item.grid_y);
    }
    const std::vector<std::uint8_t> state =
        encodedState(item);
    appendI32(
        bytes,
        static_cast<std::int32_t>(state.size()));
    bytes.insert(bytes.end(), state.begin(), state.end());
    return true;
}

bool encodeContainer(
    std::vector<std::uint8_t>& bytes,
    const std::vector<InventoryItem>& items,
    const ItemDatabase& item_database,
    std::string* error) {
    if (items.size() >
        static_cast<std::size_t>(
            std::numeric_limits<std::int32_t>::max())) {
        setError(error, "An owned item list is too large.");
        return false;
    }
    appendI32(
        bytes,
        static_cast<std::int32_t>(items.size()));
    for (const InventoryItem& item : items) {
        if (!encodeItem(
                bytes,
                item,
                item_database,
                true,
                error)) {
            return false;
        }
    }
    return true;
}

}  // namespace

bool restoreRetailOwnedItems(
    const std::vector<std::uint8_t>& payload,
    const ItemDatabase& item_database,
    std::int32_t player_level,
    PlayerInventory& inventory,
    PlayerEquipment& equipment,
    PlayerBelt& belt,
    PlayerSpecialItems& special_items,
    std::size_t* serialized_end,
    std::string* error) {
    PlayerInventory restored_inventory;
    PlayerEquipment restored_equipment;
    PlayerBelt restored_belt;
    PlayerSpecialItems restored_special_items;
    ParsedSections sections;
    if (!parseOwnedItems(
            payload,
            &item_database,
            player_level,
            &restored_inventory,
            &restored_equipment,
            &restored_belt,
            &restored_special_items,
            sections,
            error)) {
        return false;
    }
    inventory = std::move(restored_inventory);
    equipment = std::move(restored_equipment);
    belt = std::move(restored_belt);
    special_items = std::move(restored_special_items);
    if (serialized_end) {
        *serialized_end = sections.end;
    }
    if (error) {
        error->clear();
    }
    return true;
}

bool replaceRetailOwnedItems(
    std::vector<std::uint8_t>& payload,
    const ItemDatabase& item_database,
    const PlayerInventory& inventory,
    const PlayerEquipment& equipment,
    const PlayerBelt& belt,
    const PlayerSpecialItems& special_items,
    std::size_t* serialized_end,
    std::string* error) {
    ParsedSections sections;
    if (!parseOwnedItems(
            payload,
            nullptr,
            0,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            sections,
            error)) {
        return false;
    }

    std::vector<std::uint8_t> encoded;
    encoded.reserve(payload.size());
    encoded.insert(
        encoded.end(),
        payload.begin(),
        payload.begin() +
            static_cast<std::ptrdiff_t>(kItemPayloadOffset));
    for (std::size_t index = 0;
         index < kRetailEquipmentOrder.size();
         ++index) {
        const InventoryItem* item =
            equipment.item(kRetailEquipmentOrder[index]);
        appendI32(encoded, item ? 1 : 0);
        if (item &&
            !encodeItem(
                encoded,
                *item,
                item_database,
                false,
                error)) {
            return false;
        }
    }
    if (sections.has_item_stream) {
        encoded.insert(
            encoded.end(),
            payload.begin() +
                static_cast<std::ptrdiff_t>(
                    sections.extra_equipment_begin),
            payload.begin() +
                static_cast<std::ptrdiff_t>(
                    sections.extra_equipment_end));
    } else {
        appendI32(encoded, 0);
        appendI32(encoded, 0);
    }
    if (!encodeContainer(
            encoded,
            inventory.items(),
            item_database,
            error) ||
        !encodeContainer(
            encoded,
            belt.items(),
            item_database,
            error)) {
        return false;
    }
    if (!encodeContainer(
            encoded,
            special_items.items(),
            item_database,
            error)) {
        return false;
    }
    const std::size_t new_serialized_end = encoded.size();
    encoded.insert(
        encoded.end(),
        payload.begin() +
            static_cast<std::ptrdiff_t>(sections.end),
        payload.end());
    payload = std::move(encoded);
    if (serialized_end) {
        *serialized_end = new_serialized_end;
    }
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace osf
