#include "runtime_effect_system.hpp"

#include "core/retail_integer.hpp"
#include "core/retail_random.hpp"
#include "resources/effect_visual_resource.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>
#include <vector>

namespace osf {
namespace {

constexpr std::int32_t kRuntimeEffectCharacterBase = 50000000;

std::int32_t animationLength(
    const EffectVisualResource* visual,
    std::int32_t chart,
    std::int32_t direction) {
    if (!visual ||
        chart < 0 ||
        direction < 0 ||
        static_cast<std::size_t>(chart) >=
            visual->animation().charts().size() ||
        static_cast<std::size_t>(direction) >=
            visual->animation()
                .charts()[static_cast<std::size_t>(chart)]
                .directions.size()) {
        return 0;
    }
    return visual->animation()
        .charts()[static_cast<std::size_t>(chart)]
        .directions[static_cast<std::size_t>(direction)]
        .frame_count;
}

void appendAudio(
    std::vector<RuntimeEffectAudioRequest>& destination,
    std::vector<RuntimeEffectAudioRequest> source) {
    destination.insert(
        destination.end(),
        std::make_move_iterator(source.begin()),
        std::make_move_iterator(source.end()));
}

}  // namespace

void RuntimeEffectSystem::clear() {
    controllers_.clear();
    actors_.clear();
    next_actor_id_ = 0;
}

bool RuntimeEffectSystem::queue(
    const CombatEffectSpawnRequest& request,
    const TableDatabase* tables) {
    EnemyEffectController controller;
    if (!controller.initialize(request, tables)) {
        return false;
    }
    controllers_.push_back({
        std::move(controller),
        request.owner_kind,
        request.source_character_number,
    });
    return true;
}

RuntimeEffectSystemUpdate RuntimeEffectSystem::update(
    const RuntimeEffectSystemContext& context) {
    RuntimeEffectSystemUpdate result;
    if (!context.ground ||
        !context.objects ||
        !context.random ||
        !context.provide_targets ||
        !context.resolve_visual) {
        return result;
    }

    actors_.erase(
        std::remove_if(
            actors_.begin(),
            actors_.end(),
            [](const RuntimeEffectActor& actor) {
                return actor.expired() &&
                       actor.hasUpdated();
            }),
        actors_.end());

    for (RuntimeEffectActor& actor : actors_) {
        const std::vector<RuntimeEffectTargetSnapshot>
            targets =
                actor.needsTargetSnapshots()
                    ? context.provide_targets()
                    : std::vector<
                          RuntimeEffectTargetSnapshot>{};
        RuntimeEffectActorUpdate update =
            actor.update(
                *context.ground,
                *context.objects,
                targets,
                *context.random);
        appendAudio(
            result.audio,
            std::move(update.audio));
        if (!actor.hasPacket()) {
            continue;
        }
        for (const RuntimeEffectTargetContact& contact :
             update.target_contacts) {
            if (contact.receiver_action ==
                RuntimeEffectReceiverAction::none) {
                continue;
            }
            RuntimeEffectReceiverDispatch dispatch;
            dispatch.contact = contact;
            dispatch.packet = actor.packet();
            dispatch.owner_kind = actor.ownerKind();
            dispatch.source_character_number =
                actor.sourceCharacterNumber();
            result.dispatches.push_back(dispatch);
            if (context.receive) {
                context.receive(dispatch);
            }
        }
    }

    for (ControllerEntry& entry : controllers_) {
        const EnemyEffectControllerSource source =
            context.resolve_source
                ? context.resolve_source(
                      entry.owner_kind,
                      entry.source_character_number)
                : EnemyEffectControllerSource{};
        EnemyEffectControllerUpdate update =
            entry.controller.update({
                source,
                context.random,
                context.placement_is_clear,
                context.provide_observer
                    ? context.provide_observer()
                    : EnemyEffectControllerSource{},
                [&context](
                    std::int32_t resource_id,
                    std::int32_t chart,
                    std::int32_t direction) {
                    return animationLength(
                        context.resolve_visual(
                            resource_id),
                        chart,
                        direction);
                },
                [this](std::int32_t actor_identifier) {
                    const auto actor = std::find_if(
                        actors_.begin(),
                        actors_.end(),
                        [actor_identifier](
                            const RuntimeEffectActor& candidate) {
                            return candidate.actorIdentifier() ==
                                actor_identifier;
                        });
                    return actor != actors_.end()
                        ? EnemyEffectControllerSource{
                              true, actor->position()}
                        : EnemyEffectControllerSource{};
                },
            });
        if (update.camera_shake) {
            result.camera_shake = true;
            result.camera_shake_duration =
                update.camera_shake_duration;
            result.camera_shake_magnitude =
                update.camera_shake_magnitude;
        }
        for (std::size_t index = 0;
             index < update.audio_count;
             ++index) {
            result.audio.push_back({
                {0, update.audio[index].sample},
                update.audio[index].position,
                false,
            });
        }
        for (std::size_t index = 0;
             index < update.actor_spawn_count;
             ++index) {
            RuntimeEffectActorSpawnRequest request =
                update.actor_spawns[index];
            request.actor_identifier =
                retailAdd(
                    kRuntimeEffectCharacterBase,
                    next_actor_id_);
            next_actor_id_ =
                retailAdd(next_actor_id_, 1);
            const EffectVisualResource* visual =
                request.resource_id >= 0
                    ? context.resolve_visual(
                          request.resource_id)
                    : nullptr;
            EnemyEffectControllerSource spawned_actor;
            if (request.resource_id >= 0 &&
                !visual) {
                if (request.track_for_controller) {
                    entry.controller.bindSpawnedActor(
                        request.actor_identifier,
                        spawned_actor);
                }
                continue;
            }
            RuntimeEffectActor actor;
            if (actor.initialize(request, visual)) {
                spawned_actor = {
                    true, actor.position()};
                actors_.push_back(std::move(actor));
            }
            if (request.track_for_controller) {
                entry.controller.bindSpawnedActor(
                    request.actor_identifier,
                    spawned_actor);
            }
        }
    }
    controllers_.erase(
        std::remove_if(
            controllers_.begin(),
            controllers_.end(),
            [](const ControllerEntry& entry) {
                return !entry.controller.active();
            }),
        controllers_.end());
    return result;
}

const std::vector<RuntimeEffectActor>&
RuntimeEffectSystem::actors() const {
    return actors_;
}

std::size_t RuntimeEffectSystem::controllerCount() const {
    return controllers_.size();
}

}  // namespace osf
