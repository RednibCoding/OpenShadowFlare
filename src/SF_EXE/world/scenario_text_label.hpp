#ifndef OPENSHADOWFLARE_SCENARIO_TEXT_LABEL_HPP
#define OPENSHADOWFLARE_SCENARIO_TEXT_LABEL_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>
#include <string>

namespace osf {

struct ScenarioTextLabel {
    WorldPosition anchor;
    std::int32_t offset_x = 0;
    std::int32_t offset_y = 0;
    std::string text;
    std::int32_t red = 0;
    std::int32_t green = 0;
    std::int32_t blue = 0;
    std::int32_t background_opacity = 0;
};

}  // namespace osf

#endif
