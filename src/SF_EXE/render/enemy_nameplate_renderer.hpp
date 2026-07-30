#ifndef OPENSHADOWFLARE_ENEMY_NAMEPLATE_RENDERER_HPP
#define OPENSHADOWFLARE_ENEMY_NAMEPLATE_RENDERER_HPP

#include "gapi/gapi.hpp"

#include <cstdint>
#include <string_view>

namespace osf {

struct EnemyNameplate {
    std::string_view name;
    gapi::Color name_color;
    std::int32_t current_life = 0;
    std::int32_t maximum_life = 0;
    std::int32_t native_element = 0;
    std::int32_t center_x = 0;
    std::int32_t y = 0;
};

void renderEnemyNameplate(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const gapi::NjpImage* status_icons,
    const EnemyNameplate& nameplate);

}  // namespace osf

#endif
