#include "rkc_rpg_aicontrol.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>

namespace osf {
namespace {

constexpr std::size_t kHeaderSize = 16;
constexpr std::size_t kMaximumListCount = 100000;
constexpr std::size_t kMaximumActionCount = 1000000;
constexpr std::size_t kMaximumNameLength = 1024 * 1024;
constexpr std::size_t kStoredActionSize =
    sizeof(std::int32_t) *
    (1 + kAiParameterValueCount +
     kAiConditionValueCount);

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

bool readU32(
    const std::uint8_t*& cursor,
    const std::uint8_t* end,
    std::uint32_t& value) {
    if (cursor > end ||
        static_cast<std::size_t>(end - cursor) <
            sizeof(std::uint32_t)) {
        return false;
    }
    value =
        static_cast<std::uint32_t>(cursor[0]) |
        (static_cast<std::uint32_t>(cursor[1]) << 8u) |
        (static_cast<std::uint32_t>(cursor[2]) << 16u) |
        (static_cast<std::uint32_t>(cursor[3]) << 24u);
    cursor += sizeof(std::uint32_t);
    return true;
}

bool readI32(
    const std::uint8_t*& cursor,
    const std::uint8_t* end,
    std::int32_t& value) {
    std::uint32_t unsigned_value = 0;
    if (!readU32(cursor, end, unsigned_value)) {
        return false;
    }
    value = static_cast<std::int32_t>(unsigned_value);
    return true;
}

bool readFile(
    const std::filesystem::path& path,
    std::vector<std::uint8_t>& bytes) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        return false;
    }
    const std::streamoff length = stream.tellg();
    if (length < 0 ||
        static_cast<std::uintmax_t>(length) >
            std::numeric_limits<std::size_t>::max()) {
        return false;
    }
    bytes.resize(static_cast<std::size_t>(length));
    stream.seekg(0, std::ios::beg);
    return bytes.empty() ||
           static_cast<bool>(stream.read(
               reinterpret_cast<char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size())));
}

bool decodeVersion(
    const std::uint8_t* bytes,
    std::int32_t& version) {
    if (std::memcmp(bytes, "RKC_AIDATA v", 12) != 0 ||
        bytes[12] < '0' || bytes[12] > '9' ||
        bytes[13] < '0' || bytes[13] > '9' ||
        bytes[14] < '0' || bytes[14] > '9' ||
        bytes[15] != 0x1a) {
        return false;
    }
    version =
        static_cast<std::int32_t>(bytes[12] - '0') * 100 +
        static_cast<std::int32_t>(bytes[13] - '0') * 10 +
        static_cast<std::int32_t>(bytes[14] - '0');
    return true;
}

}  // namespace

const std::vector<AiActionData>&
AiEventData::actions() const {
    return actions_;
}

const std::string& AiControlList::name() const {
    return name_;
}

std::int32_t AiControlList::walkPointSpeed() const {
    return walk_point_speed_;
}

const AiEventData* AiControlList::event(
    std::int32_t event_number) const {
    return event_number >= 0 &&
                   static_cast<std::size_t>(event_number) <
                       events_.size()
               ? &events_[
                     static_cast<std::size_t>(event_number)]
               : nullptr;
}

const std::array<
    AiEventData,
    kAiControlEventCount>&
AiControlList::events() const {
    return events_;
}

std::size_t AiControlList::actionCount() const {
    std::size_t count = 0;
    for (const AiEventData& event : events_) {
        count += event.actions().size();
    }
    return count;
}

bool AiControlDatabase::load(
    const std::filesystem::path& path,
    std::string* error) {
    std::vector<std::uint8_t> bytes;
    if (!readFile(path, bytes)) {
        setError(error, "The AI-control database could not be read.");
        clear();
        return false;
    }
    return decode(bytes.data(), bytes.size(), error);
}

