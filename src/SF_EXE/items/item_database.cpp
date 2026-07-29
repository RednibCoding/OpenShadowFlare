#include "item_database.hpp"

#include "libs/RK_FUNCTION/rk_function.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <numeric>
#include <utility>

namespace osf {
namespace {

constexpr char kMagic[] = "SFItemDataV0000\x1a";
constexpr std::size_t kMaximumDecodedSize = 64u * 1024u * 1024u;
constexpr std::array<std::size_t, ItemDatabase::category_count>
    kRecordSizes = {804, 764, 672, 140, 100};

// FUN_00462f80 uses this complete substitution table. Each encrypted byte is
// an index into the table; it is not the repeating XOR used by save files.
constexpr char kSubstitutionHex[] =
    "be66b32f016e6dc81f98a546765c3d0e"
    "aa5e9dffeaa00d4b75f661855dbbdcfb"
    "8bc34f450490811e6bc9d373c6e724ba"
    "32f3c0ec57ccc4b6c1aeaf88f284ce4a"
    "fc3c9f1a56c5e2f547d9d78ccd97f07b"
    "3106e514e6da4826ac879ad8a6eb92cf0"
    "f9441b4742ad1701cd4b0c20908169bfd"
    "771d219e3635533ed0d562585f637cb58"
    "d2bd289b799a1306554409671febff4a9"
    "5bf722605a6ffa1b79e917b1009c7e522"
    "9122c78059155e3a2b9f8509513807f1"
    "127cb374e5115efa7724d8349a469de20"
    "a367df1042396c2dc723e4ddedd6f959"
    "b2ad6a7dbceee03a3fca4c2568931833"
    "280b07038202438a86db383419642e7a"
    "abf1e8440cb88fa80a8ebde13b";

static_assert(
    sizeof(kSubstitutionHex) == 256u * 2u + 1u,
    "Item.Ibn substitution table size");

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

std::uint8_t hexNibble(char value) {
    return static_cast<std::uint8_t>(
        value >= 'a'
            ? value - 'a' + 10
            : value - '0');
}

std::uint8_t substitution(std::uint8_t value) {
    const std::size_t offset =
        static_cast<std::size_t>(value) * 2u;
    return static_cast<std::uint8_t>(
        (hexNibble(kSubstitutionHex[offset]) << 4u) |
        hexNibble(kSubstitutionHex[offset + 1u]));
}

std::uint32_t readU32(
    const std::uint8_t* bytes) {
    return
        static_cast<std::uint32_t>(bytes[0]) |
        (static_cast<std::uint32_t>(bytes[1]) << 8u) |
        (static_cast<std::uint32_t>(bytes[2]) << 16u) |
        (static_cast<std::uint32_t>(bytes[3]) << 24u);
}

std::int32_t readI32(
    const std::uint8_t* bytes) {
    return static_cast<std::int32_t>(readU32(bytes));
}

class Reader {
public:
    explicit Reader(const std::vector<std::uint8_t>& bytes)
        : bytes_(bytes) {}

    bool readI32(std::int32_t& value) {
        if (remaining() < 4) {
            return false;
        }
        value = osf::readI32(bytes_.data() + position_);
        position_ += 4;
        return true;
    }

    bool readInvertedString(std::string& value) {
        std::int32_t length = 0;
        if (!readI32(length) || length < 0 ||
            static_cast<std::size_t>(length) > remaining()) {
            return false;
        }
        value.resize(static_cast<std::size_t>(length));
        for (std::size_t index = 0;
             index < value.size();
             ++index) {
            value[index] = static_cast<char>(
                ~bytes_[position_ + index]);
        }
        position_ += value.size();
        return true;
    }

    bool readBytes(
        std::size_t count,
        std::vector<std::uint8_t>& value) {
        if (count > remaining()) {
            return false;
        }
        value.assign(
            bytes_.begin() +
                static_cast<std::ptrdiff_t>(position_),
            bytes_.begin() +
                static_cast<std::ptrdiff_t>(position_ + count));
        position_ += count;
        return true;
    }

