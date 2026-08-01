#pragma once

#include <string>

namespace osf::runtime {

// Opens a platform text-input method for a single short field and returns the
// entered text.
//
// Platforms with a hardware keyboard (desktop, web) return an empty string,
// signalling the caller to keep using ordinary key/text events. Platforms
// without a keyboard show a system on-screen keyboard and return its
// result; an empty string means the user cancelled or entered nothing.
std::string openPlatformTextInput(
    const std::string& title,
    const std::string& initial_text,
    int max_length);

}  // namespace osf::runtime
