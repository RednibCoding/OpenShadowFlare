#ifndef OPENSHADOWFLARE_PLAYER_INCREASED_POWER_HPP
#define OPENSHADOWFLARE_PLAYER_INCREASED_POWER_HPP

#include <cstdint>

namespace osf {

class PlayerSpecialItems;
class RetailRandom;

class PlayerIncreasedPower {
public:
    static constexpr std::int32_t active_updates = 900;
    static constexpr std::int32_t ordinary_kill_threshold = 50;
    static constexpr std::int32_t assisted_kill_threshold = 30;

    void clear();
    void accountDirectLocalKill();
    bool ready(const PlayerSpecialItems& special_items) const;
    bool activate(const PlayerSpecialItems& special_items);
    void deactivate();
    bool update();
    void updateAura(bool visible);

    bool active() const;
    std::int32_t charge() const;
    std::int32_t remainingUpdates() const;
    std::int32_t auraFrame() const;
    std::int32_t movementSpeedTier(
        std::int32_t ordinary_tier) const;
    bool blocksSpell(std::int32_t spell) const;
    bool redirectsRangedAttack(
        std::int32_t current_job,
        std::int32_t requested_action,
        RetailRandom& random) const;

private:
    std::int32_t charge_ = 0;
    std::int32_t remaining_updates_ = 0;
    std::int32_t aura_frame_ = 0;
};

}  // namespace osf

#endif
