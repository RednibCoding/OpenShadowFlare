#include "runtime_effect_target.hpp"

#include "combat_hit_chance.hpp"
#include "core/retail_random.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace osf {
namespace {

constexpr std::int32_t kPlayerMask = 0x01;
constexpr std::int32_t kCompanionMask = 0x02;
constexpr std::int32_t kEnemyMask = 0x04;
constexpr std::int32_t kNpcMask = 0x08;
constexpr std::int32_t kScenarioObjectMask = 0x10;

std::int32_t targetMask(RuntimeEffectTargetKind kind) {
    switch (kind) {
    case RuntimeEffectTargetKind::player:
        return kPlayerMask;
    case RuntimeEffectTargetKind::companion:
        return kCompanionMask;
    case RuntimeEffectTargetKind::enemy:
        return kEnemyMask;
    case RuntimeEffectTargetKind::npc:
        return kNpcMask;
    case RuntimeEffectTargetKind::scenario_object:
        return kScenarioObjectMask;
    }
    return 0;
}

bool boundsOverlap(
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

std::int32_t positionDistance(
    WorldPosition first,
    WorldPosition second) {
    const double x =
        static_cast<double>(first.x) - second.x;
    const double y =
        static_cast<double>(first.y) - second.y;
    return static_cast<std::int32_t>(
        std::trunc(std::hypot(x, y)));
}

bool isRememberedKind(RuntimeEffectTargetKind kind) {
    return kind == RuntimeEffectTargetKind::player ||
           kind == RuntimeEffectTargetKind::companion ||
           kind == RuntimeEffectTargetKind::enemy;
}

bool eligibleState(
    const RuntimeEffectTargetQuery& query,
    const RuntimeEffectTargetSnapshot& target) {
    if (!target.present ||
        !target.same_scenario ||
        (query.target_mask & targetMask(target.kind)) == 0 ||
        (target.kind != RuntimeEffectTargetKind::player &&
         target.identifier == query.actor_identifier) ||
        (query.exact_target_only &&
         target.identifier != query.target_identifier)) {
        return false;
    }

    switch (target.kind) {
    case RuntimeEffectTargetKind::player:
        return target.character_number >= 0 &&
               target.character_number < 4 &&
               target.current_life > 0;
    case RuntimeEffectTargetKind::companion:
        return target.local_owner &&
               target.current_life > 0;
    case RuntimeEffectTargetKind::enemy:
        return target.current_life > 0 &&
               target.active;
    case RuntimeEffectTargetKind::npc:
        return true;
    case RuntimeEffectTargetKind::scenario_object:
        return target.displayed &&
               target.runtime_state == 0;
    }
    return false;
}

bool eligibleTarget(
    const RuntimeEffectTargetQuery& query,
    const RuntimeEffectTargetSnapshot& target,
    const RuntimeEffectTargetMemory& memory) {
    return boundsOverlap(
               query.actor_position,
               query.actor_judgement,
               target.position,
               target.judgement) &&
           eligibleState(query, target) &&
           (!query.remember_targets ||
            !isRememberedKind(target.kind) ||
            !memory.contains(target.identifier));
}

bool audioConfigured(const RuntimeEffectAudioPair& sound) {
    return sound.bank != -1;
}

void addAudio(
    RuntimeEffectTargetResolution& result,
    RuntimeEffectAudioPair sound,
    WorldPosition position,
    bool npc_spatial_mode) {
    if (!audioConfigured(sound)) {
        return;
    }
    result.audio.push_back({
        sound,
        position,
        npc_spatial_mode,
    });
}

RuntimeEffectTargetContact processTarget(
    const RuntimeEffectTargetQuery& query,
    const RuntimeEffectTargetSnapshot& target,
    RuntimeEffectTargetMemory& memory,
    RetailRandom& random) {
    RuntimeEffectTargetContact contact;
    contact.kind = target.kind;
    contact.character_number = target.character_number;
    contact.identifier = target.identifier;
    contact.impact_origin = query.actor_position;
    contact.distance =
        positionDistance(
            query.actor_position,
            target.position);

    if (!isRememberedKind(target.kind)) {
        return contact;
    }
    if (query.remember_targets) {
        memory.remember(target.identifier);
    }

    contact.evasion_checked = true;
    const std::int32_t evasion =
        query.has_packet && query.magical_evasion
            ? target.magical_evasion
            : target.physical_evasion;
    contact.hit_chance =
        retailCombatHitChance(
            query.has_packet ? query.hit_rating : 0,
            evasion);
    contact.hit_roll = random.next() % 100;
    contact.hit =
        contact.hit_roll < contact.hit_chance;
    if (query.has_packet) {
        contact.receiver_action =
            contact.hit
                ? RuntimeEffectReceiverAction::apply_packet
                : RuntimeEffectReceiverAction::show_miss;
    }
    return contact;
}

void applyContactEffects(
    const RuntimeEffectTargetQuery& query,
    const RuntimeEffectTargetContact& contact,
    bool process_every_target,
    bool& audio_played,
    RuntimeEffectTargetResolution& result) {
    switch (contact.kind) {
    case RuntimeEffectTargetKind::player:
    case RuntimeEffectTargetKind::companion:
    case RuntimeEffectTargetKind::enemy:
        if (query.expire_on_target &&
            !query.remember_targets) {
            result.expired = true;
        }
        if (contact.hit && !audio_played &&
            audioConfigured(query.target_audio)) {
            addAudio(
                result,
                query.target_audio,
                query.actor_position,
                false);
            audio_played = true;
        }
        break;
    case RuntimeEffectTargetKind::npc:
        if (query.expire_on_target) {
            result.expired = true;
        }
        if (!audio_played &&
            audioConfigured(query.target_audio)) {
            addAudio(
                result,
                query.target_audio,
                query.actor_position,
                process_every_target);
            audio_played = true;
        }
        break;
    case RuntimeEffectTargetKind::scenario_object:
        if (query.expire_on_object_contact) {
            result.expired = true;
        }
        if (!audio_played &&
            audioConfigured(query.object_audio)) {
            addAudio(
                result,
                query.object_audio,
                query.actor_position,
                false);
            audio_played = true;
        }
        break;
    }
}

}  // namespace

void RuntimeEffectTargetMemory::clear() {
    count_ = 0;
}

bool RuntimeEffectTargetMemory::contains(
    std::int32_t identifier) const {
    return std::find(
               identifiers_.begin(),
               identifiers_.begin() +
                   static_cast<std::ptrdiff_t>(count_),
               identifier) !=
           identifiers_.begin() +
               static_cast<std::ptrdiff_t>(count_);
}

void RuntimeEffectTargetMemory::remember(
    std::int32_t identifier) {
    if (count_ >= identifiers_.size()) {
        return;
    }
    identifiers_[count_++] = identifier;
}

std::size_t RuntimeEffectTargetMemory::count() const {
    return count_;
}

RuntimeEffectTargetResolution resolveRuntimeEffectTargets(
    const RuntimeEffectTargetQuery& query,
    const std::vector<RuntimeEffectTargetSnapshot>& targets,
    RuntimeEffectTargetMemory& memory,
    RetailRandom& random) {
    RuntimeEffectTargetResolution result;
    bool audio_played = false;

    if (query.process_every_target) {
        for (const RuntimeEffectTargetSnapshot& target :
             targets) {
            if (!eligibleTarget(query, target, memory)) {
                continue;
            }
            const RuntimeEffectTargetContact contact =
                processTarget(
                    query, target, memory, random);
            result.contacts.push_back(contact);
            applyContactEffects(
                query,
                contact,
                true,
                audio_played,
                result);
        }
        return result;
    }

    const RuntimeEffectTargetSnapshot* nearest = nullptr;
    std::int32_t nearest_distance =
        std::numeric_limits<std::int32_t>::max();
    for (const RuntimeEffectTargetSnapshot& target :
         targets) {
        if (!eligibleTarget(query, target, memory)) {
            continue;
        }
        const std::int32_t distance =
            positionDistance(
                query.actor_position,
                target.position);
        if (!nearest || distance < nearest_distance) {
            nearest = &target;
            nearest_distance = distance;
        }
    }
    if (!nearest) {
        return result;
    }

    const RuntimeEffectTargetContact contact =
        processTarget(
            query, *nearest, memory, random);
    result.contacts.push_back(contact);
    applyContactEffects(
        query,
        contact,
        false,
        audio_played,
        result);
    return result;
}

}  // namespace osf
