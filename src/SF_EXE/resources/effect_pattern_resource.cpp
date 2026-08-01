#include "effect_pattern_resource.hpp"

#include "retail_filesystem.hpp"

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

const gapi::NjpImage* EffectPatternResources::load(
    const std::filesystem::path& data_root,
    std::int32_t resource_id,
    std::string* error) {
    if (resource_id < 0 || resource_id > 99999999) {
        setError(
            error,
            "The static effect pattern resource ID is invalid.");
        return nullptr;
    }
    const auto found = resources_.find(resource_id);
    if (found != resources_.end()) {
        return found->second.get();
    }

    auto patterns = std::make_unique<gapi::NjpImage>();
    std::string resource_error;
    const std::filesystem::path directory =
        data_root / "Character" / "OPTION" /
        resourceDirectory(resource_id);
    if (!patterns->load(
            resolveRetailPath(directory, "Pattern.Njp"),
            &resource_error)) {
        setError(
            error,
            "The static effect pattern could not be loaded: " +
                resource_error);
        return nullptr;
    }
    const gapi::NjpImage* result = patterns.get();
    resources_.emplace(resource_id, std::move(patterns));
    if (error) {
        error->clear();
    }
    return result;
}

const gapi::NjpImage* EffectPatternResources::find(
    std::int32_t resource_id) const {
    const auto found = resources_.find(resource_id);
    return found == resources_.end()
        ? nullptr
        : found->second.get();
}

void EffectPatternResources::clear() {
    resources_.clear();
}

}  // namespace osf
