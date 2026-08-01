#ifndef OPENSHADOWFLARE_COMPANION_DAMAGE_RECEIVER_HPP
#define OPENSHADOWFLARE_COMPANION_DAMAGE_RECEIVER_HPP

#include "combat_damage.hpp"
#include "combat_effect_request.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>
#include <vector>

namespace osf {

class RetailRandom;
class TableDatabase;

struct CompanionDamageReceiverState {
    std::int32_t character_number = -1;
    WorldPosition position;
    ObjectBounds judgement;
    std::int32_t current_life = 0;
    std::int32_t maximum_life = 0;
    std::int32_t native_element = 0;
    std::int32_t physical_defense = 0;
    std::int32_t magical_defense = 0;

    std::int32_t presentation_action = 2;
    std::int32_t presentation_counter = 0;
    std::int32_t action_lock = 0;
    std::int32_t reaction_duration = 0;
    std::int32_t reaction_stage = 0;
    bool reaction_motion = false;
    std::int32_t reaction_additive = 0;
    double reaction_angle = 0.0;
    std::int32_t direction = 0;
    std::int32_t event_number = 0;
};

struct CompanionDamageReceiverContext {
    std::int32_t local_player_slot = -1;
};

struct CompanionDamageReceiverResult {
    bool valid = true;
    bool accepted = false;
    CompanionDamageReceiverState state;
    CombatDamageResult damage;
    std::vector<CombatEffectSpawnRequest> effects;
    std::vector<std::int32_t> audio_samples;
};

CompanionDamageReceiverResult resolveCompanionDamage(
    const CompanionDamageReceiverState& state,
    const CombatPacket& packet,
    WorldPosition impact_origin,
    const CompanionDamageReceiverContext& context,
    const TableDatabase& tables,
    RetailRandom& random);

}  // namespace osf

#endif
