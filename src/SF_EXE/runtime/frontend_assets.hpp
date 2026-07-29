#pragma once

#include "libs/RKC_DIB/rkc_dib.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "resources/save_catalog.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace osf::runtime {

class FrontendAssets {
public:
    explicit FrontendAssets(std::filesystem::path data_root);

    bool loadPattern(
        std::int32_t id,
        std::string_view retail_path);
    void releasePattern(std::int32_t id);
    const gapi::NjpImage* pattern(std::int32_t id) const;

    bool loadTitleAnimation(
        std::size_t index,
        std::string_view retail_path);
    void releaseTitleAnimation(std::size_t index);
    const gapi::CafAnimation* titleAnimation(
        std::size_t index) const;

    void loadSavedCharacters();
    std::int32_t savedGameCount() const;
    const std::vector<RetailSaveSummary>& savedGames() const;
    const std::vector<gapi::BitmapImage>& savedPreviews() const;

private:
    std::filesystem::path data_root_;
    std::unordered_map<std::int32_t, gapi::NjpImage> patterns_;
    std::array<gapi::CafAnimation, 10> title_animations_;
    std::vector<RetailSaveSummary> saved_games_;
    std::vector<gapi::BitmapImage> saved_previews_;
};

}  // namespace osf::runtime
