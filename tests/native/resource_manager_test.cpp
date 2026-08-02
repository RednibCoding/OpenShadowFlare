#include "resources/resource_manager.hpp"

#include <filesystem>
#include <iostream>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
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

    osf::ResourceManager resources(data_root);
    if (!check(
            resources.loadCommonPattern(
                0, "System\\Common\\Pattern\\Font00.njp"),
            "The common resource fixture could not be loaded.")) {
        return 1;
    }

    if (!check(
            resources.loadTitlePattern(
                4, "System\\Title\\Pattern\\Title.njp") &&
                resources.loadTitleAnimation(
                    0,
                    "System\\Title\\Pattern\\Smoke00.Caf") &&
                resources.pattern(4) != nullptr &&
                !resources.titleAnimation(0)->charts().empty(),
            "The title resource scope could not be loaded.")) {
        return 1;
    }

    resources.releaseTitleResources();
    if (!check(
            resources.pattern(4) == nullptr &&
                resources.titleAnimation(0)->charts().empty() &&
                resources.pattern(0) != nullptr,
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
                resources.pattern(5) != nullptr,
            "The gameplay resource scope could not be loaded.")) {
        return 1;
    }
    resources.releaseGameplayResources();
    return check(
               resources.pattern(5) == nullptr &&
                   resources.pattern(0) != nullptr,
               "Releasing gameplay retained its patterns or removed common data.")
        ? 0
        : 1;
#endif
}
