#ifndef OPENSHADOWFLARE_PLAYER_LAND_MINE_HPP
#define OPENSHADOWFLARE_PLAYER_LAND_MINE_HPP

#include "combat_packet.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "runtime_effect_target.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace osf {

class RetailRandom;
class TableDatabase;

struct PlayerLandMineVisual {
    std::int32_t resource_id = -1;
    bool static_pattern = false;
    WorldPosition position;
    WorldPosition previous_position;
    ObjectBounds judgement{-3, -3, 3, 3};
    std::int32_t display_height = 0;
    std::int32_t previous_display_height = 0;
    std::int32_t animation_frame = 0;
    std::int32_t lifetime = -1;
    std::int32_t age = 0;
    std::int32_t vertical_velocity = 0;
    std::int32_t vertical_acceleration = 0;
    std::int32_t bounce_count = 0;
    bool expired = false;

    WorldPosition renderPosition(double alpha) const;
    std::int32_t renderDisplayHeight(double alpha) const;
};

struct PlayerLandMineDispatch {
    RuntimeEffectTargetContact contact;
    CombatPacket packet;
    std::int32_t source_character_number = -1;
};

struct PlayerLandMineUpdate {
    std::vector<PlayerLandMineDispatch> dispatches;
    std::vector<RuntimeEffectAudioRequest> audio;
};

using PlayerLandMineAnimationLength =
    std::function<std::int32_t(std::int32_t resource_id)>;

class PlayerLandMineSystem {
public:
    void clear();
    bool ready() const;
    bool place(
        WorldPosition position,
        std::int32_t player_level,
        std::int32_t source_character_number);
    PlayerLandMineUpdate update(
        const std::vector<RuntimeEffectTargetSnapshot>& targets,
        const TableDatabase& tables,
        std::int32_t damage_bonus,
        RetailRandom& random,
        const PlayerLandMineAnimationLength& animation_length);

    const std::vector<PlayerLandMineVisual>& visuals() const;
    std::int32_t cooldown() const;
    std::size_t activeMineCount() const;

private:
    struct Mine {
        WorldPosition position;
        std::int32_t player_level = 1;
        std::int32_t source_character_number = -1;
        std::int32_t counter = 0;
        std::int32_t explosion_counter = 0;
        std::int32_t radial_distance = 0;
        bool exploded = false;
        bool expired = false;
    };

    void addAnimatedVisual(
        std::int32_t resource_id,
        WorldPosition position,
        ObjectBounds judgement,
        std::int32_t lifetime,
        std::int32_t vertical_velocity = 0,
        std::int32_t vertical_acceleration = 0);
    void beginExplosion(
        Mine& mine,
        const std::vector<RuntimeEffectTargetSnapshot>& targets,
        const TableDatabase& tables,
        std::int32_t damage_bonus,
        RetailRandom& random,
        const PlayerLandMineAnimationLength& animation_length,
        PlayerLandMineUpdate& update);
    void updateExplosionPresentation(
        Mine& mine,
        RetailRandom& random,
        const PlayerLandMineAnimationLength& animation_length);
    void updateVisuals();

    std::vector<Mine> mines_;
    std::vector<PlayerLandMineVisual> visuals_;
    std::int32_t cooldown_ = 0;
};

}  // namespace osf

#endif
