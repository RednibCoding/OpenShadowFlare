#ifndef OPENSHADOWFLARE_ITEM_WORLD_RESOURCE_HPP
#define OPENSHADOWFLARE_ITEM_WORLD_RESOURCE_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace osf {

class ItemWorldResource {
public:
    bool load(
        const std::filesystem::path& data_root,
        std::int32_t resource_id,
        std::string* error = nullptr);
    void clear();

    std::int32_t id() const;
    const gapi::NjpImage& patterns() const;
    const gapi::NjpImage& shadowPatterns() const;
    const gapi::CafAnimation& animation() const;

private:
    std::int32_t id_ = -1;
    gapi::NjpImage patterns_;
    gapi::NjpImage shadow_patterns_;
    gapi::CafAnimation animation_;
};

}  // namespace osf

#endif
