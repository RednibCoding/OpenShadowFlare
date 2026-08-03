#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include "libs/RK_FUNCTION/rk_function.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>

namespace osf::gapi {
namespace {

class MemoryReader {
public:
    explicit MemoryReader(const std::vector<std::uint8_t>& bytes)
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

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t position_ = 0;
};

class StreamReader {
public:
    StreamReader(std::ifstream& stream, std::size_t size)
        : stream_(stream), size_(size) {}

    bool readBytes(void* destination, std::size_t size) {
        if (size > size_ - position_ ||
            size > static_cast<std::size_t>(
                       std::numeric_limits<std::streamsize>::max())) {
            return false;
        }
        if (size != 0 &&
            !stream_.read(
                static_cast<char*>(destination),
                static_cast<std::streamsize>(size))) {
            return false;
        }
        position_ += size;
        return true;
    }

    bool skip(std::size_t size) {
        if (size > size_ - position_) {
            return false;
        }
        position_ += size;
        stream_.seekg(
            static_cast<std::streamoff>(position_),
            std::ios::beg);
        return static_cast<bool>(stream_);
    }

    std::size_t position() const {
        return position_;
    }

private:
    std::ifstream& stream_;
    std::size_t size_ = 0;
    std::size_t position_ = 0;
};

struct EncodedPart {
    std::size_t offset = 0;
    std::size_t encoded_size = 0;
    std::size_t decoded_size = 0;
    bool compressed = false;
};

struct ParsedNjp {
    std::int32_t version = 0;
    bool shadow = false;
    std::vector<NjpPart> parts;
    std::vector<EncodedPart> encoded_parts;
    std::vector<NjpPattern> patterns;
    std::vector<NjpPalette> palettes;
    std::vector<std::uint8_t> decoded_pattern_flags;
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

template <typename Reader>
bool readI32(Reader& input, std::int32_t& value) {
    std::uint8_t bytes[4]{};
    if (!input.readBytes(bytes, sizeof(bytes))) {
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

template <typename Reader>
bool readU32(Reader& input, std::uint32_t& value) {
    std::int32_t signed_value = 0;
    if (!readI32(input, signed_value)) {
        return false;
    }
    value = static_cast<std::uint32_t>(signed_value);
    return true;
}

template <typename Reader>
bool parseNjp(
    Reader& input,
    ParsedNjp& parsed,
    std::string* error) {
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

    parsed.version =
        (header[12] - '0') * 100 +
        (header[13] - '0') * 10 +
        (header[14] - '0');
    if (parsed.version < 0 || parsed.version > 3) {
        setError(error, "The NJP version is unsupported.");
        return false;
    }
    parsed.shadow = shadow;

    std::int32_t part_count = 0;
    if (!readI32(input, part_count) || part_count < 0) {
        setError(error, "The NJP part count is invalid.");
        return false;
    }
    if (parsed.version > 2 && !input.skip(4)) {
        setError(error, "The NJP combined-part header is truncated.");
        return false;
    }

    parsed.parts.reserve(static_cast<std::size_t>(part_count));
    parsed.encoded_parts.reserve(
        static_cast<std::size_t>(part_count));
    for (std::int32_t index = 0; index < part_count; ++index) {
        NjpPart part;
        std::int32_t compressed = 0;
        if (!readI32(input, part.bits_per_pixel) ||
            !readI32(input, part.width) ||
            !readI32(input, part.height) ||
            !readI32(input, compressed) ||
            part.height < 0) {
            setError(error, "An NJP part header is invalid.");
            return false;
        }
        if (shadow) {
            part.bits_per_pixel = 1;
        }
        if (!calculateStride(
                part.bits_per_pixel, part.width, part.stride)) {
            setError(error, "An NJP part has unsupported dimensions.");
            return false;
        }

        EncodedPart encoded;
        if (!multiplySize(
                static_cast<std::size_t>(part.stride),
                static_cast<std::size_t>(part.height),
                encoded.decoded_size)) {
            setError(error, "An NJP part is too large.");
            return false;
        }
        encoded.offset = input.position();
        encoded.compressed = compressed != 0;
        if (!encoded.compressed) {
            encoded.encoded_size = encoded.decoded_size;
            if (!input.skip(encoded.encoded_size)) {
                setError(error, "An NJP bitmap is truncated.");
                return false;
            }
        } else {
            std::uint8_t compression_header[16]{};
            if (!input.readBytes(
                    compression_header,
                    sizeof(compression_header))) {
                setError(
                    error,
                    "An NJP compression header is truncated.");
                return false;
            }
            const std::uint32_t payload_size =
                static_cast<std::uint32_t>(
                    compression_header[12]) |
                (static_cast<std::uint32_t>(
                     compression_header[13]) << 8u) |
                (static_cast<std::uint32_t>(
                     compression_header[14]) << 16u) |
                (static_cast<std::uint32_t>(
                     compression_header[15]) << 24u);
            encoded.encoded_size =
                static_cast<std::size_t>(payload_size) + 16u;
            if (encoded.encoded_size < payload_size ||
                !input.skip(static_cast<std::size_t>(payload_size))) {
                setError(
                    error,
                    "An NJP bitmap could not be decompressed.");
                return false;
            }
        }
        parsed.parts.push_back(std::move(part));
        parsed.encoded_parts.push_back(encoded);
    }

    std::int32_t pattern_count = 0;
    if (!readI32(input, pattern_count) || pattern_count < 0) {
        setError(error, "The NJP pattern count is invalid.");
        return false;
    }
    if (parsed.version > 2 && !input.skip(4)) {
        setError(error, "The NJP combined-list header is truncated.");
        return false;
    }

    parsed.patterns.reserve(
        static_cast<std::size_t>(pattern_count));
    for (std::int32_t pattern_index = 0;
         pattern_index < pattern_count;
         ++pattern_index) {
        NjpPattern pattern;
        std::int32_t list_count = 0;
        if (!readI32(input, list_count) || list_count < 0 ||
            !readI32(input, pattern.x) ||
            !readI32(input, pattern.y) ||
            !readI32(input, pattern.width) ||
            !readI32(input, pattern.height)) {
            setError(error, "An NJP pattern header is invalid.");
            return false;
        }
        if (united && !input.skip(0xa8)) {
            setError(error, "An NJP judgement block is truncated.");
            return false;
        }
        if (parsed.version > 0 &&
            !readI32(input, pattern.default_palette)) {
            setError(error, "An NJP pattern palette is missing.");
            return false;
        }

        pattern.parts.reserve(static_cast<std::size_t>(list_count));
        for (std::int32_t list_index = 0;
             list_index < list_count;
             ++list_index) {
            NjpPatternPart item;
            if (!readU32(input, item.flags) ||
                !readI32(input, item.part_index) ||
                !readI32(input, item.x) ||
                !readI32(input, item.y) ||
                !readI32(input, item.palette_offset) ||
                !readI32(input, item.scale_x) ||
                !readI32(input, item.scale_y)) {
                setError(error, "An NJP pattern part is truncated.");
                return false;
            }
            pattern.parts.push_back(item);
        }
        parsed.patterns.push_back(std::move(pattern));
    }
    parsed.decoded_pattern_flags.assign(
        parsed.patterns.size(), 1);

    std::int32_t palette_count = 0;
    if (!readI32(input, palette_count) || palette_count < 0) {
        setError(error, "The NJP palette count is invalid.");
        return false;
    }
    parsed.palettes.reserve(
        static_cast<std::size_t>(palette_count));
    for (std::int32_t palette_index = 0;
         palette_index < palette_count;
         ++palette_index) {
        NjpPalette palette{};
        for (Color& color : palette) {
            std::uint8_t entry[4]{};
            if (!input.readBytes(entry, sizeof(entry))) {
                setError(error, "An NJP palette is truncated.");
                return false;
            }
            color = {entry[0], entry[1], entry[2], 255};
        }
        parsed.palettes.push_back(palette);
    }
    return true;
}

std::vector<std::uint8_t> requiredParts(
    const ParsedNjp& parsed,
    const std::vector<std::uint8_t>* enabled_patterns) {
    std::vector<std::uint8_t> required(
        parsed.parts.size(), enabled_patterns ? 0 : 1);
    if (!enabled_patterns) {
        return required;
    }
    const std::size_t pattern_count = std::min(
        parsed.patterns.size(), enabled_patterns->size());
    for (std::size_t pattern_index = 0;
         pattern_index < pattern_count;
         ++pattern_index) {
        if ((*enabled_patterns)[pattern_index] == 0) {
            continue;
        }
        for (const NjpPatternPart& item :
             parsed.patterns[pattern_index].parts) {
            if (item.part_index >= 0 &&
                static_cast<std::size_t>(item.part_index) <
                    required.size()) {
                required[static_cast<std::size_t>(
                    item.part_index)] = 1;
            }
        }
    }
    return required;
}

void discardDisabledPatterns(
    ParsedNjp& parsed,
    const std::vector<std::uint8_t>* enabled_patterns) {
    if (!enabled_patterns) {
        return;
    }
    for (std::size_t index = 0;
         index < parsed.patterns.size();
         ++index) {
        if (index < enabled_patterns->size() &&
            (*enabled_patterns)[index] != 0) {
            continue;
        }
        std::vector<NjpPatternPart>().swap(
            parsed.patterns[index].parts);
        parsed.decoded_pattern_flags[index] = 0;
    }
}

bool decodePartBlock(
    std::vector<std::uint8_t> block,
    const EncodedPart& encoded,
    NjpPart& part) {
    if (!encoded.compressed) {
        if (block.size() != encoded.decoded_size) {
            return false;
        }
        part.pixels = std::move(block);
        return true;
    }
    return osf::decodeRclibLz(
               block.data(),
               block.size(),
               encoded.decoded_size,
               part.pixels) &&
        part.pixels.size() == encoded.decoded_size;
}

bool decodeSelectedMemoryParts(
    const std::vector<std::uint8_t>& bytes,
    const std::vector<std::uint8_t>* enabled_patterns,
    ParsedNjp& parsed,
    std::string* error) {
    const std::vector<std::uint8_t> required =
        requiredParts(parsed, enabled_patterns);
    for (std::size_t index = 0;
         index < parsed.parts.size();
         ++index) {
        if (required[index] == 0) {
            continue;
        }
        const EncodedPart& encoded = parsed.encoded_parts[index];
        if (encoded.offset > bytes.size() ||
            encoded.encoded_size > bytes.size() - encoded.offset) {
            setError(error, "An NJP bitmap is truncated.");
            return false;
        }
        std::vector<std::uint8_t> block(
            bytes.begin() + static_cast<std::ptrdiff_t>(encoded.offset),
            bytes.begin() + static_cast<std::ptrdiff_t>(
                                encoded.offset + encoded.encoded_size));
        if (!decodePartBlock(
                std::move(block),
                encoded,
                parsed.parts[index])) {
            setError(
                error,
                "An NJP bitmap could not be decompressed.");
            return false;
        }
    }
    discardDisabledPatterns(parsed, enabled_patterns);
    return true;
}

bool decodeSelectedFileParts(
    std::ifstream& stream,
    const std::vector<std::uint8_t>* enabled_patterns,
    ParsedNjp& parsed,
    std::string* error) {
    const std::vector<std::uint8_t> required =
        requiredParts(parsed, enabled_patterns);
    for (std::size_t index = 0;
         index < parsed.parts.size();
         ++index) {
        if (required[index] == 0) {
            continue;
        }
        const EncodedPart& encoded = parsed.encoded_parts[index];
        if (encoded.encoded_size > static_cast<std::size_t>(
                std::numeric_limits<std::streamsize>::max())) {
            setError(error, "An NJP bitmap is too large to read.");
            return false;
        }
        stream.clear();
        stream.seekg(
            static_cast<std::streamoff>(encoded.offset),
            std::ios::beg);
        std::vector<std::uint8_t> block(encoded.encoded_size);
        if (!stream ||
            (!block.empty() &&
             !stream.read(
                 reinterpret_cast<char*>(block.data()),
                 static_cast<std::streamsize>(block.size()))) ||
            !decodePartBlock(
                std::move(block),
                encoded,
                parsed.parts[index])) {
            setError(
                error,
                "An NJP bitmap could not be decompressed.");
            return false;
        }
    }
    discardDisabledPatterns(parsed, enabled_patterns);
    return true;
}

bool decodeNjpMemory(
    const std::vector<std::uint8_t>& bytes,
    const std::vector<std::uint8_t>* enabled_patterns,
    ParsedNjp& parsed,
    std::string* error) {
    MemoryReader input(bytes);
    return parseNjp(input, parsed, error) &&
        decodeSelectedMemoryParts(
            bytes, enabled_patterns, parsed, error);
}

bool decodeNjpFile(
    const std::filesystem::path& path,
    const std::vector<std::uint8_t>* enabled_patterns,
    ParsedNjp& parsed,
    std::string* error) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        setError(error, "Could not open the NJP file.");
        return false;
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff stream_size = stream.tellg();
    if (stream_size < 0 ||
        static_cast<std::uintmax_t>(stream_size) >
            std::numeric_limits<std::size_t>::max()) {
        setError(error, "Could not determine the NJP file size.");
        return false;
    }
    stream.seekg(0, std::ios::beg);
    StreamReader input(
        stream, static_cast<std::size_t>(stream_size));
    return parseNjp(input, parsed, error) &&
        decodeSelectedFileParts(
            stream, enabled_patterns, parsed, error);
}

}  // namespace

bool NjpPart::hasDecodedPixels() const {
    if (width <= 0 || height <= 0) {
        return true;
    }
    if (stride <= 0 ||
        static_cast<std::size_t>(height) >
            std::numeric_limits<std::size_t>::max() /
                static_cast<std::size_t>(stride)) {
        return false;
    }
    return pixels.size() >=
        static_cast<std::size_t>(stride) *
            static_cast<std::size_t>(height);
}

bool NjpImage::decode(
    const std::vector<std::uint8_t>& bytes,
    std::string* error) {
    clear();
    ParsedNjp parsed;
    if (!decodeNjpMemory(bytes, nullptr, parsed, error)) {
        return false;
    }
    version_ = parsed.version;
    shadow_ = parsed.shadow;
    parts_ = std::move(parsed.parts);
    patterns_ = std::move(parsed.patterns);
    palettes_ = std::move(parsed.palettes);
    decoded_pattern_flags_ =
        std::move(parsed.decoded_pattern_flags);
    if (error) {
        error->clear();
    }
    return true;
}

bool NjpImage::decodeSelectedPatterns(
    const std::vector<std::uint8_t>& bytes,
    const std::vector<std::uint8_t>& enabled_patterns,
    std::string* error) {
    clear();
    ParsedNjp parsed;
    if (!decodeNjpMemory(
            bytes, &enabled_patterns, parsed, error)) {
        return false;
    }
    version_ = parsed.version;
    shadow_ = parsed.shadow;
    parts_ = std::move(parsed.parts);
    patterns_ = std::move(parsed.patterns);
    palettes_ = std::move(parsed.palettes);
    decoded_pattern_flags_ =
        std::move(parsed.decoded_pattern_flags);
    if (error) {
        error->clear();
    }
    return true;
}

bool NjpImage::load(
    const std::filesystem::path& path,
    std::string* error) {
    clear();
    ParsedNjp parsed;
    if (!decodeNjpFile(path, nullptr, parsed, error)) {
        return false;
    }
    version_ = parsed.version;
    shadow_ = parsed.shadow;
    parts_ = std::move(parsed.parts);
    patterns_ = std::move(parsed.patterns);
    palettes_ = std::move(parsed.palettes);
    decoded_pattern_flags_ =
        std::move(parsed.decoded_pattern_flags);
    if (error) {
        error->clear();
    }
    return true;
}

bool NjpImage::loadSelectedPatterns(
    const std::filesystem::path& path,
    const std::vector<std::uint8_t>& enabled_patterns,
    std::string* error) {
    clear();
    ParsedNjp parsed;
    if (!decodeNjpFile(
            path, &enabled_patterns, parsed, error)) {
        return false;
    }
    version_ = parsed.version;
    shadow_ = parsed.shadow;
    parts_ = std::move(parsed.parts);
    patterns_ = std::move(parsed.patterns);
    palettes_ = std::move(parsed.palettes);
    decoded_pattern_flags_ =
        std::move(parsed.decoded_pattern_flags);
    if (error) {
        error->clear();
    }
    return true;
}

void NjpImage::clear() {
    version_ = 0;
    shadow_ = false;
    parts_.clear();
    patterns_.clear();
    palettes_.clear();
    decoded_pattern_flags_.clear();
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

bool NjpImage::patternDecoded(std::size_t pattern_index) const {
    return pattern_index < decoded_pattern_flags_.size() &&
        decoded_pattern_flags_[pattern_index] != 0;
}

const std::vector<std::uint8_t>&
NjpImage::decodedPatternFlags() const {
    return decoded_pattern_flags_;
}

}  // namespace osf::gapi
