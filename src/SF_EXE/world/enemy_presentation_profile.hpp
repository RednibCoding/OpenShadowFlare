#ifndef OPENSHADOWFLARE_ENEMY_PRESENTATION_PROFILE_HPP
#define OPENSHADOWFLARE_ENEMY_PRESENTATION_PROFILE_HPP

#include <array>
#include <cstdint>

namespace osf {

struct EnemyPresentationProfile {
    // Runtime initializer word 6 is copied into attack/effect packets.
    // Its gameplay meaning is not named until its packet consumer is
    // reconstructed.
    std::int32_t packet_source_value = 0;

    std::array<std::int32_t, 3>
        direct_maximum_target_distance{};
    std::array<std::int32_t, 3>
        direct_animation_chart{};
    std::array<std::int32_t, 3>
        direct_animation_speed_index{};

    std::array<std::int32_t, 3> effect_type{};
    std::array<std::int32_t, 3> effect_subtype{};
    std::array<std::int32_t, 3> effect_parameter{};
    std::array<std::int32_t, 3> effect_additive{};
    std::array<std::int32_t, 3>
        effect_animation_chart{};
    std::array<std::int32_t, 3>
        effect_animation_speed_index{};
};

}  // namespace osf

#endif
