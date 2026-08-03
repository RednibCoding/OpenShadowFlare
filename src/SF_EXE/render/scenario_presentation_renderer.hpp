#ifndef OPENSHADOWFLARE_SCENARIO_PRESENTATION_RENDERER_HPP
#define OPENSHADOWFLARE_SCENARIO_PRESENTATION_RENDERER_HPP

namespace osf {

class WorldScene;

namespace gapi {
class Backend;
}

void renderScenarioVisual(
    gapi::Backend& renderer,
    const WorldScene& world);

void renderScenarioScreenParticles(
    gapi::Backend& renderer,
    const WorldScene& world);

}  // namespace osf

#endif
