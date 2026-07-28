#ifndef OPENSHADOWFLARE_LIBS_RKC_UPDIB_HPP
#define OPENSHADOWFLARE_LIBS_RKC_UPDIB_HPP

#include "gapi/gapi.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf::gapi {

struct NjpPart {
    std::int32_t bits_per_pixel = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t stride = 0;
    std::vector<std::uint8_t> pixels;
};

struct NjpPatternPart {
    std::uint32_t flags = 0;
    std::int32_t part_index = -1;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t palette_offset = 0;
    std::int32_t scale_x = 1000;
    std::int32_t scale_y = 1000;
};

struct NjpPattern {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t default_palette = -1;
    std::vector<NjpPatternPart> parts;
};

using NjpPalette = std::array<Color, 256>;

class NjpImage {
public:
    bool decode(
        const std::vector<std::uint8_t>& bytes,
        std::string* error = nullptr);
    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);

    void clear();
    std::int32_t version() const;
    bool isShadow() const;
    const std::vector<NjpPart>& parts() const;
    const std::vector<NjpPattern>& patterns() const;
    const std::vector<NjpPalette>& palettes() const;

private:
    std::int32_t version_ = 0;
    bool shadow_ = false;
    std::vector<NjpPart> parts_;
    std::vector<NjpPattern> patterns_;
    std::vector<NjpPalette> palettes_;
};

}  // namespace osf::gapi

#endif
