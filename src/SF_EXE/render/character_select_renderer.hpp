#ifndef OPENSHADOWFLARE_CHARACTER_SELECT_RENDERER_HPP
#define OPENSHADOWFLARE_CHARACTER_SELECT_RENDERER_HPP

#include "gapi/bitmap.hpp"
#include "gapi/gapi.hpp"
#include "gapi/njp.hpp"
#include "states/menu_states.hpp"
#include "states/save_catalog.hpp"

#include <vector>

namespace osf {

void renderCharacterSelect(
    gapi::Backend& renderer,
    const gapi::NjpImage& select,
    const gapi::NjpImage* font,
    const CharacterSelectStateData& data,
    const CharacterSelectFrameResult& frame,
    const CharacterSelectFrameInput& input,
    const std::vector<RetailSaveSummary>& saved_games,
    const std::vector<gapi::BitmapImage>& saved_previews);

}  // namespace osf

#endif
