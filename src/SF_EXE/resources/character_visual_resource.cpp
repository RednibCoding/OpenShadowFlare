#include "character_visual_resource.hpp"

#include "resource_memory.hpp"

#include <algorithm>
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

struct SelectedPatterns {
    std::vector<std::uint8_t> normal;
    std::vector<std::uint8_t> shadow;
};

void enablePattern(
    std::vector<std::uint8_t>& patterns,
    std::int32_t pattern_index) {
    if (pattern_index < 0) {
        return;
    }
    const std::size_t index =
        static_cast<std::size_t>(pattern_index);
    if (patterns.size() <= index) {
        patterns.resize(index + 1, 0);
    }
    patterns[index] = 1;
}

SelectedPatterns patternsForParts(
    const gapi::CafAnimation& animation,
    const std::vector<std::uint8_t>& enabled_parts) {
    SelectedPatterns selected;
    for (const gapi::CafChart& chart : animation.charts()) {
        for (const gapi::CafDirection& direction :
             chart.directions) {
            const std::size_t part_count = std::min(
                direction.parts.size(), enabled_parts.size());
            for (std::size_t part_index = 0;
                 part_index < part_count;
                 ++part_index) {
                if (enabled_parts[part_index] == 0) {
                    continue;
                }
                for (const gapi::CafCell& cell :
                     direction.parts[part_index]) {
                    enablePattern(
                        selected.normal, cell.pattern_index);
                    if ((cell.status & 8) != 0) {
                        enablePattern(
                            selected.shadow, cell.pattern_index);
                    }
                }
            }
        }
    }
    return selected;
}

}  // namespace

bool CharacterVisualResource::load(
    const std::filesystem::path& directory,
    const std::string& stem,
    std::string* error) {
    clear();
    std::string resource_error;
    if (!animation_.load(
            directory / (stem + ".Caf"), &resource_error) ||
        !patterns_.load(
            directory / (stem + ".Njp"), &resource_error) ||
        !shadow_patterns_.load(
            directory / (stem + ".Sdw"), &resource_error)) {
        setError(error, resource_error);
        clear();
        return false;
    }
    directory_ = directory;
    stem_ = stem;
    selected_parts_.assign(animation_.maxPartCount(), 1);
    patterns_loaded_ = true;
    if (error) {
        error->clear();
    }
    return true;
}

bool CharacterVisualResource::loadAnimation(
    const std::filesystem::path& directory,
    const std::string& stem,
    std::string* error) {
    clear();
    if (!animation_.load(directory / (stem + ".Caf"), error)) {
        clear();
        return false;
    }
    directory_ = directory;
    stem_ = stem;
    if (error) {
        error->clear();
    }
    return true;
}

bool CharacterVisualResource::loadSelectedParts(
    const std::vector<std::uint8_t>& enabled_parts,
    std::string* error) {
    if (directory_.empty() || stem_.empty()) {
        setError(error, "The character animation source is not loaded.");
        return false;
    }
    std::vector<std::uint8_t> selection(
        animation_.maxPartCount(), 0);
    std::copy_n(
        enabled_parts.begin(),
        std::min(enabled_parts.size(), selection.size()),
        selection.begin());
    if (patterns_loaded_ && selection == selected_parts_) {
        if (error) {
            error->clear();
        }
        return true;
    }

    const SelectedPatterns selected =
        patternsForParts(animation_, selection);
    patterns_.clear();
    shadow_patterns_.clear();
    selected_parts_.clear();
    patterns_loaded_ = false;
    std::string resource_error;
    if (!patterns_.loadSelectedPatterns(
            directory_ / (stem_ + ".Njp"),
            selected.normal,
            &resource_error) ||
        !shadow_patterns_.loadSelectedPatterns(
            directory_ / (stem_ + ".Sdw"),
            selected.shadow,
            &resource_error)) {
        patterns_.clear();
        shadow_patterns_.clear();
        setError(error, resource_error);
        return false;
    }
    selected_parts_ = std::move(selection);
    patterns_loaded_ = true;
    if (error) {
        error->clear();
    }
    return true;
}

void CharacterVisualResource::clear() {
    patterns_.clear();
    shadow_patterns_.clear();
    animation_.clear();
    directory_.clear();
    stem_.clear();
    selected_parts_.clear();
    patterns_loaded_ = false;
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

std::uint64_t CharacterVisualResource::memoryUsageBytes() const {
    return decodedMemoryUsageBytes(patterns_) +
        decodedMemoryUsageBytes(shadow_patterns_) +
        decodedMemoryUsageBytes(animation_);
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

std::uint64_t CharacterVisualResources::memoryUsageBytes() const {
    std::uint64_t bytes = 0;
    for (const auto& entry : resources_) {
        bytes += entry.second->memoryUsageBytes();
    }
    return bytes;
}

}  // namespace osf
