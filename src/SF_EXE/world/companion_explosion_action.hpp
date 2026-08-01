#ifndef OPENSHADOWFLARE_COMPANION_EXPLOSION_ACTION_HPP
#define OPENSHADOWFLARE_COMPANION_EXPLOSION_ACTION_HPP

#include "combat_packet.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace osf {

class RetailRandom;
class TableDatabase;

namespace gapi {
class CafAnimation;
}

struct CompanionExplosionAnimationTiming {
    std::int32_t departure_frame_count = 0;
    std::int32_t arrival_frame_count = 0;
    std::vector<std::int16_t> arrival_frame_statuses;
};

struct CompanionExplosionActionEvent {
    bool relocate_due = false;
    bool impact_due = false;
    bool completed = false;
};

bool buildCompanionExplosionAnimationTiming(
    const gapi::CafAnimation& animation,
    CompanionExplosionAnimationTiming& timing);

class CompanionExplosionActionController {
public:
    bool start(
        WorldPosition destination,
        CompanionExplosionAnimationTiming timing);
    CompanionExplosionActionEvent update();
    void cancel();

    bool active() const;
    WorldPosition destination() const;
    std::int32_t animationChart() const;
    std::int32_t animationFrame() const;

private:
    CompanionExplosionAnimationTiming timing_;
    WorldPosition destination_;
    std::int32_t counter_ = 0;
    std::int32_t animation_chart_ = 6;
    std::int32_t animation_frame_ = 0;
    bool active_ = false;
};

struct CompanionExplosionPacketInput {
    std::int32_t source_character_number = -1;
    std::int32_t source_level = 1;
    std::int32_t scaling_level = 1;
    std::int32_t damage_value = 1;
    std::int32_t magical_hit_rate = 0;
    std::array<std::int32_t, 8> element_affinities{};
    std::array<std::int32_t, 17> state_words{};
};

CombatPacket buildCompanionExplosionPacket(
    const CompanionExplosionPacketInput& input,
    const TableDatabase& tables,
    RetailRandom& random);

}  // namespace osf

#endif
