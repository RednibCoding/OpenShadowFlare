#ifndef OPENSHADOWFLARE_PLAYER_JOB_HPP
#define OPENSHADOWFLARE_PLAYER_JOB_HPP

#include <cstdint>
#include <optional>

namespace osf {

enum class PlayerJob : std::int32_t {
    hunter = 5,
    warrior = 6,
    spellcaster = 9,
    mercenary = 16,
};

constexpr std::int32_t playerJobValue(PlayerJob job) {
    return static_cast<std::int32_t>(job);
}

std::int32_t retailScriptJobSelection(std::int32_t job);
std::optional<PlayerJob> retailJobForScriptSelection(
    std::int32_t selection);

}  // namespace osf

#endif
