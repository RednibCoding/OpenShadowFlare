#ifndef OPENSHADOWFLARE_OBJECT_VISUAL_RESOURCE_HPP
#define OPENSHADOWFLARE_OBJECT_VISUAL_RESOURCE_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace osf {

class ObjectVisualResource {
public:
    bool load(
        const std::filesystem::path& data_root,
        std::int32_t resource_id,
        std::string* error = nullptr);
    void clear();

    std::int32_t id() const;
    bool hasStaticPatterns() const;
    bool hasStaticShadows() const;
    bool hasAnimation() const;
    const gapi::NjpImage& staticPatterns() const;
    const gapi::NjpImage& staticShadows() const;
    const gapi::NjpImage& animationPatterns() const;
    const gapi::CafAnimation& animation() const;
    std::uint64_t memoryUsageBytes() const;

private:
    std::int32_t id_ = -1;
    bool has_static_patterns_ = false;
    bool has_static_shadows_ = false;
    bool has_animation_ = false;
    gapi::NjpImage static_patterns_;
    gapi::NjpImage static_shadows_;
    gapi::NjpImage animation_patterns_;
    gapi::CafAnimation animation_;
};

class ObjectVisualResources {
public:
    const ObjectVisualResource* load(
        const std::filesystem::path& data_root,
        std::int32_t resource_id,
        std::string* error = nullptr);
    const ObjectVisualResource* find(
        std::int32_t resource_id) const;
    void clear();
    std::uint64_t memoryUsageBytes() const;

private:
    std::unordered_map<
        std::int32_t,
        std::unique_ptr<ObjectVisualResource>> resources_;
};

}  // namespace osf

#endif
