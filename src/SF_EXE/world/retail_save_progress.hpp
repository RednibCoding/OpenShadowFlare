#ifndef OPENSHADOWFLARE_RETAIL_SAVE_PROGRESS_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_PROGRESS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

bool restoreRetailTransportFlags(
    const std::vector<std::uint8_t>& payload,
    std::size_t owned_items_end,
    std::vector<std::int32_t>& flags,
    std::string* error = nullptr);

}  // namespace osf

#endif
