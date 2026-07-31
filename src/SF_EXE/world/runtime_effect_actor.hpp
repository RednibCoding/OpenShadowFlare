#ifndef OPENSHADOWFLARE_RUNTIME_EFFECT_ACTOR_HPP
#define OPENSHADOWFLARE_RUNTIME_EFFECT_ACTOR_HPP

#include "combat_packet.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "runtime_effect_target.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {

class EffectVisualResource;
class RetailRandom;

struct RuntimeEffectActorSpawnRequest {
    std::int32_t actor_identifier = -1;
    std::int32_t controller_effect_number = -1;
    std::int32_t resource_id = -1;
    std::int32_t owner_kind = 0;
    std::int32_t source_character_number = -1;
    std::int32_t target_mask = 0;
    std::int32_t target_identifier = 0;
    bool exact_target_only = false;
    bool home_toward_target = false;
    std::int32_t homing_turn_speed = 0;
    double direction_radians = 0.0;
    std::int32_t travel_speed = 0;
    WorldPosition position;
    ObjectBounds judgement;
    std::int32_t display_height = 0;
    std::int32_t lifetime = -1;
    bool lifetime_from_animation = false;
    std::int32_t lifetime_animation_chart = -1;
    bool collide_with_environment = true;
    bool expire_on_environment_collision = false;
    std::int32_t target_collision_start = -1;
    std::int32_t target_collision_end = -1;
    bool process_every_target = false;
    bool expire_on_target = false;
    bool remember_targets = false;
    RuntimeEffectAudioPair target_audio;
    RuntimeEffectAudioPair environment_audio;
    std::int32_t animation_chart = 0;
    std::int32_t animation_direction = 8;
    std::int32_t animation_speed = 1000;
    std::int32_t additional_display_status = 0;
    bool visible = true;
    bool track_for_controller = false;
    bool has_packet = false;
    CombatPacket packet;
};

struct RuntimeEffectActorUpdate {
    WorldPosition intended_position;
    bool target_collision_active = false;
    bool environment_collision = false;
    bool expired = false;
    std::vector<RuntimeEffectTargetContact>
        target_contacts;
    std::vector<RuntimeEffectAudioRequest> audio;
};

class RuntimeEffectActor {
public:
    bool initialize(
        const RuntimeEffectActorSpawnRequest& request,
        const EffectVisualResource& visual);
    bool initialize(
        const RuntimeEffectActorSpawnRequest& request,
        const EffectVisualResource* visual);
    RuntimeEffectActorUpdate update(
        const GroundMap& ground,
        const ObjectMap& objects);
    RuntimeEffectActorUpdate update(
        const GroundMap& ground,
        const ObjectMap& objects,
        const std::vector<RuntimeEffectTargetSnapshot>& targets,
        RetailRandom& random);

    std::int32_t controllerEffectNumber() const;
    std::int32_t actorIdentifier() const;
    std::int32_t resourceId() const;
    std::int32_t ownerKind() const;
    std::int32_t sourceCharacterNumber() const;
    WorldPosition position() const;
    WorldPosition previousPosition() const;
    WorldPosition renderPosition(double alpha) const;
    const ObjectBounds& judgement() const;
    std::int32_t animationChart() const;
    std::int32_t animationDirection() const;
    std::int32_t animationFrame() const;
    std::int32_t displayHeight() const;
    std::int32_t additionalDisplayStatus() const;
    std::int32_t counter() const;
    std::int32_t movementCounter() const;
    std::int32_t lifetime() const;
    bool visible() const;
    bool expired() const;
    bool hasUpdated() const;
    bool targetCollisionActive() const;
    bool needsTargetSnapshots() const;
    bool hasPacket() const;
    const CombatPacket& packet() const;
    std::size_t rememberedTargetCount() const;
    bool partEnabled(std::size_t part) const;
    const gapi::NjpImage& patterns() const;
    const gapi::CafAnimation& animation() const;

private:
    RuntimeEffectActorSpawnRequest request_;
    WorldPosition start_position_;
    WorldPosition position_;
    WorldPosition previous_position_;
    std::int32_t animation_frame_ = 0;
    std::int32_t counter_ = 0;
    std::int32_t movement_counter_ = 0;
    std::int32_t lifetime_ = -1;
    double direction_radians_ = 0.0;
    std::int32_t animation_direction_ = 8;
    bool homing_active_ = false;
    bool expired_ = false;
    bool has_updated_ = false;
    RuntimeEffectTargetMemory target_memory_;
    const EffectVisualResource* visual_ = nullptr;
};

}  // namespace osf

#endif
