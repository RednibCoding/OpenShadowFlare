#ifndef OPENSHADOWFLARE_GAPI_HPP
#define OPENSHADOWFLARE_GAPI_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace osf::gapi {

class BitmapImage;
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

struct Viewport {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
};

struct PatternDraw {
    constexpr PatternDraw(
        std::int32_t draw_x = 0,
        std::int32_t draw_y = 0,
        std::int32_t draw_scale_x = 1000,
        std::int32_t draw_scale_y = 1000,
        std::int32_t draw_brightness = 1000,
        std::int32_t draw_opacity = 1000,
        std::int32_t draw_red_strength = 1000,
        std::int32_t draw_green_strength = 1000,
        std::int32_t draw_blue_strength = 1000,
        std::int32_t draw_palette = -1,
        Viewport draw_clip = {})
        : x(draw_x),
          y(draw_y),
          scale_x(draw_scale_x),
          scale_y(draw_scale_y),
          brightness(draw_brightness),
          opacity(draw_opacity),
          red_strength(draw_red_strength),
          green_strength(draw_green_strength),
          blue_strength(draw_blue_strength),
          palette(draw_palette),
          clip(draw_clip) {}

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t scale_x = 1000;
    std::int32_t scale_y = 1000;
    std::int32_t brightness = 1000;
    std::int32_t opacity = 1000;
    std::int32_t red_strength = 1000;
    std::int32_t green_strength = 1000;
    std::int32_t blue_strength = 1000;
    std::int32_t palette = -1;
    Viewport clip;
};

struct BitmapDraw {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t scale_x = 1000;
    std::int32_t scale_y = 1000;
    std::int32_t brightness = 1000;
};

struct TextDraw {
    std::int32_t x = 0;
    std::int32_t y = 0;
    Color color{255, 255, 255, 255};
    std::int32_t brightness = 1000;
    std::int32_t letter_spacing = 0;
    std::int32_t line_spacing = 0;
};

struct RectangleDraw {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    Color color{255, 255, 255, 255};
    std::int32_t brightness = 1000;
    std::int32_t opacity = 1000;
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
    virtual bool drawBitmap(
        const BitmapImage& image,
        const BitmapDraw& draw = {}) = 0;
    virtual bool drawText(
        const NjpImage& font,
        std::string_view text,
        const TextDraw& draw = {}) = 0;
    virtual bool drawRectangle(
        const RectangleDraw& draw) = 0;
    virtual void endFrame() = 0;
};

}  // namespace osf::gapi

#endif
