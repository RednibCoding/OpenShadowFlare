#include "core/retail_random.hpp"
#include "gapi/gapi.hpp"
#include "render/scenario_presentation_renderer.hpp"
#include "world/scenario_screen_particles.hpp"
#include "world/scenario_visual_presentation.hpp"
#include "world/world_scene.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

class RecordingBackend final : public osf::gapi::Backend {
public:
    void beginFrame(osf::gapi::Color) override {}

    bool drawPattern(
        const osf::gapi::NjpImage& image,
        std::size_t pattern,
        const osf::gapi::PatternDraw& draw) override {
        pattern_images.push_back(&image);
        patterns.push_back(pattern);
        pattern_draws.push_back(draw);
        return true;
    }

    bool drawBitmap(
        const osf::gapi::BitmapImage&,
        const osf::gapi::BitmapDraw&) override {
        return true;
    }

    bool drawText(
        const osf::gapi::NjpImage&,
        std::string_view,
        const osf::gapi::TextDraw&) override {
        return true;
    }

    bool drawRectangle(
        const osf::gapi::RectangleDraw&) override {
        return true;
    }

    bool drawLine(const osf::gapi::LineDraw& draw) override {
        lines.push_back(draw);
        return true;
    }

    void endFrame() override {}

    std::vector<const osf::gapi::NjpImage*> pattern_images;
    std::vector<std::size_t> patterns;
    std::vector<osf::gapi::PatternDraw> pattern_draws;
    std::vector<osf::gapi::LineDraw> lines;
};

bool testVisualTiming() {
    osf::ScenarioVisualPresentation visual;
    visual.begin(2, 2);
    if (!check(
            visual.active() && visual.visualId() == 2 &&
                visual.page() == 0 && visual.counter() == 0 &&
                visual.fadeStrength() == 0 &&
                !visual.continueVisible(),
            "The scenario visual did not begin on its retail dark frame.")) {
        return false;
    }

    visual.requestAdvance();
    visual.advanceFrame();
    for (std::int32_t update = 1; update < 120; ++update) {
        visual.advanceFrame();
    }
    if (!check(
            visual.counter() == 120 &&
                visual.fadeStrength() == 1000 &&
                !visual.continueVisible() &&
                visual.page() == 0,
            "The scenario visual fade or early-input gate differs from retail.")) {
        return false;
    }
    for (std::int32_t update = 120; update < 299; ++update) {
        visual.advanceFrame();
    }
    visual.requestAdvance();
    visual.advanceFrame();
    if (!check(
            visual.page() == 1 && visual.counter() == 1 &&
                visual.fadeStrength() == 8,
            "The second scenario-visual page did not restart at retail counter one.")) {
        return false;
    }
    for (std::int32_t update = 1; update < 300; ++update) {
        visual.advanceFrame();
    }
    if (!check(
            visual.continueVisible() &&
                visual.continueOffset() == 0,
            "The scenario visual confirmation marker appeared at the wrong time.")) {
        return false;
    }
    visual.requestAdvance();
    visual.advanceFrame();
    if (!check(
            !visual.active() && visual.page() == 1,
            "The final scenario-visual frame did not close after rendering.")) {
        return false;
    }
    visual.advanceFrame();
    return check(
        !visual.active(),
        "The scenario visual did not release after its final retail frame.");
}

