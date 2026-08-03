#ifndef OPENSHADOWFLARE_RESOURCES_RESOURCE_MANAGER_HPP
#define OPENSHADOWFLARE_RESOURCES_RESOURCE_MANAGER_HPP

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

namespace osf {

class ResourceManager {
public:
    explicit ResourceManager(std::filesystem::path data_root);

    bool loadCommonPattern(
        std::int32_t id,
        std::string_view retail_path);
    bool loadCommonPattern(
        std::int32_t id,
        std::string_view retail_path,
        const std::vector<std::uint8_t>& enabled_patterns);
    void releaseCommonPattern(std::int32_t id);

    bool loadTitlePattern(
        std::int32_t id,
        std::string_view retail_path);
    void releaseTitlePattern(std::int32_t id);
    bool loadTitleAnimation(
        std::size_t index,
        std::string_view retail_path);
    void releaseTitleAnimation(std::size_t index);
    void releaseTitleResources();

    bool loadCharacterSelectPattern(
        std::int32_t id,
        std::string_view retail_path);
    void releaseCharacterSelectPattern(std::int32_t id);
    void loadSavedCharacters();
    void releaseCharacterSelectResources();

    bool loadGameplayPattern(
        std::int32_t id,
        std::string_view retail_path);
    bool prepareGameplayPattern(
        std::int32_t id,
        std::string_view retail_path,
        bool required);
    bool prepareGameplayPattern(
        std::int32_t id,
        std::string_view retail_path,
        const std::vector<std::uint8_t>& enabled_patterns,
        bool required);
    void releaseGameplayPattern(std::int32_t id);
    void releaseGameplayResources();

    const gapi::NjpImage* pattern(std::int32_t id) const;
    const gapi::CafAnimation* titleAnimation(
        std::size_t index) const;
    std::int32_t savedGameCount() const;
    const std::vector<RetailSaveSummary>& savedGames() const;
    const std::vector<gapi::BitmapImage>& savedPreviews() const;
    std::uint64_t memoryUsageBytes() const;

private:
    using PatternMap =
        std::unordered_map<std::int32_t, gapi::NjpImage>;

    struct TitleResources {
        PatternMap patterns;
        std::array<gapi::CafAnimation, 10> animations;
    };

    struct CharacterSelectResources {
        PatternMap patterns;
        std::vector<RetailSaveSummary> saved_games;
        std::vector<gapi::BitmapImage> saved_previews;
    };

    bool loadPattern(
        PatternMap& patterns,
        std::int32_t id,
        std::string_view retail_path);
    bool loadPattern(
        PatternMap& patterns,
        std::int32_t id,
        std::string_view retail_path,
        const std::vector<std::uint8_t>& enabled_patterns);
    const gapi::NjpImage* findPattern(
        const PatternMap& patterns,
        std::int32_t id) const;

    std::filesystem::path data_root_;
    PatternMap common_patterns_;
    TitleResources title_;
    CharacterSelectResources character_select_;
    PatternMap gameplay_patterns_;
};

}  // namespace osf

#endif
