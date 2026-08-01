#ifndef OPENSHADOWFLARE_RETAIL_SAVE_FILE_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_FILE_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf {

class ItemDatabase;
class PlayerBelt;
class PlayerData;
class PlayerEquipment;
class PlayerGiantWarehouse;
class PlayerInventory;
class PlayerMagic;
class PlayerAutomaticItems;
class PlayerSpecialItems;
struct RetailSaveProgress;

bool readRetailSavePayload(
    const std::filesystem::path& path,
    std::vector<std::uint8_t>& payload,
    std::string* error = nullptr);

bool writeRetailSave(
    const std::filesystem::path& path,
    const PlayerData& player,
    std::uint8_t xor_key,
    std::string* error = nullptr);
bool writeRetailSave(
    const std::filesystem::path& path,
    const PlayerData& player,
    const ItemDatabase& item_database,
    const PlayerInventory& inventory,
    const PlayerEquipment& equipment,
    const PlayerBelt& belt,
    const PlayerSpecialItems& special_items,
    std::uint8_t xor_key,
    std::string* error = nullptr);
bool writeRetailSave(
    const std::filesystem::path& path,
    const PlayerData& player,
    const ItemDatabase& item_database,
    const PlayerInventory& inventory,
    const PlayerEquipment& equipment,
    const PlayerBelt& belt,
    const PlayerSpecialItems& special_items,
    const RetailSaveProgress& progress,
    std::uint8_t xor_key,
    std::string* error = nullptr);
bool writeRetailSave(
    const std::filesystem::path& path,
    const PlayerData& player,
    const ItemDatabase& item_database,
    const PlayerInventory& inventory,
    const PlayerEquipment& equipment,
    const PlayerBelt& belt,
    const PlayerSpecialItems& special_items,
    const RetailSaveProgress& progress,
    const PlayerMagic& magic,
    std::uint8_t xor_key,
    std::string* error = nullptr);
bool writeRetailSave(
    const std::filesystem::path& path,
    const PlayerData& player,
    const ItemDatabase& item_database,
    const PlayerInventory& inventory,
    const PlayerEquipment& equipment,
    const PlayerBelt& belt,
    const PlayerSpecialItems& special_items,
    const RetailSaveProgress& progress,
    const PlayerMagic& magic,
    std::int32_t mine_count,
    const PlayerGiantWarehouse& giant_warehouse,
    const PlayerAutomaticItems& automatic_items,
    std::uint8_t xor_key,
    std::string* error = nullptr);

}  // namespace osf

#endif
