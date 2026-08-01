#ifndef OPENSHADOWFLARE_RUNTIME_EFFECT_SYSTEM_HPP
#define OPENSHADOWFLARE_RUNTIME_EFFECT_SYSTEM_HPP

#include "enemy_effect_controller.hpp"
#include "runtime_effect_actor.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace osf {

class EffectVisualResource;
class RetailRandom;
class TableDatabase;

struct RuntimeEffectReceiverDispatch {
    RuntimeEffectTargetContact contact;
    CombatPacket packet;
    std::int32_t owner_kind = 0;
    std::int32_t source_character_number = -1;
};

using RuntimeEffectSourceResolver =
    std::function<EnemyEffectControllerSource(
        std::int32_t owner_kind,
        std::int32_t source_character_number)>;
using RuntimeEffectTargetProvider =
    std::function<std::vector<RuntimeEffectTargetSnapshot>()>;
using RuntimeEffectVisualResolver =
    std::function<const EffectVisualResource*(
        std::int32_t resource_id)>;
using RuntimeEffectReceiver =
    std::function<void(
        const RuntimeEffectReceiverDispatch& dispatch)>;
using RuntimeEffectObserverProvider =
    std::function<EnemyEffectControllerSource()>;

struct RuntimeEffectSystemContext {
    const GroundMap* ground = nullptr;
    const ObjectMap* objects = nullptr;
    RetailRandom* random = nullptr;
    RuntimeEffectSourceResolver resolve_source;
    RuntimeEffectTargetProvider provide_targets;
    RuntimeEffectVisualResolver resolve_visual;
    RuntimeEffectReceiver receive;
    EnemyEffectPlacementTest placement_is_clear;
    RuntimeEffectObserverProvider provide_observer;
};

struct RuntimeEffectSystemUpdate {
    std::vector<RuntimeEffectReceiverDispatch> dispatches;
    std::vector<RuntimeEffectAudioRequest> audio;
    bool camera_shake = false;
    std::int32_t camera_shake_duration = 0;
    std::int32_t camera_shake_magnitude = 0;
};

class RuntimeEffectSystem {
public:
    void clear();
    bool queue(
        const CombatEffectSpawnRequest& request,
        const TableDatabase* tables = nullptr);
    bool queueActor(
        const RuntimeEffectActorSpawnRequest& request);
    RuntimeEffectSystemUpdate update(
        const RuntimeEffectSystemContext& context);

    const std::vector<RuntimeEffectActor>& actors() const;
    std::size_t controllerCount() const;

private:
    struct ControllerEntry {
        EnemyEffectController controller;
        std::int32_t owner_kind = 0;
        std::int32_t source_character_number = -1;
    };

    std::vector<ControllerEntry> controllers_;
    std::vector<RuntimeEffectActorSpawnRequest>
        pending_actors_;
    std::vector<RuntimeEffectActor> actors_;
    std::int32_t next_actor_id_ = 0;
};

}  // namespace osf

#endif