    std::size_t remaining() const {
        return bytes_.size() - position_;
    }

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t position_ = 0;
};

bool decodePayload(
    const std::vector<std::uint8_t>& file,
    std::vector<std::uint8_t>& payload,
    std::int32_t& stored_checksum,
    std::string* error) {
    if (file.size() < 28 ||
        std::memcmp(file.data(), kMagic, sizeof(kMagic) - 1u) != 0) {
        setError(error, "The item database signature is invalid.");
        return false;
    }
    stored_checksum = readI32(file.data() + 16);
    const std::int32_t compressed =
        readI32(file.data() + 20);
    if (compressed == 0) {
        const std::int32_t size = readI32(file.data() + 24);
        if (size < 0 ||
            static_cast<std::size_t>(size) >
                kMaximumDecodedSize ||
            file.size() != 28u + static_cast<std::size_t>(size)) {
            setError(error, "The uncompressed item payload is invalid.");
            return false;
        }
        payload.assign(file.begin() + 28, file.end());
        return true;
    }
    if (compressed != 1 || file.size() < 40 ||
        std::memcmp(file.data() + 24, "RCLIB-L", 7) != 0) {
        setError(error, "The compressed item payload header is invalid.");
        return false;
    }

    const std::uint32_t decoded_size =
        readU32(file.data() + 32);
    const std::uint32_t encoded_size =
        readU32(file.data() + 36);
    if (decoded_size > kMaximumDecodedSize ||
        file.size() != 40u + encoded_size) {
        setError(error, "The compressed item payload size is invalid.");
        return false;
    }
    std::vector<std::uint8_t> block(
        file.begin() + 24, file.end());
    for (std::size_t index = 16;
         index < block.size();
         ++index) {
        block[index] = substitution(block[index]);
    }
    if (!decodeRclibLz(
            block.data(),
            block.size(),
            decoded_size,
            payload)) {
        setError(error, "The item database RCLIB-L payload is invalid.");
        return false;
    }
    return true;
}

}  // namespace

bool ItemDatabase::load(
    const std::filesystem::path& path,
    std::string* error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        setError(error, "The item database could not be opened.");
        return false;
    }
    const std::vector<std::uint8_t> bytes{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
    return decode(bytes, error);
}

bool ItemDatabase::decode(
    const std::vector<std::uint8_t>& bytes,
    std::string* error) {
    clear();
    std::vector<std::uint8_t> payload;
    std::int32_t stored_checksum = 0;
    if (!decodePayload(
            bytes, payload, stored_checksum, error)) {
        return false;
    }

    std::int64_t checksum = 0;
    for (std::uint8_t value : payload) {
        checksum += static_cast<std::int8_t>(value);
    }
    if (checksum != stored_checksum) {
        setError(error, "The item database checksum does not match.");
        return false;
    }

    Reader input(payload);
    for (std::size_t category = 0;
         category < definitions_.size();
         ++category) {
        std::int32_t count = 0;
        if (!input.readI32(count) || count < 0 ||
            static_cast<std::size_t>(count) >
                input.remaining() / kRecordSizes[category]) {
            setError(error, "An item category count is invalid.");
            clear();
            return false;
        }
        auto& definitions = definitions_[category];
        definitions.reserve(static_cast<std::size_t>(count));
        for (std::int32_t index = 0; index < count; ++index) {
            ItemDefinition definition;
            definition.category =
                static_cast<std::int32_t>(category);
            if (!input.readInvertedString(definition.name) ||
                !input.readInvertedString(definition.description) ||
                !input.readBytes(
                    kRecordSizes[category],
                    definition.raw_fields)) {
                setError(error, "An item definition is truncated.");
                clear();
                return false;
            }
            const auto field = [&definition](std::size_t offset) {
                return readI32(
                    definition.raw_fields.data() + offset);
            };
            definition.id = field(4);
            definition.subtype = field(8);
            definition.inventory_pattern_group = field(40);
            definition.inventory_pattern = field(44);
            definition.ground_resource_id = field(48);
            definition.ground_animation_chart = field(52);
            definition.inventory_shadow_pattern = field(56);
            definition.ground_red_strength = field(60);
            definition.ground_green_strength = field(64);
            definition.ground_blue_strength = field(68);
            definitions.push_back(std::move(definition));
        }
    }
    if (input.remaining() != 0) {
        setError(error, "The item database has unexpected trailing data.");
        clear();
        return false;
    }
    return true;
}

void ItemDatabase::clear() {
    for (auto& definitions : definitions_) {
        definitions.clear();
    }
}

const ItemDefinition* ItemDatabase::find(
    std::int32_t category,
    std::int32_t id) const {
    if (category < 0 ||
        static_cast<std::size_t>(category) >=
            definitions_.size()) {
        return nullptr;
    }
    const auto& definitions =
        definitions_[static_cast<std::size_t>(category)];
    const auto found = std::find_if(
        definitions.begin(),
        definitions.end(),
        [id](const ItemDefinition& definition) {
            return definition.id == id;
        });
    return found == definitions.end() ? nullptr : &*found;
}

const std::vector<ItemDefinition>& ItemDatabase::definitions(
    std::size_t category) const {
    static const std::vector<ItemDefinition> empty;
    return category < definitions_.size()
        ? definitions_[category]
        : empty;
}

std::size_t ItemDatabase::definitionCount() const {
    return std::accumulate(
        definitions_.begin(),
        definitions_.end(),
        std::size_t{0},
        [](std::size_t count, const auto& definitions) {
            return count + definitions.size();
        });
}

}  // namespace osf
