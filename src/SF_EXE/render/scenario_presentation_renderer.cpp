#include "scenario_presentation_renderer.hpp"

#include "gapi/gapi.hpp"
#include "world/world_scene.hpp"

#include <cstddef>

namespace osf {

void renderScenarioVisual(
    gapi::Backend& renderer,
    const WorldScene& world) {
    const ScenarioVisualPresentation& visual =
        world.scenarioVisual();
    if (!visual.active()) {
        return;
    }

    const gapi::NjpImage& patterns =
        world.scenarioVisualPatterns();
    const std::size_t pattern =
        visual.visualId() == 0 ? 4 : visual.page();
    const std::int32_t strength = visual.fadeStrength();
    renderer.drawPattern(
        patterns,
        pattern,
        {0, 0, 1000, 1000, 1000, 1000,
         strength, strength, strength});

    if (visual.continueVisible()) {
        renderer.drawPattern(
            world.scenarioVisualContinuePatterns(),
            0,
            {590 + visual.continueOffset(),
             440,
             1000,
             1000,
             1000,
             1000,
             strength,
             strength,
             strength});
    }
}

void renderScenarioScreenParticles(
    gapi::Backend& renderer,
    const WorldScene& world) {
    for (const ScenarioScreenParticle& particle :
         world.scenarioScreenParticles().particles()) {
        renderer.drawLine({
            particle.start_x,
            particle.start_y,
            particle.end_x,
            particle.end_y,
            particle.color,
            1000,
            particle.opacity,
            {},
        });
    }
}

}  // namespace osf
