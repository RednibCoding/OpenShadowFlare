#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include "libs/RK_FUNCTION/rk_function.hpp"

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

std::int64_t floorDivide(
    std::int64_t numerator,
    std::int64_t denominator) {
    std::int64_t result = numerator / denominator;
    if (numerator < 0 && numerator % denominator != 0) {
        --result;
    }
    return result;
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

    // GROUNDBLOCK::SetAreaSize derives the judgement dimensions from the
    // 15:10 screen projection before ReadFile decodes the second GND block.
    const std::int64_t real_base_x =
        static_cast<std::int64_t>(
            base_magnification_x_) *
        15 / 100;
    const std::int64_t real_base_y =
        static_cast<std::int64_t>(
            base_magnification_y_) *
        10 / 100;
    const std::int64_t divisor =
        real_base_x * real_base_y * 2;
    if (real_base_x <= 0 || real_base_y <= 0 ||
        divisor <= 0) {
        setError(error, "The ground judgement scale is invalid.");
        clear();
        return false;
    }
    const std::int64_t map_pixel_width =
        static_cast<std::int64_t>(width_) *
        chip_width_;
    const std::int64_t map_pixel_height =
        static_cast<std::int64_t>(height_) *
        chip_height_;
    const auto groundPosition =
        [real_base_x, real_base_y, divisor](
            std::int64_t x,
            std::int64_t y) {
            return std::array<std::int64_t, 2>{
                floorDivide(
                    real_base_x * y + real_base_y * x,
                    divisor),
                floorDivide(
                    real_base_x * y - real_base_y * x,
                    divisor),
            };
        };
    const auto top = groundPosition(0, 0);
    const auto right =
        groundPosition(map_pixel_width, 0);
    const auto left =
        groundPosition(0, map_pixel_height);
    const auto bottom =
        groundPosition(map_pixel_width, map_pixel_height);
    const std::int64_t judge_width =
        bottom[0] - top[0] + 2;
    const std::int64_t judge_height =
        left[1] - right[1] + 2;
    if (judge_width < 0 || judge_height < 0 ||
        judge_width >
            std::numeric_limits<std::int32_t>::max() ||
        judge_height >
            std::numeric_limits<std::int32_t>::max() ||
        (judge_width != 0 &&
         judge_height >
             static_cast<std::int64_t>(
                 std::numeric_limits<std::size_t>::max() /
                 static_cast<std::size_t>(judge_width)))) {
        setError(error, "The ground judgement area is too large.");
        clear();
        return false;
    }
    judge_width_ = static_cast<std::int32_t>(judge_width);
    judge_height_ = static_cast<std::int32_t>(judge_height);
    judge_offset_x_ =
        static_cast<std::int32_t>(top[0] - 1);
    judge_offset_y_ =
        static_cast<std::int32_t>(right[1] - 1);
    const std::size_t judge_count =
        static_cast<std::size_t>(judge_width_) *
        static_cast<std::size_t>(judge_height_);
    if (judge_count >
        std::numeric_limits<std::size_t>::max() / 2u) {
        setError(error, "The ground judgement data is too large.");
        clear();
        return false;
    }
    std::vector<std::uint8_t> judge_data;
    if (!input.readEncoded(judge_count * 2u, judge_data)) {
        setError(error, "The ground judgement data could not be decoded.");
        clear();
        return false;
    }
    judgement_.resize(judge_count);
    for (std::size_t index = 0;
         index < judge_count;
         ++index) {
        judgement_[index] =
            readI16(judge_data, index * 2u);
    }
    if (error) {
        error->clear();
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
    judge_width_ = 0;
    judge_height_ = 0;
    judge_offset_x_ = 0;
    judge_offset_y_ = 0;
    cells_.clear();
    judgement_.clear();
}

std::uint64_t GroundMap::memoryUsageBytes() const {
    return static_cast<std::uint64_t>(cells_.capacity()) *
            sizeof(GroundCell) +
        static_cast<std::uint64_t>(judgement_.capacity()) *
            sizeof(std::int16_t);
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

std::int32_t GroundMap::judgeWidth() const {
    return judge_width_;
}

std::int32_t GroundMap::judgeHeight() const {
    return judge_height_;
}

std::int32_t GroundMap::judgeOffsetX() const {
    return judge_offset_x_;
}

std::int32_t GroundMap::judgeOffsetY() const {
    return judge_offset_y_;
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

const std::int16_t* GroundMap::judge(
    std::int32_t x,
    std::int32_t y) const {
    const std::int32_t local_x = x - judge_offset_x_;
    const std::int32_t local_y = y - judge_offset_y_;
    if (local_x < 0 || local_y < 0 ||
        local_x >= judge_width_ ||
        local_y >= judge_height_) {
        return nullptr;
    }
    return &judgement_[
        static_cast<std::size_t>(local_y) *
            static_cast<std::size_t>(judge_width_) +
        static_cast<std::size_t>(local_x)];
}

}  // namespace osf
