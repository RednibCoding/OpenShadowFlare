#include "item_inventory_resource.hpp"

#include "resource_memory.hpp"
#include "retail_filesystem.hpp"

#include <algorithm>
#include <cstdio>
#include <utility>

namespace osf {
namespace {

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

bool matchesPatternSelection(
    const gapi::NjpImage& image,
    const std::vector<std::uint8_t>& enabled_patterns) {
    for (std::size_t index = 0;
         index < image.patterns().size();
         ++index) {
        const bool requested =
            index < enabled_patterns.size() &&
            enabled_patterns[index] != 0;
        if (image.patternDecoded(index) != requested) {
            return false;
        }
    }
    for (std::size_t index = image.patterns().size();
         index < enabled_patterns.size();
         ++index) {
        if (enabled_patterns[index] != 0) {
            return false;
        }
    }
    return true;
}

}  // namespace

bool ItemInventoryResource::load(
    const std::filesystem::path& data_root,
    std::string* error) {
    clear();
    data_root_ = data_root;
    if (error) {
        error->clear();
    }
    return true;
}

bool ItemInventoryResource::prepareGroups(
    const std::array<std::uint8_t, group_count>& enabled_groups,
    std::string* error) {
    if (data_root_.empty()) {
        setError(
            error,
            "The inventory artwork resource has not been initialized.");
        return false;
    }
    for (std::size_t index = 0;
         index < enabled_groups.size();
         ++index) {
        if (enabled_groups[index] == 0) {
            groups_[index] = {};
            continue;
        }
        if (!groups_[index].patterns().empty() &&
            groups_[index].decodedPatternFlags().size() ==
                groups_[index].patterns().size() &&
            std::all_of(
                groups_[index].decodedPatternFlags().begin(),
                groups_[index].decodedPatternFlags().end(),
                [](std::uint8_t decoded) { return decoded != 0; })) {
            continue;
        }
        char retail_path[64]{};
        std::snprintf(
            retail_path,
            sizeof(retail_path),
            "System\\Game\\Pattern\\Item%04lu.njp",
            static_cast<unsigned long>(index));
        std::string resource_error;
        if (!groups_[index].load(
                resolveRetailPath(data_root_, retail_path),
                &resource_error)) {
            setError(
                error,
                "An inventory item pattern could not be loaded: " +
                    std::string(retail_path) + " (" +
                    resource_error + ")");
            return false;
        }
    }
    if (error) {
        error->clear();
    }
    return true;
}

bool ItemInventoryResource::preparePatterns(
    const PatternSelection& enabled_patterns,
    std::string* error) {
    if (data_root_.empty()) {
        setError(
            error,
            "The inventory artwork resource has not been initialized.");
        return false;
    }

    for (std::size_t index = 0; index < groups_.size(); ++index) {
        if (enabled_patterns[index].empty()) {
            groups_[index] = {};
        }
    }
    for (std::size_t index = 0;
         index < groups_.size();
         ++index) {
        if (enabled_patterns[index].empty() ||
            matchesPatternSelection(
                groups_[index], enabled_patterns[index])) {
            continue;
        }
        char retail_path[64]{};
        std::snprintf(
            retail_path,
            sizeof(retail_path),
            "System\\Game\\Pattern\\Item%04lu.njp",
            static_cast<unsigned long>(index));
        std::string resource_error;
        if (!groups_[index].loadSelectedPatterns(
                resolveRetailPath(
                    data_root_,
                    retail_path),
                enabled_patterns[index],
                &resource_error)) {
            setError(
                error,
                "An inventory item pattern could not be loaded: " +
                    std::string(retail_path) + " (" +
                    resource_error + ")");
            return false;
        }
    }
    if (error) {
        error->clear();
    }
    return true;
}

void ItemInventoryResource::clear() {
    for (gapi::NjpImage& group : groups_) {
        group = {};
    }
    data_root_.clear();
}

const gapi::NjpImage* ItemInventoryResource::group(
    std::int32_t index) const {
    if (index < 0 ||
        static_cast<std::size_t>(index) >=
            groups_.size() ||
        groups_[static_cast<std::size_t>(index)].patterns().empty()) {
        return nullptr;
    }
    return &groups_[static_cast<std::size_t>(index)];
}

std::uint64_t ItemInventoryResource::memoryUsageBytes() const {
    std::uint64_t bytes = 0;
    for (const gapi::NjpImage& group : groups_) {
        bytes += decodedMemoryUsageBytes(group);
    }
    return bytes;
}

}  // namespace osf
