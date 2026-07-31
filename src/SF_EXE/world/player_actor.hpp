#ifndef OPENSHADOWFLARE_PLAYER_ACTOR_HPP
#define OPENSHADOWFLARE_PLAYER_ACTOR_HPP

#include "actor_direction.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "movement_controller.hpp"
#include "player_attack_action.hpp"
#include "player_spell_action.hpp"

#include <cstdint>
#include <vector>

namespace osf {

enum class PlayerMotion {
    idle,
    walking,
    running,
    attacking,
    casting,
    reacting,
    defeated,
};

enum class MovementPace {
    walk,
    run,
};

struct PlayerDamagePresentation {
    std::int32_t action = 1;
    std::int32_t counter = 0;
    std::int32_t action_lock = 0;
    std::int32_t reaction_duration = 0;
    std::int32_t reaction_stage = 0;
    bool suppress_displacement = false;
    std::int32_t reaction_additive = 0;
    double reaction_angle = 0.0;
    std::int32_t direction = 0;
    std::int32_t event_number = 0;
};

class PlayerActor {
public:
    void reset(
        WorldPosition position,
        std::int32_t direction,
        std::int32_t walking_speed_tier = 5);
    void clear();
    void relocate(
        WorldPosition position,
        std::int32_t direction);

    void moveTo(WorldPosition destination);
    void followTo(WorldPosition destination);
    void cancelMovement();
    void faceToward(WorldPosition position);
    bool beginAttack(
        PlayerAttackAction action,
        std::int32_t target_id,
        std::int32_t attack_speed_tier,
        const gapi::CafAnimation& animation);
    bool beginSpellCast(
        PlayerSpellAction action,
        std::int32_t spell,
        std::int32_t target_character_number,
        WorldPosition aim_position,
        std::int32_t speed_tier,
        const TableData* speed_table,
        const gapi::CafAnimation& animation);
    void toggleMovementPace();
    void setMovementPace(MovementPace pace);
    void update(
        const GroundMap& ground,
        const ObjectMap& objects,
        const std::vector<MovementBlocker>* dynamic_blockers = nullptr,
        std::int32_t attack_speed_tier = -1,
        const gapi::CafAnimation* animation = nullptr,
        const TableData* spell_speed_table = nullptr);

    WorldPosition position() const;
    WorldPosition renderPosition(double alpha) const;
    WorldPosition destination() const;
    const ObjectBounds& judgement() const;
    std::int32_t direction() const;
    std::int32_t walkingSpeedTier() const;
    std::int32_t walkingSpeed() const;
    std::int32_t runningSpeed() const;
    MovementPace movementPace() const;
    PlayerMotion motion() const;
    bool actionLocked() const;
    bool attackActive() const;
    std::int32_t attackTargetId() const;
    PlayerAttackActionEvent takeAttackEvent();
    bool spellActive() const;
    std::int32_t spellTargetCharacterNumber() const;
    PlayerSpellActionEvent takeSpellEvent();
    std::int32_t takeFootstepSample();
    bool takeDeathVoiceRequest();
    bool takeRespawnRequest();
    PlayerDamagePresentation damagePresentation() const;
    void applyDamagePresentation(
        const PlayerDamagePresentation& presentation);
    std::int32_t animationChart() const;
    std::int32_t animationFrame() const;

private:
    WorldPosition position_;
    WorldPosition previous_position_;
    WorldPosition destination_;
    ObjectBounds judgement_{-80, -80, 79, 79};
    std::int32_t direction_ = 0;
    std::int32_t walking_speed_tier_ = 5;
    std::int32_t walking_speed_ = 20;
    std::int32_t running_speed_ = 40;
    std::int32_t action_counter_ = 0;
    std::int32_t animation_chart_ = 0;
    std::int32_t animation_frame_ = 0;
    MovementPace movement_pace_ = MovementPace::walk;
    PlayerMotion motion_ = PlayerMotion::idle;
    PlayerMotion previous_action_ = PlayerMotion::idle;
    PlayerAttackActionController attack_controller_;
    PlayerAttackActionEvent pending_attack_event_;
    PlayerSpellActionController spell_controller_;
    PlayerSpellActionEvent pending_spell_event_;
    std::int32_t pending_footstep_sample_ = -1;
    bool pending_death_voice_request_ = false;
    bool respawn_requested_ = false;
    bool pending_respawn_request_ = false;
    MovementController movement_controller_;
    PlayerDamagePresentation damage_presentation_;
};

}  // namespace osf

#endif
