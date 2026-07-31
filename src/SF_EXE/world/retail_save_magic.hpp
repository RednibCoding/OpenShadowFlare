#ifndef OPENSHADOWFLARE_RETAIL_SAVE_MAGIC_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_MAGIC_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class PlayerMagic;

bool restoreRetailMagic(
    const std::vector<std::uint8_t>& payload,
    std::size_t progress_end,
    PlayerMagic& magic,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

bool replaceRetailMagic(
    std::vector<std::uint8_t>& payload,
    std::size_t progress_end,
    const PlayerMagic& magic,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

}  // namespace osf

#endif
