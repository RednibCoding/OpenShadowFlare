#include "retail_save_file.hpp"

#include "player_data.hpp"
#include "player_magic.hpp"
#include "retail_save_items.hpp"
#include "retail_save_magic.hpp"
#include "retail_save_progress.hpp"
#include "items/player_special_items.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace osf {
namespace {

constexpr std::array<char, 16> kSaveSignature{{
    'S', 'h', 'a', 'd', 'o', 'w', 'F', 'l',
    'a', 'r', 'e', '0', '0', '0', '5', '\0',
}};

constexpr char kSaveSubstitutionHex[] =
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
    sizeof(kSaveSubstitutionHex) == 256u * 2u + 1u,
    "Retail save substitution table size");

constexpr std::size_t kPlainHeaderSize =
    kSaveSignature.size() + PlayerData::retail_record_size;
constexpr std::size_t kEnvelopeHeaderSize = 9;
constexpr std::size_t kMaximumPayloadSize =
    64u * 1024u * 1024u;

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

std::uint8_t hexNibble(char value) {
    return static_cast<std::uint8_t>(
        value >= 'a' ? value - 'a' + 10 : value - '0');
}

std::array<std::uint8_t, 256> saveSubstitution() {
    std::array<std::uint8_t, 256> table{};
    for (std::size_t index = 0; index < table.size(); ++index) {
        const std::size_t offset = index * 2u;
        table[index] = static_cast<std::uint8_t>(
            (hexNibble(kSaveSubstitutionHex[offset]) << 4u) |
            hexNibble(kSaveSubstitutionHex[offset + 1u]));
    }
    return table;
}

std::uint32_t readU32(const std::uint8_t* bytes) {
    return
        static_cast<std::uint32_t>(bytes[0]) |
        (static_cast<std::uint32_t>(bytes[1]) << 8u) |
        (static_cast<std::uint32_t>(bytes[2]) << 16u) |
        (static_cast<std::uint32_t>(bytes[3]) << 24u);
}

void appendU32(
    std::vector<std::uint8_t>& bytes,
    std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8u));
    bytes.push_back(static_cast<std::uint8_t>(value >> 16u));
    bytes.push_back(static_cast<std::uint8_t>(value >> 24u));
}

std::uint32_t signedByteChecksum(
    const std::vector<std::uint8_t>& payload) {
    std::uint32_t checksum = 0;
    for (std::uint8_t value : payload) {
        checksum += static_cast<std::uint32_t>(
            static_cast<std::int32_t>(
                static_cast<std::int8_t>(value)));
    }
    return checksum;
}

bool decodeRetailSavePayload(
    const std::filesystem::path& path,
    std::vector<std::uint8_t>& payload,
    std::string* error) {
    std::ifstream stream(path, std::ios::binary);
    const std::vector<std::uint8_t> file{
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>()};
    if (!stream.eof() && stream.fail()) {
        setError(error, "The existing save could not be read.");
        return false;
    }
    if (file.size() < kPlainHeaderSize ||
        !std::equal(
            kSaveSignature.begin(),
            kSaveSignature.end(),
            file.begin())) {
        setError(
            error,
            "The existing save has an invalid header; it was not replaced.");
        return false;
    }

    if (file.size() == kPlainHeaderSize) {
        payload.assign(
            file.begin() +
                static_cast<std::ptrdiff_t>(kSaveSignature.size()),
            file.end());
        return true;
    }
    if (file.size() < kPlainHeaderSize + kEnvelopeHeaderSize) {
        setError(
            error,
            "The existing save envelope is truncated; it was not replaced.");
        return false;
    }

    const std::uint8_t* envelope =
        file.data() + kPlainHeaderSize;
    const std::uint32_t payload_size = readU32(envelope);
    if (payload_size > kMaximumPayloadSize ||
        file.size() !=
            kPlainHeaderSize + kEnvelopeHeaderSize +
                static_cast<std::size_t>(payload_size)) {
        setError(
            error,
            "The existing save payload size is invalid; it was not replaced.");
        return false;
    }

    const std::uint8_t xor_key = envelope[4];
    const std::uint32_t stored_checksum =
        readU32(envelope + 5);
    const auto substitution = saveSubstitution();
    payload.resize(payload_size);
    for (std::size_t index = 0; index < payload.size(); ++index) {
        payload[index] = static_cast<std::uint8_t>(
            substitution[envelope[kEnvelopeHeaderSize + index]] ^
            xor_key);
    }
    if (signedByteChecksum(payload) != stored_checksum) {
        setError(
            error,
            "The existing save checksum is invalid; it was not replaced.");
        payload.clear();
        return false;
    }
    return true;
}

