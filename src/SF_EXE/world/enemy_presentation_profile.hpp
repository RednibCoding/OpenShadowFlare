#ifndef OPENSHADOWFLARE_ENEMY_PRESENTATION_PROFILE_HPP
#define OPENSHADOWFLARE_ENEMY_PRESENTATION_PROFILE_HPP

#include <array>
#include <cstdint>

namespace osf {

struct EnemyPresentationProfile {
    // Pre-AI initializer word 7 is copied to packet word 31 by
    // both attack families. Pre-AI word 6 is copied to direct
    // packet word 32. Their gameplay meanings stay unnamed until
    // the packet consumer is reconstructed.
    std::int32_t packet_word_31 = 0;
    std::int32_t direct_packet_word_32 = 0;

    std::array<std::int32_t, 3>
        direct_packet_word_4{};
    std::array<std::int32_t, 3>
        direct_hit_rate{};
    std::array<std::int32_t, 3>
        direct_packet_word_40{};
    std::array<std::int32_t, 3>
        direct_packet_word_41{};
    std::array<std::int32_t, 3>
        direct_packet_word_43{};
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

    std::int32_t direct_special_effect_number = -1;
    std::int32_t direct_special_constructor_value_6 = 0;
    std::int32_t direct_special_constructor_value_7 = 0;
    std::int32_t direct_special_constructor_value_21 = 0;
    std::int32_t direct_special_variant = -1;
};

}  // namespace osf

#endif
