#ifndef OPENSHADOWFLARE_RETAIL_SAVE_MINES_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_MINES_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

bool restoreRetailMineCount(
    const std::vector<std::uint8_t>& payload,
    std::size_t magic_end,
    std::int32_t& mine_count,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

bool replaceRetailMineCount(
    std::vector<std::uint8_t>& payload,
    std::size_t magic_end,
    std::int32_t mine_count,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

}  // namespace osf

#endif
