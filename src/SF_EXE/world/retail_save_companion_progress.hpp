#ifndef OPENSHADOWFLARE_RETAIL_SAVE_COMPANION_PROGRESS_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_COMPANION_PROGRESS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class PlayerData;

bool restoreRetailCompanionProgress(
    const std::vector<std::uint8_t>& payload,
    std::size_t magic_end,
    PlayerData& player,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

bool replaceRetailCompanionProgress(
    std::vector<std::uint8_t>& payload,
    std::size_t magic_end,
    const PlayerData& player,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

}  // namespace osf

#endif