bool testParticleEmitter() {
    osf::RetailRandom random(1);
    osf::ScenarioScreenParticles particles;
    particles.request(224, 64, 64, 5);
    particles.update(random);
    if (!check(
            particles.particles().size() == 5,
            "The scripted particle emitter did not create its evaluated count.")) {
        return false;
    }
    for (const osf::ScenarioScreenParticle& particle :
         particles.particles()) {
        if (!check(
                particle.start_y == -30 &&
                    particle.color.red == 224 &&
                    particle.color.green == 64 &&
                    particle.color.blue == 64 &&
                    particle.opacity >= 300 &&
                    particle.opacity <= 1000,
                "A scripted particle lost its retail origin, color, or opacity.")) {
            return false;
        }
    }
    const osf::ScenarioScreenParticle& first_created =
        particles.particles().back();
    if (!check(
            first_created.start_x == -139 &&
                first_created.start_y == -30 &&
                first_created.end_x == -140 &&
                first_created.end_y == -22 &&
                first_created.opacity == 863 &&
                random.next() == 12382,
            "The particle constructor changed its five-draw retail random "
            "order or trigonometric projection.")) {
        return false;
    }

    particles.update(random);
    if (!check(
            particles.particles().size() == 5 &&
                particles.particles().front().start_y > -30,
            "Scripted particles did not advance without an extra spawn request.")) {
        return false;
    }
    for (std::int32_t update = 0; update < 40; ++update) {
        particles.update(random);
    }
    return check(
        particles.particles().empty(),
        "Scripted particles did not expire at the retail screen boundary.");
}

bool testShippedCommandsAndRendering() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    std::string error;
    osf::WorldScene visual_world;
    if (!check(
            visual_world.loadInitialScenario(
                data_root,
                osf::PlayerLoadRequest{},
                {3900000, 0, 0},
                &error),
            "The shipped Visual03 scenario could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    visual_world.commandPlayerMovement(500, 200);
    const osf::PlayerMotion motion_before_visual =
        visual_world.playerMotion();
    const std::int32_t player_x_before_visual =
        visual_world.playerWorldX();
    const std::int32_t player_y_before_visual =
        visual_world.playerWorldY();
    visual_world.update();
    if (!check(
            visual_world.scenarioVisualActive() &&
                visual_world.scenarioVisual().visualId() == 3 &&
                visual_world.scenarioVisualPatterns().patterns().size() == 1 &&
                motion_before_visual != osf::PlayerMotion::idle &&
                visual_world.playerMotion() == motion_before_visual &&
                visual_world.playerWorldX() == player_x_before_visual &&
                visual_world.playerWorldY() == player_y_before_visual,
            "Shipped opcode 64 did not open Visual03 while freezing the "
            "existing player action.")) {
        return false;
    }
    visual_world.update();
    if (!check(
            visual_world.playerMotion() == motion_before_visual &&
                visual_world.playerWorldX() == player_x_before_visual &&
                visual_world.playerWorldY() == player_y_before_visual,
            "An active scenario visual advanced or cancelled the player.")) {
        return false;
    }
    RecordingBackend visual_renderer;
    osf::renderScenarioVisual(visual_renderer, visual_world);
    if (!check(
            visual_renderer.patterns.size() == 1 &&
                visual_renderer.patterns[0] == 0 &&
                visual_renderer.pattern_draws[0].red_strength == 0,
            "The authored visual did not render its first fully dark frame.")) {
        return false;
    }

    osf::WorldScene particle_world;
    if (!check(
            particle_world.loadInitialScenario(
                data_root,
                osf::PlayerLoadRequest{},
                {3010001, 0, 0},
                &error),
            "The shipped red-particle scenario could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    particle_world.update();
    if (!check(
            particle_world.scenarioScreenParticles()
                    .particles().size() == 5,
            "Shipped opcode 65 did not emit its five authored particles.")) {
        return false;
    }
    RecordingBackend particle_renderer;
    osf::renderScenarioScreenParticles(
        particle_renderer, particle_world);
    return check(
        particle_renderer.lines.size() == 5 &&
            particle_renderer.lines[0].color.red == 224 &&
            particle_renderer.lines[0].color.green == 64 &&
            particle_renderer.lines[0].color.blue == 64,
        "The shipped screen particles did not reach the GAPI line boundary.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testVisualTiming() &&
                   testParticleEmitter() &&
                   testShippedCommandsAndRendering()
               ? 0
               : 1;
}
