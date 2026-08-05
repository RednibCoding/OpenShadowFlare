#pragma once

#include <string>

namespace osf::runtime {

// Opens a platform text-input method for one short field. An empty result
// means that the user cancelled or did not enter any text.
std::string openPlatformTextInput(
    const std::string& title,
    const std::string& initialText,
    int maxLength);

}  // namespace osf::runtime
