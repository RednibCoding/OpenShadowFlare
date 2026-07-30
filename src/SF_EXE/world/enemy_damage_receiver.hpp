#ifndef OPENSHADOWFLARE_ENEMY_DAMAGE_RECEIVER_HPP
#define OPENSHADOWFLARE_ENEMY_DAMAGE_RECEIVER_HPP

#include "combat_damage.hpp"
#include "enemy_effect_impact.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {

class RetailRandom;
class TableDatabase;

constexpr std::size_t kEnemyDamageSourceSlotCount = 10;

struct EnemyDamageReceiverState {
    std::int32_t character_number = -1;
    std::int32_t scenario_number = -1;
    WorldPosition position;
    ObjectBounds judgement;
    bool has_visual = false;

    std::int32_t current_life = 0;
    std::int32_t maximum_life = 0;
    std::int32_t native_element = 0;
    std::int32_t physical_defense = 0;
    std::int32_t magical_defense = 0;
    std::int32_t reaction_chance_defense = 0;
    std::int32_t reaction_duration_defense = 0;
    bool always_suppress_reaction_displacement = false;

    std::int32_t presentation_action = 7;
    std::int32_t presentation_counter = 0;
    std::int32_t action_lock = 0;
    std::int32_t reaction_duration = 0;
    std::int32_t reaction_stage = 0;
    bool reaction_displacement_suppressed = false;
    std::int32_t reaction_additive = 0;
    double reaction_angle = 0.0;
    std::int32_t direction = 0;
    std::int32_t event_number = 0;

    std::array<
        std::int32_t,
        kEnemyDamageSourceSlotCount>
        attributed_damage{};
    std::int32_t death_counter = 0;
    bool defeated_by_effect = false;
    std::int32_t defeat_source_character_number = -1;
};

struct EnemyDamageReceiverContext {
    std::int32_t local_player_slot = 0;
    // Retail uses zero for single-player, one for the server,
    // and two for a client.
    std::int32_t network_mode = 0;
    bool local_player_available = true;
    bool source_player_available = false;
    WorldPosition source_player_position;
    bool apply_status_7_on_kill = false;
    bool apply_status_8_on_kill = false;
    bool apply_status_9_on_kill = false;
};

struct EnemyDamageStatusRequest {
    std::int32_t status_number = -1;
    std::int32_t mode = 0;
};

enum class EnemyDamageNetworkAction {
    none,
    send_to_server,
    broadcast_to_clients,
};

struct EnemyDamageNetworkRequest {
    EnemyDamageNetworkAction action =
        EnemyDamageNetworkAction::none;
    std::int32_t scenario_number = -1;
    std::int32_t source_slot = -1;
    std::int32_t target_character_number = -1;
    bool effect_family = false;
    std::int32_t damage = 0;
    bool source_is_character_number = false;
    WorldPosition target_position;
};

struct EnemyDamageReceiverResult {
    bool valid = true;
    bool accepted = false;
    EnemyDamageReceiverState state;
    CombatDamageResult damage;
    std::int32_t source_player_lookup_count = 0;
    bool source_scenario_actor_lookup_requested = false;
    bool source_owner_lookup_requested = false;
    std::vector<EnemyDamageStatusRequest>
        local_player_statuses;
    EnemyDamageNetworkRequest network;
    bool kill_requested = false;
    std::vector<CombatEffectSpawnRequest> effects;
    std::vector<std::int32_t> audio_samples;
};

EnemyDamageReceiverResult resolveEnemyDamage(
    const EnemyDamageReceiverState& state,
    const CombatPacket& packet,
    WorldPosition impact_origin,
    const EnemyDamageReceiverContext& context,
    const TableDatabase& tables,
    RetailRandom& random);

}  // namespace osf

#endif
