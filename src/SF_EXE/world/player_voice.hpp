#ifndef OPENSHADOWFLARE_PLAYER_VOICE_HPP
#define OPENSHADOWFLARE_PLAYER_VOICE_HPP

#include <cstdint>

namespace osf {

std::int32_t retailPlayerAttackVoiceSample(
    std::int32_t portable_gender);
std::int32_t retailPlayerDeathVoiceSample(
    std::int32_t portable_gender);

}  // namespace osf

#endif