std::filesystem::path unusedSiblingPath(
    const std::filesystem::path& target,
    std::string_view suffix) {
    for (std::int32_t index = 0; index < 100; ++index) {
        std::filesystem::path candidate = target;
        candidate += suffix;
        candidate += std::to_string(index);
        std::error_code error;
        if (!std::filesystem::exists(candidate, error)) {
            return candidate;
        }
    }
    return {};
}

bool replaceFile(
    const std::filesystem::path& temporary,
    const std::filesystem::path& target,
    std::string* error) {
    std::error_code filesystem_error;
    if (!std::filesystem::exists(target, filesystem_error)) {
        std::filesystem::rename(
            temporary, target, filesystem_error);
        if (!filesystem_error) {
            return true;
        }
        setError(error, "The completed save could not be installed.");
        return false;
    }

    const std::filesystem::path backup =
        unusedSiblingPath(target, ".backup.");
    if (backup.empty()) {
        setError(error, "A temporary save backup could not be reserved.");
        return false;
    }
    std::filesystem::rename(target, backup, filesystem_error);
    if (filesystem_error) {
        setError(error, "The existing save could not be protected.");
        return false;
    }
    std::filesystem::rename(temporary, target, filesystem_error);
    if (filesystem_error) {
        std::error_code restore_error;
        std::filesystem::rename(backup, target, restore_error);
        setError(error, "The completed save could not be installed.");
        return false;
    }
    std::filesystem::remove(backup, filesystem_error);
    return true;
}

}  // namespace

bool readRetailSavePayload(
    const std::filesystem::path& path,
    std::vector<std::uint8_t>& payload,
    std::string* error) {
    payload.clear();
    return decodeRetailSavePayload(path, payload, error);
}

