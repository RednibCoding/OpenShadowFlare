#include "libs/RKC_DBFCONTROL/rkc_dbfcontrol.hpp"

#include "libs/RKC_DIB/rkc_dib.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"

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

std::uint8_t applyColorStrength(
    std::uint8_t value,
    std::int32_t strength) {
    const std::int32_t amount =
        std::clamp(strength, 0, 2000) - 1000;
    if (amount < 0) {
        return static_cast<std::uint8_t>(
            static_cast<std::int32_t>(value) +
            static_cast<std::int32_t>(value) * amount / 1000);
    }
    return static_cast<std::uint8_t>(
        static_cast<std::int32_t>(value) +
        (255 - static_cast<std::int32_t>(value)) *
            amount / 1000);
}

std::uint8_t blendChannel(
    std::uint8_t destination,
    std::uint8_t source,
    std::int32_t opacity) {
    opacity = std::clamp(opacity, 0, 1000);
    return static_cast<std::uint8_t>(
        (static_cast<std::int32_t>(source) * opacity +
         static_cast<std::int32_t>(destination) *
             (1000 - opacity)) /
        1000);
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
                if (palette_index == 0) {
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
                color.red = applyColorStrength(
                    color.red, draw.red_strength);
                color.green = applyColorStrength(
                    color.green, draw.green_strength);
                color.blue = applyColorStrength(
                    color.blue, draw.blue_strength);
                Color& destination =
                    pixels_[
                        static_cast<std::size_t>(target_y) *
                            width_ +
                        static_cast<std::size_t>(target_x)];
                if (draw.opacity >= 1000) {
                    destination = color;
                } else if (draw.opacity > 0) {
                    destination.red = blendChannel(
                        destination.red, color.red, draw.opacity);
                    destination.green = blendChannel(
                        destination.green,
                        color.green,
                        draw.opacity);
                    destination.blue = blendChannel(
                        destination.blue, color.blue, draw.opacity);
                    destination.alpha = 255;
                }
            }
        }
    }
    return true;
}

bool SoftwareBackend::drawBitmap(
    const BitmapImage& image,
    const BitmapDraw& draw) {
    if (image.width() <= 0 || image.height() <= 0 ||
        draw.scale_x <= 0 || draw.scale_y <= 0) {
        return false;
    }
    const std::int32_t destinationWidth =
        static_cast<std::int32_t>(
            static_cast<std::int64_t>(image.width()) *
            draw.scale_x / 1000);
    const std::int32_t destinationHeight =
        static_cast<std::int32_t>(
            static_cast<std::int64_t>(image.height()) *
            draw.scale_y / 1000);
    if (destinationWidth <= 0 || destinationHeight <= 0) {
        return false;
    }

    for (std::int32_t y = 0; y < destinationHeight; ++y) {
        const std::int32_t targetY = draw.y + y;
        if (targetY < 0 || targetY >= height_) {
            continue;
        }
        const std::int32_t sourceY =
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(y) *
                image.height() / destinationHeight);
        for (std::int32_t x = 0; x < destinationWidth; ++x) {
            const std::int32_t targetX = draw.x + x;
            if (targetX < 0 || targetX >= width_) {
                continue;
            }
            const std::int32_t sourceX =
                static_cast<std::int32_t>(
                    static_cast<std::int64_t>(x) *
                    image.width() / destinationWidth);
            Color color = image.pixels()[
                static_cast<std::size_t>(sourceY) *
                    static_cast<std::size_t>(image.width()) +
                static_cast<std::size_t>(sourceX)];
            color.red =
                applyBrightness(color.red, draw.brightness);
            color.green =
                applyBrightness(color.green, draw.brightness);
            color.blue =
                applyBrightness(color.blue, draw.brightness);
            pixels_[
                static_cast<std::size_t>(targetY) *
                    static_cast<std::size_t>(width_) +
                static_cast<std::size_t>(targetX)] = color;
        }
    }
    return true;
}

