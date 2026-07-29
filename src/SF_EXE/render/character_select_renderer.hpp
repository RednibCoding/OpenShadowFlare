#ifndef OPENSHADOWFLARE_CHARACTER_SELECT_RENDERER_HPP
#define OPENSHADOWFLARE_CHARACTER_SELECT_RENDERER_HPP

#include "gapi/gapi.hpp"
#include "libs/RKC_DIB/rkc_dib.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "states/character_select_state.hpp"
#include "states/save_catalog.hpp"

#include <vector>

namespace osf {

void renderCharacterSelect(
    gapi::Backend& renderer,
    const gapi::NjpImage& select,
    const gapi::NjpImage* font,
    const CharacterSelectStateData& data,
    const CharacterSelectFrameResult& frame,
    const std::vector<RetailSaveSummary>& saved_games,
    const std::vector<gapi::BitmapImage>& saved_previews);

}  // namespace osf

#endif
