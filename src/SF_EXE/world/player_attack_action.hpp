#ifndef OPENSHADOWFLARE_PLAYER_ATTACK_ACTION_HPP
#define OPENSHADOWFLARE_PLAYER_ATTACK_ACTION_HPP

#include <cstdint>
#include <vector>

namespace osf {

class TableData;
struct ItemDefinition;

namespace gapi {
class CafAnimation;
}

enum class PlayerAttackAction : std::int32_t {
    basic = 7,
    weapon_8 = 8,
    weapon_9 = 9,
    weapon_10 = 10,
    ranged_19 = 19,
    ranged_20 = 20,
};

struct PlayerAttackAnimationTiming {
    std::int32_t first_chart = -1;
    std::int32_t recovery_chart = -1;
    std::int32_t first_frame_count = 0;
    std::int32_t recovery_frame_count = 0;
    std::vector<std::int16_t> first_frame_statuses;
};

struct PlayerAttackActionEvent {
    PlayerAttackAction action = PlayerAttackAction::basic;
    std::int32_t target_id = -1;
    bool impact_due = false;
    bool swing_sound_due = false;
    bool completed = false;
};

PlayerAttackAction retailPlayerAttackAction(
    const ItemDefinition* main_hand);
bool playerAttackActionIsSupported(PlayerAttackAction action);
bool buildPlayerAttackAnimationTiming(
    const gapi::CafAnimation& animation,
    PlayerAttackAction action,
    std::int32_t direction,
    PlayerAttackAnimationTiming& timing);
std::int32_t retailPlayerAttackSpeedTier(
    std::int32_t derived_attack_speed,
    std::int32_t equipped_weight,
    std::int32_t weight_capacity,
    const TableData* speed_table);
class PlayerAttackActionController {
public:
    bool start(
        PlayerAttackAction action,
        std::int32_t target_id,
        std::int32_t attack_speed_tier,
        PlayerAttackAnimationTiming timing,
        PlayerAttackActionEvent* event = nullptr);
    PlayerAttackActionEvent update(
        std::int32_t attack_speed_tier = -1);
    void cancel();

    bool active() const;
    PlayerAttackAction action() const;
    std::int32_t targetId() const;
    std::int32_t animationChart() const;
    std::int32_t animationFrame() const;
    std::int32_t actionCounter() const;
    std::int32_t displayedFrame() const;

private:
    PlayerAttackActionEvent eventForCurrentFrame();
    void selectRenderedFrame();

    PlayerAttackAnimationTiming timing_;
    PlayerAttackAction action_ = PlayerAttackAction::basic;
    std::int32_t target_id_ = -1;
    std::int32_t attack_speed_tier_ = 0;
    std::int32_t action_counter_ = 0;
    std::int32_t displayed_frame_ = 0;
    std::int32_t previous_scanned_frame_ = -1;
    std::int32_t animation_chart_ = 0;
    std::int32_t animation_frame_ = 0;
    bool active_ = false;
};

}  // namespace osf

#endif
