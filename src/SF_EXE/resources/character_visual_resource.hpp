#ifndef OPENSHADOWFLARE_CHARACTER_VISUAL_RESOURCE_HPP
#define OPENSHADOWFLARE_CHARACTER_VISUAL_RESOURCE_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace osf {

class CharacterVisualResource {
public:
    bool load(
        const std::filesystem::path& directory,
        const std::string& stem,
        std::string* error = nullptr);
    void clear();

    const gapi::NjpImage& patterns() const;
    const gapi::NjpImage& shadowPatterns() const;
    const gapi::CafAnimation& animation() const;
    std::uint64_t memoryUsageBytes() const;

private:
    gapi::NjpImage patterns_;
    gapi::NjpImage shadow_patterns_;
    gapi::CafAnimation animation_;
};

class CharacterVisualResources {
public:
    explicit CharacterVisualResources(
        std::string category);

    const CharacterVisualResource* load(
        const std::filesystem::path& data_root,
        std::int32_t resource_id,
        std::string* error = nullptr);
    const CharacterVisualResource* find(
        std::int32_t resource_id) const;
    void clear();
    std::uint64_t memoryUsageBytes() const;

private:
    std::string category_;
    std::unordered_map<
        std::int32_t,
        std::unique_ptr<CharacterVisualResource>> resources_;
};

}  // namespace osf

#endif
