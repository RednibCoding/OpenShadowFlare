#ifndef OPENSHADOWFLARE_RETAIL_SAVE_FILE_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_FILE_HPP

#include <cstdint>
#include <filesystem>
#include <string>

namespace osf {

class PlayerData;

bool writeRetailSave(
    const std::filesystem::path& path,
    const PlayerData& player,
    std::uint8_t xor_key,
    std::string* error = nullptr);

}  // namespace osf

#endif
