#ifndef OPENSHADOWFLARE_COMPANION_ATTACK_ACTION_HPP
#define OPENSHADOWFLARE_COMPANION_ATTACK_ACTION_HPP

#include <cstdint>
#include <vector>

namespace osf {

namespace gapi {
class CafAnimation;
}

struct CompanionAttackAnimationTiming {
    std::int32_t frame_count = 0;
    std::vector<std::int16_t> frame_statuses;
};

struct CompanionAttackActionEvent {
    bool impact_due = false;
    bool swing_sound_due = false;
    bool completed = false;
};

bool buildCompanionAttackAnimationTiming(
    const gapi::CafAnimation& animation,
    std::int32_t direction,
    CompanionAttackAnimationTiming& timing);

std::int32_t retailCompanionAttackSpeedTier(
    std::int32_t attack_speed_rating);

class CompanionAttackActionController {
public:
    bool start(
        std::int32_t attack_speed_rating,
        CompanionAttackAnimationTiming timing);
    CompanionAttackActionEvent update();
    void cancel();

    bool active() const;
    std::int32_t animationFrame() const;
    std::int32_t actionCounter() const;

private:
    CompanionAttackAnimationTiming timing_;
    std::int32_t attack_speed_tier_ = 0;
    std::int32_t action_counter_ = 0;
    std::int32_t animation_frame_ = 0;
    std::int32_t previous_scanned_frame_ = -1;
    bool active_ = false;
};

}  // namespace osf

#endif
