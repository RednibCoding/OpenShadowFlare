#include "libs/RKC_DSOUND/rkc_dsound.hpp"

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
        if (size > remaining()) {
            return false;
        }
        if (size != 0) {
            std::memcpy(
                destination, bytes_.data() + position_, size);
        }
        position_ += size;
        return true;
    }

    bool readU16(std::uint16_t& value) {
        std::uint8_t bytes[2]{};
        if (!readBytes(bytes, sizeof(bytes))) {
            return false;
        }
        value =
            static_cast<std::uint16_t>(bytes[0]) |
            static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(bytes[1]) << 8u);
        return true;
    }

    bool readU32(std::uint32_t& value) {
        std::uint8_t bytes[4]{};
        if (!readBytes(bytes, sizeof(bytes))) {
            return false;
        }
        value =
            static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8u) |
            (static_cast<std::uint32_t>(bytes[2]) << 16u) |
            (static_cast<std::uint32_t>(bytes[3]) << 24u);
        return true;
    }

    bool readFixedString(std::size_t size, std::string& value) {
        if (size > remaining()) {
            return false;
        }
        const char* begin = reinterpret_cast<const char*>(
            bytes_.data() + position_);
        std::size_t length = 0;
        while (length < size && begin[length] != '\0') {
            ++length;
        }
        value.assign(begin, length);
        position_ += size;
        return true;
    }

    bool skip(std::size_t size) {
        if (size > remaining()) {
            return false;
        }
        position_ += size;
        return true;
    }

    std::size_t remaining() const {
        return bytes_.size() - position_;
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

bool readFormat(Reader& input, VocPcmFormat& format) {
    return input.readU16(format.format_tag) &&
           input.readU16(format.channels) &&
           input.readU32(format.sample_rate) &&
           input.readU32(format.average_bytes_per_second) &&
           input.readU16(format.frame_stride_bytes) &&
           input.readU16(format.bits_per_sample) &&
           input.readU16(format.extra_size);
}

}  // namespace

bool VocFile::decode(
    const std::vector<std::uint8_t>& bytes,
    std::string* error) {
    clear();
    Reader input(bytes);

    char header[16]{};
    if (!input.readBytes(header, sizeof(header))) {
        setError(error, "The VOC header is truncated.");
        return false;
    }
    if (std::memcmp(header, "VoiceData  V001", 15) == 0) {
        version_ = 1;
    } else if (
        std::memcmp(header, "VoiceData  V003", 15) == 0) {
        version_ = 3;
    } else {
        setError(error, "The VOC signature or version is invalid.");
        return false;
    }

    std::uint32_t sample_count = 0;
    std::uint32_t variants = 0;
    if (!input.readU32(sample_count) ||
        (version_ == 3 && !input.readU32(variants))) {
        setError(error, "The VOC sample counts are truncated.");
        clear();
        return false;
    }
    const std::size_t minimum_entry_size =
        version_ == 3 ? 516u : 278u;
    if (sample_count >
        input.remaining() / minimum_entry_size) {
        setError(error, "The VOC sample count is invalid.");
        clear();
        return false;
    }
    if (variants >
        static_cast<std::uint32_t>(
            std::numeric_limits<std::int32_t>::max())) {
        setError(error, "The VOC variant count is invalid.");
        clear();
        return false;
    }
    variant_count_ = static_cast<std::int32_t>(variants);

    samples_.reserve(static_cast<std::size_t>(sample_count));
    for (std::uint32_t index = 0;
         index < sample_count;
         ++index) {
        VocSample sample;
        std::uint32_t flags = 0;
        if ((version_ == 3 && !input.readU32(flags)) ||
            !input.readFixedString(256, sample.name)) {
            setError(error, "A VOC sample header is truncated.");
            clear();
            return false;
        }

        if (version_ == 3 && (flags & 1u) != 0u) {
            if (!input.readFixedString(256, sample.name)) {
                setError(
                    error,
                    "A VOC reference name is truncated.");
                clear();
                return false;
            }
            sample.reference_index = -2;
            samples_.push_back(std::move(sample));
            continue;
        }

        if (version_ == 3 && !input.skip(256)) {
            setError(error, "A VOC alternate name is truncated.");
            clear();
            return false;
        }

        std::uint32_t pcm_size = 0;
        if (!readFormat(input, sample.format) ||
            !input.readU32(pcm_size) ||
            pcm_size > input.remaining()) {
            setError(error, "A VOC PCM block is invalid.");
            clear();
            return false;
        }
        sample.pcm.resize(static_cast<std::size_t>(pcm_size));
        if (!input.readBytes(
                sample.pcm.data(), sample.pcm.size())) {
            setError(error, "A VOC PCM block is truncated.");
            clear();
            return false;
        }
        samples_.push_back(std::move(sample));
    }

    for (std::size_t index = 0; index < samples_.size(); ++index) {
        VocSample& sample = samples_[index];
        if (sample.reference_index != -2) {
            continue;
        }
        sample.reference_index = -1;
        for (std::size_t source = 0;
             source < samples_.size();
             ++source) {
            if (source != index &&
                samples_[source].reference_index == -1 &&
                samples_[source].name == sample.name) {
                sample.reference_index =
                    static_cast<std::int32_t>(source);
                sample.format = samples_[source].format;
                break;
            }
        }
        if (sample.reference_index < 0) {
            setError(error, "A VOC sample reference is unresolved.");
            clear();
            return false;
        }
    }

    if (error) {
        error->clear();
    }
    return true;
}

bool VocFile::load(
    const std::filesystem::path& path,
    std::string* error) {
    clear();
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        setError(error, "Could not open the VOC file.");
        return false;
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size < 0 ||
        static_cast<std::uint64_t>(size) >
            std::numeric_limits<std::size_t>::max()) {
        setError(error, "Could not determine the VOC file size.");
        return false;
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(
        static_cast<std::size_t>(size));
    if (!bytes.empty() &&
        !stream.read(
            reinterpret_cast<char*>(bytes.data()), size)) {
        setError(error, "Could not read the complete VOC file.");
        return false;
    }
    return decode(bytes, error);
}

void VocFile::clear() {
    version_ = 0;
    variant_count_ = 0;
    samples_.clear();
}

std::int32_t VocFile::version() const {
    return version_;
}

std::int32_t VocFile::variant_count() const {
    return variant_count_;
}

const std::vector<VocSample>& VocFile::samples() const {
    return samples_;
}

}  // namespace osf
