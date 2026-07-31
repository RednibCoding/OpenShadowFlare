#ifndef OPENSHADOWFLARE_PLAYER_RESOURCE_RATE_HPP
#define OPENSHADOWFLARE_PLAYER_RESOURCE_RATE_HPP

#include <cstdint>

namespace osf {

struct PlayerResourceRateUpdate {
    std::int32_t value = 0;
    bool changed = false;
};

class PlayerResourceRateController {
public:
    PlayerResourceRateUpdate update(
        std::int32_t current_value,
        std::int32_t maximum_value,
        std::int32_t percentage_rate,
        std::int32_t minimum_value,
        bool resource_active = true);
    void clear();

    std::int32_t remainder() const;
    std::int32_t updateCounter() const;

private:
    std::int32_t remainder_ = 0;
    std::int32_t update_counter_ = 0;
};

}  // namespace osf

#endif
