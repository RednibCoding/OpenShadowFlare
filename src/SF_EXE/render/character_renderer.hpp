#ifndef OPENSHADOWFLARE_CHARACTER_RENDERER_HPP
#define OPENSHADOWFLARE_CHARACTER_RENDERER_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace osf {

namespace gapi {
class Backend;
class NjpImage;
}

struct CharacterColorStrength {
    std::int32_t red = 1000;
    std::int32_t green = 1000;
    std::int32_t blue = 1000;
};

using CharacterPartEnabled =
    std::function<bool(std::size_t)>;
using CharacterPartColor =
    std::function<CharacterColorStrength(std::size_t)>;

void renderCharacterAnimationPass(
    gapi::Backend& renderer,
    const gapi::CafAnimation& animation,
    const gapi::NjpImage& patterns,
    const gapi::NjpImage& shadow_patterns,
    WorldPosition position,
    std::int32_t chart_index,
    std::int32_t direction_index,
    std::int32_t animation_frame,
    const CharacterPartEnabled& part_enabled,
    const CharacterPartColor& part_color,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity,
    std::int32_t screen_height = 0,
    std::int32_t opacity = 1000);

}  // namespace osf

#endif
