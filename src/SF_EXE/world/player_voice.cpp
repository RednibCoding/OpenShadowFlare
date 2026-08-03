#include "player_voice.hpp"

#include <algorithm>

namespace osf {
std::int32_t retailPlayerAttackVoiceSample(
    std::int32_t retail_gender) {
    // FUN_00435e60 chooses sample 96 for raw gender one and 99 otherwise.
    return retail_gender == 1 ? 96 : 99;
}

std::int32_t retailPlayerComboVoiceSample(
    std::int32_t retail_gender,
    std::int32_t combo_step) {
    // The three linked right-click actions use consecutive Voice00 samples.
    // Retail stores female as zero and male as one.
    const std::int32_t first = retail_gender == 1 ? 96 : 99;
    return first + std::clamp<std::int32_t>(combo_step, 0, 2);
}

std::int32_t retailPlayerDeathVoiceSample(
    std::int32_t retail_gender) {
    // FUN_00435b60 plays Voice00 sample 14 minus the raw gender field.
    return 14 - (retail_gender == 1 ? 1 : 0);
}

}  // namespace osf
