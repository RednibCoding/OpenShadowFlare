#ifndef OPENSHADOWFLARE_PLAYER_ATTACK_TARGET_HPP
#define OPENSHADOWFLARE_PLAYER_ATTACK_TARGET_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>

namespace osf {

constexpr std::int32_t kRetailPlayerAttackRange = 0x9f;

enum class PlayerAttackTargetDisposition {
    rejected,
    approach,
    ready,
};

struct PlayerAttackTargetSnapshot {
    std::int32_t id = -1;
    WorldPosition position;
    ObjectBounds judgement;
    std::int32_t life = 0;
    bool visible = false;
    bool pointer_enabled = false;
};

PlayerAttackTargetDisposition classifyPlayerAttackTarget(
    WorldPosition player_position,
    const ObjectBounds& player_judgement,
    const PlayerAttackTargetSnapshot& target,
    std::int32_t attack_range = kRetailPlayerAttackRange);

class PlayerAttackTargetController {
public:
    PlayerAttackTargetDisposition command(
        WorldPosition player_position,
        const ObjectBounds& player_judgement,
        const PlayerAttackTargetSnapshot& target);
    PlayerAttackTargetDisposition refresh(
        WorldPosition player_position,
        const ObjectBounds& player_judgement,
        const PlayerAttackTargetSnapshot* target);
    bool validateReady(
        const PlayerAttackTargetSnapshot* target);
    void cancel();

    std::int32_t approachTargetId() const;
    std::int32_t readyTargetId() const;

private:
    std::int32_t approach_target_id_ = -1;
    std::int32_t ready_target_id_ = -1;
};

}  // namespace osf

#endif
