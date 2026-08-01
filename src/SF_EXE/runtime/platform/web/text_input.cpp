#include "runtime/platform/platform_text_input.hpp"

namespace osf::runtime {

// The browser delivers text through ordinary keyboard events, so there is no
// separate input method to open.
std::string openPlatformTextInput(
    const std::string&, const std::string&, int) {
    return std::string();
}

}  // namespace osf::runtime
