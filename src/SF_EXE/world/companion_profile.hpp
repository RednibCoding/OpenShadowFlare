#ifndef OPENSHADOWFLARE_COMPANION_PROFILE_HPP
#define OPENSHADOWFLARE_COMPANION_PROFILE_HPP

#include <cstdint>
#include <string>

namespace osf {

class TableDatabase;

struct CompanionProfile {
    std::int32_t type = -1;
    std::int32_t level = 0;
    std::string name;
    std::int32_t resource_id = -1;
    std::int32_t red_strength = 1000;
    std::int32_t green_strength = 1000;
    std::int32_t blue_strength = 1000;
    std::int32_t native_element = 0;
    std::int32_t walking_speed = 0;
    std::int32_t running_speed = 0;
    std::int32_t maximum_life = 0;
    std::int32_t physical_attack = 0;
    std::int32_t hit_rate = 0;
    std::int32_t physical_defense = 0;
    std::int32_t physical_evasion = 0;
    std::int32_t magical_attack = 0;
    std::int32_t magical_hit_rate = 0;
    std::int32_t magical_defense = 0;
    std::int32_t magical_evasion = 0;
    std::int32_t attack_speed = 0;
    std::int32_t experience_threshold = 0;
};

bool decodeCompanionProfile(
    const TableDatabase& tables,
    std::int32_t companion_type,
    std::int32_t companion_level,
    CompanionProfile& profile,
    std::string* error = nullptr);

}  // namespace osf

#endif
