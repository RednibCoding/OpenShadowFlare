#ifndef OPENSHADOWFLARE_NPC_ACTOR_HPP
#define OPENSHADOWFLARE_NPC_ACTOR_HPP

#include "core/retail_random.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "movement_controller.hpp"
#include "scenario_data.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class CharacterVisualResource;

namespace gapi {
class NjpImage;
}

class NpcActor {
public:
    bool initialize(
        const ScenarioPerson& person,
        const CharacterVisualResource& visual,
        std::string* error = nullptr);
    void clear();
    void update(
        const GroundMap& ground,
        const ObjectMap& objects);
    void beginInteraction(WorldPosition player_position);
    void endInteraction();

    std::int32_t id() const;
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
    bool partEnabled(std::size_t part) const;
    std::int32_t partRedStrength(std::size_t part) const;
    std::int32_t partGreenStrength(std::size_t part) const;
    std::int32_t partBlueStrength(std::size_t part) const;
    const gapi::NjpImage& patterns() const;
    const gapi::NjpImage& shadowPatterns() const;
    const gapi::CafAnimation& animation() const;

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
    std::int32_t walk_speed_ = 0;
    std::int32_t walk_duration_ = 0;
    std::int32_t idle_duration_ = 0;
    WorldPosition wander_min_;
    WorldPosition wander_max_;
    WorldPosition destination_;
    bool wandering_enabled_ = false;
    bool walking_ = false;
    bool interaction_active_ = false;
    RetailRandom random_;
    std::vector<std::int32_t> part_visibility_;
    std::vector<std::int16_t> red_strength_;
    std::vector<std::int16_t> green_strength_;
    std::vector<std::int16_t> blue_strength_;
    const CharacterVisualResource* visual_ = nullptr;
};

}  // namespace osf

#endif
