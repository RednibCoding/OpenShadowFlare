#include "libs/RKC_DIB/rkc_dib.hpp"

#include <fstream>
#include <limits>
#include <utility>

namespace osf::gapi {
namespace {

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

bool readU16(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::uint16_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < 2) {
        return false;
    }
    value =
        static_cast<std::uint16_t>(bytes[offset]) |
        static_cast<std::uint16_t>(bytes[offset + 1] << 8u);
    return true;
}

bool readU32(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::uint32_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        return false;
    }
    value =
        static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
        (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
    return true;
}

}  // namespace

bool BitmapImage::decode(
    const std::vector<std::uint8_t>& bytes,
    std::string* error) {
    clear();
    if (bytes.size() < 54 ||
        bytes[0] != 'B' || bytes[1] != 'M') {
        setError(error, "The BMP header is invalid.");
        return false;
    }

    std::uint32_t pixelOffset = 0;
    std::uint32_t dibSize = 0;
    std::uint32_t rawWidth = 0;
    std::uint32_t rawHeight = 0;
    std::uint16_t planes = 0;
    std::uint16_t bitsPerPixel = 0;
    std::uint32_t compression = 0;
    if (!readU32(bytes, 10, pixelOffset) ||
        !readU32(bytes, 14, dibSize) ||
        !readU32(bytes, 18, rawWidth) ||
        !readU32(bytes, 22, rawHeight) ||
        !readU16(bytes, 26, planes) ||
        !readU16(bytes, 28, bitsPerPixel) ||
        !readU32(bytes, 30, compression) ||
        dibSize < 40 || planes != 1 || compression != 0 ||
        (bitsPerPixel != 24 && bitsPerPixel != 32)) {
        setError(error, "The BMP format is unsupported.");
        return false;
    }

    const std::int32_t signedWidth =
        static_cast<std::int32_t>(rawWidth);
    const std::int32_t signedHeight =
        static_cast<std::int32_t>(rawHeight);
    if (signedWidth <= 0 || signedHeight == 0 ||
        signedHeight == std::numeric_limits<std::int32_t>::min()) {
        setError(error, "The BMP dimensions are invalid.");
        return false;
    }

    const std::int32_t absoluteHeight =
        signedHeight < 0 ? -signedHeight : signedHeight;
    const std::uint64_t rowStride =
        ((static_cast<std::uint64_t>(signedWidth) *
          bitsPerPixel + 31u) /
         32u) *
        4u;
    const std::uint64_t pixelBytes =
        rowStride * static_cast<std::uint64_t>(absoluteHeight);
    const std::uint64_t pixelEnd =
        static_cast<std::uint64_t>(pixelOffset) + pixelBytes;
    const std::uint64_t pixelCount =
        static_cast<std::uint64_t>(signedWidth) *
        static_cast<std::uint64_t>(absoluteHeight);
    if (pixelEnd > bytes.size() ||
        pixelCount > std::numeric_limits<std::size_t>::max()) {
        setError(error, "The BMP pixel data is truncated.");
        return false;
    }

    width_ = signedWidth;
    height_ = absoluteHeight;
    pixels_.resize(static_cast<std::size_t>(pixelCount));
    const std::size_t bytesPerPixel = bitsPerPixel / 8u;
    for (std::int32_t y = 0; y < height_; ++y) {
        const std::int32_t sourceY =
            signedHeight < 0 ? y : height_ - y - 1;
        const std::size_t row =
            static_cast<std::size_t>(pixelOffset) +
            static_cast<std::size_t>(sourceY) *
                static_cast<std::size_t>(rowStride);
        for (std::int32_t x = 0; x < width_; ++x) {
            const std::size_t source =
                row + static_cast<std::size_t>(x) * bytesPerPixel;
            pixels_[
                static_cast<std::size_t>(y) *
                    static_cast<std::size_t>(width_) +
                static_cast<std::size_t>(x)] = {
                bytes[source + 2],
                bytes[source + 1],
                bytes[source],
                255,
            };
        }
    }

    if (error) {
        error->clear();
    }
    return true;
}

bool BitmapImage::load(
    const std::filesystem::path& path,
    std::string* error) {
    clear();
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        setError(error, "Could not open the BMP file.");
        return false;
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size < 0 ||
        static_cast<std::uint64_t>(size) >
            std::numeric_limits<std::size_t>::max()) {
        setError(error, "The BMP file size is invalid.");
        return false;
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(
        static_cast<std::size_t>(size));
    if (!bytes.empty() &&
        !stream.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()))) {
        setError(error, "Could not read the BMP file.");
        return false;
    }
    return decode(bytes, error);
}

void BitmapImage::clear() {
    width_ = 0;
    height_ = 0;
    pixels_.clear();
}

std::int32_t BitmapImage::width() const {
    return width_;
}

std::int32_t BitmapImage::height() const {
    return height_;
}

const std::vector<Color>& BitmapImage::pixels() const {
    return pixels_;
}

}  // namespace osf::gapi
