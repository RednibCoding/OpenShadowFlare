#include "world_scene.hpp"

#include <cinttypes>
#include <cstdio>

namespace osf {

void WorldScene::beginScenarioVisual(std::int32_t visual_id) {
    char filename[32]{};
    if (visual_id == 0) {
        std::snprintf(
            filename, sizeof(filename), "Waiting.njp");
    } else {
        std::snprintf(
            filename,
            sizeof(filename),
            "Visual%02" PRId32 ".njp",
            visual_id);
    }
    const std::filesystem::path pattern_root =
        data_root_ / "System" / "Common" / "Pattern";
    scenario_visual_patterns_.load(
        pattern_root / filename, nullptr);
    scenario_visual_continue_patterns_.load(
        pattern_root / "WaitIcon.njp", nullptr);
    scenario_visual_.begin(
        visual_id,
        visual_id == 0
            ? 1
            : scenario_visual_patterns_.patterns().size());
}

}  // namespace osf
