#ifndef OPENSHADOWFLARE_ENEMY_EFFECT_IMPACT_HPP
#define OPENSHADOWFLARE_ENEMY_EFFECT_IMPACT_HPP

#include "enemy_target_selector.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>

namespace osf {

class RetailRandom;
class TableDatabase;

constexpr std::size_t kEnemyEffectPacketWordCount = 77;

struct EnemyEffectImpactInput {
    std::int32_t source_character_number = -1;
    WorldPosition source_position;
    ObjectBounds source_judgement;
    double direction_radians = 0.0;
    std::int32_t type = -1;
    std::int32_t subtype = 0;
    std::int32_t parameter = 0;
    std::int32_t additive = 0;
    std::int32_t packet_source_value = 0;
    EnemyAiTarget target;
};

struct EnemyEffectSpawnRequest {
    bool valid = false;
    std::int32_t effect_number = -1;
    std::int32_t owner_kind = 4;
    std::int32_t source_character_number = -1;
    std::int32_t target_kind = 19;
    std::int32_t target_identifier = -1;
    std::int32_t table_35_value = -1;
    std::int32_t duration = 0;
    double direction_radians = 0.0;
    bool has_explicit_origin = false;
    WorldPosition origin;
    ObjectBounds source_judgement;
    std::int32_t constructor_value_12 = 0;
    std::array<
        std::int32_t,
        kEnemyEffectPacketWordCount>
        packet{};
    std::bitset<kEnemyEffectPacketWordCount>
        written_packet_words;
    std::int32_t packet_kind = 8;
    std::int32_t instance_identifier = -1;
    std::int32_t subtype = 0;
    std::int32_t constructor_value_21 = 200;
    std::int32_t table_21_value = -1;
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
