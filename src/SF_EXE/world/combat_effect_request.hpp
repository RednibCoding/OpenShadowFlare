#ifndef OPENSHADOWFLARE_COMBAT_EFFECT_REQUEST_HPP
#define OPENSHADOWFLARE_COMBAT_EFFECT_REQUEST_HPP

#include "combat_packet.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>

namespace osf {

struct CombatEffectSpawnRequest {
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
    bool has_packet = false;
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

}  // namespace osf

#endif