bool SoftwareBackend::drawText(
    const NjpImage& font,
    std::string_view text,
    const TextDraw& draw) {
    if (font.patterns().empty() || font.parts().empty() ||
        font.palettes().empty()) {
        return false;
    }
    const NjpPattern& basePattern = font.patterns().front();
    const std::int32_t cellWidth = basePattern.width / 16;
    const std::int32_t cellHeight = basePattern.height / 16;
    if (cellWidth <= 0 || cellHeight <= 0) {
        return false;
    }

    Color color = draw.color;
    color.red = applyBrightness(color.red, draw.brightness);
    color.green = applyBrightness(color.green, draw.brightness);
    color.blue = applyBrightness(color.blue, draw.brightness);
    std::int32_t cursorX = draw.x;
    std::int32_t cursorY = draw.y;
    for (std::size_t byteIndex = 0;
         byteIndex < text.size();
         ++byteIndex) {
        const std::uint8_t first =
            static_cast<std::uint8_t>(text[byteIndex]);
        if (first == '\n') {
            cursorX = draw.x;
            cursorY += cellHeight + draw.line_spacing;
            continue;
        }
        if (first == ' ') {
            cursorX += cellWidth + draw.letter_spacing;
            continue;
        }

        std::size_t patternIndex = 0;
        std::uint8_t glyph = first;
        std::int32_t glyphWidth = cellWidth;
        const bool shiftJisLead =
            (first >= 0x80u && first <= 0x9fu) ||
            first >= 0xe0u;
        if (shiftJisLead) {
            if (byteIndex + 1 >= text.size()) {
                break;
            }
            patternIndex = first >= 0xe0u
                ? static_cast<std::size_t>(first - 0xbfu)
                : static_cast<std::size_t>(first - 0x7fu);
            glyph = static_cast<std::uint8_t>(text[++byteIndex]);
            glyphWidth *= 2;
        }
        if (patternIndex >= font.patterns().size()) {
            cursorX += glyphWidth + draw.letter_spacing;
            continue;
        }

        const NjpPattern& pattern = font.patterns()[patternIndex];
        if (pattern.default_palette < 0 ||
            static_cast<std::size_t>(pattern.default_palette) >=
                font.palettes().size()) {
            return false;
        }
        const std::int32_t sourceLeft =
            static_cast<std::int32_t>(glyph & 0x0fu) * glyphWidth;
        const std::int32_t sourceTop =
            static_cast<std::int32_t>(glyph >> 4u) * cellHeight;
        for (const NjpPatternPart& item : pattern.parts) {
            if (item.part_index < 0 ||
                static_cast<std::size_t>(item.part_index) >=
                    font.parts().size()) {
                return false;
            }
            const NjpPart& part =
                font.parts()[static_cast<std::size_t>(
                    item.part_index)];
            for (std::int32_t y = 0; y < cellHeight; ++y) {
                const std::int32_t targetY = cursorY + y;
                const std::int32_t sourceY =
                    sourceTop + y - item.y;
                if (targetY < 0 || targetY >= height_ ||
                    sourceY < 0 || sourceY >= part.height) {
                    continue;
                }
                for (std::int32_t x = 0; x < glyphWidth; ++x) {
                    const std::int32_t targetX = cursorX + x;
                    const std::int32_t sourceX =
                        sourceLeft + x - item.x;
                    if (targetX < 0 || targetX >= width_ ||
                        sourceX < 0 || sourceX >= part.width) {
                        continue;
                    }
                    if (readPixelIndex(part, sourceX, sourceY) == 0) {
                        continue;
                    }
                    pixels_[
                        static_cast<std::size_t>(targetY) *
                            static_cast<std::size_t>(width_) +
                        static_cast<std::size_t>(targetX)] = color;
                }
            }
        }
        cursorX += glyphWidth + draw.letter_spacing;
    }
    return true;
}

bool SoftwareBackend::drawRectangle(
    const RectangleDraw& draw) {
    if (draw.width <= 0 || draw.height <= 0) {
        return false;
    }

    Color color = draw.color;
    color.red = applyBrightness(color.red, draw.brightness);
    color.green = applyBrightness(color.green, draw.brightness);
    color.blue = applyBrightness(color.blue, draw.brightness);
    const std::int32_t left = static_cast<std::int32_t>(
        std::clamp<std::int64_t>(draw.x, 0, width_));
    const std::int32_t top = static_cast<std::int32_t>(
        std::clamp<std::int64_t>(draw.y, 0, height_));
    const std::int32_t right = static_cast<std::int32_t>(
        std::clamp<std::int64_t>(
            static_cast<std::int64_t>(draw.x) + draw.width,
            0,
            width_));
    const std::int32_t bottom = static_cast<std::int32_t>(
        std::clamp<std::int64_t>(
            static_cast<std::int64_t>(draw.y) + draw.height,
            0,
            height_));
    if (right <= left || bottom <= top) {
        return true;
    }
    for (std::int32_t y = top; y < bottom; ++y) {
        for (std::int32_t x = left; x < right; ++x) {
            Color& destination =
                pixels_[static_cast<std::size_t>(y) * width_ + x];
            if (draw.opacity >= 1000) {
                destination = color;
            } else if (draw.opacity > 0) {
                destination.red = blendChannel(
                    destination.red, color.red, draw.opacity);
                destination.green = blendChannel(
                    destination.green, color.green, draw.opacity);
                destination.blue = blendChannel(
                    destination.blue, color.blue, draw.opacity);
                destination.alpha = 255;
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
