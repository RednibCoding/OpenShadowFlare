#include "software_backend.hpp"

#include "njp.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>

namespace osf::gapi {
namespace {

std::uint8_t applyBrightness(
    std::uint8_t value,
    std::int32_t brightness) {
    brightness = std::clamp(brightness, 0, 1000);
    return static_cast<std::uint8_t>(
        static_cast<std::int32_t>(value) * brightness / 1000);
}

std::uint8_t readPixelIndex(
    const NjpPart& part,
    std::int32_t x,
    std::int32_t y) {
    const std::int32_t source_y = part.height - y - 1;
    const std::size_t row =
        static_cast<std::size_t>(source_y) * part.stride;
    if (part.bits_per_pixel == 8) {
        return part.pixels[row + static_cast<std::size_t>(x)];
    }
    if (part.bits_per_pixel == 4) {
        const std::uint8_t packed =
            part.pixels[row + static_cast<std::size_t>(x / 2)];
        return static_cast<std::uint8_t>(
            (packed >> ((1 - (x & 1)) * 4)) & 0x0f);
    }
    const std::uint8_t packed =
        part.pixels[row + static_cast<std::size_t>(x / 8)];
    return static_cast<std::uint8_t>(
        (packed >> (7 - (x & 7))) & 1);
}

}  // namespace

SoftwareBackend::SoftwareBackend(
    std::int32_t width,
    std::int32_t height,
    PresentCallback present)
    : width_(std::max(width, 0)),
      height_(std::max(height, 0)),
      pixels_(
          static_cast<std::size_t>(width_) *
          static_cast<std::size_t>(height_)),
      present_(std::move(present)) {}

void SoftwareBackend::beginFrame(Color clear_color) {
    std::fill(pixels_.begin(), pixels_.end(), clear_color);
}

bool SoftwareBackend::drawPattern(
    const NjpImage& image,
    std::size_t pattern_index,
    const PatternDraw& draw) {
    if (pattern_index >= image.patterns().size()) {
        return false;
    }
    const NjpPattern& pattern = image.patterns()[pattern_index];
    if (pattern.default_palette < 0 ||
        static_cast<std::size_t>(pattern.default_palette) >=
            image.palettes().size()) {
        return false;
    }
    const NjpPalette& palette =
        image.palettes()[static_cast<std::size_t>(
            pattern.default_palette)];

    for (const NjpPatternPart& item : pattern.parts) {
        if (item.part_index < 0 ||
            static_cast<std::size_t>(item.part_index) >=
                image.parts().size()) {
            return false;
        }
        const NjpPart& part =
            image.parts()[static_cast<std::size_t>(item.part_index)];
        if (part.width <= 0 || part.height <= 0 ||
            item.scale_x <= 0 || item.scale_y <= 0 ||
            draw.scale_x <= 0 || draw.scale_y <= 0) {
            continue;
        }

        const std::int32_t combined_scale_x =
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(item.scale_x) *
                draw.scale_x / 1000);
        const std::int32_t combined_scale_y =
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(item.scale_y) *
                draw.scale_y / 1000);
        const std::int32_t destination_width =
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(part.width) *
                combined_scale_x / 1000);
        const std::int32_t destination_height =
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(part.height) *
                combined_scale_y / 1000);
        const std::int32_t destination_x =
            draw.x +
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(item.x) *
                draw.scale_x / 1000);
        const std::int32_t destination_y =
            draw.y +
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(item.y) *
                draw.scale_y / 1000);
        if (destination_width <= 0 || destination_height <= 0) {
            continue;
        }

        for (std::int32_t y = 0; y < destination_height; ++y) {
            const std::int32_t target_y = destination_y + y;
            if (target_y < 0 || target_y >= height_) {
                continue;
            }
            const std::int32_t source_y =
                static_cast<std::int32_t>(
                    static_cast<std::int64_t>(y) *
                    part.height / destination_height);
            for (std::int32_t x = 0; x < destination_width; ++x) {
                const std::int32_t target_x = destination_x + x;
                if (target_x < 0 || target_x >= width_) {
                    continue;
                }
                const std::int32_t source_x =
                    static_cast<std::int32_t>(
                        static_cast<std::int64_t>(x) *
                        part.width / destination_width);
                std::uint8_t palette_index =
                    readPixelIndex(part, source_x, source_y);
                if (part.bits_per_pixel <= 4 &&
                    palette_index == 0) {
                    continue;
                }
                palette_index = static_cast<std::uint8_t>(
                    palette_index + item.palette_offset);
                Color color = palette[palette_index];
                color.red =
                    applyBrightness(color.red, draw.brightness);
                color.green =
                    applyBrightness(color.green, draw.brightness);
                color.blue =
                    applyBrightness(color.blue, draw.brightness);
                pixels_[
                    static_cast<std::size_t>(target_y) * width_ +
                    static_cast<std::size_t>(target_x)] = color;
            }
        }
    }
    return true;
}

void SoftwareBackend::endFrame() {
    if (present_) {
        present_(surface());
    }
}

SurfaceView SoftwareBackend::surface() const {
    return {pixels_.data(), width_, height_};
}

}  // namespace osf::gapi
