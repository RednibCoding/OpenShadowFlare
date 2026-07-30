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
constexpr std::uint32_t kMaximumEntityCount = 4096;
constexpr std::uint32_t kMaximumEntityStringSize = 65535;
constexpr std::size_t kObjectEntityTailSize = 0x34;

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

class Reader {
public:
    Reader(
        const std::vector<std::uint8_t>& bytes,
        std::size_t offset)
        : bytes_(bytes), offset_(offset) {}

    bool readU32(std::uint32_t& value) {
        if (!has(4)) {
            return false;
        }
        value = osf::readU32(bytes_, offset_);
        offset_ += 4;
        return true;
    }

    bool readI32(std::int32_t& value) {
        if (!has(4)) {
            return false;
        }
        value = osf::readI32(bytes_, offset_);
        offset_ += 4;
        return true;
    }

    bool readI16(std::int16_t& value) {
        if (!has(2)) {
            return false;
        }
        const std::uint16_t raw =
            static_cast<std::uint16_t>(bytes_[offset_]) |
            (static_cast<std::uint16_t>(bytes_[offset_ + 1])
             << 8);
        value = raw <= static_cast<std::uint16_t>(
                           std::numeric_limits<std::int16_t>::max())
                    ? static_cast<std::int16_t>(raw)
                    : static_cast<std::int16_t>(
                          -static_cast<std::int16_t>(
                              static_cast<std::uint16_t>(~raw)) -
                          1);
        offset_ += 2;
        return true;
    }

    bool readString(std::size_t size, std::string& value) {
        if (!has(size)) {
            return false;
        }
        value.assign(
            bytes_.begin() +
                static_cast<std::ptrdiff_t>(offset_),
            bytes_.begin() +
                static_cast<std::ptrdiff_t>(offset_ + size));
        offset_ += size;
        return true;
    }

    bool skip(std::size_t size) {
        if (!has(size)) {
            return false;
        }
        offset_ += size;
        return true;
    }

    std::size_t remaining() const {
        return bytes_.size() - offset_;
    }

private:
    bool has(std::size_t size) const {
        return size <= bytes_.size() - offset_;
    }

    const std::vector<std::uint8_t>& bytes_;
    std::size_t offset_;
};

struct CommonEntity {
    std::int32_t id = 0;
    std::int32_t resource_id = 0;
    std::string name;
    std::uint32_t name_color = 0;
    std::int32_t label_height = 0;
    std::int32_t world_x = 0;
    std::int32_t world_y = 0;
    std::array<std::int32_t, 4> judgement{};
    std::int32_t direction = 0;
    std::vector<std::int32_t> part_overrides;
    std::vector<std::int32_t> part_visibility;
    std::vector<std::int16_t> red_strength;
    std::vector<std::int16_t> green_strength;
    std::vector<std::int16_t> blue_strength;
};

bool plausibleCount(
    std::uint32_t count,
    std::size_t remaining,
    std::size_t item_size) {
    return count <= kMaximumEntityCount &&
           (item_size == 0 ||
            static_cast<std::size_t>(count) <=
                remaining / item_size);
}

bool skipI32List(Reader& input) {
    std::uint32_t count = 0;
    return input.readU32(count) &&
           plausibleCount(count, input.remaining(), 4) &&
           input.skip(static_cast<std::size_t>(count) * 4);
}

bool readI32Vector(
    Reader& input,
    std::uint32_t count,
    std::vector<std::int32_t>& values) {
    if (!plausibleCount(count, input.remaining(), 4)) {
        return false;
    }
    values.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        std::int32_t value = 0;
        if (!input.readI32(value)) {
            return false;
        }
        values.push_back(value);
    }
    return true;
}

bool readI16Vector(
    Reader& input,
    std::uint32_t count,
    std::vector<std::int16_t>& values) {
    if (!plausibleCount(count, input.remaining(), 2)) {
        return false;
    }
    values.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        std::int16_t value = 0;
        if (!input.readI16(value)) {
            return false;
        }
        values.push_back(value);
    }
    return true;
}

