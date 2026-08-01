#ifndef OPENSHADOWFLARE_PLAYER_DAMAGE_RECEIVER_HPP
#define OPENSHADOWFLARE_PLAYER_DAMAGE_RECEIVER_HPP

#include "combat_damage.hpp"
#include "combat_effect_request.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "items/player_special_items.hpp"
#include "player_combat_defense.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace osf {

class ItemDatabase;
class RetailRandom;
class TableDatabase;

constexpr std::size_t kPlayerSpellCount = 22;

struct PlayerDamageReceiverState {
    PlayerCombatDefenseSnapshot defense;
    WorldPosition position;
    ObjectBounds judgement;
    std::int32_t effect_owner_identifier = -1;
    std::int32_t level = 1;
    std::int32_t maximum_life = 1;
    std::int32_t current_life = 1;
    std::int32_t maximum_mana = 0;
    std::int32_t current_mana = 0;

    PlayerEquipment equipment;
    PlayerInventory inventory;
    PlayerSpecialItems special_items;

    std::array<std::int32_t, kPlayerSpellCount>
        spell_levels{};
    std::int32_t magic_level_modifier = 0;
    std::int32_t selected_magic = 0;
    std::int32_t increased_power_updates = 0;
    bool energy_shield_active = false;
    bool magic_shield_active = false;
    bool counter_burst_active = false;

    std::int32_t presentation_action = 0;
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

struct PlayerReflectionTarget {
    bool found = false;
    std::int32_t character_number = -1;
    std::int32_t actor_kind = -1;
    std::int32_t active_value = 0;
    std::int32_t damage_scale_value = 0;
    WorldPosition position;
};

struct PlayerDamageReceiverContext {
    std::int32_t local_player_character_number = -1;
    PlayerReflectionTarget reflection_target;
};

struct PlayerSpellTrainingRequest {
    std::int32_t spell_number = -1;
    std::int32_t mode = 0;
};

struct PlayerReflectedDamageRequest {
    bool valid = false;
    std::int32_t target_character_number = -1;
    CombatPacket packet;
    WorldPosition impact_origin;
};

struct PlayerDamageReceiverResult {
    bool valid = true;
    bool accepted = true;
    PlayerDamageReceiverState state;
    CombatDamageResult damage;
    bool revived = false;
    bool equipment_sync_requested = false;
    bool derived_values_refresh_requested = false;
    PlayerReflectedDamageRequest reflection;
    std::vector<PlayerSpellTrainingRequest> spell_training;
    std::vector<CombatEffectSpawnRequest> effects;
    std::vector<std::int32_t> audio_samples;
};

PlayerDamageReceiverResult resolvePlayerDamage(
    const PlayerDamageReceiverState& state,
    const CombatPacket& packet,
    WorldPosition impact_origin,
    const PlayerDamageReceiverContext& context,
    const ItemDatabase& items,
    const TableDatabase& tables,
    RetailRandom& random);

}  // namespace osf

#endif
