#include "world_scene.hpp"

namespace osf {

bool WorldScene::switchOwnedCompanion(std::int32_t type) {
    if (!has_player_ || data_root_.empty()) {
        return false;
    }

    CompanionProfile base_profile;
    if (!decodeCompanionProfile(
            parameter_tables_,
            type,
            player_data_.companionLevel(type),
            base_profile)) {
        return false;
    }
    const CharacterVisualResource* visual =
        companion_visuals_.load(
            data_root_, base_profile.resource_id);
    if (!visual || !player_data_.switchCompanion(type)) {
        return false;
    }

    const CompanionProfile runtime_profile =
        applyPlayerMoonCompanionModifiers(
            base_profile,
            player_moon_spell_,
            parameter_tables_);
    return companion_.initialize(
        runtime_profile,
        *visual,
        scenario_world_.localPlayerNumber(),
        player_.position(),
        player_.direction());
}

}  // namespace osf
