#include "ground_map.hpp"

#include "core/rclib_lz.hpp"

#include <cstring>
#include <fstream>
#include <limits>
#include <utility>

namespace osf {
namespace {

class Reader {
public:
    explicit Reader(const std::vector<std::uint8_t>& bytes)
        : bytes_(bytes) {}

    bool readBytes(void* destination, std::size_t size) {
        if (size > bytes_.size() - position_) {
            return false;
        }
        if (size != 0) {
            std::memcpy(
                destination, bytes_.data() + position_, size);
        }
        position_ += size;
        return true;
    }

    bool readI32(std::int32_t& value) {
        std::uint8_t bytes[4]{};
        if (!readBytes(bytes, sizeof(bytes))) {
            return false;
        }
        value = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8u) |
            (static_cast<std::uint32_t>(bytes[2]) << 16u) |
            (static_cast<std::uint32_t>(bytes[3]) << 24u));
        return true;
    }

    bool readEncoded(
        std::size_t expected_size,
        std::vector<std::uint8_t>& output) {
        std::uint8_t compressed = 0;
        if (!readBytes(&compressed, 1)) {
            return false;
        }
        if (compressed == 0) {
            output.resize(expected_size);
            return readBytes(output.data(), output.size());
        }
        if (bytes_.size() - position_ < 16) {
            return false;
        }
        const std::uint8_t* block = bytes_.data() + position_;
        const std::uint32_t payload_size =
            static_cast<std::uint32_t>(block[12]) |
            (static_cast<std::uint32_t>(block[13]) << 8u) |
            (static_cast<std::uint32_t>(block[14]) << 16u) |
            (static_cast<std::uint32_t>(block[15]) << 24u);
        const std::size_t block_size =
            static_cast<std::size_t>(payload_size) + 16u;
        if (block_size < payload_size ||
            block_size > bytes_.size() - position_ ||
            !decodeRclibLz(
                block, block_size, expected_size, output)) {
            return false;
        }
        position_ += block_size;
        return true;
    }

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t position_ = 0;
};

std::int16_t readI16(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset) {
    return static_cast<std::int16_t>(
        static_cast<std::uint16_t>(bytes[offset]) |
        (static_cast<std::uint16_t>(bytes[offset + 1]) << 8u));
}

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

}  // namespace

bool GroundMap::load(
    const std::filesystem::path& path,
    std::string* error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        setError(error, "The ground file could not be opened.");
        return false;
    }
    std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    return decode(bytes, error);
}

bool GroundMap::decode(
    const std::vector<std::uint8_t>& bytes,
    std::string* error) {
    clear();
    Reader input(bytes);

    char header[16]{};
    if (!input.readBytes(header, sizeof(header)) ||
        std::memcmp(header, "RPGSCRN_GNDv", 12) != 0 ||
        !input.readI32(width_) ||
        !input.readI32(height_) ||
        !input.readI32(chip_width_) ||
        !input.readI32(chip_height_) ||
        !input.readI32(base_magnification_x_) ||
        !input.readI32(base_magnification_y_) ||
        width_ < 0 || height_ < 0 ||
        chip_width_ <= 0 || chip_height_ <= 0) {
        setError(error, "The ground header is invalid.");
        clear();
        return false;
    }

    const std::size_t width =
        static_cast<std::size_t>(width_);
    const std::size_t height =
        static_cast<std::size_t>(height_);
    if (width != 0 &&
        height >
            std::numeric_limits<std::size_t>::max() / width) {
        setError(error, "The ground dimensions are too large.");
        clear();
        return false;
    }
    const std::size_t count = width * height;
    if (count >
        std::numeric_limits<std::size_t>::max() / 6u) {
        setError(error, "The ground cell data is too large.");
        clear();
        return false;
    }

    std::vector<std::uint8_t> map_data;
    if (!input.readEncoded(count * 6u, map_data)) {
        setError(error, "The ground cells could not be decoded.");
        clear();
        return false;
    }
    cells_.resize(count);
    for (std::size_t index = 0; index < count; ++index) {
        cells_[index].status = readI16(map_data, index * 2u);
        cells_[index].pattern_set =
            readI16(map_data, (count + index) * 2u);
        cells_[index].pattern =
            readI16(map_data, (count * 2u + index) * 2u);
    }
    return true;
}

void GroundMap::clear() {
    width_ = 0;
    height_ = 0;
    chip_width_ = 0;
    chip_height_ = 0;
    base_magnification_x_ = 0;
    base_magnification_y_ = 0;
    cells_.clear();
}

std::int32_t GroundMap::width() const {
    return width_;
}

std::int32_t GroundMap::height() const {
    return height_;
}

std::int32_t GroundMap::chipWidth() const {
    return chip_width_;
}

std::int32_t GroundMap::chipHeight() const {
    return chip_height_;
}

std::int32_t GroundMap::baseMagnificationX() const {
    return base_magnification_x_;
}

std::int32_t GroundMap::baseMagnificationY() const {
    return base_magnification_y_;
}

const GroundCell* GroundMap::cell(
    std::int32_t x,
    std::int32_t y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return nullptr;
    }
    return &cells_[
        static_cast<std::size_t>(y) *
            static_cast<std::size_t>(width_) +
        static_cast<std::size_t>(x)];
}

}  // namespace osf
