#include "item_inventory_resource.hpp"

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
    for (std::size_t index = 0;
         index < groups_.size();
         ++index) {
        char retail_path[64]{};
        std::snprintf(
            retail_path,
            sizeof(retail_path),
            "System\\Game\\Pattern\\Item%04zu.njp",
            index);
        std::string resource_error;
        if (!groups_[index].load(
                resolveRetailPath(
                    data_root,
                    retail_path),
                &resource_error)) {
            setError(
                error,
                "An inventory item pattern could not be loaded: " +
                    std::string(retail_path) + " (" +
                    resource_error + ")");
            clear();
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
        group.clear();
    }
}

const gapi::NjpImage* ItemInventoryResource::group(
    std::int32_t index) const {
    if (index < 0 ||
        static_cast<std::size_t>(index) >=
            groups_.size()) {
        return nullptr;
    }
    return &groups_[static_cast<std::size_t>(index)];
}

}  // namespace osf
