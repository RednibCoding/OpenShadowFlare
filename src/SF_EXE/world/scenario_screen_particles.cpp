#include "scenario_screen_particles.hpp"

#include "core/retail_random.hpp"

#include <algorithm>
#include <cmath>

namespace osf {
namespace {

constexpr double kRetailThreeQuarterTurn = 4.712388;
constexpr double kRetailPi = 3.141592;

std::int32_t retailProjection(double value) {
    return static_cast<std::int32_t>(value);
}

std::int32_t clampColor(std::int32_t value) {
    return std::clamp(
        value, std::int32_t{0}, std::int32_t{255});
}

}  // namespace

void ScenarioScreenParticles::clear() {
    states_.clear();
    particles_.clear();
    requested_color_ = {};
    requested_count_ = 0;
    requested_ = false;
}

void ScenarioScreenParticles::request(
    std::int32_t red,
    std::int32_t green,
    std::int32_t blue,
    std::int32_t count) {
    requested_color_ = {
        static_cast<std::uint8_t>(clampColor(red)),
        static_cast<std::uint8_t>(clampColor(green)),
        static_cast<std::uint8_t>(clampColor(blue)),
        255,
    };
    requested_count_ = count;
    requested_ = true;
}

void ScenarioScreenParticles::update(RetailRandom& random) {
    if (requested_ && requested_count_ > 0) {
        for (std::int32_t index = 0;
             index < requested_count_;
             ++index) {
            ParticleState particle;
            particle.origin_x = random.next() % 1000 - 180;
            particle.speed = random.next() % 1000 + 2000;
            particle.step = random.next() % 10 + 5;
            particle.opacity = random.next() % 701 + 300;
            particle.color = requested_color_;
            particle.angle =
                kRetailThreeQuarterTurn -
                kRetailPi /
                    static_cast<double>(random.next() % 10 + 10);
            states_.insert(states_.begin(), particle);
        }
    }
    requested_ = false;

    particles_.clear();
    for (auto state = states_.begin(); state != states_.end();) {
        const std::int32_t distance =
            state->speed * state->age / 100;
        const std::int32_t start_x =
            state->origin_x + retailProjection(
                std::cos(state->angle) * distance);
        const std::int32_t start_y =
            state->origin_y - retailProjection(
                std::sin(state->angle) * distance);
        if (start_y >= 479) {
            state = states_.erase(state);
            continue;
        }
        const std::int32_t end_x =
            start_x + retailProjection(
                std::cos(state->angle) * state->step);
        const std::int32_t end_y =
            start_y - retailProjection(
                std::sin(state->angle) * state->step);
        particles_.push_back({
            start_x,
            start_y,
            end_x,
            end_y,
            state->opacity,
            state->color,
        });
        ++state->age;
        ++state;
    }
}

const std::vector<ScenarioScreenParticle>&
ScenarioScreenParticles::particles() const {
    return particles_;
}

}  // namespace osf
