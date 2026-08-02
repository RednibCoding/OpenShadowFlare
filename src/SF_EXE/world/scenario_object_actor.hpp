#ifndef OPENSHADOWFLARE_SCENARIO_OBJECT_ACTOR_HPP
#define OPENSHADOWFLARE_SCENARIO_OBJECT_ACTOR_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "scenario_data.hpp"
#include "scenario_entity_state.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class ObjectVisualResource;

namespace gapi {
class CafAnimation;
class NjpImage;
}

class ScenarioObjectActor {
public:
    bool initialize(
        const ScenarioObject& object,
        const ObjectVisualResource* visual,
        std::string* error = nullptr);
    void clear();
    void update();

    std::int32_t stateValue(
        ScenarioEntityStateChannel channel) const;
    void setStateValue(
        ScenarioEntityStateChannel channel,
        std::int32_t value);
    void setStateOverride(
        std::int32_t visible,
        std::int32_t pointer,
        std::int32_t judgement);
    void setDrawStrength(std::int32_t strength);
    bool stateOverrideEnabled() const;

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
    std::int32_t staticPattern() const;
    std::int32_t animationChart() const;
    std::int32_t animationFrame() const;
    std::int32_t displayStatus() const;
    std::int32_t displayHeight() const;
    std::int32_t drawStrength() const;
    std::int32_t redDrawStrength() const;
    std::int32_t greenDrawStrength() const;
    std::int32_t blueDrawStrength() const;
    bool partEnabled(std::size_t part) const;
    std::int32_t partRedStrength(std::size_t part) const;
    std::int32_t partGreenStrength(std::size_t part) const;
    std::int32_t partBlueStrength(std::size_t part) const;
    bool hasStaticVisual() const;
    bool hasStaticShadow() const;
    bool hasAnimatedVisual() const;
    bool drawEnabled() const;
    const gapi::NjpImage& staticPatterns() const;
    const gapi::NjpImage& staticShadows() const;
    const gapi::NjpImage& animationPatterns() const;
    const gapi::CafAnimation& animation() const;
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
    std::int32_t visual_mode_ = 0;
    std::int32_t static_pattern_ = -1;
    std::int32_t animation_chart_ = -1;
    std::int32_t animation_frame_ = 0;
    std::int32_t display_status_ = 0;
    std::int32_t height_ = 0;
    std::int32_t draw_flags_ = 0;
    std::int32_t draw_strength_ = 0;
    std::int32_t red_draw_strength_ = 1000;
    std::int32_t green_draw_strength_ = 1000;
    std::int32_t blue_draw_strength_ = 1000;
    ScenarioEntityState state_;
    std::vector<std::int32_t> part_visibility_;
    std::vector<std::int16_t> red_strength_;
    std::vector<std::int16_t> green_strength_;
    std::vector<std::int16_t> blue_strength_;
    const ObjectVisualResource* visual_ = nullptr;
};

}  // namespace osf

#endif
