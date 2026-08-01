#ifndef OPENSHADOWFLARE_RETAIL_SAVE_EXTENSION_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_EXTENSION_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {

struct RetailSavePortableExtension {
    bool present = false;
    std::size_t start = 0;
    std::uint32_t size = 0;
    std::uint32_t version = 0;
    bool running = false;
    bool has_mine_count = false;
    std::int32_t mine_count = 0;
};

RetailSavePortableExtension inspectRetailSavePortableExtension(
    const std::vector<std::uint8_t>& payload);

void replaceRetailSavePortableExtension(
    std::vector<std::uint8_t>& payload,
    bool running,
    std::int32_t mine_count,
    bool include_mine_count);

}  // namespace osf

#endif
