#include "player_voice.hpp"

namespace osf {
std::int32_t retailPlayerAttackVoiceSample(
    std::int32_t retail_gender) {
    // FUN_00435e60 chooses sample 96 for raw gender one and 99 otherwise.
    return retail_gender == 1 ? 96 : 99;
}

std::int32_t retailPlayerDeathVoiceSample(
    std::int32_t retail_gender) {
    // FUN_00435b60 plays Voice00 sample 14 minus the raw gender field.
    return 14 - (retail_gender == 1 ? 1 : 0);
}

}  // namespace osf
