#ifndef OPENSHADOWFLARE_GAME_CONFIG_HPP
#define OPENSHADOWFLARE_GAME_CONFIG_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>

namespace openshadowflare {

constexpr std::size_t kGameConfigValueCount = 16;
constexpr std::size_t kGameConfigByteSize =
    kGameConfigValueCount * sizeof(std::int32_t);

// This is the understood portion of the retail SFlare.Cfg file. Unknown fields
// keep their original global addresses until their meaning is proven.
struct GameConfig {
    bool windowed_at_start = false;             // 0x0048d8b8
    bool semi_transparent_shadow = true;        // 0x0048d530
    bool semi_transparent_objects = true;       // 0x0048d534
    bool display_darkness = true;               // 0x0048d538
    bool unknown_48d528 = true;
    bool unknown_48d540 = true;
    bool save_image_at_game_end = true;         // 0x0048d544
    std::int32_t click_range = 2;                // 0x0048d734
    bool click_range_enabled = true;             // 0x0048d738
    std::array<std::int32_t, 5> click_priority{{4, 2, 3, 1, 0}};
    std::int32_t effect_volume = 0;              // 0x0048d8e4
    std::int32_t bgm_volume = 0;                 // 0x0048d8e8
};

// Reads and validates fields in the same order as retail function 0x00401eb0.
// Like the original, fields read before a later failure remain changed.
bool loadGameConfig(std::istream& input, GameConfig& config);
bool loadGameConfigFile(const std::string& path, GameConfig& config);

// Writes the exact 64-byte little-endian retail layout (0x00401d10).
bool saveGameConfig(std::ostream& output, const GameConfig& config);
bool saveGameConfigFile(const std::string& path, const GameConfig& config);

}  // namespace openshadowflare

#endif
