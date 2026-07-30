#ifndef OPENSHADOWFLARE_ENEMY_EFFECT_IMPACT_HPP
#define OPENSHADOWFLARE_ENEMY_EFFECT_IMPACT_HPP

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

struct EnemyEffectSpawnRequest {
    bool valid = false;
    std::int32_t effect_number = -1;
    std::int32_t owner_kind = 4;
    std::int32_t source_character_number = -1;
    std::int32_t target_kind = 19;
    std::int32_t target_identifier = -1;
    std::int32_t constructor_value_6 = -1;
    std::int32_t constructor_value_7 = 0;
    double direction_radians = 0.0;
    bool has_explicit_origin = false;
    WorldPosition origin;
    bool has_source_judgement = false;
    ObjectBounds source_judgement;
    std::int32_t constructor_value_12 = 0;
    CombatPacket packet;
    std::int32_t packet_kind = 8;
    std::int32_t instance_identifier = -1;
    std::int32_t constructor_value_16 = 0;
    std::int32_t constructor_value_17 = 0;
    std::int32_t constructor_value_18 = 0;
    std::int32_t constructor_value_19 = 0;
    std::int32_t constructor_value_20 = 0;
    std::int32_t constructor_value_21 = 200;
    std::int32_t constructor_value_22 = -1;
};

std::int32_t retailEnemyEffectParameter(
    const TableDatabase& tables,
    std::int32_t type,
    std::int32_t subtype,
    std::int32_t selector);

EnemyEffectSpawnRequest resolveEnemyEffectImpact(
    const EnemyEffectImpactInput& input,
    const TableDatabase& tables,
    RetailRandom& random);

}  // namespace osf

#endif
