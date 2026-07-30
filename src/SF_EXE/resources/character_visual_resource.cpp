#include "character_visual_resource.hpp"

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

bool CharacterVisualResource::load(
    const std::filesystem::path& directory,
    const std::string& stem,
    std::string* error) {
    clear();
    std::string resource_error;
    if (!patterns_.load(
            directory / (stem + ".Njp"), &resource_error) ||
        !shadow_patterns_.load(
            directory / (stem + ".Sdw"), &resource_error) ||
        !animation_.load(
            directory / (stem + ".Caf"), &resource_error)) {
        setError(error, resource_error);
        clear();
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

void CharacterVisualResource::clear() {
    patterns_.clear();
    shadow_patterns_.clear();
    animation_.clear();
}

const gapi::NjpImage&
CharacterVisualResource::patterns() const {
    return patterns_;
}

const gapi::NjpImage&
CharacterVisualResource::shadowPatterns() const {
    return shadow_patterns_;
}

const gapi::CafAnimation&
CharacterVisualResource::animation() const {
    return animation_;
}

CharacterVisualResources::CharacterVisualResources(
    std::string category)
    : category_(std::move(category)) {}

const CharacterVisualResource* CharacterVisualResources::load(
    const std::filesystem::path& data_root,
    std::int32_t resource_id,
    std::string* error) {
    if (resource_id < 0 || resource_id > 99999999) {
        setError(
            error,
            "The " + category_ +
                " animation resource ID is invalid.");
        return nullptr;
    }
    const auto found = resources_.find(resource_id);
    if (found != resources_.end()) {
        return found->second.get();
    }

    auto resource = std::make_unique<CharacterVisualResource>();
    std::string resource_error;
    if (!resource->load(
            data_root / "Character" / category_ /
                resourceDirectory(resource_id),
            "Animation",
            &resource_error)) {
        setError(
            error,
            "The " + category_ +
                " animation could not be loaded: " +
                resource_error);
        return nullptr;
    }
    const CharacterVisualResource* result = resource.get();
    resources_.emplace(resource_id, std::move(resource));
    if (error) {
        error->clear();
    }
    return result;
}

const CharacterVisualResource* CharacterVisualResources::find(
    std::int32_t resource_id) const {
    const auto found = resources_.find(resource_id);
    return found == resources_.end()
               ? nullptr
               : found->second.get();
}

void CharacterVisualResources::clear() {
    resources_.clear();
}

}  // namespace osf
