#include "resource_manager.hpp"

#include "resource_memory.hpp"
#include "resources/retail_filesystem.hpp"

#include <cstdio>
#include <string>
#include <utility>

namespace osf {
namespace {

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

ResourceManager::ResourceManager(std::filesystem::path data_root)
    : data_root_(std::move(data_root)) {}

bool ResourceManager::loadCommonPattern(
    std::int32_t id,
    std::string_view retail_path) {
    return loadPattern(common_patterns_, id, retail_path);
}

bool ResourceManager::loadCommonPattern(
    std::int32_t id,
    std::string_view retail_path,
    const std::vector<std::uint8_t>& enabled_patterns) {
    return loadPattern(
        common_patterns_, id, retail_path, enabled_patterns);
}

void ResourceManager::releaseCommonPattern(std::int32_t id) {
    common_patterns_.erase(id);
}

bool ResourceManager::loadTitlePattern(
    std::int32_t id,
    std::string_view retail_path) {
    return loadPattern(title_.patterns, id, retail_path);
}

void ResourceManager::releaseTitlePattern(std::int32_t id) {
    title_.patterns.erase(id);
}

bool ResourceManager::loadTitleAnimation(
    std::size_t index,
    std::string_view retail_path) {
    if (index >= title_.animations.size()) {
        return false;
    }

    std::string error;
    const std::filesystem::path path =
        resolveRetailPath(data_root_, retail_path);
    if (!title_.animations[index].load(path, &error)) {
        std::fprintf(
            stderr,
            "Could not load %s: %s\n",
            path.string().c_str(),
            error.c_str());
        return false;
    }
    return true;
}

void ResourceManager::releaseTitleAnimation(std::size_t index) {
    if (index < title_.animations.size()) {
        title_.animations[index].clear();
    }
}

void ResourceManager::releaseTitleResources() {
    title_ = {};
}

bool ResourceManager::loadCharacterSelectPattern(
    std::int32_t id,
    std::string_view retail_path) {
    return loadPattern(character_select_.patterns, id, retail_path);
}

void ResourceManager::releaseCharacterSelectPattern(
    std::int32_t id) {
    character_select_.patterns.erase(id);
}

void ResourceManager::loadSavedCharacters() {
    character_select_.saved_games =
        loadRetailSaveCatalog(data_root_);
    character_select_.saved_previews.clear();
    character_select_.saved_previews.reserve(
        character_select_.saved_games.size());
    for (const RetailSaveSummary& save :
         character_select_.saved_games) {
        gapi::BitmapImage preview;
        std::string error;
        preview.load(save.preview_path, &error);
        character_select_.saved_previews.push_back(
            std::move(preview));
    }
}

void ResourceManager::releaseCharacterSelectResources() {
    character_select_ = {};
}

bool ResourceManager::loadGameplayPattern(
    std::int32_t id,
    std::string_view retail_path) {
    return loadPattern(gameplay_patterns_, id, retail_path);
}

bool ResourceManager::prepareGameplayPattern(
    std::int32_t id,
    std::string_view retail_path,
    bool required) {
    if (!required) {
        releaseGameplayPattern(id);
        return true;
    }
    if (findPattern(gameplay_patterns_, id)) {
        return true;
    }
    return loadGameplayPattern(id, retail_path);
}

bool ResourceManager::prepareGameplayPattern(
    std::int32_t id,
    std::string_view retail_path,
    const std::vector<std::uint8_t>& enabled_patterns,
    bool required) {
    if (!required) {
        releaseGameplayPattern(id);
        return true;
    }
    const gapi::NjpImage* current =
        findPattern(gameplay_patterns_, id);
    if (current &&
        matchesPatternSelection(*current, enabled_patterns)) {
        return true;
    }
    return loadPattern(
        gameplay_patterns_, id, retail_path, enabled_patterns);
}

void ResourceManager::releaseGameplayPattern(std::int32_t id) {
    gameplay_patterns_.erase(id);
}

void ResourceManager::releaseGameplayResources() {
    gameplay_patterns_ = {};
}

const gapi::NjpImage* ResourceManager::pattern(
    std::int32_t id) const {
    if (const auto* image = findPattern(gameplay_patterns_, id)) {
        return image;
    }
    if (const auto* image =
            findPattern(character_select_.patterns, id)) {
        return image;
    }
    if (const auto* image = findPattern(title_.patterns, id)) {
        return image;
    }
    return findPattern(common_patterns_, id);
}

const gapi::CafAnimation* ResourceManager::titleAnimation(
    std::size_t index) const {
    return index < title_.animations.size()
        ? &title_.animations[index]
        : nullptr;
}

std::int32_t ResourceManager::savedGameCount() const {
    return static_cast<std::int32_t>(
        character_select_.saved_games.size());
}

const std::vector<RetailSaveSummary>&
ResourceManager::savedGames() const {
    return character_select_.saved_games;
}

const std::vector<gapi::BitmapImage>&
ResourceManager::savedPreviews() const {
    return character_select_.saved_previews;
}

std::uint64_t ResourceManager::memoryUsageBytes() const {
    const auto pattern_bytes = [](const PatternMap& patterns) {
        std::uint64_t bytes = 0;
        for (const auto& entry : patterns) {
            bytes += decodedMemoryUsageBytes(entry.second);
        }
        return bytes;
    };

    std::uint64_t bytes = pattern_bytes(common_patterns_) +
        pattern_bytes(title_.patterns) +
        pattern_bytes(character_select_.patterns) +
        pattern_bytes(gameplay_patterns_);
    for (const gapi::CafAnimation& animation : title_.animations) {
        bytes += decodedMemoryUsageBytes(animation);
    }
    for (const gapi::BitmapImage& preview :
         character_select_.saved_previews) {
        bytes += decodedMemoryUsageBytes(preview);
    }
    return bytes;
}

bool ResourceManager::loadPattern(
    PatternMap& patterns,
    std::int32_t id,
    std::string_view retail_path) {
    gapi::NjpImage image;
    std::string error;
    const std::filesystem::path path =
        resolveRetailPath(data_root_, retail_path);
    if (!image.load(path, &error)) {
        std::fprintf(
            stderr,
            "Could not load %s: %s\n",
            path.string().c_str(),
            error.c_str());
        return false;
    }
    patterns.insert_or_assign(id, std::move(image));
    return true;
}

bool ResourceManager::loadPattern(
    PatternMap& patterns,
    std::int32_t id,
    std::string_view retail_path,
    const std::vector<std::uint8_t>& enabled_patterns) {
    gapi::NjpImage image;
    std::string error;
    const std::filesystem::path path =
        resolveRetailPath(data_root_, retail_path);
    if (!image.loadSelectedPatterns(
            path, enabled_patterns, &error)) {
        std::fprintf(
            stderr,
            "Could not load %s: %s\n",
            path.string().c_str(),
            error.c_str());
        return false;
    }
    patterns.insert_or_assign(id, std::move(image));
    return true;
}

const gapi::NjpImage* ResourceManager::findPattern(
    const PatternMap& patterns,
    std::int32_t id) const {
    const auto found = patterns.find(id);
    return found == patterns.end() ? nullptr : &found->second;
}

}  // namespace osf
