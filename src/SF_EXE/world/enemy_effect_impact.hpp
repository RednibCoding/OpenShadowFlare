#ifndef OPENSHADOWFLARE_ENEMY_EFFECT_IMPACT_HPP
#define OPENSHADOWFLARE_ENEMY_EFFECT_IMPACT_HPP

#include "combat_effect_request.hpp"
#include "combat_packet.hpp"
#include "enemy_target_selector.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>

namespace osf {

class RetailRandom;
class TableDatabase;

struct EnemyEffectImpactInput {
    std::int32_t source_character_number = -1;
    WorldPosition source_position;
    ObjectBounds source_judgement;
    double direction_radians = 0.0;
    std::int32_t type = -1;
    std::int32_t subtype = 0;
    std::int32_t parameter = 0;
    std::int32_t additive = 0;
    std::int32_t packet_word_31 = 0;
    EnemyAiTarget target;
};

std::int32_t retailEffectParameter(
    const TableDatabase& tables,
    std::int32_t type,
    std::int32_t subtype,
    std::int32_t selector);

CombatEffectSpawnRequest resolveEnemyEffectImpact(
    const EnemyEffectImpactInput& input,
    const TableDatabase& tables,
    RetailRandom& random);

}  // namespace osf

#endif
