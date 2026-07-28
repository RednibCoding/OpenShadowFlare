#ifndef OPENSHADOWFLARE_TITLE_RENDERER_HPP
#define OPENSHADOWFLARE_TITLE_RENDERER_HPP

#include "gapi/gapi.hpp"
#include "states/menu_states.hpp"

#include <array>

namespace osf::gapi {
class CafAnimation;
class NjpImage;
}

namespace osf {

struct TitleSmokeAsset {
    const gapi::NjpImage* patterns = nullptr;
    const gapi::CafAnimation* animation = nullptr;
};

void renderTitle(
    gapi::Backend& renderer,
    const gapi::NjpImage& title,
    const std::array<TitleSmokeAsset, 10>& smoke,
    const TitleFrameResult& frame);

}  // namespace osf

#endif
