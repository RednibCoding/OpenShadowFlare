#ifndef OPENSHADOWFLARE_RESOURCES_FONT_RESOURCE_HPP
#define OPENSHADOWFLARE_RESOURCES_FONT_RESOURCE_HPP

#include <cstdint>
#include <vector>

namespace osf {

// Pattern zero contains the Latin glyphs. Pattern two contains the Shift-JIS
// spacing and bracket characters used by the English retail interface.
const std::vector<std::uint8_t>&
englishRetailFontPatternSelection();

}  // namespace osf

#endif
