#ifndef OPENSHADOWFLARE_PLAYER_TRANSPORT_SPELL_HPP
#define OPENSHADOWFLARE_PLAYER_TRANSPORT_SPELL_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {

class ScenarioData;
class RetailRandom;

struct PlayerTransportEndpoint {
    std::int32_t scenario_id = -1;
    WorldPosition position;
};

struct PlayerTransportBeam {
    std::int32_t height = 300;
    std::int32_t strength = 0;
    std::int32_t delay = 0;
};

struct PlayerTransportPresentationUpdate {
    bool start_sound_due = false;
    bool loop_sound_due = false;
};

class PlayerTransportSpell {
public:
    void clear();
    bool create(
        std::int32_t owner,
        std::int32_t field_scenario,
        WorldPosition field_position,
        const ScenarioData& field_data,
        const ScenarioData& town_data,
        RetailRandom& random);

    bool active() const;
    const PlayerTransportEndpoint* endpoint(
        std::int32_t scenario_id) const;
    const PlayerTransportEndpoint* fieldEndpoint() const;
    std::int32_t townEntryValue() const;
    std::int32_t owner() const;
    bool updateContact(
        std::int32_t scenario_id,
        WorldPosition player_position,
        const ObjectBounds& player_judgement);
    bool atTownEndpoint(std::int32_t scenario_id) const;
    const std::array<PlayerTransportBeam, 4>& beams() const;
    bool centerVisible() const;
    std::int32_t animationFrame(
        std::int32_t scenario_id) const;
    PlayerTransportPresentationUpdate updatePresentation(
        std::int32_t scenario_id,
        std::int32_t animation_frame_count,
        RetailRandom& random);
    void consume();

private:
    std::int32_t owner_ = -1;
    PlayerTransportEndpoint field_;
    PlayerTransportEndpoint town_;
    std::int32_t town_entry_value_ = -1;
    std::array<PlayerTransportBeam, 4> beams_{{
        {300, 0, 0},
        {300, 0, 7},
        {300, 0, 14},
        {300, 0, 21},
    }};
    std::array<std::int32_t, 2> animation_wait_{};
    std::array<std::int32_t, 2> animation_frame_{};
    std::int32_t presentation_counter_ = 0;
    bool active_ = false;
    bool player_inside_ = false;
};

bool chooseRetailPlayerTransportPosition(
    const GroundMap& ground,
    const ObjectMap& objects,
    WorldPosition player_position,
    WorldPosition aim_position,
    WorldPosition& result);

}  // namespace osf

#endif
