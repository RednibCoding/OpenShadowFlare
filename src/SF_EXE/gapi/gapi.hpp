#ifndef OPENSHADOWFLARE_GAPI_HPP
#define OPENSHADOWFLARE_GAPI_HPP

#include <cstddef>
#include <cstdint>

namespace osf::gapi {

class NjpImage;

struct Color {
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;
    std::uint8_t alpha = 255;
};

struct SurfaceView {
    const Color* pixels = nullptr;
    std::int32_t width = 0;
    std::int32_t height = 0;
};

struct PatternDraw {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t scale_x = 1000;
    std::int32_t scale_y = 1000;
    std::int32_t brightness = 1000;
};

struct Viewport {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
};

Viewport fitViewport(
    std::int32_t source_width,
    std::int32_t source_height,
    std::int32_t target_width,
    std::int32_t target_height);

class Backend {
public:
    virtual ~Backend() = default;

    virtual void beginFrame(Color clear_color) = 0;
    virtual bool drawPattern(
        const NjpImage& image,
        std::size_t pattern_index,
        const PatternDraw& draw = {}) = 0;
    virtual void endFrame() = 0;
};

}  // namespace osf::gapi

#endif