bool readCommonEntity(Reader& input, CommonEntity& entity) {
    std::uint32_t name_size = 0;
    if (!input.readI32(entity.id) ||
        !input.readI32(entity.resource_id) ||
        !input.readU32(name_size) ||
        name_size > kMaximumEntityStringSize ||
        !input.readString(name_size, entity.name)) {
        return false;
    }
    if (name_size != 0 &&
        !input.readU32(entity.name_color)) {
        return false;
    }

    std::int32_t unknown = 0;
    if (!input.readI32(entity.label_height) ||
        !input.readI32(entity.world_x) ||
        !input.readI32(entity.world_y)) {
        return false;
    }
    for (std::int32_t& edge : entity.judgement) {
        if (!input.readI32(edge)) {
            return false;
        }
    }
    if (!input.readI32(entity.direction)) {
        return false;
    }

    std::uint32_t override_count = 0;
    if (!input.readU32(override_count) ||
        !readI32Vector(
            input, override_count, entity.part_overrides)) {
        return false;
    }

    std::int32_t has_custom_parts = 0;
    if (!input.readI32(has_custom_parts)) {
        return false;
    }
    if (has_custom_parts == 1) {
        std::uint32_t part_count = 0;
        if (!input.readU32(part_count) ||
            !readI32Vector(
                input, part_count, entity.part_visibility) ||
            !readI16Vector(
                input, part_count, entity.red_strength) ||
            !readI16Vector(
                input, part_count, entity.green_strength) ||
            !readI16Vector(
                input, part_count, entity.blue_strength)) {
            return false;
        }
    }
    return input.readI32(unknown);
}

bool readPeople(
    const std::vector<std::uint8_t>& bytes,
    std::vector<ScenarioPerson>& people) {
    Reader input(bytes, kFixedHeaderSize);

    // These three ID lists are loaded ahead of the runtime entity groups.
    if (!skipI32List(input) ||
        !skipI32List(input) ||
        !skipI32List(input)) {
        return false;
    }

    std::uint32_t object_count = 0;
    if (!input.readU32(object_count) ||
        !plausibleCount(
            object_count, input.remaining(), 4)) {
        return false;
    }
    for (std::uint32_t index = 0;
         index < object_count;
         ++index) {
        CommonEntity ignored;
        if (!readCommonEntity(input, ignored) ||
            !input.skip(kObjectEntityTailSize)) {
            return false;
        }
    }

    std::uint32_t people_count = 0;
    if (!input.readU32(people_count) ||
        !plausibleCount(
            people_count, input.remaining(), 4)) {
        return false;
    }
    people.reserve(people_count);
    for (std::uint32_t index = 0;
         index < people_count;
         ++index) {
        CommonEntity common;
        std::array<std::int32_t, 11> people_fields{};
        if (!readCommonEntity(input, common)) {
            return false;
        }
        for (std::int32_t& field : people_fields) {
            if (!input.readI32(field)) {
                return false;
            }
        }
        people.push_back({
            common.id,
            common.resource_id,
            std::move(common.name),
            common.name_color,
            common.label_height,
            common.world_x,
            common.world_y,
            common.judgement[0],
            common.judgement[1],
            common.judgement[2],
            common.judgement[3],
            common.direction,
            std::move(common.part_overrides),
            std::move(common.part_visibility),
            std::move(common.red_strength),
            std::move(common.green_strength),
            std::move(common.blue_strength),
            people_fields[0],
            people_fields[1],
            people_fields[2],
            people_fields[3] == 0,
            people_fields[4],
            people_fields[5],
            people_fields[6],
            people_fields[7],
            people_fields[9] == 0,
            people_fields[8] != 0,
            people_fields[10],
        });
    }
    return true;
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
    if (!readPeople(bytes, people_)) {
        setError(
            error,
            "The scenario object or people block is invalid.");
        clear();
        return false;
    }

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
    people_.clear();
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

const std::vector<ScenarioPerson>& ScenarioData::people() const {
    return people_;
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
