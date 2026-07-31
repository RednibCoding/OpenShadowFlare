#ifndef OPENSHADOWFLARE_PLAYER_MOON_SPELL_HPP
#define OPENSHADOWFLARE_PLAYER_MOON_SPELL_HPP

#include "companion_profile.hpp"
#include "player_sustained_spell.hpp"

#include <cstdint>

namespace osf {

class TableDatabase;

CompanionProfile applyPlayerMoonCompanionModifiers(
    const CompanionProfile& base,
    const PlayerSustainedSpell& moon,
    const TableDatabase& tables);

}  // namespace osf

#endif
