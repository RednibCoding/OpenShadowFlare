#ifndef OPENSHADOWFLARE_PLAYER_MOON_SPELL_HPP
#define OPENSHADOWFLARE_PLAYER_MOON_SPELL_HPP

#include "companion_profile.hpp"

#include <cstdint>

namespace osf {

class TableDatabase;

struct PlayerMoonManaUpdate {
    std::int32_t mana = 0;
    bool changed = false;
    bool deactivated = false;
};

class PlayerMoonSpell {
public:
    bool toggle(
        std::int32_t effective_level,
        const TableDatabase& tables);
    PlayerMoonManaUpdate updateMana(
        std::int32_t current_mana,
        std::int32_t maximum_mana);
    void updateAura(bool displayed);
    void clear();

    bool active() const;
    std::int32_t effectiveLevel() const;
    std::int32_t manaChangeRate() const;
    std::int32_t auraFrame() const;

private:
    std::int32_t effective_level_ = 0;
    std::int32_t mana_change_rate_ = 0;
    std::int32_t mana_remainder_ = 0;
    std::int32_t update_counter_ = 0;
    std::int32_t aura_frame_ = 0;
};

CompanionProfile applyPlayerMoonCompanionModifiers(
    const CompanionProfile& base,
    const PlayerMoonSpell& moon,
    const TableDatabase& tables);

}  // namespace osf

#endif
