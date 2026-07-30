#ifndef OPENSHADOWFLARE_ENEMY_ACTOR_HPP
#define OPENSHADOWFLARE_ENEMY_ACTOR_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "scenario_data.hpp"
#include "scenario_entity_state.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class CharacterVisualResource;
class AiControlList;

namespace gapi {
class NjpImage;
}

class EnemyActor {
public:
    bool initialize(
        const ScenarioEnemy& enemy,
        const CharacterVisualResource* visual,
        const AiControlList& ai_control,
        std::int32_t ai_control_index,
        std::string* error = nullptr);
    void clear();
    void update();

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
    const ObjectBounds& judgement() const;
    std::int32_t direction() const;
    std::int32_t animationChart() const;
    std::int32_t animationFrame() const;
    const std::string& aiControlName() const;
    const AiControlList* aiControl() const;
    std::int32_t aiControlIndex() const;
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

private:
    std::int32_t id_ = -1;
    std::int32_t resource_id_ = -1;
    std::string name_;
    std::uint32_t name_color_ = 0;
    std::int32_t label_height_ = 0;
    WorldPosition position_;
    ObjectBounds judgement_;
    std::int32_t direction_ = 0;
    std::int32_t animation_frame_ = 0;
    std::int32_t action_counter_ = 0;
    std::string ai_control_name_;
    const AiControlList* ai_control_ = nullptr;
    std::int32_t ai_control_index_ = -1;
    ScenarioEntityState state_;
    std::vector<std::int32_t> part_visibility_;
    std::vector<std::int16_t> red_strength_;
    std::vector<std::int16_t> green_strength_;
    std::vector<std::int16_t> blue_strength_;
    const CharacterVisualResource* visual_ = nullptr;
};

}  // namespace osf

#endif
