#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include "libs/RK_FUNCTION/rk_function.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>

namespace osf::gapi {
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
        const std::uint32_t unsigned_value =
            static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8u) |
            (static_cast<std::uint32_t>(bytes[2]) << 16u) |
            (static_cast<std::uint32_t>(bytes[3]) << 24u);
        value = static_cast<std::int32_t>(unsigned_value);
        return true;
    }

    bool readU32(std::uint32_t& value) {
        std::int32_t signed_value = 0;
        if (!readI32(signed_value)) {
            return false;
        }
        value = static_cast<std::uint32_t>(signed_value);
        return true;
    }

    bool skip(std::size_t size) {
        if (size > bytes_.size() - position_) {
            return false;
        }
        position_ += size;
        return true;
    }

    std::size_t position() const {
        return position_;
    }

    const std::uint8_t* current() const {
        return bytes_.data() + position_;
    }

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t position_ = 0;
};

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

bool multiplySize(
    std::size_t first,
    std::size_t second,
    std::size_t& result) {
    if (first != 0 &&
        second > std::numeric_limits<std::size_t>::max() / first) {
        return false;
    }
    result = first * second;
    return true;
}

bool calculateStride(
    std::int32_t bits_per_pixel,
    std::int32_t width,
    std::int32_t& stride) {
    if (width < 0 ||
        (bits_per_pixel != 1 &&
         bits_per_pixel != 4 &&
         bits_per_pixel != 8)) {
        return false;
    }

    std::int64_t bytes = 0;
    if (bits_per_pixel == 1) {
        bytes = (static_cast<std::int64_t>(width) + 7) / 8;
    } else if (bits_per_pixel == 4) {
        bytes = (static_cast<std::int64_t>(width) + 1) / 2;
    } else {
        bytes = width;
    }
    bytes = (bytes + 3) & ~std::int64_t{3};
    if (bytes > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }
    stride = static_cast<std::int32_t>(bytes);
    return true;
}

}  // namespace

