#ifndef OPENSHADOWFLARE_ENEMY_PRESENTATION_AUDIO_HPP
#define OPENSHADOWFLARE_ENEMY_PRESENTATION_AUDIO_HPP

#include <cstdint>

namespace osf {

constexpr std::int32_t kNoEnemyPresentationSample = -1;

std::int32_t retailEnemyPresentationSample(
    std::int32_t resource_id,
    std::int32_t animation_chart,
    std::int32_t marker_slot);

std::int32_t retailEnemyDeathSample(
    std::int32_t resource_id);

}  // namespace osf

#endif
