#ifndef OPENSHADOWFLARE_ITEM_INVENTORY_RESOURCE_HPP
#define OPENSHADOWFLARE_ITEM_INVENTORY_RESOURCE_HPP

#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

namespace osf {

class ItemInventoryResource {
public:
    static constexpr std::size_t group_count = 14;

    bool load(
        const std::filesystem::path& data_root,
        std::string* error = nullptr);
    void clear();

    const gapi::NjpImage* group(
        std::int32_t index) const;
    std::uint64_t memoryUsageBytes() const;

private:
    std::array<gapi::NjpImage, group_count> groups_;
};

}  // namespace osf

#endif