namespace {

bool writeRetailSaveImpl(
    const std::filesystem::path& path,
    const PlayerData& player,
    const ItemDatabase* item_database,
    const PlayerInventory* inventory,
    const PlayerEquipment* equipment,
    const PlayerBelt* belt,
    const PlayerSpecialItems* special_items,
    const RetailSaveProgress* progress,
    const PlayerMagic* magic,
    std::uint8_t xor_key,
    std::string* error) {
    if (!player.valid()) {
        setError(error, "There is no valid player record to save.");
        return false;
    }
    if (path.empty() || path.filename().empty()) {
        setError(error, "The save path is empty.");
        return false;
    }

    std::error_code filesystem_error;
    const std::filesystem::file_status path_status =
        std::filesystem::status(path, filesystem_error);
    const bool path_missing =
        filesystem_error ==
            std::errc::no_such_file_or_directory ||
        path_status.type() ==
            std::filesystem::file_type::not_found;
    if (path_missing) {
        filesystem_error.clear();
    } else if (filesystem_error) {
        setError(error, "The save path could not be inspected.");
        return false;
    }
    const bool exists = !path_missing;
    if (exists &&
        !std::filesystem::is_regular_file(path_status)) {
        setError(error, "The save path is not a regular file.");
        return false;
    }

    std::vector<std::uint8_t> payload;
    if (exists &&
        !decodeRetailSavePayload(path, payload, error)) {
        return false;
    }
    const auto& record = player.retailRecord();
    if (payload.size() < record.size()) {
        payload.resize(record.size());
    }
    std::copy(record.begin(), record.end(), payload.begin());
    std::size_t owned_items_end = 0;
    if (item_database &&
        (!inventory || !equipment || !belt ||
         !special_items ||
         !replaceRetailOwnedItems(
             payload,
             *item_database,
             *inventory,
             *equipment,
             *belt,
             *special_items,
             &owned_items_end,
             error))) {
        return false;
    }
    std::size_t progress_end = owned_items_end;
    if (progress &&
        (!item_database ||
         !replaceRetailProgress(
             payload,
             owned_items_end,
             *progress,
             &progress_end,
             error))) {
        return false;
    }
    if (magic &&
        (!progress ||
         !replaceRetailMagic(
             payload,
             progress_end,
             *magic,
             nullptr,
             error))) {
        return false;
    }
    if (payload.size() >
        std::numeric_limits<std::uint32_t>::max()) {
        setError(error, "The save payload is too large.");
        return false;
    }

    const auto substitution = saveSubstitution();
    std::array<std::uint8_t, 256> inverse{};
    for (std::size_t index = 0; index < substitution.size(); ++index) {
        inverse[substitution[index]] =
            static_cast<std::uint8_t>(index);
    }

    std::vector<std::uint8_t> file;
    file.reserve(
        kPlainHeaderSize + kEnvelopeHeaderSize + payload.size());
    file.insert(
        file.end(), kSaveSignature.begin(), kSaveSignature.end());
    file.insert(file.end(), record.begin(), record.end());
    appendU32(
        file, static_cast<std::uint32_t>(payload.size()));
    file.push_back(xor_key);
    appendU32(file, signedByteChecksum(payload));
    for (std::uint8_t value : payload) {
        file.push_back(inverse[value ^ xor_key]);
    }

    const std::filesystem::path parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(
            parent, filesystem_error);
        if (filesystem_error) {
            setError(error, "The save directory could not be created.");
            return false;
        }
    }
    const std::filesystem::path temporary =
        unusedSiblingPath(path, ".temporary.");
    if (temporary.empty()) {
        setError(error, "A temporary save path could not be reserved.");
        return false;
    }
    {
        std::ofstream stream(
            temporary, std::ios::binary | std::ios::trunc);
        if (!stream ||
            !stream.write(
                reinterpret_cast<const char*>(file.data()),
                static_cast<std::streamsize>(file.size())) ||
            !stream.flush()) {
            setError(error, "The temporary save could not be written.");
            std::filesystem::remove(temporary, filesystem_error);
            return false;
        }
    }
    if (!replaceFile(temporary, path, error)) {
        std::filesystem::remove(temporary, filesystem_error);
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace

bool writeRetailSave(
    const std::filesystem::path& path,
    const PlayerData& player,
    std::uint8_t xor_key,
    std::string* error) {
    return writeRetailSaveImpl(
        path,
        player,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        xor_key,
        error);
}

bool writeRetailSave(
    const std::filesystem::path& path,
    const PlayerData& player,
    const ItemDatabase& item_database,
    const PlayerInventory& inventory,
    const PlayerEquipment& equipment,
    const PlayerBelt& belt,
    const PlayerSpecialItems& special_items,
    std::uint8_t xor_key,
    std::string* error) {
    return writeRetailSaveImpl(
        path,
        player,
        &item_database,
        &inventory,
        &equipment,
        &belt,
        &special_items,
        nullptr,
        nullptr,
        xor_key,
        error);
}

bool writeRetailSave(
    const std::filesystem::path& path,
    const PlayerData& player,
    const ItemDatabase& item_database,
    const PlayerInventory& inventory,
    const PlayerEquipment& equipment,
    const PlayerBelt& belt,
    const PlayerSpecialItems& special_items,
    const RetailSaveProgress& progress,
    std::uint8_t xor_key,
    std::string* error) {
    return writeRetailSaveImpl(
        path,
        player,
        &item_database,
        &inventory,
        &equipment,
        &belt,
        &special_items,
        &progress,
        nullptr,
        xor_key,
        error);
}

bool writeRetailSave(
    const std::filesystem::path& path,
    const PlayerData& player,
    const ItemDatabase& item_database,
    const PlayerInventory& inventory,
    const PlayerEquipment& equipment,
    const PlayerBelt& belt,
    const PlayerSpecialItems& special_items,
    const RetailSaveProgress& progress,
    const PlayerMagic& magic,
    std::uint8_t xor_key,
    std::string* error) {
    return writeRetailSaveImpl(
        path,
        player,
        &item_database,
        &inventory,
        &equipment,
        &belt,
        &special_items,
        &progress,
        &magic,
        xor_key,
        error);
}

}  // namespace osf
