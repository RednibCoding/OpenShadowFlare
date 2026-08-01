#include "effect_visual_resource.hpp"

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
    name << std::setfill('0') << std::setw(8) << resource_id;
    return name.str();
}

}  // namespace

bool EffectVisualResource::load(
    const std::filesystem::path& directory,
    std::string* error) {
    return load(directory, "Animation", error);
}

bool EffectVisualResource::load(
    const std::filesystem::path& directory,
    std::string_view stem,
    std::string* error) {
    clear();
    std::string resource_error;
    const std::string filename_stem(stem);
    if (!patterns_.load(
            directory / (filename_stem + ".Njp"),
            &resource_error) ||
        !animation_.load(
            directory / (filename_stem + ".Caf"),
            &resource_error)) {
        setError(error, resource_error);
        clear();
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

void EffectVisualResource::clear() {
    patterns_.clear();
    animation_.clear();
}

const gapi::NjpImage& EffectVisualResource::patterns() const {
    return patterns_;
}

const gapi::CafAnimation&
EffectVisualResource::animation() const {
    return animation_;
}

const EffectVisualResource* EffectVisualResources::load(
    const std::filesystem::path& data_root,
    std::int32_t resource_id,
    std::string* error) {
    if (resource_id < 0 || resource_id > 99999999) {
        setError(error, "The effect animation resource ID is invalid.");
        return nullptr;
    }
    const auto found = resources_.find(resource_id);
    if (found != resources_.end()) {
        return found->second.get();
    }

    auto resource = std::make_unique<EffectVisualResource>();
    std::string resource_error;
    if (!resource->load(
            data_root / "Character" / "OPTION" /
                resourceDirectory(resource_id),
            &resource_error)) {
        setError(
            error,
            "The effect animation could not be loaded: " +
                resource_error);
        return nullptr;
    }
    const EffectVisualResource* result = resource.get();
    resources_.emplace(resource_id, std::move(resource));
    if (error) {
        error->clear();
    }
    return result;
}

const EffectVisualResource* EffectVisualResources::find(
    std::int32_t resource_id) const {
    const auto found = resources_.find(resource_id);
    return found == resources_.end()
        ? nullptr
        : found->second.get();
}

void EffectVisualResources::clear() {
    resources_.clear();
}

}  // namespace osf
