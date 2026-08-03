#include "player_transport_spell.hpp"

#include "core/retail_random.hpp"
#include "scenario_data.hpp"

#include <array>
#include <cstddef>
#include <cstdlib>

namespace osf {
namespace {

constexpr std::int32_t kTransportDistance = 500;
constexpr ObjectBounds kTransportJudgement{-80, -80, 79, 79};

struct TransportDirection {
    WorldPosition offset;
    ObjectBounds corridor;
};

constexpr std::array<TransportDirection, 4> kDirections{{
    {{0, -kTransportDistance}, {-80, -580, 79, 79}},
    {{0, kTransportDistance}, {-80, -80, 79, 579}},
    {{-kTransportDistance, 0}, {-580, -80, 79, 79}},
    {{kTransportDistance, 0}, {-80, -80, 579, 79}},
}};

bool overlaps(
    WorldPosition first_position,
    const ObjectBounds& first,
    WorldPosition second_position,
    const ObjectBounds& second) {
    return first_position.x + first.left <=
               second_position.x + second.right &&
           second_position.x + second.left <=
               first_position.x + first.right &&
           first_position.y + first.top <=
               second_position.y + second.bottom &&
           second_position.y + second.top <=
               first_position.y + first.bottom;
}

}  // namespace

void PlayerTransportSpell::clear() {
    *this = {};
}

bool PlayerTransportSpell::create(
    std::int32_t owner,
    std::int32_t field_scenario,
    WorldPosition field_position,
    const ScenarioData& field_data,
    const ScenarioData& town_data,
    RetailRandom& random) {
    const std::int32_t town_scenario =
        field_data.footerValues()[0];
    const std::int32_t entry_key = 400 + owner;
    const ScenarioEntry* town_entry =
        town_data.findEntry(entry_key);
    if (owner < 0 || owner > 3 ||
        town_scenario == field_scenario ||
        !town_entry) {
        return false;
    }
    owner_ = owner;
    field_ = {field_scenario, field_position};
    town_ = {
        town_scenario,
        {town_entry->world_x, town_entry->world_y},
    };
    town_entry_value_ = entry_key / 4;
    beams_ = {{
        {300, 0, 0},
        {300, 0, 7},
        {300, 0, 14},
        {300, 0, 21},
    }};
    animation_frame_ = {};
    presentation_counter_ = 0;
    animation_wait_[0] = random.next() % 90 + 1;
    animation_wait_[1] = random.next() % 90 + 1;
    active_ = true;
    player_inside_ = false;
    return true;
}

bool PlayerTransportSpell::active() const {
    return active_;
}

const PlayerTransportEndpoint* PlayerTransportSpell::endpoint(
    std::int32_t scenario_id) const {
    if (!active_) {
        return nullptr;
    }
    if (scenario_id == field_.scenario_id) {
        return &field_;
    }
    if (scenario_id == town_.scenario_id) {
        return &town_;
    }
    return nullptr;
}

const PlayerTransportEndpoint*
PlayerTransportSpell::fieldEndpoint() const {
    return active_ ? &field_ : nullptr;
}

std::int32_t PlayerTransportSpell::townEntryValue() const {
    return town_entry_value_;
}

std::int32_t PlayerTransportSpell::owner() const {
    return owner_;
}

bool PlayerTransportSpell::updateContact(
    std::int32_t scenario_id,
    WorldPosition player_position,
    const ObjectBounds& player_judgement) {
    const PlayerTransportEndpoint* current = endpoint(scenario_id);
    const bool inside = current && overlaps(
        player_position,
        player_judgement,
        current->position,
        kTransportJudgement);
    const bool entered = inside && !player_inside_;
    player_inside_ = inside;
    return entered;
}

bool PlayerTransportSpell::atTownEndpoint(
    std::int32_t scenario_id) const {
    return active_ && scenario_id == town_.scenario_id;
}

const std::array<PlayerTransportBeam, 4>&
PlayerTransportSpell::beams() const {
    return beams_;
}

bool PlayerTransportSpell::centerVisible() const {
    return active_ && beams_.back().height == 0;
}

std::int32_t PlayerTransportSpell::animationFrame(
    std::int32_t scenario_id) const {
    return animation_frame_[atTownEndpoint(scenario_id) ? 1 : 0];
}

PlayerTransportPresentationUpdate
PlayerTransportSpell::updatePresentation(
    std::int32_t scenario_id,
    std::int32_t animation_frame_count,
    RetailRandom& random) {
    PlayerTransportPresentationUpdate result;
    if (!endpoint(scenario_id)) {
        return result;
    }
    result.start_sound_due = presentation_counter_ == 0;

    const std::size_t endpoint_index =
        atTownEndpoint(scenario_id) ? 1u : 0u;
    if (centerVisible() && animation_frame_count > 0) {
        std::int32_t& wait = animation_wait_[endpoint_index];
        std::int32_t& frame = animation_frame_[endpoint_index];
        if (wait > 0) {
            --wait;
        } else {
            result.loop_sound_due = frame == 0;
            if (frame >= animation_frame_count - 1) {
                frame = 0;
                wait = random.next() % 60 + 30;
            } else {
                ++frame;
            }
        }
    }

    for (PlayerTransportBeam& beam : beams_) {
        if (beam.delay > presentation_counter_) {
            continue;
        }
        beam.height = std::max<std::int32_t>(beam.height - 50, 0);
        beam.strength =
            std::min<std::int32_t>(beam.strength + 200, 1000);
    }
    ++presentation_counter_;
    return result;
}

void PlayerTransportSpell::consume() {
    clear();
}

bool chooseRetailPlayerTransportPosition(
    const GroundMap& ground,
    const ObjectMap& objects,
    WorldPosition player_position,
    WorldPosition aim_position,
    WorldPosition& result) {
    const std::int32_t dx = aim_position.x - player_position.x;
    const std::int32_t dy = aim_position.y - player_position.y;
    std::int32_t preferred = 0;
    if (std::abs(dy) < std::abs(dx)) {
        preferred = dx >= 0 ? 3 : 2;
    } else {
        preferred = dy >= 0 ? 1 : 0;
    }

    std::array<std::int32_t, 4> order{
        preferred, 0, 1, 2,
    };
    std::size_t used = 1;
    for (std::int32_t direction = 0; direction < 4; ++direction) {
        if (direction != preferred) {
            order[used++] = direction;
        }
    }
    for (std::int32_t direction : order) {
        const TransportDirection& candidate =
            kDirections[static_cast<std::size_t>(direction)];
        if (!positionIsWalkable(
                ground,
                objects,
                player_position,
                candidate.corridor,
                true)) {
            continue;
        }
        result = {
            player_position.x + candidate.offset.x,
            player_position.y + candidate.offset.y,
        };
        return true;
    }
    return false;
}

}  // namespace osf
