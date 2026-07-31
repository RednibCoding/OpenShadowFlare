#include "core/retail_random.hpp"
#include "resources/effect_visual_resource.hpp"
#include "world/runtime_effect_system.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

osf::CombatEffectSpawnRequest request() {
    osf::CombatEffectSpawnRequest request;
    request.valid = true;
    request.effect_number = 10001;
    request.owner_kind = 4;
    request.source_character_number = 14000017;
    request.target_kind = 1;
    request.constructor_value_6 = 60;
    request.constructor_value_7 = 250;
    request.constructor_value_12 = 0;
    request.has_packet = true;
    request.packet.write(1, 3);
    request.packet.write(36, 1000);
    return request;
}

bool testControllerActorAndReceiverOrder() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(
            OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::EffectVisualResources visuals;
    std::string visual_error;
    osf::RetailRandom random(1);
    const osf::GroundMap ground;
    const osf::ObjectMap objects;
    osf::RuntimeEffectSystem system;
    if (!check(
            system.queue(request()) &&
                system.controllerCount() == 1,
            "A supported live effect request did not enter "
            "the controller owner.")) {
        return false;
    }

    std::int32_t target_provider_calls = 0;
    std::int32_t receiver_calls = 0;
    const osf::RuntimeEffectSystemContext context{
        &ground,
        &objects,
        &random,
        [](std::int32_t owner, std::int32_t source) {
            return osf::EnemyEffectControllerSource{
                owner == 4 && source == 14000017,
                {0, 0},
            };
        },
        [&target_provider_calls] {
            ++target_provider_calls;
            osf::RuntimeEffectTargetSnapshot target;
            target.kind =
                osf::RuntimeEffectTargetKind::player;
            target.character_number = 0;
            target.identifier = 0;
            target.position = {180, 0};
            target.current_life = 100;
            return std::vector<
                osf::RuntimeEffectTargetSnapshot>{target};
        },
        [&visuals, &data_root, &visual_error](
            std::int32_t resource_id) {
            return visuals.load(
                data_root,
                resource_id,
                &visual_error);
        },
        [&receiver_calls](
            const osf::RuntimeEffectReceiverDispatch&
                dispatch) {
            if (dispatch.contact.receiver_action ==
                    osf::RuntimeEffectReceiverAction::
                        apply_packet &&
                dispatch.source_character_number ==
                    14000017 &&
                dispatch.packet[36] == 1000) {
                ++receiver_calls;
            }
        },
        {},
        {},
    };

    const osf::RuntimeEffectSystemUpdate first =
        system.update(context);
    if (!check(
            visual_error.empty() &&
                system.controllerCount() == 0 &&
                system.actors().size() == 2 &&
                !system.actors()[0].hasUpdated() &&
                !system.actors()[1].hasUpdated() &&
                first.dispatches.empty() &&
                first.audio.size() == 1 &&
                first.audio[0].sound.bank == 0 &&
                first.audio[0].sound.sample == 19 &&
                first.audio[0].position.x == 180 &&
                target_provider_calls == 0 &&
                receiver_calls == 0,
            "The controller did not spawn both unadvanced "
            "actors and sample 19 after the actor phase.")) {
        return false;
    }

    const osf::RuntimeEffectSystemUpdate second =
        system.update(context);
    if (!check(
            system.actors().size() == 2 &&
                system.actors()[0].hasUpdated() &&
                system.actors()[1].hasUpdated() &&
                system.actors()[1].expired() &&
                second.dispatches.size() == 1 &&
                second.dispatches[0].contact.identifier == 0 &&
                second.dispatches[0].contact.impact_origin.x ==
                    180 &&
                second.audio.size() == 1 &&
                second.audio[0].sound.sample == 20 &&
                target_provider_calls == 1 &&
                receiver_calls == 1,
            "The next actor phase did not resolve the live "
            "target, receiver, and configured hit sample.")) {
        return false;
    }

    const osf::RuntimeEffectSystemUpdate third =
        system.update(context);
    return check(
        system.actors().size() == 1 &&
            third.dispatches.empty() &&
            receiver_calls == 1,
        "A contact-expired actor was not retained for one render "
        "interval and pruned before the following update.");
#else
    return true;
#endif
}

bool testUnsupportedRequestStaysOutsideOwner() {
    osf::CombatEffectSpawnRequest unsupported = request();
    unsupported.effect_number = 10014;
    osf::RuntimeEffectSystem system;
    return check(
        !system.queue(unsupported) &&
            system.controllerCount() == 0 &&
        system.actors().empty(),
        "An unsupported specialized family entered the live "
        "implemented controller owner.");
}

}  // namespace

int main() {
    if (!testControllerActorAndReceiverOrder() ||
        !testUnsupportedRequestStaysOutsideOwner()) {
        return 1;
    }
    return 0;
}