bool NjpImage::decode(
    const std::vector<std::uint8_t>& bytes,
    std::string* error) {
    clear();
    Reader input(bytes);

    char header[16]{};
    if (!input.readBytes(header, sizeof(header))) {
        setError(error, "The NJP header is truncated.");
        return false;
    }
    const bool united =
        std::memcmp(header, "UnitePatData", 12) == 0;
    const bool no_judgement =
        std::memcmp(header, "NJudgeUniPat", 12) == 0;
    const bool shadow =
        std::memcmp(header, "ShadowLowPat", 12) == 0;
    if (!united && !no_judgement && !shadow) {
        setError(error, "The NJP signature is not recognized.");
        return false;
    }
    if (header[12] < '0' || header[12] > '9' ||
        header[13] < '0' || header[13] > '9' ||
        header[14] < '0' || header[14] > '9') {
        setError(error, "The NJP version is invalid.");
        return false;
    }

    version_ =
        (header[12] - '0') * 100 +
        (header[13] - '0') * 10 +
        (header[14] - '0');
    if (version_ < 0 || version_ > 3) {
        setError(error, "The NJP version is unsupported.");
        return false;
    }
    shadow_ = shadow;

    std::int32_t part_count = 0;
    if (!input.readI32(part_count) || part_count < 0) {
        setError(error, "The NJP part count is invalid.");
        return false;
    }
    if (version_ > 2 && !input.skip(4)) {
        setError(error, "The NJP combined-part header is truncated.");
        return false;
    }

    parts_.reserve(static_cast<std::size_t>(part_count));
    for (std::int32_t index = 0; index < part_count; ++index) {
        NjpPart part;
        std::int32_t compressed = 0;
        if (!input.readI32(part.bits_per_pixel) ||
            !input.readI32(part.width) ||
            !input.readI32(part.height) ||
            !input.readI32(compressed) ||
            part.height < 0) {
            setError(error, "An NJP part header is invalid.");
            clear();
            return false;
        }
        if (shadow) {
            part.bits_per_pixel = 1;
        }
        if (!calculateStride(
                part.bits_per_pixel, part.width, part.stride)) {
            setError(error, "An NJP part has unsupported dimensions.");
            clear();
            return false;
        }

        std::size_t bitmap_size = 0;
        if (!multiplySize(
                static_cast<std::size_t>(part.stride),
                static_cast<std::size_t>(part.height),
                bitmap_size)) {
            setError(error, "An NJP part is too large.");
            clear();
            return false;
        }

        if (compressed == 0) {
            part.pixels.resize(bitmap_size);
            if (!input.readBytes(
                    part.pixels.data(), part.pixels.size())) {
                setError(error, "An NJP bitmap is truncated.");
                clear();
                return false;
            }
        } else {
            if (bytes.size() - input.position() < 16) {
                setError(error, "An NJP compression header is truncated.");
                clear();
                return false;
            }
            const std::uint8_t* compressed_bytes = input.current();
            const std::uint32_t payload_size =
                static_cast<std::uint32_t>(compressed_bytes[12]) |
                (static_cast<std::uint32_t>(compressed_bytes[13]) << 8u) |
                (static_cast<std::uint32_t>(compressed_bytes[14]) << 16u) |
                (static_cast<std::uint32_t>(compressed_bytes[15]) << 24u);
            const std::size_t block_size =
                static_cast<std::size_t>(payload_size) + 16u;
            if (block_size < payload_size ||
                bytes.size() - input.position() < block_size ||
                !osf::decodeRclibLz(
                    compressed_bytes,
                    block_size,
                    bitmap_size,
                    part.pixels) ||
                part.pixels.size() != bitmap_size ||
                !input.skip(block_size)) {
                setError(error, "An NJP bitmap could not be decompressed.");
                clear();
                return false;
            }
        }
        parts_.push_back(std::move(part));
    }

    std::int32_t pattern_count = 0;
    if (!input.readI32(pattern_count) || pattern_count < 0) {
        setError(error, "The NJP pattern count is invalid.");
        clear();
        return false;
    }
    if (version_ > 2 && !input.skip(4)) {
        setError(error, "The NJP combined-list header is truncated.");
        clear();
        return false;
    }

    patterns_.reserve(static_cast<std::size_t>(pattern_count));
    for (std::int32_t pattern_index = 0;
         pattern_index < pattern_count;
         ++pattern_index) {
        NjpPattern pattern;
        std::int32_t list_count = 0;
        if (!input.readI32(list_count) || list_count < 0 ||
            !input.readI32(pattern.x) ||
            !input.readI32(pattern.y) ||
            !input.readI32(pattern.width) ||
            !input.readI32(pattern.height)) {
            setError(error, "An NJP pattern header is invalid.");
            clear();
            return false;
        }
        if (united && !input.skip(0xa8)) {
            setError(error, "An NJP judgement block is truncated.");
            clear();
            return false;
        }
        if (version_ > 0 &&
            !input.readI32(pattern.default_palette)) {
            setError(error, "An NJP pattern palette is missing.");
            clear();
            return false;
        }

        pattern.parts.reserve(static_cast<std::size_t>(list_count));
        for (std::int32_t list_index = 0;
             list_index < list_count;
             ++list_index) {
            NjpPatternPart item;
            if (!input.readU32(item.flags) ||
                !input.readI32(item.part_index) ||
                !input.readI32(item.x) ||
                !input.readI32(item.y) ||
                !input.readI32(item.palette_offset) ||
                !input.readI32(item.scale_x) ||
                !input.readI32(item.scale_y)) {
                setError(error, "An NJP pattern part is truncated.");
                clear();
                return false;
            }
            pattern.parts.push_back(item);
        }
        patterns_.push_back(std::move(pattern));
    }

    std::int32_t palette_count = 0;
    if (!input.readI32(palette_count) || palette_count < 0) {
        setError(error, "The NJP palette count is invalid.");
        clear();
        return false;
    }
    palettes_.reserve(static_cast<std::size_t>(palette_count));
    for (std::int32_t palette_index = 0;
         palette_index < palette_count;
         ++palette_index) {
        NjpPalette palette{};
        for (Color& color : palette) {
            std::uint8_t entry[4]{};
            if (!input.readBytes(entry, sizeof(entry))) {
                setError(error, "An NJP palette is truncated.");
                clear();
                return false;
            }
            color = {entry[0], entry[1], entry[2], 255};
        }
        palettes_.push_back(palette);
    }

    if (error) {
        error->clear();
    }
    return true;
}

bool NjpImage::load(
    const std::filesystem::path& path,
    std::string* error) {
    clear();
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        setError(error, "Could not open the NJP file.");
        return false;
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size < 0) {
        setError(error, "Could not determine the NJP file size.");
        return false;
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(
        static_cast<std::size_t>(size));
    if (!bytes.empty() &&
        !stream.read(
            reinterpret_cast<char*>(bytes.data()), size)) {
        setError(error, "Could not read the complete NJP file.");
        return false;
    }
    return decode(bytes, error);
}

void NjpImage::clear() {
    version_ = 0;
    shadow_ = false;
    parts_.clear();
    patterns_.clear();
    palettes_.clear();
}

std::int32_t NjpImage::version() const {
    return version_;
}

bool NjpImage::isShadow() const {
    return shadow_;
}

const std::vector<NjpPart>& NjpImage::parts() const {
    return parts_;
}

const std::vector<NjpPattern>& NjpImage::patterns() const {
    return patterns_;
}

const std::vector<NjpPalette>& NjpImage::palettes() const {
    return palettes_;
}

}  // namespace osf::gapi
