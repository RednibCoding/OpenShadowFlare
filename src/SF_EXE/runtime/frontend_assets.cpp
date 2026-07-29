#include "frontend_assets.hpp"

#include "resources/retail_filesystem.hpp"

#include <cstdio>
#include <string>
#include <utility>

namespace osf::runtime {

FrontendAssets::FrontendAssets(std::filesystem::path data_root)
    : data_root_(std::move(data_root)) {}

bool FrontendAssets::loadPattern(
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
    patterns_.insert_or_assign(id, std::move(image));
    return true;
}

void FrontendAssets::releasePattern(std::int32_t id) {
    patterns_.erase(id);
}

const gapi::NjpImage* FrontendAssets::pattern(
    std::int32_t id) const {
    const auto found = patterns_.find(id);
    return found == patterns_.end() ? nullptr : &found->second;
}

bool FrontendAssets::loadTitleAnimation(
    std::size_t index,
    std::string_view retail_path) {
    if (index >= title_animations_.size()) {
        return false;
    }

    std::string error;
    const std::filesystem::path path =
        resolveRetailPath(data_root_, retail_path);
    if (!title_animations_[index].load(path, &error)) {
        std::fprintf(
            stderr,
            "Could not load %s: %s\n",
            path.string().c_str(),
            error.c_str());
        return false;
    }
    return true;
}

void FrontendAssets::releaseTitleAnimation(std::size_t index) {
    if (index < title_animations_.size()) {
        title_animations_[index].clear();
    }
}

const gapi::CafAnimation* FrontendAssets::titleAnimation(
    std::size_t index) const {
    return index < title_animations_.size()
        ? &title_animations_[index]
        : nullptr;
}

void FrontendAssets::loadSavedCharacters() {
    saved_games_ = loadRetailSaveCatalog(data_root_);
    saved_previews_.clear();
    saved_previews_.reserve(saved_games_.size());
    for (const RetailSaveSummary& save : saved_games_) {
        gapi::BitmapImage preview;
        std::string error;
        preview.load(save.preview_path, &error);
        saved_previews_.push_back(std::move(preview));
    }
}

std::int32_t FrontendAssets::savedGameCount() const {
    return static_cast<std::int32_t>(saved_games_.size());
}

const std::vector<RetailSaveSummary>&
FrontendAssets::savedGames() const {
    return saved_games_;
}

const std::vector<gapi::BitmapImage>&
FrontendAssets::savedPreviews() const {
    return saved_previews_;
}

}  // namespace osf::runtime
