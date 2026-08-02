#ifndef OPENSHADOWFLARE_SCENARIO_SCREEN_PARTICLES_HPP
#define OPENSHADOWFLARE_SCENARIO_SCREEN_PARTICLES_HPP

#include "gapi/gapi.hpp"

#include <cstdint>
#include <vector>

namespace osf {

class RetailRandom;

struct ScenarioScreenParticle {
    std::int32_t start_x = 0;
    std::int32_t start_y = 0;
    std::int32_t end_x = 0;
    std::int32_t end_y = 0;
    std::int32_t opacity = 1000;
    gapi::Color color;
};

class ScenarioScreenParticles {
public:
    void clear();
    void request(
        std::int32_t red,
        std::int32_t green,
        std::int32_t blue,
        std::int32_t count);
    void update(RetailRandom& random);

    const std::vector<ScenarioScreenParticle>& particles() const;

private:
    struct ParticleState {
        std::int32_t age = 0;
        std::int32_t origin_x = 0;
        std::int32_t origin_y = -30;
        double angle = 0.0;
        std::int32_t speed = 0;
        std::int32_t step = 0;
        std::int32_t opacity = 1000;
        gapi::Color color;
    };

    std::vector<ParticleState> states_;
    std::vector<ScenarioScreenParticle> particles_;
    gapi::Color requested_color_;
    std::int32_t requested_count_ = 0;
    bool requested_ = false;
};

}  // namespace osf

#endif
