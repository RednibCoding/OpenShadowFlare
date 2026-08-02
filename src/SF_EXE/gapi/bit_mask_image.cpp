#include "bit_mask_image.hpp"

#include <algorithm>
#include <limits>

namespace osf::gapi {

bool BitMaskImage::create(
    std::int32_t width,
    std::int32_t height,
    bool fill) {
    clear();
    if (width <= 0 || height <= 0) {
        return false;
    }
    const std::uint64_t stride =
        (static_cast<std::uint64_t>(width) + 7u) / 8u;
    const std::uint64_t size =
        stride * static_cast<std::uint64_t>(height);
    if (stride > std::numeric_limits<std::size_t>::max() ||
        size > std::numeric_limits<std::size_t>::max()) {
        return false;
    }
    width_ = width;
    height_ = height;
    stride_bytes_ = static_cast<std::size_t>(stride);
    bytes_.assign(
        static_cast<std::size_t>(size),
        fill ? std::uint8_t{0xff} : std::uint8_t{0});
    return true;
}

void BitMaskImage::clear() {
    width_ = 0;
    height_ = 0;
    stride_bytes_ = 0;
    bytes_.clear();
}

void BitMaskImage::fillRectangle(
    std::int32_t x,
    std::int32_t y,
    std::int32_t width,
    std::int32_t height,
    bool value) {
    if (width <= 0 || height <= 0 ||
        width_ <= 0 || height_ <= 0) {
        return;
    }
    const std::int64_t first_x = std::max<std::int64_t>(x, 0);
    const std::int64_t first_y = std::max<std::int64_t>(y, 0);
    const std::int64_t last_x = std::min<std::int64_t>(
        static_cast<std::int64_t>(x) + width, width_);
    const std::int64_t last_y = std::min<std::int64_t>(
        static_cast<std::int64_t>(y) + height, height_);
    if (first_x >= last_x || first_y >= last_y) {
        return;
    }
    for (std::int64_t row = first_y; row < last_y; ++row) {
        const std::size_t row_offset =
            static_cast<std::size_t>(row) * stride_bytes_;
        for (std::int64_t column = first_x;
             column < last_x;
             ++column) {
            std::uint8_t& byte = bytes_[
                row_offset + static_cast<std::size_t>(column / 8)];
            const std::uint8_t bit = static_cast<std::uint8_t>(
                0x80u >> (column & 7));
            if (value) {
                byte = static_cast<std::uint8_t>(byte | bit);
            } else {
                byte = static_cast<std::uint8_t>(byte & ~bit);
            }
        }
    }
}

bool BitMaskImage::value(
    std::int32_t x,
    std::int32_t y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return false;
    }
    const std::size_t offset =
        static_cast<std::size_t>(y) * stride_bytes_ +
        static_cast<std::size_t>(x / 8);
    return (bytes_[offset] & (0x80u >> (x & 7))) != 0;
}

std::int32_t BitMaskImage::width() const {
    return width_;
}

std::int32_t BitMaskImage::height() const {
    return height_;
}

std::size_t BitMaskImage::strideBytes() const {
    return stride_bytes_;
}

const std::vector<std::uint8_t>& BitMaskImage::bytes() const {
    return bytes_;
}

std::uint64_t BitMaskImage::memoryUsageBytes() const {
    return static_cast<std::uint64_t>(bytes_.capacity());
}

}  // namespace osf::gapi
