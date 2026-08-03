#include "runtime/platform/platform_text_input.hpp"

#include <switch.h>

#include <algorithm>

namespace osf::runtime {

std::string openPlatformTextInput(
    const std::string& title,
    const std::string& initial_text,
    int max_length) {
    SwkbdConfig keyboard;
    char text[64]{};

    max_length = std::clamp(max_length, 1, 63);
    if (R_FAILED(swkbdCreate(&keyboard, 0))) {
        return {};
    }

    swkbdConfigMakePresetUserName(&keyboard);
    swkbdConfigSetHeaderText(&keyboard, title.c_str());
    swkbdConfigSetGuideText(
        &keyboard, "Enter a name for your character.");
    swkbdConfigSetStringLenMax(&keyboard, max_length);
    swkbdConfigSetInitialText(&keyboard, initial_text.c_str());

    const bool accepted =
        R_SUCCEEDED(swkbdShow(&keyboard, text, sizeof(text)));
    swkbdClose(&keyboard);
    return accepted ? std::string(text) : std::string();
}

}  // namespace osf::runtime
