#include "font_resource.hpp"

namespace osf {

const std::vector<std::uint8_t>&
englishRetailFontPatternSelection() {
    static const std::vector<std::uint8_t> selection{1, 0, 1};
    return selection;
}

}  // namespace osf
