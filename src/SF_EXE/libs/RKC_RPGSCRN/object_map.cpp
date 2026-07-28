#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstring>
#include <fstream>
#include <iterator>
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

    bool readI16(std::int16_t& value) {
        std::uint8_t bytes[2]{};
        if (!readBytes(bytes, sizeof(bytes))) {
            return false;
        }
        value = static_cast<std::int16_t>(
            static_cast<std::uint16_t>(bytes[0]) |
            (static_cast<std::uint16_t>(bytes[1]) << 8u));
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

bool readBounds(Reader& input, ObjectBounds& bounds) {
    return input.readI32(bounds.left) &&
           input.readI32(bounds.top) &&
           input.readI32(bounds.right) &&
           input.readI32(bounds.bottom);
}

}  // namespace

bool ObjectMap::load(
    const std::filesystem::path& path,
    std::string* error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        setError(error, "The object file could not be opened.");
        return false;
    }
    std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    return decode(bytes, error);
}

bool ObjectMap::decode(
    const std::vector<std::uint8_t>& bytes,
    std::string* error) {
    clear();
    Reader input(bytes);
    char header[16]{};
    std::int32_t count = 0;
    if (!input.readBytes(header, sizeof(header)) ||
        std::memcmp(header, "RPGSCRN_OBJv", 12) != 0 ||
        header[12] < '0' || header[12] > '9' ||
        header[13] < '0' || header[13] > '9' ||
        header[14] < '0' || header[14] > '9' ||
        !input.readI32(count) || count < 0) {
        setError(error, "The object header is invalid.");
        return false;
    }
    version_ =
        (header[12] - '0') * 100 +
        (header[13] - '0') * 10 +
        (header[14] - '0');
    if (version_ < 0 || version_ > 1) {
        setError(error, "The object version is unsupported.");
        clear();
        return false;
    }

    const std::size_t record_size =
        version_ > 0 ? 42u : 36u;
    if (static_cast<std::uint64_t>(count) >
            std::numeric_limits<std::size_t>::max() /
                record_size ||
        static_cast<std::size_t>(count) * record_size >
            input.remaining()) {
        setError(error, "The object records are truncated.");
        clear();
        return false;
    }

    objects_.reserve(static_cast<std::size_t>(count));
    for (std::int32_t index = 0; index < count; ++index) {
        MapObject object;
        if (!input.readI32(object.world_x) ||
            !input.readI32(object.world_y) ||
            !input.readI16(object.pattern_set) ||
            !input.readI16(object.pattern) ||
            !input.readI16(object.palette) ||
            !input.readI16(object.opacity) ||
            !input.readI16(object.status) ||
            !input.readI16(object.height)) {
            setError(error, "An object record is truncated.");
            clear();
            return false;
        }
        if (version_ > 0 &&
            (!input.readI16(object.red_strength) ||
             !input.readI16(object.green_strength) ||
             !input.readI16(object.blue_strength))) {
            setError(error, "An object color record is truncated.");
            clear();
            return false;
        }
        if (!readBounds(input, object.judgement)) {
            setError(error, "An object judgement record is truncated.");
            clear();
            return false;
        }
        objects_.push_back(object);
    }
    if (error) {
        error->clear();
    }
    return true;
}

void ObjectMap::clear() {
    version_ = 0;
    objects_.clear();
}

std::int32_t ObjectMap::version() const {
    return version_;
}

const std::vector<MapObject>& ObjectMap::objects() const {
    return objects_;
}

}  // namespace osf
