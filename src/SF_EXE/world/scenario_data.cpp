#include "scenario_data.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <limits>
#include <utility>

namespace osf {
namespace {

constexpr std::array<std::uint8_t, 16> kMcedHeader = {
    'M', 'C', 'E', 'D', ' ', 'D', 'A', 'T',
    'A', ' ', 'v', '0', '0', '0', '0', 0x1a,
};
constexpr std::size_t kControllerPathOffset = 0x10;
constexpr std::size_t kMapPathOffset = 0x114;
constexpr std::size_t kMusicTrackOffset = 0x220;
constexpr std::size_t kTitleOffset = 0x224;
constexpr std::size_t kFixedHeaderSize = 0x324;
constexpr std::size_t kFixedStringSize = 260;
constexpr std::size_t kTitleSize = 256;
constexpr std::size_t kEntrySize = 16;
constexpr std::size_t kFooterSize = 12;
constexpr std::uint32_t kMaximumEntryCount = 4096;

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

std::uint32_t readU32(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::int32_t readI32(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset) {
    const std::uint32_t raw = readU32(bytes, offset);
    if (raw <= static_cast<std::uint32_t>(
                   std::numeric_limits<std::int32_t>::max())) {
        return static_cast<std::int32_t>(raw);
    }
    return -static_cast<std::int32_t>(~raw) - 1;
}

std::string readFixedString(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::size_t size) {
    const auto first = bytes.begin() +
        static_cast<std::ptrdiff_t>(offset);
    const auto last = first +
        static_cast<std::ptrdiff_t>(size);
    const auto terminator = std::find(first, last, 0);
    return std::string(first, terminator);
}

bool locateEntryTable(
    const std::vector<std::uint8_t>& bytes,
    std::size_t* table_offset,
    std::uint32_t* entry_count) {
    if (bytes.size() < kFixedHeaderSize + sizeof(std::uint32_t) +
                           kFooterSize) {
        return false;
    }

    // The original loader reads the entry table immediately before three
    // final 32-bit scenario fields. The entity section ahead of it is
    // variable-sized, so work backwards from that fixed footer. A table
    // candidate is accepted only when every direction has the retail
    // eight-way range. If entity data happens to resemble a smaller table,
    // the largest valid count is the real outer table.
    const std::size_t available =
        bytes.size() - kFooterSize - sizeof(std::uint32_t);
    const std::uint32_t maximum_count =
        static_cast<std::uint32_t>(std::min<std::size_t>(
            available / kEntrySize, kMaximumEntryCount));

    std::uint32_t best_count = 0;
    std::size_t best_offset = 0;
    for (std::uint32_t count = 1; count <= maximum_count; ++count) {
        const std::size_t table_size =
            sizeof(std::uint32_t) +
            static_cast<std::size_t>(count) * kEntrySize;
        if (table_size + kFooterSize > bytes.size()) {
            break;
        }
        const std::size_t offset =
            bytes.size() - kFooterSize - table_size;
        if (offset < kFixedHeaderSize ||
            readU32(bytes, offset) != count) {
            continue;
        }

        bool valid = true;
        for (std::uint32_t index = 0; index < count; ++index) {
            const std::size_t record =
                offset + sizeof(std::uint32_t) +
                static_cast<std::size_t>(index) * kEntrySize;
            const std::int32_t direction =
                readI32(bytes, record + 12);
            if (direction < 0 || direction > 7) {
                valid = false;
                break;
            }
        }
        if (valid && count > best_count) {
            best_count = count;
            best_offset = offset;
        }
    }

    if (best_count == 0) {
        return false;
    }
    *table_offset = best_offset;
    *entry_count = best_count;
    return true;
}

}  // namespace

bool ScenarioData::load(
    const std::filesystem::path& path,
    std::string* error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        setError(
            error,
            "The scenario file could not be opened: " +
                path.string());
        clear();
        return false;
    }

    const std::vector<std::uint8_t> bytes{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()};
    if (!file.eof() && file.fail()) {
        setError(
            error,
            "The scenario file could not be read: " +
                path.string());
        clear();
        return false;
    }
    return decode(bytes, error);
}

bool ScenarioData::decode(
    const std::vector<std::uint8_t>& bytes,
    std::string* error) {
    clear();
    if (bytes.size() < kFixedHeaderSize ||
        !std::equal(
            kMcedHeader.begin(),
            kMcedHeader.end(),
            bytes.begin())) {
        setError(error, "The scenario file has an invalid MCED header.");
        return false;
    }

    std::size_t table_offset = 0;
    std::uint32_t entry_count = 0;
    if (!locateEntryTable(bytes, &table_offset, &entry_count)) {
        setError(
            error,
            "The scenario entry table could not be located.");
        return false;
    }

    controller_path_ = readFixedString(
        bytes, kControllerPathOffset, kFixedStringSize);
    map_path_ =
        readFixedString(bytes, kMapPathOffset, kFixedStringSize);
    title_ = readFixedString(bytes, kTitleOffset, kTitleSize);
    music_track_ = readI32(bytes, kMusicTrackOffset);

    entries_.reserve(entry_count);
    for (std::uint32_t index = 0; index < entry_count; ++index) {
        const std::size_t record =
            table_offset + sizeof(std::uint32_t) +
            static_cast<std::size_t>(index) * kEntrySize;
        entries_.push_back({
            readI32(bytes, record),
            readI32(bytes, record + 4),
            readI32(bytes, record + 8),
            readI32(bytes, record + 12),
        });
    }
    return true;
}

void ScenarioData::clear() {
    controller_path_.clear();
    map_path_.clear();
    title_.clear();
    music_track_ = -1;
    entries_.clear();
}

const std::string& ScenarioData::controllerPath() const {
    return controller_path_;
}

const std::string& ScenarioData::mapPath() const {
    return map_path_;
}

const std::string& ScenarioData::title() const {
    return title_;
}

std::int32_t ScenarioData::musicTrack() const {
    return music_track_;
}

const std::vector<ScenarioEntry>& ScenarioData::entries() const {
    return entries_;
}

const ScenarioEntry* ScenarioData::findEntry(
    std::int32_t key) const {
    const auto found = std::find_if(
        entries_.begin(),
        entries_.end(),
        [key](const ScenarioEntry& entry) {
            return entry.key == key;
        });
    return found == entries_.end() ? nullptr : &*found;
}

}  // namespace osf
