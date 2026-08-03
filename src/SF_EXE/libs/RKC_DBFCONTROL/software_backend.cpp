#include "libs/RKC_DBFCONTROL/rkc_dbfcontrol.hpp"

#include "gapi/bit_mask_image.hpp"
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
    brightness = std::clamp(brightness, std::int32_t{0}, std::int32_t{1000});
    return static_cast<std::uint8_t>(
        static_cast<std::int32_t>(value) * brightness / 1000);
}

std::uint8_t applyColorStrength(
    std::uint8_t value,
    std::int32_t strength) {
    const std::int32_t amount =
        std::clamp(strength, std::int32_t{0}, std::int32_t{2000}) - 1000;
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
    opacity = std::clamp(opacity, std::int32_t{0}, std::int32_t{1000});
    return static_cast<std::uint8_t>(
        (static_cast<std::int32_t>(source) * opacity +
         static_cast<std::int32_t>(destination) *
             (1000 - opacity)) /
        1000);
}

std::uint8_t addChannel(
    std::uint8_t destination,
    std::uint8_t source,
    std::int32_t opacity) {
    const std::int32_t source_amount =
        opacity <= 1000
            ? static_cast<std::int32_t>(source) * opacity / 1000
            : static_cast<std::int32_t>(source) +
                  (255 - static_cast<std::int32_t>(source)) *
                      (opacity - 1000) / 1000;
    return static_cast<std::uint8_t>(
        std::min(
            static_cast<std::int32_t>(destination) +
                source_amount,
            255));
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
    : width_(std::max(width, std::int32_t{0})),
      height_(std::max(height, std::int32_t{0})),
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
    if (!image.patternDecoded(pattern_index)) {
        return false;
    }
    const std::int32_t palette_index =
        draw.palette >= 0
            ? draw.palette
            : pattern.default_palette;
    if (palette_index < 0 ||
        static_cast<std::size_t>(palette_index) >=
            image.palettes().size()) {
        return false;
    }
    const NjpPalette& palette =
        image.palettes()[static_cast<std::size_t>(
            palette_index)];
    NjpPalette adjusted_palette;
    const bool palette_unchanged =
        draw.brightness == 1000 &&
        draw.red_strength == 1000 &&
        draw.green_strength == 1000 &&
        draw.blue_strength == 1000;
    const NjpPalette* render_palette = &palette;
    if (!palette_unchanged) {
        for (std::size_t index = 0;
             index < adjusted_palette.size();
             ++index) {
            Color color = palette[index];
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
            adjusted_palette[index] = color;
        }
        render_palette = &adjusted_palette;
    }

    const bool additive =
        draw.blend_mode == PatternBlendMode::additive;
    const std::int32_t opacity = std::clamp(
        draw.opacity, 0, additive ? 2000 : 1000);

    for (const NjpPatternPart& item : pattern.parts) {
        if (item.part_index < 0 ||
            static_cast<std::size_t>(item.part_index) >=
                image.parts().size()) {
            return false;
        }
        const NjpPart& part =
            image.parts()[static_cast<std::size_t>(item.part_index)];
        if (!part.hasDecodedPixels()) {
            return false;
        }
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

        std::int32_t first_y =
            std::max(std::int32_t{0}, -destination_y);
        std::int32_t last_y =
            std::min(destination_height, height_ - destination_y);
        std::int32_t first_x =
            std::max(std::int32_t{0}, -destination_x);
        std::int32_t last_x =
            std::min(destination_width, width_ - destination_x);
        if (draw.clip.width > 0 && draw.clip.height > 0) {
            first_y = std::max(
                first_y, draw.clip.y - destination_y);
            last_y = std::min(
                last_y,
                draw.clip.y + draw.clip.height -
                    destination_y);
            first_x = std::max(
                first_x, draw.clip.x - destination_x);
            last_x = std::min(
                last_x,
                draw.clip.x + draw.clip.width -
                    destination_x);
        }
        if (first_x >= last_x || first_y >= last_y) {
            continue;
        }

        const bool identity_scale_x =
            destination_width == part.width;
        const bool identity_scale_y =
            destination_height == part.height;

        if (identity_scale_x && identity_scale_y &&
            part.bits_per_pixel == 8 && !additive &&
            opacity >= 1000) {
            for (std::int32_t y = first_y; y < last_y; ++y) {
                const std::size_t source_row_index =
                    static_cast<std::size_t>(part.height - y - 1) *
                    part.stride;
                const std::uint8_t* source_row =
                    part.pixels.data() + source_row_index;
                Color* destination_row =
                    pixels_.data() +
                    static_cast<std::size_t>(destination_y + y) *
                        static_cast<std::size_t>(width_) +
                    static_cast<std::size_t>(
                        destination_x + first_x);
                for (std::int32_t x = first_x; x < last_x; ++x) {
                    std::uint8_t palette_index = source_row[x];
                    if (palette_index == 0) {
                        continue;
                    }
                    palette_index = static_cast<std::uint8_t>(
                        palette_index + item.palette_offset);
                    destination_row[x - first_x] =
                        (*render_palette)[palette_index];
                }
            }
            continue;
        }

        for (std::int32_t y = first_y; y < last_y; ++y) {
            const std::int32_t source_y = identity_scale_y
                ? y
                : static_cast<std::int32_t>(
                    static_cast<std::int64_t>(y) *
                    part.height / destination_height);
            Color* destination_row =
                pixels_.data() +
                static_cast<std::size_t>(destination_y + y) *
                    static_cast<std::size_t>(width_) +
                static_cast<std::size_t>(
                    destination_x + first_x);
            for (std::int32_t x = first_x; x < last_x; ++x) {
                const std::int32_t source_x = identity_scale_x
                    ? x
                    : static_cast<std::int32_t>(
                        static_cast<std::int64_t>(x) *
                        part.width / destination_width);
                std::uint8_t palette_index =
                    readPixelIndex(part, source_x, source_y);
                if (palette_index == 0) {
                    continue;
                }
                palette_index = static_cast<std::uint8_t>(
                    palette_index + item.palette_offset);
                const Color color =
                    (*render_palette)[palette_index];
                Color& destination = destination_row[x - first_x];
                if (additive) {
                    destination.red = addChannel(
                        destination.red, color.red, opacity);
                    destination.green = addChannel(
                        destination.green,
                        color.green,
                        opacity);
                    destination.blue = addChannel(
                        destination.blue, color.blue, opacity);
                    destination.alpha = 255;
                } else if (opacity >= 1000) {
                    destination = color;
                } else if (opacity > 0) {
                    destination.red = blendChannel(
                        destination.red, color.red, opacity);
                    destination.green = blendChannel(
                        destination.green,
                        color.green,
                        opacity);
                    destination.blue = blendChannel(
                        destination.blue, color.blue, opacity);
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

    std::int32_t firstY = 0;
    std::int32_t lastY = destinationHeight;
    std::int32_t firstX = 0;
    std::int32_t lastX = destinationWidth;
    if (draw.clip.width > 0 && draw.clip.height > 0) {
        firstY = std::max(
            firstY, draw.clip.y - draw.y);
        lastY = std::min(
            lastY,
            draw.clip.y + draw.clip.height - draw.y);
        firstX = std::max(
            firstX, draw.clip.x - draw.x);
        lastX = std::min(
            lastX,
            draw.clip.x + draw.clip.width - draw.x);
    }
    firstY = std::max(firstY, -draw.y);
    lastY = std::min(lastY, height_ - draw.y);
    firstX = std::max(firstX, -draw.x);
    lastX = std::min(lastX, width_ - draw.x);
    if (firstX >= lastX || firstY >= lastY) {
        return true;
    }

    const std::int32_t sourceWidth = image.width();
    const std::int32_t sourceHeight = image.height();
    const std::vector<Color>& sourcePixels = image.pixels();
    const bool identityScaleX = destinationWidth == sourceWidth;
    const bool identityScaleY = destinationHeight == sourceHeight;
    const bool identityBrightness = draw.brightness == 1000;
    for (std::int32_t y = firstY; y < lastY; ++y) {
        const std::int32_t sourceY = identityScaleY
            ? y
            : static_cast<std::int32_t>(
                static_cast<std::int64_t>(y) *
                sourceHeight / destinationHeight);
        const Color* sourceRow =
            sourcePixels.data() +
            static_cast<std::size_t>(sourceY) *
                static_cast<std::size_t>(sourceWidth);
        Color* destinationRow =
            pixels_.data() +
            static_cast<std::size_t>(draw.y + y) *
                static_cast<std::size_t>(width_) +
            static_cast<std::size_t>(draw.x + firstX);
        for (std::int32_t x = firstX; x < lastX; ++x) {
            const std::int32_t sourceX = identityScaleX
                ? x
                : static_cast<std::int32_t>(
                    static_cast<std::int64_t>(x) *
                    sourceWidth / destinationWidth);
            Color color = sourceRow[sourceX];
            if (color.alpha == 0) {
                continue;
            }
            if (!identityBrightness) {
                color.red =
                    applyBrightness(color.red, draw.brightness);
                color.green =
                    applyBrightness(color.green, draw.brightness);
                color.blue =
                    applyBrightness(color.blue, draw.brightness);
            }
            Color& destination = destinationRow[x - firstX];
            if (color.alpha == 255) {
                destination = color;
            } else {
                destination.red = static_cast<std::uint8_t>(
                    (static_cast<std::uint32_t>(color.red) *
                         color.alpha +
                     static_cast<std::uint32_t>(
                         destination.red) *
                         (255u - color.alpha)) /
                    255u);
                destination.green = static_cast<std::uint8_t>(
                    (static_cast<std::uint32_t>(color.green) *
                         color.alpha +
                     static_cast<std::uint32_t>(
                         destination.green) *
                         (255u - color.alpha)) /
                    255u);
                destination.blue = static_cast<std::uint8_t>(
                    (static_cast<std::uint32_t>(color.blue) *
                         color.alpha +
                     static_cast<std::uint32_t>(
                         destination.blue) *
                         (255u - color.alpha)) /
                    255u);
                destination.alpha = 255;
            }
        }
    }
    return true;
}

bool SoftwareBackend::drawBitMask(
    const BitMaskImage& image,
    const BitMaskDraw& draw) {
    if (image.width() <= 0 || image.height() <= 0 ||
        draw.scale_x <= 0 || draw.scale_y <= 0) {
        return false;
    }
    const std::int32_t destination_width =
        static_cast<std::int32_t>(
            static_cast<std::int64_t>(image.width()) *
            draw.scale_x / 1000);
    const std::int32_t destination_height =
        static_cast<std::int32_t>(
            static_cast<std::int64_t>(image.height()) *
            draw.scale_y / 1000);
    if (destination_width <= 0 || destination_height <= 0) {
        return false;
    }

    std::int32_t first_y = 0;
    std::int32_t last_y = destination_height;
    std::int32_t first_x = 0;
    std::int32_t last_x = destination_width;
    if (draw.clip.width > 0 && draw.clip.height > 0) {
        first_y = std::max(first_y, draw.clip.y - draw.y);
        last_y = std::min(
            last_y,
            draw.clip.y + draw.clip.height - draw.y);
        first_x = std::max(first_x, draw.clip.x - draw.x);
        last_x = std::min(
            last_x,
            draw.clip.x + draw.clip.width - draw.x);
    }
    first_y = std::max(first_y, -draw.y);
    last_y = std::min(last_y, height_ - draw.y);
    first_x = std::max(first_x, -draw.x);
    last_x = std::min(last_x, width_ - draw.x);
    if (first_x >= last_x || first_y >= last_y) {
        return true;
    }

    const std::int32_t opacity =
        std::clamp(draw.opacity, 0, 1000) * draw.color.alpha / 255;
    if (opacity <= 0) {
        return true;
    }
    Color color = draw.color;
    color.alpha = 255;
    const bool identity_scale_x =
        destination_width == image.width();
    const bool identity_scale_y =
        destination_height == image.height();
    const std::vector<std::uint8_t>& source = image.bytes();
    for (std::int32_t y = first_y; y < last_y; ++y) {
        const std::int32_t source_y = identity_scale_y
            ? y
            : static_cast<std::int32_t>(
                  static_cast<std::int64_t>(y) *
                  image.height() / destination_height);
        const std::uint8_t* source_row =
            source.data() +
            static_cast<std::size_t>(source_y) *
                image.strideBytes();
        Color* destination_row =
            pixels_.data() +
            static_cast<std::size_t>(draw.y + y) *
                static_cast<std::size_t>(width_) +
            static_cast<std::size_t>(draw.x + first_x);
        for (std::int32_t x = first_x; x < last_x; ++x) {
            const std::int32_t source_x = identity_scale_x
                ? x
                : static_cast<std::int32_t>(
                      static_cast<std::int64_t>(x) *
                      image.width() / destination_width);
            if ((source_row[source_x / 8] &
                 (0x80u >> (source_x & 7))) == 0) {
                continue;
            }
            Color& destination = destination_row[x - first_x];
            if (opacity >= 1000) {
                destination = color;
            } else {
                destination.red = blendChannel(
                    destination.red, color.red, opacity);
                destination.green = blendChannel(
                    destination.green, color.green, opacity);
                destination.blue = blendChannel(
                    destination.blue, color.blue, opacity);
                destination.alpha = 255;
            }
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
        if (!font.patternDecoded(patternIndex)) {
            return false;
        }
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
            if (!part.hasDecodedPixels()) {
                return false;
            }
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
    const std::int32_t opacity =
        std::clamp(draw.opacity, 0, 1000);
    if (opacity >= 1000) {
        for (std::int32_t y = top; y < bottom; ++y) {
            Color* row = pixels_.data() +
                static_cast<std::size_t>(y) *
                    static_cast<std::size_t>(width_) +
                static_cast<std::size_t>(left);
            std::fill(row, row + (right - left), color);
        }
    } else if (opacity > 0) {
        for (std::int32_t y = top; y < bottom; ++y) {
            Color* row = pixels_.data() +
                static_cast<std::size_t>(y) *
                    static_cast<std::size_t>(width_) +
                static_cast<std::size_t>(left);
            for (std::int32_t x = 0; x < right - left; ++x) {
                Color& destination = row[x];
                destination.red = blendChannel(
                    destination.red, color.red, opacity);
                destination.green = blendChannel(
                    destination.green, color.green, opacity);
                destination.blue = blendChannel(
                    destination.blue, color.blue, opacity);
                destination.alpha = 255;
            }
        }
    }
    return true;
}

bool SoftwareBackend::drawLine(const LineDraw& draw) {
    Color color = draw.color;
    color.red = applyBrightness(color.red, draw.brightness);
    color.green = applyBrightness(color.green, draw.brightness);
    color.blue = applyBrightness(color.blue, draw.brightness);

    const bool clipped =
        draw.clip.width > 0 && draw.clip.height > 0;
    const std::int32_t opacity =
        std::clamp(draw.opacity, 0, 1000);
    const auto draw_point = [this, &draw, color, clipped, opacity](
                                std::int32_t x,
                                std::int32_t y) {
        if (x < 0 || x >= width_ || y < 0 || y >= height_ ||
            (clipped &&
             (x < draw.clip.x || y < draw.clip.y ||
              static_cast<std::int64_t>(x) >=
                  static_cast<std::int64_t>(draw.clip.x) +
                      draw.clip.width ||
              static_cast<std::int64_t>(y) >=
                  static_cast<std::int64_t>(draw.clip.y) +
                      draw.clip.height))) {
            return;
        }
        Color& destination =
            pixels_[static_cast<std::size_t>(y) * width_ + x];
        if (opacity >= 1000) {
            destination = color;
        } else if (opacity > 0) {
            destination.red = blendChannel(
                destination.red, color.red, opacity);
            destination.green = blendChannel(
                destination.green, color.green, opacity);
            destination.blue = blendChannel(
                destination.blue, color.blue, opacity);
            destination.alpha = 255;
        }
    };

    const std::int64_t difference_x =
        static_cast<std::int64_t>(draw.end_x) - draw.start_x;
    const std::int64_t difference_y =
        static_cast<std::int64_t>(draw.end_y) - draw.start_y;
    const std::int64_t span_x = std::abs(difference_x) + 1;
    const std::int64_t span_y = std::abs(difference_y) + 1;
    const std::int64_t direction_x = difference_x < 0 ? -1 : 1;
    const std::int64_t direction_y = difference_y < 0 ? -1 : 1;

    if (difference_y == 0) {
        for (std::int64_t step = 0; step < span_x; ++step) {
            draw_point(
                static_cast<std::int32_t>(
                    draw.start_x + direction_x * step),
                draw.start_y);
        }
    } else if (difference_x == 0) {
        for (std::int64_t step = 0; step < span_y; ++step) {
            draw_point(
                draw.start_x,
                static_cast<std::int32_t>(
                    draw.start_y + direction_y * step));
        }
    } else if (span_y < span_x) {
        std::int64_t accumulator = 0;
        for (std::int64_t step = 0; step < span_x; ++step) {
            draw_point(
                static_cast<std::int32_t>(
                    draw.start_x + direction_x * step),
                static_cast<std::int32_t>(
                    draw.start_y +
                    direction_y * accumulator / span_x));
            accumulator += span_y;
        }
    } else {
        std::int64_t accumulator = 0;
        for (std::int64_t step = 0; step < span_y; ++step) {
            draw_point(
                static_cast<std::int32_t>(
                    draw.start_x +
                    direction_x * accumulator / span_y),
                static_cast<std::int32_t>(
                    draw.start_y + direction_y * step));
            accumulator += span_x;
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
