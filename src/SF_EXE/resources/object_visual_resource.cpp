#include "object_visual_resource.hpp"

#include "resource_memory.hpp"
#include "retail_filesystem.hpp"

#include <iomanip>
#include <sstream>
#include <system_error>
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
    name << std::setfill('0') << std::setw(8) << resource_id;
    return name.str();
}

bool fileExists(const std::filesystem::path& path) {
    std::error_code error;
    return std::filesystem::is_regular_file(path, error);
}

}  // namespace

bool ObjectVisualResource::load(
    const std::filesystem::path& data_root,
    std::int32_t resource_id,
    std::string* error) {
    clear();
    if (resource_id < 0 || resource_id > 99999999) {
        setError(error, "The object visual resource ID is invalid.");
        return false;
    }

    const std::filesystem::path directory =
        data_root / "Character" / "OBJECT" /
        resourceDirectory(resource_id);
    const std::filesystem::path static_patterns_path =
        resolveRetailPath(directory, "Pattern.Njp");
    const std::filesystem::path static_shadows_path =
        resolveRetailPath(directory, "Pattern.Sdw");
    const std::filesystem::path animation_patterns_path =
        resolveRetailPath(directory, "Animation.Njp");
    const std::filesystem::path animation_path =
        resolveRetailPath(directory, "Animation.Caf");

    std::string resource_error;
    if (fileExists(static_patterns_path)) {
        if (!static_patterns_.load(
                static_patterns_path, &resource_error)) {
            setError(error, resource_error);
            clear();
            return false;
        }
        has_static_patterns_ = true;
    }
    if (fileExists(static_shadows_path)) {
        if (!has_static_patterns_) {
            setError(
                error,
                "The object has static shadows but no static patterns.");
            clear();
            return false;
        }
        if (!static_shadows_.load(
                static_shadows_path, &resource_error)) {
            setError(error, resource_error);
            clear();
            return false;
        }
        has_static_shadows_ = true;
    }

    const bool has_animation_patterns =
        fileExists(animation_patterns_path);
    const bool has_animation_chart = fileExists(animation_path);
    if (has_animation_patterns != has_animation_chart) {
        setError(
            error,
            "The object animation patterns and chart must both exist.");
        clear();
        return false;
    }
    if (has_animation_patterns) {
        if (!animation_patterns_.load(
                animation_patterns_path, &resource_error) ||
            !animation_.load(animation_path, &resource_error)) {
            setError(error, resource_error);
            clear();
            return false;
        }
        has_animation_ = true;
    }

    if (!has_static_patterns_ && !has_animation_) {
        setError(error, "The object visual resource is empty.");
        clear();
        return false;
    }
    id_ = resource_id;
    if (error) {
        error->clear();
    }
    return true;
}

void ObjectVisualResource::clear() {
    id_ = -1;
    has_static_patterns_ = false;
    has_static_shadows_ = false;
    has_animation_ = false;
    static_patterns_.clear();
    static_shadows_.clear();
    animation_patterns_.clear();
    animation_.clear();
}

std::int32_t ObjectVisualResource::id() const {
    return id_;
}

bool ObjectVisualResource::hasStaticPatterns() const {
    return has_static_patterns_;
}

bool ObjectVisualResource::hasStaticShadows() const {
    return has_static_shadows_;
}

bool ObjectVisualResource::hasAnimation() const {
    return has_animation_;
}

const gapi::NjpImage&
ObjectVisualResource::staticPatterns() const {
    return static_patterns_;
}

const gapi::NjpImage&
ObjectVisualResource::staticShadows() const {
    return static_shadows_;
}

const gapi::NjpImage&
ObjectVisualResource::animationPatterns() const {
    return animation_patterns_;
}

const gapi::CafAnimation&
ObjectVisualResource::animation() const {
    return animation_;
}

std::uint64_t ObjectVisualResource::memoryUsageBytes() const {
    return decodedMemoryUsageBytes(static_patterns_) +
        decodedMemoryUsageBytes(static_shadows_) +
        decodedMemoryUsageBytes(animation_patterns_) +
        decodedMemoryUsageBytes(animation_);
}

const ObjectVisualResource* ObjectVisualResources::load(
    const std::filesystem::path& data_root,
    std::int32_t resource_id,
    std::string* error) {
    const auto found = resources_.find(resource_id);
    if (found != resources_.end()) {
        return found->second.get();
    }

    auto resource = std::make_unique<ObjectVisualResource>();
    std::string resource_error;
    if (!resource->load(
            data_root, resource_id, &resource_error)) {
        setError(
            error,
            "The scenario object visual could not be loaded: " +
                resource_error);
        return nullptr;
    }
    const ObjectVisualResource* result = resource.get();
    resources_.emplace(resource_id, std::move(resource));
    if (error) {
        error->clear();
    }
    return result;
}

const ObjectVisualResource* ObjectVisualResources::find(
    std::int32_t resource_id) const {
    const auto found = resources_.find(resource_id);
    return found == resources_.end()
               ? nullptr
               : found->second.get();
}

void ObjectVisualResources::clear() {
    resources_.clear();
}

std::uint64_t ObjectVisualResources::memoryUsageBytes() const {
    std::uint64_t bytes = 0;
    for (const auto& entry : resources_) {
        bytes += entry.second->memoryUsageBytes();
    }
    return bytes;
}

}  // namespace osf
