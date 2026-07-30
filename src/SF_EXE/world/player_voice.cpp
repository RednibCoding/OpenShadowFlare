#include "player_voice.hpp"

namespace osf {
namespace {

std::int32_t retailGender(
    std::int32_t portable_gender) {
    // Portable character/save data uses zero for male and one for female.
    // The executable's live actor field uses the opposite convention.
    return portable_gender == 1 ? 0 : 1;
}

}  // namespace

std::int32_t retailPlayerAttackVoiceSample(
    std::int32_t portable_gender) {
    // FUN_00435e60 chooses sample 96 for raw gender one and 99 otherwise.
    return retailGender(portable_gender) == 1 ? 96 : 99;
}

std::int32_t retailPlayerDeathVoiceSample(
    std::int32_t portable_gender) {
    // FUN_00435b60 plays Voice00 sample 14 minus the raw gender field.
    return 14 - retailGender(portable_gender);
}

}  // namespace osf
