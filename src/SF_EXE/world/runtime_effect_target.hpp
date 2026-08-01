#ifndef OPENSHADOWFLARE_RUNTIME_EFFECT_TARGET_HPP
#define OPENSHADOWFLARE_RUNTIME_EFFECT_TARGET_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {

class RetailRandom;

constexpr std::size_t kRuntimeEffectRememberedTargetLimit = 500;

enum class RuntimeEffectTargetKind : std::int32_t {
    player,
    companion,
    enemy,
    npc,
    scenario_object,
};

struct RuntimeEffectTargetSnapshot {
    RuntimeEffectTargetKind kind =
        RuntimeEffectTargetKind::scenario_object;
    std::int32_t character_number = -1;
    std::int32_t identifier = -1;
    WorldPosition position;
    ObjectBounds judgement;
    bool present = true;
    bool same_scenario = true;
    bool local_owner = true;
    std::int32_t current_life = 1;
    bool active = true;
    bool displayed = true;
    std::int32_t runtime_state = 0;
    std::int32_t physical_evasion = 0;
    std::int32_t magical_evasion = 0;
};

struct RuntimeEffectAudioPair {
    std::int32_t bank = -1;
    std::int32_t sample = -1;
};

struct RuntimeEffectAudioRequest {
    RuntimeEffectAudioPair sound;
    WorldPosition position;
    bool npc_spatial_mode = false;
};

enum class RuntimeEffectReceiverAction : std::int32_t {
    none,
    apply_packet,
    show_miss,
};

struct RuntimeEffectTargetContact {
    RuntimeEffectTargetKind kind =
        RuntimeEffectTargetKind::scenario_object;
    std::int32_t character_number = -1;
    std::int32_t identifier = -1;
    std::int32_t distance = 0;
    WorldPosition impact_origin;
    bool evasion_checked = false;
    std::int32_t hit_chance = 0;
    std::int32_t hit_roll = -1;
    bool hit = true;
    RuntimeEffectReceiverAction receiver_action =
        RuntimeEffectReceiverAction::none;
};

struct RuntimeEffectTargetQuery {
    std::int32_t actor_identifier = -1;
    WorldPosition actor_position;
    ObjectBounds actor_judgement;
    std::int32_t target_mask = 0;
    std::int32_t target_identifier = -1;
    bool exact_target_only = false;
    bool process_every_target = false;
    bool expire_on_target = false;
    bool expire_on_object_contact = false;
    bool remember_targets = false;
    bool has_packet = false;
    bool magical_evasion = false;
    std::int32_t hit_rating = 0;
    RuntimeEffectAudioPair target_audio;
    RuntimeEffectAudioPair object_audio;
};

struct RuntimeEffectTargetResolution {
    std::vector<RuntimeEffectTargetContact> contacts;
    std::vector<RuntimeEffectAudioRequest> audio;
    bool expired = false;
};

class RuntimeEffectTargetMemory {
public:
    void clear();
    bool contains(std::int32_t identifier) const;
    void remember(std::int32_t identifier);
    std::size_t count() const;

private:
    std::array<
        std::int32_t,
        kRuntimeEffectRememberedTargetLimit>
        identifiers_{};
    std::size_t count_ = 0;
};

RuntimeEffectTargetResolution resolveRuntimeEffectTargets(
    const RuntimeEffectTargetQuery& query,
    const std::vector<RuntimeEffectTargetSnapshot>& targets,
    RuntimeEffectTargetMemory& memory,
    RetailRandom& random);

}  // namespace osf

#endif
