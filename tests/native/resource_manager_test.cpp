#include "resources/resource_manager.hpp"
#include "resources/character_visual_resource.hpp"
#include "resources/font_resource.hpp"
#include "resources/item_inventory_resource.hpp"
#include "resources/effect_pattern_resource.hpp"
#include "resources/effect_visual_resource.hpp"

#include <array>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool patternDecoded(
    const osf::gapi::NjpImage& image,
    std::int32_t pattern_index) {
    if (pattern_index < 0 ||
        static_cast<std::size_t>(pattern_index) >=
            image.patterns().size() ||
        !image.patternDecoded(static_cast<std::size_t>(
            pattern_index))) {
        return false;
    }
    for (const osf::gapi::NjpPatternPart& part :
         image.patterns()[static_cast<std::size_t>(
             pattern_index)].parts) {
        if (part.part_index < 0 ||
            static_cast<std::size_t>(part.part_index) >=
                image.parts().size() ||
            !image.parts()[static_cast<std::size_t>(
                part.part_index)].hasDecodedPixels()) {
            return false;
        }
    }
    return true;
}

bool selectedAnimationPatternsDecoded(
    const osf::CharacterVisualResource& visual,
    const osf::CharacterVisualResource& reference,
    const std::vector<std::uint8_t>& enabled_parts) {
    for (const osf::gapi::CafChart& chart :
         visual.animation().charts()) {
        for (const osf::gapi::CafDirection& direction :
             chart.directions) {
            for (std::size_t part_index = 0;
                 part_index < direction.parts.size();
                 ++part_index) {
                if (part_index >= enabled_parts.size() ||
                    enabled_parts[part_index] == 0) {
                    continue;
                }
                for (const osf::gapi::CafCell& cell :
                     direction.parts[part_index]) {
                    if (cell.pattern_index < 0) {
                        continue;
                    }
                    const bool normal_required = patternDecoded(
                        reference.patterns(), cell.pattern_index);
                    const bool shadow_required =
                        (cell.status & 8) != 0 &&
                        patternDecoded(
                            reference.shadowPatterns(),
                            cell.pattern_index);
                    if ((normal_required &&
                         !patternDecoded(
                             visual.patterns(),
                             cell.pattern_index)) ||
                        (shadow_required &&
                         !patternDecoded(
                             visual.shadowPatterns(),
                             cell.pattern_index))) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

}  // namespace

int main() {
#ifndef OPENSHADOWFLARE_SOURCE_DIR
    return 0;
#else
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp/ShadowFlare";
    if (!std::filesystem::is_directory(data_root)) {
        return 0;
    }

    const std::filesystem::path female_root =
        data_root / "Player" / "Female";
    osf::CharacterVisualResource full_player;
    osf::CharacterVisualResource selected_player;
    std::string player_error;
    if (!check(
            full_player.load(
                female_root, "Animation00", &player_error) &&
                selected_player.loadAnimation(
                    female_root, "Animation00", &player_error),
            player_error.c_str())) {
        return 1;
    }
    std::vector<std::uint8_t> selected_parts(
        selected_player.animation().maxPartCount(), 0);
    for (std::size_t part : {std::size_t{0}, std::size_t{1},
                             std::size_t{5}}) {
        if (part < selected_parts.size()) {
            selected_parts[part] = 1;
        }
    }
    const bool selected_loaded =
        selected_player.loadSelectedParts(
            selected_parts, &player_error);
    const bool selected_compact =
        selected_player.memoryUsageBytes() * 2 <
            full_player.memoryUsageBytes();
    const bool selected_complete =
        selectedAnimationPatternsDecoded(
            selected_player, full_player, selected_parts);
    if (!check(
            selected_loaded && selected_compact && selected_complete,
            "Selected player layers were not decoded completely or "
            "did not reduce their allocation.")) {
        std::cerr
            << "full=" << full_player.memoryUsageBytes()
            << " selected=" << selected_player.memoryUsageBytes()
            << " loaded=" << selected_loaded
            << " complete=" << selected_complete
            << " error=" << player_error << '\n';
        return 1;
    }
    const std::uint64_t equipped_bytes =
        selected_player.memoryUsageBytes();
    if (selected_parts.size() > 5) {
        selected_parts[5] = 0;
    }
    if (!check(
            selected_player.loadSelectedParts(
                selected_parts, &player_error) &&
                selected_player.memoryUsageBytes() <
                    equipped_bytes &&
                selectedAnimationPatternsDecoded(
                    selected_player,
                    full_player,
                    selected_parts),
            "Changing equipment layers did not release unused player "
            "bitmaps.")) {
        return 1;
    }

    osf::ResourceManager resources(data_root);
    if (!check(
            resources.loadCommonPattern(
                0, "System\\Common\\Pattern\\Font00.njp"),
            "The common resource fixture could not be loaded.")) {
        return 1;
    }
    const std::uint64_t common_bytes =
        resources.memoryUsageBytes();
    if (!check(
            common_bytes > 0,
            "The common resource memory was not accounted.")) {
        return 1;
    }

    osf::ResourceManager selected_font(data_root);
    if (!check(
            selected_font.loadCommonPattern(
                0,
                "System\\Common\\Pattern\\Font00.njp",
                osf::englishRetailFontPatternSelection()) &&
                selected_font.pattern(0) &&
                selected_font.pattern(0)->patternDecoded(0) &&
                !selected_font.pattern(0)->patternDecoded(1) &&
                selected_font.pattern(0)->patternDecoded(2) &&
                selected_font.memoryUsageBytes() * 10 < common_bytes,
            "The English font sheet was not decoded selectively.")) {
        return 1;
    }
    selected_font.releaseCommonPattern(0);
    if (!check(
            selected_font.pattern(0) == nullptr &&
                selected_font.memoryUsageBytes() == 0,
            "Releasing a state-scoped common pattern retained memory.")) {
        return 1;
    }

    osf::ItemInventoryResource inventory_artwork;
    std::array<
        std::uint8_t,
        osf::ItemInventoryResource::group_count> item_groups{};
    item_groups.fill(1);
    std::string inventory_error;
    if (!check(
            inventory_artwork.load(data_root, &inventory_error) &&
                inventory_artwork.group(0) == nullptr &&
                inventory_artwork.prepareGroups(
                    item_groups, &inventory_error),
            "The lazy inventory artwork fixture could not load.")) {
        return 1;
    }
    const std::uint64_t all_item_bytes =
        inventory_artwork.memoryUsageBytes();
    item_groups.fill(0);
    item_groups[3] = 1;
    if (!check(
            inventory_artwork.prepareGroups(
                item_groups, &inventory_error) &&
                inventory_artwork.group(0) == nullptr &&
                inventory_artwork.group(3) != nullptr &&
                inventory_artwork.memoryUsageBytes() * 5 <
                    all_item_bytes,
            "Closing inventory containers retained their artwork sheets.")) {
        return 1;
    }

    osf::EffectVisualResources effect_visuals;
    if (!check(
            effect_visuals.load(data_root, 11000040) &&
                effect_visuals.load(data_root, 11000240),
            "The effect-cache fixture could not be loaded.")) {
        return 1;
    }
    const std::uint64_t all_effect_visual_bytes =
        effect_visuals.memoryUsageBytes();
    effect_visuals.retainOnly({11000240});
    if (!check(
            effect_visuals.find(11000040) == nullptr &&
                effect_visuals.find(11000240) != nullptr &&
                effect_visuals.memoryUsageBytes() <
                    all_effect_visual_bytes,
            "The effect animation cache retained an inactive resource.")) {
        return 1;
    }

    osf::EffectPatternResources effect_patterns;
    if (!check(
            effect_patterns.load(data_root, 10000020) &&
                effect_patterns.load(data_root, 11000011),
            "The static-effect cache fixture could not be loaded.")) {
        return 1;
    }
    const std::uint64_t all_effect_pattern_bytes =
        effect_patterns.memoryUsageBytes();
    effect_patterns.retainOnly({10000020});
    if (!check(
            effect_patterns.find(11000011) == nullptr &&
                effect_patterns.find(10000020) != nullptr &&
                effect_patterns.memoryUsageBytes() <
                    all_effect_pattern_bytes,
            "The static-effect cache retained an inactive resource.")) {
        return 1;
    }

    if (!check(
            resources.loadTitlePattern(
                4, "System\\Title\\Pattern\\Title.njp") &&
                resources.loadTitleAnimation(
                    0,
                    "System\\Title\\Pattern\\Smoke00.Caf") &&
                resources.pattern(4) != nullptr &&
                !resources.titleAnimation(0)->charts().empty() &&
                resources.memoryUsageBytes() > common_bytes,
            "The title resource scope could not be loaded.")) {
        return 1;
    }

    resources.releaseTitleResources();
    if (!check(
                resources.pattern(4) == nullptr &&
                resources.titleAnimation(0)->charts().empty() &&
                resources.pattern(0) != nullptr &&
                resources.memoryUsageBytes() == common_bytes,
            "Releasing the title scope retained title data or removed common data.")) {
        return 1;
    }

    if (!check(
            resources.loadCharacterSelectPattern(
                4, "System\\Select\\Pattern\\Select.njp") &&
                resources.pattern(4) != nullptr,
            "The character-select resource scope could not be loaded.")) {
        return 1;
    }
    resources.loadSavedCharacters();
    resources.releaseCharacterSelectResources();
    if (!check(
            resources.pattern(4) == nullptr &&
                resources.savedGameCount() == 0 &&
                resources.savedGames().empty() &&
                resources.savedPreviews().empty() &&
                resources.pattern(0) != nullptr,
            "Releasing character select retained saves, previews, or patterns.")) {
        return 1;
    }

    if (!check(
            resources.loadGameplayPattern(
                5, "System\\Game\\Pattern\\Bar.njp") &&
                resources.pattern(5) != nullptr &&
                resources.prepareGameplayPattern(
                    6,
                    "System\\Game\\Pattern\\Status.njp",
                    true) &&
                resources.pattern(6) != nullptr &&
                resources.prepareGameplayPattern(
                    6,
                    "System\\Game\\Pattern\\Status.njp",
                    false) &&
                resources.pattern(6) == nullptr,
            "The gameplay resource scope could not be loaded.")) {
        return 1;
    }
    std::vector<std::uint8_t> status_selection(121, 0);
    status_selection[5] = 1;
    if (!check(
            resources.prepareGameplayPattern(
                6,
                "System\\Game\\Pattern\\Status.njp",
                status_selection,
                true) &&
                resources.pattern(6) &&
                resources.pattern(6)->patternDecoded(5) &&
                !resources.pattern(6)->patternDecoded(2),
            "A gameplay panel did not replace its full sheet with the "
            "requested pattern selection.")) {
        return 1;
    }
    resources.releaseGameplayPattern(6);
    resources.releaseGameplayResources();
    return check(
               resources.pattern(5) == nullptr &&
                   resources.pattern(0) != nullptr,
               "Releasing gameplay retained its patterns or removed common data.")
        ? 0
        : 1;
#endif
}
