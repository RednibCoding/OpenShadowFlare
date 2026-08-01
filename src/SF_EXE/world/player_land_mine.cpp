#include "player_land_mine.hpp"

#include "actor_direction.hpp"
#include "core/retail_integer.hpp"
#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "movement_controller.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kMinePattern = 1000;
constexpr std::int32_t kExplosionPattern = 1001;
constexpr std::int32_t kFirstRingPattern = 1002;
constexpr std::int32_t kLastRingPattern = 1004;
constexpr std::int32_t kFirstBouncePattern = 1005;
constexpr std::int32_t kLastBouncePattern = 1008;
constexpr std::int32_t kTargetMask = 0x04 | 0x10;
constexpr ObjectBounds kMineJudgement{-150, -150, 150, 150};
constexpr ObjectBounds kExplosionJudgement{-600, -600, 600, 600};
constexpr ObjectBounds kDebrisJudgement{-3, -3, 3, 3};

WorldPosition projectedPosition(
    WorldPosition origin,
    std::int32_t angle_degrees,
    std::int32_t distance) {
    const double angle =
        static_cast<double>(angle_degrees) *
        kRetailRadiansPerDegree;
    return {
        retailAdd(
            origin.x,
            static_cast<std::int32_t>(std::cos(angle) * distance)),
        retailSubtract(
            origin.y,
            static_cast<std::int32_t>(std::sin(angle) * distance)),
    };
}

CombatPacket minePacket(
    std::int32_t source_character_number,
    std::int32_t player_level,
    std::int32_t damage,
    RetailRandom& random) {
    CombatPacket packet;
    packet.write(0, 0);
    packet.write(1, 0);
    packet.write(2, source_character_number);
    packet.write(3, 0);
    packet.write(4, std::max(damage, 1));
    packet.write(31, player_level);
    packet.write(34, random.next() % 4 + 21000);
    packet.write(35, 8);
    packet.write(36, 99999);
    packet.write(37, 0);
    packet.write(38, 1);
    packet.write(41, -1);
    packet.write(43, -1);
    packet.write(72, 1);
    packet.write(73, -1);
    packet.write(74, -1);
    packet.write(75, 8);
    packet.write(76, 0);
    return packet;
}

std::int32_t mineDamage(
    const TableDatabase& tables,
    std::int32_t player_level,
    std::int32_t bonus) {
    const TableData* damage = tables.find(23);
    const std::int32_t row = std::max(player_level - 1, 0);
    return std::max(
        damage && damage->contains(row, 0)
            ? retailAdd(damage->value(row, 0), bonus)
            : retailAdd(1, bonus),
        1);
}

bool mineTriggered(
    WorldPosition mine_position,
    const std::vector<RuntimeEffectTargetSnapshot>& targets,
    RetailRandom& random) {
    RuntimeEffectTargetQuery query;
    query.actor_position = mine_position;
    query.actor_judgement = kMineJudgement;
    query.target_mask = kTargetMask;
    query.expire_on_target = true;
    query.expire_on_object_contact = true;
    RuntimeEffectTargetMemory memory;
    return resolveRuntimeEffectTargets(
               query, targets, memory, random)
        .expired;
}

}  // namespace

WorldPosition PlayerLandMineVisual::renderPosition(
    double alpha) const {
    return interpolateWorldPosition(
        previous_position, position, alpha);
}

std::int32_t PlayerLandMineVisual::renderDisplayHeight(
    double alpha) const {
    return static_cast<std::int32_t>(
        previous_display_height +
        (display_height - previous_display_height) * alpha);
}

void PlayerLandMineSystem::clear() {
    mines_.clear();
    visuals_.clear();
    cooldown_ = 0;
}

bool PlayerLandMineSystem::ready() const {
    return cooldown_ == 0;
}

bool PlayerLandMineSystem::place(
    WorldPosition position,
    std::int32_t player_level,
    std::int32_t source_character_number) {
    if (!ready() || source_character_number < 0) {
        return false;
    }
    mines_.push_back({
        position,
        std::max(player_level, 1),
        source_character_number,
    });
    PlayerLandMineVisual visual;
    visual.resource_id = kMinePattern;
    visual.static_pattern = true;
    visual.position = position;
    visual.previous_position = position;
    visual.judgement = kMineJudgement;
    visuals_.push_back(visual);
    cooldown_ = 10;
    return true;
}

void PlayerLandMineSystem::addAnimatedVisual(
    std::int32_t resource_id,
    WorldPosition position,
    ObjectBounds judgement,
    std::int32_t lifetime,
    std::int32_t vertical_velocity,
    std::int32_t vertical_acceleration) {
    PlayerLandMineVisual visual;
    visual.resource_id = resource_id;
    visual.position = position;
    visual.previous_position = position;
    visual.judgement = judgement;
    visual.lifetime = std::max(lifetime, 1);
    visual.vertical_velocity = vertical_velocity;
    visual.vertical_acceleration = vertical_acceleration;
    visuals_.push_back(visual);
}

