#ifndef OPENSHADOWFLARE_COMPANION_ACTOR_HPP
#define OPENSHADOWFLARE_COMPANION_ACTOR_HPP

#include "companion_attack_action.hpp"
#include "companion_damage_receiver.hpp"
#include "companion_explosion_action.hpp"
#include "companion_profile.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "movement_controller.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {

class CharacterVisualResource;

namespace gapi {
class NjpImage;
}

enum class CompanionMotion {
    idle,
    walking,
    running,
    attacking,
    exploding,
    reacting,
    defeated,
    reviving,
};

struct CompanionActorUpdate {
    bool impact_due = false;
    bool swing_sound_due = false;
    bool attack_completed = false;
};

struct CompanionExplosionUpdate {
    bool handled = false;
    bool relocated = false;
    bool impact_due = false;
    bool completed = false;
};

struct CompanionPresentationUpdate {
    bool handled = false;
    bool death_started = false;
    bool revive_completed = false;
};

class CompanionActor {
public:
    bool initialize(
        const CompanionProfile& profile,
        const CharacterVisualResource& visual,
        std::int32_t owner_slot,
        WorldPosition position,
        std::int32_t direction);
    void clear();
    void relocate(
        WorldPosition position,
        std::int32_t direction);
    void updateFollow(
        WorldPosition owner_position,
        const ObjectBounds& owner_bounds,
        const GroundMap& ground,
        const ObjectMap& objects,
        const std::vector<MovementBlocker>*
            dynamic_blockers = nullptr);
    void updateCombatApproach(
        WorldPosition target_position,
        const GroundMap& ground,
        const ObjectMap& objects,
        const std::vector<MovementBlocker>*
            dynamic_blockers = nullptr);
    bool beginAttack(
        std::int32_t target_character_number,
        WorldPosition target_position);
    void trackCombatTarget(
        std::int32_t target_character_number);
    CompanionActorUpdate updateAttack();
    bool beginExplosion(WorldPosition destination);
    bool activatePendingExplosion();
    CompanionExplosionUpdate updateExplosion();
    void leaveCombat();
    CompanionPresentationUpdate
    updateDamagePresentation(
        const GroundMap& ground,
        const ObjectMap& objects,
        const std::vector<MovementBlocker>*
            dynamic_blockers = nullptr);
    CompanionDamageReceiverState
        damageReceiverState() const;
    void applyDamageReceiverState(
        const CompanionDamageReceiverState& state);
    void beginDefeatedWait();
    void beginRevive(WorldPosition owner_position);
    void applyLevelProfile(
        const CompanionProfile& profile);
    void applyRuntimeProfile(
        const CompanionProfile& profile);

    bool valid() const;
    std::int32_t characterNumber() const;
    std::int32_t movementBlockerId() const;
    const CompanionProfile& profile() const;
    WorldPosition position() const;
    WorldPosition renderPosition(double alpha) const;
    const ObjectBounds& judgement() const;
    std::int32_t direction() const;
    CompanionMotion motion() const;
    std::int32_t animationChart() const;
    std::int32_t animationDirection() const;
    std::int32_t animationFrame() const;
    std::int32_t drawOpacity() const;
    std::int32_t presentationAction() const;
    std::int32_t currentLife() const;
    std::int32_t maximumLife() const;
    std::int32_t combatTargetCharacterNumber() const;
    bool attackActive() const;
    bool explosionPending() const;
    bool explosionActive() const;
    bool partEnabled(std::size_t part) const;
    std::int32_t partRedStrength(std::size_t part) const;
    std::int32_t partGreenStrength(std::size_t part) const;
    std::int32_t partBlueStrength(std::size_t part) const;
    const gapi::NjpImage& patterns() const;
    const gapi::NjpImage& shadowPatterns() const;
    const gapi::CafAnimation& animation() const;
    bool visible() const;
    bool judgementEnabled() const;

private:
    void selectMotion(CompanionMotion motion);

    CompanionProfile profile_;
    std::int32_t owner_slot_ = -1;
    WorldPosition position_;
    WorldPosition previous_position_;
    ObjectBounds judgement_{-80, -80, 79, 79};
    std::int32_t direction_ = 0;
    CompanionMotion motion_ = CompanionMotion::idle;
    std::int32_t action_counter_ = 0;
    std::int32_t close_linger_counter_ = 0;
    std::int32_t current_life_ = 0;
    std::int32_t presentation_action_ = 2;
    std::int32_t presentation_counter_ = 0;
    std::int32_t presentation_animation_frame_ = 0;
    std::int32_t action_lock_ = 0;
    std::int32_t reaction_duration_ = 0;
    std::int32_t reaction_stage_ = 0;
    bool reaction_displacement_suppressed_ = false;
    std::int32_t reaction_additive_ = 0;
    double reaction_angle_ = 0.0;
    std::int32_t event_number_ = 0;
    std::int32_t draw_opacity_ = 1000;
    std::int32_t combat_target_character_number_ = -1;
    MovementController movement_controller_;
    CompanionAttackActionController attack_action_;
    CompanionExplosionActionController explosion_action_;
    WorldPosition pending_explosion_destination_;
    bool explosion_pending_ = false;
    const CharacterVisualResource* visual_ = nullptr;
};

}  // namespace osf

#endif
