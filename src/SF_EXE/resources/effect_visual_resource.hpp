#ifndef OPENSHADOWFLARE_EFFECT_VISUAL_RESOURCE_HPP
#define OPENSHADOWFLARE_EFFECT_VISUAL_RESOURCE_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace osf {

class EffectVisualResource {
public:
    bool load(
        const std::filesystem::path& directory,
        std::string* error = nullptr);
    bool load(
        const std::filesystem::path& directory,
        std::string_view stem,
        std::string* error = nullptr);
    void clear();

    const gapi::NjpImage& patterns() const;
    const gapi::CafAnimation& animation() const;
    std::uint64_t memoryUsageBytes() const;

private:
    gapi::NjpImage patterns_;
    gapi::CafAnimation animation_;
};

class EffectVisualResources {
public:
    const EffectVisualResource* load(
        const std::filesystem::path& data_root,
        std::int32_t resource_id,
        std::string* error = nullptr);
    const EffectVisualResource* find(
        std::int32_t resource_id) const;
    void clear();
    std::uint64_t memoryUsageBytes() const;

private:
    std::unordered_map<
        std::int32_t,
        std::unique_ptr<EffectVisualResource>> resources_;
};

}  // namespace osf

#endif
