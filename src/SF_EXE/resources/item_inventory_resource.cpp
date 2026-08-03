#include "item_inventory_resource.hpp"

#include "resource_memory.hpp"
#include "retail_filesystem.hpp"

#include <cstdio>
#include <utility>

namespace osf {
namespace {

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
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

    // Release no-longer-visible sheets before decoding replacements. This
    // keeps panel transitions within the same bounded artwork budget.
    for (std::size_t index = 0; index < groups_.size(); ++index) {
        if (enabled_groups[index] == 0 && loaded_groups_[index]) {
            groups_[index] = {};
            loaded_groups_[index] = false;
        }
    }
    for (std::size_t index = 0;
         index < groups_.size();
         ++index) {
        if (enabled_groups[index] == 0 || loaded_groups_[index]) {
            continue;
        }
        char retail_path[64]{};
        std::snprintf(
            retail_path,
            sizeof(retail_path),
            "System\\Game\\Pattern\\Item%04zu.njp",
            index);
        std::string resource_error;
        if (!groups_[index].load(
                resolveRetailPath(
                    data_root_,
                    retail_path),
                &resource_error)) {
            setError(
                error,
                "An inventory item pattern could not be loaded: " +
                    std::string(retail_path) + " (" +
                    resource_error + ")");
            return false;
        }
        loaded_groups_[index] = true;
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
    loaded_groups_.fill(false);
    data_root_.clear();
}

const gapi::NjpImage* ItemInventoryResource::group(
    std::int32_t index) const {
    if (index < 0 ||
        static_cast<std::size_t>(index) >=
            groups_.size() ||
        !loaded_groups_[static_cast<std::size_t>(index)]) {
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
