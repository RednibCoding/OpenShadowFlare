#ifndef OPENSHADOWFLARE_COMMAND_LINE_HPP
#define OPENSHADOWFLARE_COMMAND_LINE_HPP

#include <string_view>

namespace openshadowflare {

struct GameConfig;

// Mirrors retail function 0x004014a0. /w selects windowed mode and /f selects
// fullscreen. Later switches win, and bytes following Shift-JIS lead bytes are
// not interpreted as switches.
void applyRetailCommandLine(std::string_view command_line, GameConfig& config);

}  // namespace openshadowflare

#endif
