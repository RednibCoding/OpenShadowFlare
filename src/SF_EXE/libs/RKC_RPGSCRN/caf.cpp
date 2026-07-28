#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

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

    bool readI16(std::int16_t& value) {
        std::uint8_t bytes[2]{};
        if (!readBytes(bytes, sizeof(bytes))) {
            return false;
        }
        const std::uint16_t raw =
            static_cast<std::uint16_t>(bytes[0]) |
            (static_cast<std::uint16_t>(bytes[1]) << 8u);
        value = static_cast<std::int16_t>(raw);
        return true;
    }

    bool readI32(std::int32_t& value) {
        std::uint8_t bytes[4]{};
        if (!readBytes(bytes, sizeof(bytes))) {
            return false;
        }
        const std::uint32_t raw =
            static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8u) |
            (static_cast<std::uint32_t>(bytes[2]) << 16u) |
            (static_cast<std::uint32_t>(bytes[3]) << 24u);
        value = static_cast<std::int32_t>(raw);
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

bool plausibleCount(
    std::int32_t count,
    std::size_t remaining,
    std::size_t minimum_item_size) {
    return count >= 0 &&
           static_cast<std::uint64_t>(count) <=
               remaining / minimum_item_size;
}

}  // namespace

bool CafAnimation::decode(
    const std::vector<std::uint8_t>& bytes,
    std::string* error) {
    clear();
    Reader input(bytes);

    char header[16]{};
    if (!input.readBytes(header, sizeof(header))) {
        setError(error, "The CAF header is truncated.");
        return false;
    }
    if (std::memcmp(header, "CHRAnimation", 12) != 0 ||
        header[12] < '0' || header[12] > '9' ||
        header[13] < '0' || header[13] > '9' ||
        header[14] < '0' || header[14] > '9') {
        setError(error, "The CAF signature or version is invalid.");
        return false;
    }
    version_ =
        (header[12] - '0') * 100 +
        (header[13] - '0') * 10 +
        (header[14] - '0');

    std::int32_t chart_count = 0;
    if (!input.readI32(chart_count) ||
        !plausibleCount(chart_count, input.remaining(), 56)) {
        setError(error, "The CAF chart count is invalid.");
        clear();
        return false;
    }

    charts_.reserve(static_cast<std::size_t>(chart_count));
    for (std::int32_t chart_index = 0;
         chart_index < chart_count;
         ++chart_index) {
        CafChart chart;
        if (!input.readI16(chart.status)) {
            setError(error, "A CAF chart header is truncated.");
            clear();
            return false;
        }

        for (CafDirection& direction : chart.directions) {
                std::int32_t part_count = 0;
                if (!input.readI32(part_count) ||
                    !plausibleCount(
                    part_count, input.remaining(), 4) ||
                !input.readI16(direction.frame_count) ||
                direction.frame_count < 0) {
                setError(error, "A CAF direction is invalid.");
                clear();
                return false;
            }

            direction.parts.reserve(
                static_cast<std::size_t>(part_count));
            for (std::int32_t part_index = 0;
                 part_index < part_count;
                 ++part_index) {
                std::int32_t cell_count = 0;
                const std::size_t cell_size =
                    version_ < 2 ? 8u : 10u;
                if (!input.readI32(cell_count) ||
                    !plausibleCount(
                        cell_count,
                        input.remaining(),
                        cell_size)) {
                    setError(error, "A CAF part is invalid.");
                    clear();
                    return false;
                }

                std::vector<CafCell> cells;
                cells.reserve(static_cast<std::size_t>(cell_count));
                for (std::int32_t cell_index = 0;
                     cell_index < cell_count;
                     ++cell_index) {
                    CafCell cell;
                    if (!input.readI16(cell.status) ||
                        !input.readI16(cell.transparency)) {
                        setError(error, "A CAF cell is truncated.");
                        clear();
                        return false;
                    }
                    if (version_ < 2) {
                        std::int16_t pattern_index = 0;
                        if (!input.readI16(pattern_index)) {
                            setError(
                                error,
                                "A CAF pattern index is truncated.");
                            clear();
                            return false;
                        }
                        cell.pattern_index = pattern_index;
                    } else if (!input.readI32(cell.pattern_index)) {
                        setError(
                            error,
                            "A CAF pattern index is truncated.");
                        clear();
                        return false;
                    }
                    if (!input.readI16(cell.priority)) {
                        setError(error, "A CAF priority is truncated.");
                        clear();
                        return false;
                    }
                    cells.push_back(cell);
                }
                direction.parts.push_back(std::move(cells));
            }
        }
        charts_.push_back(std::move(chart));
    }

    if (version_ != 0 &&
        (!input.readI32(palette_mode_) ||
         !input.readI32(chart_priority_stride_))) {
        setError(error, "The CAF trailer is truncated.");
        clear();
        return false;
    }

    if (error) {
        error->clear();
    }
    return true;
}

bool CafAnimation::load(
    const std::filesystem::path& path,
    std::string* error) {
    clear();
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        setError(error, "Could not open the CAF file.");
        return false;
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size < 0 ||
        static_cast<std::uint64_t>(size) >
            std::numeric_limits<std::size_t>::max()) {
        setError(error, "Could not determine the CAF file size.");
        return false;
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(
        static_cast<std::size_t>(size));
    if (!bytes.empty() &&
        !stream.read(
            reinterpret_cast<char*>(bytes.data()), size)) {
        setError(error, "Could not read the complete CAF file.");
        return false;
    }
    return decode(bytes, error);
}

void CafAnimation::clear() {
    version_ = 0;
    palette_mode_ = 0;
    chart_priority_stride_ = 0;
    charts_.clear();
}

std::int32_t CafAnimation::version() const {
    return version_;
}

std::int32_t CafAnimation::palette_mode() const {
    return palette_mode_;
}

std::int32_t CafAnimation::chart_priority_stride() const {
    return chart_priority_stride_;
}

std::size_t CafAnimation::maxPartCount() const {
    std::size_t maximum = 0;
    for (const CafChart& chart : charts_) {
        for (const CafDirection& direction : chart.directions) {
            maximum = std::max(maximum, direction.parts.size());
        }
    }
    return maximum;
}

const std::vector<CafChart>& CafAnimation::charts() const {
    return charts_;
}

}  // namespace osf::gapi
