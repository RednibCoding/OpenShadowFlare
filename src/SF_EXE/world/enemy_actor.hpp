#ifndef OPENSHADOWFLARE_ENEMY_ACTOR_HPP
#define OPENSHADOWFLARE_ENEMY_ACTOR_HPP

#include "combat_effect_request.hpp"
#include "enemy_ai_action.hpp"
#include "enemy_damage_receiver.hpp"
#include "enemy_direct_impact.hpp"
#include "enemy_presentation.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "movement_controller.hpp"
#include "movement_destination_selector.hpp"
#include "scenario_data.hpp"
#include "scenario_entity_state.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class CharacterVisualResource;
class AiControlList;
class RetailRandom;
class TableDatabase;

namespace gapi {
class NjpImage;
}

struct EnemyActorUpdate {
    CombatEffectSpawnRequest effect_spawn;
    EnemyDirectImpactResult direct_impact;
    std::vector<std::int32_t> audio_samples;
    bool death_started = false;
    bool death_finished = false;
    bool expired = false;
};

struct EnemyActorUpdateContext {
    const GroundMap* ground = nullptr;
    const ObjectMap* objects = nullptr;
    const std::vector<MovementBlocker>* dynamic_blockers = nullptr;
    const TableDatabase* parameter_tables = nullptr;
    RetailRandom* random = nullptr;
    bool ai_active = true;
    EnemyTargetSearch target_in_range;
    EnemyDefaultTargetSearch default_target;
    EnemyDirectImpactTargetSearch direct_impact_target;
    MovementTargetResolver resolve_movement_target;
};

class EnemyActor {
public:
    bool initialize(
        const ScenarioEnemy& enemy,
        const CharacterVisualResource* visual,
        const AiControlList& ai_control,
        std::int32_t ai_control_index,
        std::string* error = nullptr);
    void clear();
    EnemyActorUpdate update(
        const EnemyActorUpdateContext& context);
    EnemyActorUpdate update(
        const GroundMap& ground,
        const ObjectMap& objects,
        const std::vector<MovementBlocker>* dynamic_blockers);

    std::int32_t stateValue(
        ScenarioEntityStateChannel channel) const;
    void setStateValue(
        ScenarioEntityStateChannel channel,
        std::int32_t value);

    std::int32_t id() const;
    std::int32_t characterNumber() const;
    std::int32_t movementBlockerId() const;
    std::int32_t resourceId() const;
    const std::string& name() const;
    std::uint32_t nameColor() const;
    std::int32_t labelHeight() const;
    WorldPosition position() const;
    WorldPosition renderPosition(double alpha) const;
    const ObjectBounds& judgement() const;
    std::int32_t direction() const;
    std::int32_t animationChart() const;
    std::int32_t animationFrame() const;
    std::int32_t drawStrength() const;
    bool expired() const;
    const std::string& aiControlName() const;
    const AiControlList* aiControl() const;
    std::int32_t aiControlIndex() const;
    const ObjectBounds& patrolBounds() const;
    std::int32_t currentLife() const;
    std::int32_t maximumLife() const;
    std::int32_t nativeElement() const;
    std::int32_t physicalDefense() const;
    std::int32_t physicalEvasion() const;
    std::int32_t magicalDefense() const;
    std::int32_t magicalEvasion() const;
    std::int32_t experienceReward() const;
    std::int32_t lootTableRow() const;
    std::int32_t goldDropChance() const;
    std::int32_t goldMinimum() const;
    std::int32_t goldMaximum() const;
    std::int32_t reactionChanceDefense() const;
    std::int32_t reactionDurationDefense() const;
    bool alwaysSuppressReactionDisplacement() const;
    std::int32_t movementSpeedScale() const;
    const EnemyPresentationProfile&
    presentationProfile() const;
    bool partEnabled(std::size_t part) const;
    std::int32_t partRedStrength(
        std::size_t part) const;
    std::int32_t partGreenStrength(
        std::size_t part) const;
    std::int32_t partBlueStrength(
        std::size_t part) const;
    const gapi::NjpImage& patterns() const;
    const gapi::NjpImage& shadowPatterns() const;
    const gapi::CafAnimation& animation() const;
    bool hasVisual() const;
    bool visible() const;
    bool pointerEnabled() const;
    bool judgementEnabled() const;
    EnemyDamageReceiverState damageReceiverState(
        std::int32_t scenario_number) const;
    void applyDamageReceiverState(
        const EnemyDamageReceiverState& state);

private:
    std::int32_t id_ = -1;
    std::int32_t resource_id_ = -1;
    std::string name_;
    std::uint32_t name_color_ = 0;
    std::int32_t label_height_ = 0;
    WorldPosition position_;
    WorldPosition previous_position_;
    ObjectBounds judgement_;
    std::int32_t direction_ = 0;
    std::int32_t animation_chart_ = 0;
    std::int32_t animation_frame_ = 0;
    std::int32_t action_counter_ = 0;
    std::string ai_control_name_;
    const AiControlList* ai_control_ = nullptr;
    std::int32_t ai_control_index_ = -1;
    ObjectBounds patrol_bounds_;
    std::int32_t current_life_ = 0;
    std::int32_t maximum_life_ = 0;
    std::int32_t native_element_ = 0;
    std::int32_t physical_defense_ = 0;
    std::int32_t physical_evasion_ = 0;
    std::int32_t magical_defense_ = 0;
    std::int32_t magical_evasion_ = 0;
    std::int32_t experience_reward_ = 0;
    std::int32_t loot_table_row_ = -1;
    std::int32_t gold_drop_chance_ = 0;
    std::int32_t gold_minimum_ = 0;
    std::int32_t gold_maximum_ = 0;
    std::int32_t reaction_chance_defense_ = 0;
    std::int32_t reaction_duration_defense_ = 0;
    bool always_suppress_reaction_displacement_ = false;
    std::int32_t presentation_action_ = 7;
    std::int32_t action_lock_ = 0;
    std::int32_t reaction_duration_ = 0;
    std::int32_t reaction_stage_ = 0;
    bool reaction_displacement_suppressed_ = false;
    std::int32_t reaction_additive_ = 0;
    double reaction_angle_ = 0.0;
    std::int32_t event_number_ = 0;
    std::array<
        std::int32_t,
        kEnemyDamageSourceSlotCount>
        attributed_damage_{};
    std::int32_t death_counter_ = 0;
    bool defeated_by_effect_ = false;
    std::int32_t defeat_source_character_number_ = -1;
    std::int32_t draw_strength_ = 1000;
    bool expired_ = false;
    std::int32_t movement_speed_scale_ = 0;
    WorldPosition spawn_position_;
    WorldPosition walk_point_;
    EnemyAiActionController ai_action_;
    EnemyPresentationController presentation_;
    MovementDestinationSelector movement_destination_;
    MovementController movement_controller_;
    std::int32_t movement_speed_ = 0;
    std::int32_t movement_action_counter_ = 0;
    EnemyPresentationProfile presentation_profile_;
    ScenarioEntityState state_;
    std::vector<std::int32_t> part_visibility_;
    std::vector<std::int16_t> red_strength_;
    std::vector<std::int16_t> green_strength_;
    std::vector<std::int16_t> blue_strength_;
    const CharacterVisualResource* visual_ = nullptr;
};

bool enemyBlocksMovement(const EnemyActor& enemy);

}  // namespace osf

#endif
