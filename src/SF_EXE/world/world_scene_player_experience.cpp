#include "world_scene.hpp"

namespace osf {

void WorldScene::presentPlayerLevelUp(
    const PlayerLevelUpResult& result) {
    if (!result.level_gained) {
        return;
    }
    level_up_notice_ = {
        result.notice,
        result.notice_counter,
    };
    pending_audio_samples_.insert(
        pending_audio_samples_.end(),
        result.audio_samples.begin(),
        result.audio_samples.end());
    refreshPlayerRuntimeProfile();
    refreshPlayerAppearance();
}

}  // namespace osf