void PlayerLandMineSystem::beginExplosion(
    Mine& mine,
    const std::vector<RuntimeEffectTargetSnapshot>& targets,
    const TableDatabase& tables,
    std::int32_t damage_bonus,
    RetailRandom& random,
    const PlayerLandMineAnimationLength& animation_length,
    PlayerLandMineUpdate& update) {
    mine.exploded = true;
    mine.explosion_counter = 1;
    const auto base = std::find_if(
        visuals_.begin(),
        visuals_.end(),
        [&mine](const PlayerLandMineVisual& visual) {
            return visual.static_pattern &&
                   visual.position.x == mine.position.x &&
                   visual.position.y == mine.position.y;
        });
    if (base != visuals_.end()) {
        visuals_.erase(base);
    }

    addAnimatedVisual(
        kExplosionPattern,
        mine.position,
        kExplosionJudgement,
        animation_length(kExplosionPattern));
    update.audio.push_back({{0, 29}, mine.position, false});

    const CombatPacket packet = minePacket(
        mine.source_character_number,
        mine.player_level,
        mineDamage(tables, mine.player_level, damage_bonus),
        random);
    RuntimeEffectTargetQuery query;
    query.actor_position = mine.position;
    query.actor_judgement = kExplosionJudgement;
    query.target_mask = kTargetMask;
    query.process_every_target = true;
    query.has_packet = true;
    query.hit_rating = packet[36];
    RuntimeEffectTargetMemory memory;
    const RuntimeEffectTargetResolution resolution =
        resolveRuntimeEffectTargets(
            query, targets, memory, random);
    for (const RuntimeEffectTargetContact& contact :
         resolution.contacts) {
        if (contact.receiver_action !=
            RuntimeEffectReceiverAction::none) {
            update.dispatches.push_back({
                contact,
                packet,
                mine.source_character_number,
            });
        }
    }
}

void PlayerLandMineSystem::updateExplosionPresentation(
    Mine& mine,
    RetailRandom& random,
    const PlayerLandMineAnimationLength& animation_length) {
    const std::int32_t counter = mine.explosion_counter;
    if (counter >= 12 && counter <= 40 && counter % 2 == 0) {
        mine.radial_distance = retailAdd(mine.radial_distance, 50);
        for (std::int32_t resource = kFirstRingPattern;
             resource <= kLastRingPattern;
             ++resource) {
            const WorldPosition position = projectedPosition(
                mine.position,
                random.next() % 360,
                mine.radial_distance);
            addAnimatedVisual(
                resource,
                position,
                kDebrisJudgement,
                animation_length(resource));
        }
    }
    if (counter == 12) {
        for (std::int32_t resource = kFirstBouncePattern;
             resource <= kLastBouncePattern;
             ++resource) {
            const WorldPosition position = projectedPosition(
                mine.position,
                random.next() % 360,
                50);
            addAnimatedVisual(
                resource,
                position,
                kDebrisJudgement,
                60,
                1500,
                -100);
            addAnimatedVisual(
                kLastRingPattern,
                position,
                kDebrisJudgement,
                60,
                1500,
                -100);
        }
    }
    mine.explosion_counter = retailAdd(
        mine.explosion_counter, 1);
    if (mine.explosion_counter >= 80) {
        mine.expired = true;
    }
}

void PlayerLandMineSystem::updateVisuals() {
    for (PlayerLandMineVisual& visual : visuals_) {
        if (visual.expired || visual.static_pattern) {
            continue;
        }
        visual.previous_position = visual.position;
        visual.previous_display_height = visual.display_height;
        visual.animation_frame = visual.age;
        if (visual.vertical_velocity != 0 ||
            visual.vertical_acceleration != 0) {
            visual.display_height = retailAdd(
                visual.display_height,
                visual.vertical_velocity / 10);
            visual.vertical_velocity = retailAdd(
                visual.vertical_velocity,
                visual.vertical_acceleration);
            if (visual.display_height < 0) {
                visual.display_height = 0;
                if (visual.bounce_count == 0) {
                    visual.vertical_velocity = 400;
                    ++visual.bounce_count;
                } else {
                    visual.vertical_velocity = 0;
                    visual.vertical_acceleration = 0;
                }
            }
        }
        ++visual.age;
        if (visual.lifetime >= 0 &&
            visual.age >= visual.lifetime) {
            visual.expired = true;
        }
    }
    visuals_.erase(
        std::remove_if(
            visuals_.begin(),
            visuals_.end(),
            [](const PlayerLandMineVisual& visual) {
                return visual.expired;
            }),
        visuals_.end());
}

PlayerLandMineUpdate PlayerLandMineSystem::update(
    const std::vector<RuntimeEffectTargetSnapshot>& targets,
    const TableDatabase& tables,
    std::int32_t damage_bonus,
    RetailRandom& random,
    const PlayerLandMineAnimationLength& animation_length) {
    PlayerLandMineUpdate result;
    if (cooldown_ > 0) {
        --cooldown_;
    }
    updateVisuals();
    for (Mine& mine : mines_) {
        if (mine.expired) {
            continue;
        }
        if (mine.exploded) {
            updateExplosionPresentation(
                mine, random, animation_length);
            continue;
        }
        ++mine.counter;
        if (mine.counter >= 40 && mine.counter % 20 == 0) {
            result.audio.push_back({{0, 54}, mine.position, false});
        }
        if (mine.counter >= 40 &&
            (mineTriggered(mine.position, targets, random) ||
             mine.counter >= 300)) {
            beginExplosion(
                mine,
                targets,
                tables,
                damage_bonus,
                random,
                animation_length,
                result);
        }
    }
    mines_.erase(
        std::remove_if(
            mines_.begin(),
            mines_.end(),
            [](const Mine& mine) { return mine.expired; }),
        mines_.end());
    return result;
}

const std::vector<PlayerLandMineVisual>&
PlayerLandMineSystem::visuals() const {
    return visuals_;
}

std::int32_t PlayerLandMineSystem::cooldown() const {
    return cooldown_;
}

std::size_t PlayerLandMineSystem::activeMineCount() const {
    return mines_.size();
}

}  // namespace osf
