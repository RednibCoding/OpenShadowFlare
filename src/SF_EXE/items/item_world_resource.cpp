#include "item_world_resource.hpp"

#include <iomanip>
#include <sstream>
#include <utility>

namespace osf {
namespace {

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

std::string resourceDirectory(std::int32_t resource_id) {
    std::ostringstream name;
    name << std::setw(8) << std::setfill('0') << resource_id;
    return name.str();
}

}  // namespace

bool ItemWorldResource::load(
    const std::filesystem::path& data_root,
    std::int32_t resource_id,
    std::string* error) {
    clear();
    if (resource_id < 0) {
        setError(error, "The ground-item resource ID is invalid.");
        return false;
    }
    const std::filesystem::path root =
        data_root / "Character" / "ITEM" /
        resourceDirectory(resource_id);
    std::string resource_error;
    if (!patterns_.load(
            root / "Animation.Njp", &resource_error) ||
        !shadow_patterns_.load(
            root / "Animation.Sdw", &resource_error) ||
        !animation_.load(
            root / "Animation.Caf", &resource_error)) {
        setError(
            error,
            "The ground-item animation could not be loaded: " +
                resource_error);
        clear();
        return false;
    }
    id_ = resource_id;
    return true;
}

void ItemWorldResource::clear() {
    id_ = -1;
    patterns_.clear();
    shadow_patterns_.clear();
    animation_.clear();
}

std::int32_t ItemWorldResource::id() const {
    return id_;
}

const gapi::NjpImage& ItemWorldResource::patterns() const {
    return patterns_;
}

const gapi::NjpImage&
ItemWorldResource::shadowPatterns() const {
    return shadow_patterns_;
}

const gapi::CafAnimation& ItemWorldResource::animation() const {
    return animation_;
}

}  // namespace osf
