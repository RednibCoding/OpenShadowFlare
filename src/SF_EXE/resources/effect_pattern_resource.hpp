#ifndef OPENSHADOWFLARE_EFFECT_PATTERN_RESOURCE_HPP
#define OPENSHADOWFLARE_EFFECT_PATTERN_RESOURCE_HPP

#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace osf {

class EffectPatternResources {
public:
    const gapi::NjpImage* load(
        const std::filesystem::path& data_root,
        std::int32_t resource_id,
        std::string* error = nullptr);
    const gapi::NjpImage* find(
        std::int32_t resource_id) const;
    void clear();
    std::uint64_t memoryUsageBytes() const;

private:
    std::unordered_map<
        std::int32_t,
        std::unique_ptr<gapi::NjpImage>> resources_;
};

}  // namespace osf

#endif
