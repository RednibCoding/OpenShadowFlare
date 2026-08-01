#ifndef OPENSHADOWFLARE_GENERIC_EFFECT_ACTOR_HPP
#define OPENSHADOWFLARE_GENERIC_EFFECT_ACTOR_HPP

#include "combat_effect_request.hpp"
#include "runtime_effect_actor.hpp"

namespace osf {

bool buildGenericEffectActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition resolved_source,
    RuntimeEffectActorSpawnRequest& actor);

}  // namespace osf

#endif