bool AiControlDatabase::decode(
    const std::uint8_t* bytes,
    std::size_t size,
    std::string* error) {
    clear();
    std::int32_t parsed_version = 0;
    if (!bytes || size < kHeaderSize + 8 ||
        !decodeVersion(bytes, parsed_version)) {
        setError(error, "The AI-control database header is invalid.");
        return false;
    }

    const std::uint8_t* cursor = bytes + kHeaderSize;
    const std::uint8_t* end = bytes + size;
    std::int32_t list_count = 0;
    std::int32_t event_count = 0;
    if (!readI32(cursor, end, list_count) ||
        !readI32(cursor, end, event_count) ||
        list_count < 0 ||
        static_cast<std::size_t>(list_count) >
            kMaximumListCount ||
        event_count !=
            static_cast<std::int32_t>(
                kAiControlEventCount)) {
        setError(error, "The AI-control catalog dimensions are invalid.");
        return false;
    }

    std::vector<AiControlList> parsed;
    parsed.reserve(static_cast<std::size_t>(list_count));
    std::size_t total_action_count = 0;
    for (std::int32_t list_index = 0;
         list_index < list_count;
         ++list_index) {
        std::int32_t name_length = 0;
        if (!readI32(cursor, end, name_length) ||
            name_length < 0 ||
            static_cast<std::size_t>(name_length) >
                kMaximumNameLength ||
            static_cast<std::size_t>(name_length) >
                static_cast<std::size_t>(end - cursor)) {
            setError(error, "An AI-control list name is invalid.");
            return false;
        }

        AiControlList list;
        list.name_.assign(
            reinterpret_cast<const char*>(cursor),
            static_cast<std::size_t>(name_length));
        cursor += name_length;
        if (parsed_version > 0 &&
            !readI32(
                cursor, end, list.walk_point_speed_)) {
            setError(
                error,
                "An AI-control walk-point speed is truncated.");
            return false;
        }

        for (std::size_t event_index = 0;
             event_index < kAiControlEventCount;
             ++event_index) {
            std::int32_t action_count = 0;
            if (!readI32(cursor, end, action_count) ||
                action_count < 0 ||
                static_cast<std::size_t>(action_count) >
                    kMaximumActionCount -
                        total_action_count ||
                static_cast<std::size_t>(action_count) >
                    static_cast<std::size_t>(end - cursor) /
                        kStoredActionSize) {
                setError(
                    error,
                    "An AI-control event action count is invalid.");
                return false;
            }
            total_action_count +=
                static_cast<std::size_t>(action_count);
            AiEventData& event = list.events_[event_index];
            event.actions_.reserve(
                static_cast<std::size_t>(action_count));
            for (std::int32_t action_index = 0;
                 action_index < action_count;
                 ++action_index) {
                AiActionData action;
                action.event_number =
                    static_cast<std::int32_t>(event_index);
                if (!readI32(
                        cursor,
                        end,
                        action.action_number)) {
                    setError(
                        error,
                        "An AI-control action is truncated.");
                    return false;
                }
                for (std::int32_t& value :
                     action.parameters) {
                    if (!readI32(cursor, end, value)) {
                        setError(
                            error,
                            "An AI-control parameter block is "
                            "truncated.");
                        return false;
                    }
                }
                for (std::int32_t& value :
                     action.conditions) {
                    if (!readI32(cursor, end, value)) {
                        setError(
                            error,
                            "An AI-control condition block is "
                            "truncated.");
                        return false;
                    }
                }
                event.actions_.push_back(
                    std::move(action));
            }
        }
        parsed.push_back(std::move(list));
    }

    if (cursor != end) {
        setError(
            error,
            "The AI-control database has trailing data.");
        return false;
    }

    version_ = parsed_version;
    lists_ = std::move(parsed);
    if (error) {
        error->clear();
    }
    return true;
}

void AiControlDatabase::clear() {
    version_ = 0;
    lists_.clear();
}

std::int32_t AiControlDatabase::version() const {
    return version_;
}

const AiControlList* AiControlDatabase::list(
    std::int32_t index) const {
    return index >= 0 &&
                   static_cast<std::size_t>(index) <
                       lists_.size()
               ? &lists_[static_cast<std::size_t>(index)]
               : nullptr;
}

const AiControlList* AiControlDatabase::find(
    std::string_view name) const {
    const auto found = std::find_if(
        lists_.begin(),
        lists_.end(),
        [name](const AiControlList& list) {
            return list.name() == name;
        });
    return found == lists_.end() ? nullptr : &*found;
}

std::int32_t AiControlDatabase::indexOf(
    const AiControlList* list_value) const {
    for (std::size_t index = 0;
         index < lists_.size();
         ++index) {
        if (&lists_[index] == list_value) {
            return static_cast<std::int32_t>(index);
        }
    }
    return -1;
}

const std::vector<AiControlList>&
AiControlDatabase::lists() const {
    return lists_;
}

}  // namespace osf
