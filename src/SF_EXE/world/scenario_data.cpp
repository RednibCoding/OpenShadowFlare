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
constexpr std::uint32_t kMaximumEntryCount = 4096;
constexpr std::uint32_t kMaximumEntityCount = 4096;
constexpr std::uint32_t kMaximumEntityStringSize = 65535;
constexpr std::size_t kObjectEntityTailSize = 0x34;
constexpr std::size_t kPeopleEntityTailSize = 0x2c;
constexpr std::size_t kEnemyEntityTailSize = 0x13c;
constexpr std::size_t kItemEntityTailSize = 0x10;
static_assert(
    13 * sizeof(std::int32_t) == kObjectEntityTailSize);
static_assert(
    11 * sizeof(std::int32_t) == kPeopleEntityTailSize);

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
    std::int32_t unknown_common_value = 0;
};

struct VariableData {
    std::vector<std::int32_t> object_resource_ids;
    std::vector<std::int32_t> people_resource_ids;
    std::vector<std::int32_t> enemy_resource_ids;
    std::vector<ScenarioObject> objects;
    std::vector<ScenarioPerson> people;
    std::vector<ScenarioEntry> entries;
    std::array<std::int32_t, 3> footer_values{};
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

bool readI32List(
    Reader& input,
    std::vector<std::int32_t>& values) {
    std::uint32_t count = 0;
    return input.readU32(count) &&
           readI32Vector(input, count, values);
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
    return input.readI32(entity.unknown_common_value);
}

bool readScenarioObject(
    Reader& input,
    ScenarioObject& object) {
    CommonEntity common;
    std::array<std::int32_t, 13> fields{};
    if (!readCommonEntity(input, common)) {
        return false;
    }
    for (std::int32_t& field : fields) {
        if (!input.readI32(field)) {
            return false;
        }
    }

    object = {
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
        common.unknown_common_value,
        fields[0],
        fields[1],
        fields[2],
        fields[3] != 0,
        fields[4],
        fields[5],
        fields[6],
        fields[7],
        fields[8],
        fields[9],
        fields[10],
        fields[11],
        fields[12],
    };
    return true;
}

bool readScenarioPerson(
    Reader& input,
    ScenarioPerson& person) {
    CommonEntity common;
    std::array<std::int32_t, 11> fields{};
    if (!readCommonEntity(input, common)) {
        return false;
    }
    for (std::int32_t& field : fields) {
        if (!input.readI32(field)) {
            return false;
        }
    }

    person = {
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
        common.unknown_common_value,
        fields[0],
        fields[1],
        fields[2],
        fields[3] == 0,
        fields[4],
        fields[5],
        fields[6],
        fields[7],
        fields[9] == 0,
        fields[8] != 0,
        fields[10],
    };
    return true;
}

bool skipEntityGroup(
    Reader& input,
    std::size_t tail_size) {
    std::uint32_t count = 0;
    if (!input.readU32(count) ||
        !plausibleCount(count, input.remaining(), 4)) {
        return false;
    }
    for (std::uint32_t index = 0; index < count; ++index) {
        CommonEntity ignored;
        if (!readCommonEntity(input, ignored) ||
            !input.skip(tail_size)) {
            return false;
        }
    }
    return true;
}

bool readVariableData(
    const std::vector<std::uint8_t>& bytes,
    VariableData& data) {
    Reader input(bytes, kFixedHeaderSize);
    if (!readI32List(input, data.object_resource_ids) ||
        !readI32List(input, data.people_resource_ids) ||
        !readI32List(input, data.enemy_resource_ids)) {
        return false;
    }

    std::uint32_t object_count = 0;
    if (!input.readU32(object_count) ||
        !plausibleCount(
            object_count, input.remaining(), kObjectEntityTailSize)) {
        return false;
    }
    data.objects.reserve(object_count);
    for (std::uint32_t index = 0;
         index < object_count;
         ++index) {
        ScenarioObject object;
        if (!readScenarioObject(input, object)) {
            return false;
        }
        data.objects.push_back(std::move(object));
    }

    std::uint32_t people_count = 0;
    if (!input.readU32(people_count) ||
        !plausibleCount(
            people_count, input.remaining(), kPeopleEntityTailSize)) {
        return false;
    }
    data.people.reserve(people_count);
    for (std::uint32_t index = 0;
         index < people_count;
         ++index) {
        ScenarioPerson person;
        if (!readScenarioPerson(input, person)) {
            return false;
        }
        data.people.push_back(std::move(person));
    }

    if (!skipEntityGroup(input, kEnemyEntityTailSize) ||
        !skipEntityGroup(input, kItemEntityTailSize)) {
        return false;
    }

    std::uint32_t entry_count = 0;
    if (!input.readU32(entry_count) ||
        entry_count > kMaximumEntryCount ||
        !plausibleCount(entry_count, input.remaining(), kEntrySize)) {
        return false;
    }
    data.entries.reserve(entry_count);
    for (std::uint32_t index = 0;
         index < entry_count;
         ++index) {
        ScenarioEntry entry;
        if (!input.readI32(entry.key) ||
            !input.readI32(entry.world_x) ||
            !input.readI32(entry.world_y) ||
            !input.readI32(entry.direction) ||
            entry.direction < 0 ||
            entry.direction > 7) {
            return false;
        }
        data.entries.push_back(entry);
    }
    for (std::int32_t& value : data.footer_values) {
        if (!input.readI32(value)) {
            return false;
        }
    }
    return input.remaining() == 0;
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

    VariableData variable;
    if (!readVariableData(bytes, variable)) {
        setError(
            error,
            "The scenario entity or entry block is invalid.");
        return false;
    }

    controller_path_ = readFixedString(
        bytes, kControllerPathOffset, kFixedStringSize);
    map_path_ =
        readFixedString(bytes, kMapPathOffset, kFixedStringSize);
    title_ = readFixedString(bytes, kTitleOffset, kTitleSize);
    music_track_ = readI32(bytes, kMusicTrackOffset);
    object_resource_ids_ =
        std::move(variable.object_resource_ids);
    people_resource_ids_ =
        std::move(variable.people_resource_ids);
    enemy_resource_ids_ =
        std::move(variable.enemy_resource_ids);
    objects_ = std::move(variable.objects);
    people_ = std::move(variable.people);
    entries_ = std::move(variable.entries);
    footer_values_ = variable.footer_values;
    return true;
}

void ScenarioData::clear() {
    controller_path_.clear();
    map_path_.clear();
    title_.clear();
    music_track_ = -1;
    object_resource_ids_.clear();
    people_resource_ids_.clear();
    enemy_resource_ids_.clear();
    objects_.clear();
    people_.clear();
    entries_.clear();
    footer_values_.fill(0);
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

const std::vector<std::int32_t>&
ScenarioData::objectResourceIds() const {
    return object_resource_ids_;
}

const std::vector<std::int32_t>&
ScenarioData::peopleResourceIds() const {
    return people_resource_ids_;
}

const std::vector<std::int32_t>&
ScenarioData::enemyResourceIds() const {
    return enemy_resource_ids_;
}

const std::vector<ScenarioObject>&
ScenarioData::objects() const {
    return objects_;
}

const std::vector<ScenarioPerson>& ScenarioData::people() const {
    return people_;
}

const std::vector<ScenarioEntry>& ScenarioData::entries() const {
    return entries_;
}

const std::array<std::int32_t, 3>&
ScenarioData::footerValues() const {
    return footer_values_;
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
