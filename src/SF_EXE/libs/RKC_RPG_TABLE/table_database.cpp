#include "rkc_rpg_table.hpp"

#include "libs/RK_FUNCTION/rk_function.hpp"

#include <cstring>
#include <fstream>
#include <limits>
#include <utility>

namespace osf {
namespace {

constexpr std::size_t kHeaderSize = 16;
constexpr std::size_t kMaximumTableCount = 100000;
constexpr std::size_t kMaximumCellCount = 100000000;
constexpr std::size_t kMaximumDecodedSize = 512u * 1024u * 1024u;

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
        static_cast<std::size_t>(end - cursor) < 4) {
        return false;
    }
    value =
        static_cast<std::uint32_t>(cursor[0]) |
        (static_cast<std::uint32_t>(cursor[1]) << 8u) |
        (static_cast<std::uint32_t>(cursor[2]) << 16u) |
        (static_cast<std::uint32_t>(cursor[3]) << 24u);
    cursor += 4;
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

bool extractPayload(
    const std::uint8_t* bytes,
    std::size_t size,
    std::vector<std::uint8_t>& decoded,
    const std::uint8_t*& payload,
    std::size_t& payload_size,
    std::string* error) {
    if (!bytes || size < 24 ||
        std::memcmp(bytes, "TABLE DATA V", 12) != 0) {
        setError(error, "The table database header is invalid.");
        return false;
    }

    const std::uint8_t* cursor = bytes + kHeaderSize;
    const std::uint8_t* end = bytes + size;
    std::uint32_t compressed = 0;
    if (!readU32(cursor, end, compressed)) {
        setError(error, "The table database header is truncated.");
        return false;
    }

    if (compressed == 0) {
        std::uint32_t stored_size = 0;
        if (!readU32(cursor, end, stored_size) ||
            stored_size > static_cast<std::size_t>(end - cursor)) {
            setError(error, "The table database payload is truncated.");
            return false;
        }
        payload = cursor;
        payload_size = stored_size;
        return true;
    }

    if (static_cast<std::size_t>(end - cursor) < 16 ||
        std::memcmp(cursor, "RCLIB-L", 7) != 0) {
        setError(error, "The compressed table payload is invalid.");
        return false;
    }
    const std::size_t expected_size =
        static_cast<std::size_t>(cursor[8]) |
        (static_cast<std::size_t>(cursor[9]) << 8u) |
        (static_cast<std::size_t>(cursor[10]) << 16u) |
        (static_cast<std::size_t>(cursor[11]) << 24u);
    if (expected_size > kMaximumDecodedSize) {
        setError(error, "The decoded table payload is too large.");
        return false;
    }
    if (!decodeRclibLz(
            cursor,
            static_cast<std::size_t>(end - cursor),
            expected_size,
            decoded)) {
        setError(error, "The compressed table payload could not be decoded.");
        return false;
    }
    payload = decoded.data();
    payload_size = decoded.size();
    return true;
}

}  // namespace

std::int32_t TableData::number() const {
    return number_;
}

std::int32_t TableData::rowCount() const {
    return row_count_;
}

std::int32_t TableData::columnCount() const {
    return column_count_;
}

bool TableData::contains(
    std::int32_t row,
    std::int32_t column) const {
    return row >= 0 && row < row_count_ &&
           column >= 0 && column < column_count_;
}

std::size_t TableData::cellIndex(
    std::int32_t row,
    std::int32_t column) const {
    return static_cast<std::size_t>(row) *
               static_cast<std::size_t>(column_count_) +
           static_cast<std::size_t>(column);
}

std::int32_t TableData::value(
    std::int32_t row,
    std::int32_t column) const {
    return contains(row, column)
        ? values_[cellIndex(row, column)]
        : 0;
}

std::string_view TableData::text(
    std::int32_t row,
    std::int32_t column) const {
    return contains(row, column)
        ? std::string_view(strings_[cellIndex(row, column)])
        : std::string_view{};
}

bool TableDatabase::load(
    const std::filesystem::path& path,
    std::string* error) {
    std::vector<std::uint8_t> bytes;
    if (!readFile(path, bytes)) {
        setError(error, "The table database could not be read.");
        clear();
        return false;
    }
    return decode(bytes.data(), bytes.size(), error);
}

bool TableDatabase::decode(
    const std::uint8_t* bytes,
    std::size_t size,
    std::string* error) {
    clear();

    std::vector<std::uint8_t> decoded;
    const std::uint8_t* payload = nullptr;
    std::size_t payload_size = 0;
    if (!extractPayload(
            bytes,
            size,
            decoded,
            payload,
            payload_size,
            error)) {
        return false;
    }

    const std::uint8_t* cursor = payload;
    const std::uint8_t* end = payload + payload_size;
    std::int32_t table_count = 0;
    if (!readI32(cursor, end, table_count) ||
        table_count < 0 ||
        static_cast<std::size_t>(table_count) >
            kMaximumTableCount) {
        setError(error, "The table count is invalid.");
        return false;
    }

    std::vector<TableData> parsed;
    parsed.reserve(static_cast<std::size_t>(table_count));
    for (std::int32_t table_index = 0;
         table_index < table_count;
         ++table_index) {
        TableData table;
        if (!readI32(cursor, end, table.number_) ||
            !readI32(cursor, end, table.row_count_) ||
            !readI32(cursor, end, table.column_count_) ||
            table.row_count_ < 0 ||
            table.column_count_ < 0) {
            setError(error, "A table header is invalid.");
            return false;
        }
        const std::size_t rows =
            static_cast<std::size_t>(table.row_count_);
        const std::size_t columns =
            static_cast<std::size_t>(table.column_count_);
        if (rows != 0 &&
            columns >
                std::numeric_limits<std::size_t>::max() / rows) {
            setError(error, "A table is too large.");
            return false;
        }
        const std::size_t cell_count = rows * columns;
        if (cell_count > kMaximumCellCount ||
            cell_count >
                static_cast<std::size_t>(end - cursor) / 4) {
            setError(error, "A table's numeric cells are truncated.");
            return false;
        }

        table.values_.resize(cell_count);
        table.strings_.resize(cell_count);
        for (std::int32_t& value : table.values_) {
            if (!readI32(cursor, end, value)) {
                setError(error, "A table's numeric cells are truncated.");
                return false;
            }
        }
        for (std::string& string : table.strings_) {
            std::uint32_t length = 0;
            if (!readU32(cursor, end, length) ||
                length > static_cast<std::size_t>(end - cursor)) {
                setError(error, "A table's string cells are truncated.");
                return false;
            }
            string.resize(length);
            for (std::size_t index = 0; index < length; ++index) {
                string[index] = static_cast<char>(~cursor[index]);
            }
            cursor += length;
        }
        parsed.push_back(std::move(table));
    }

    if (cursor != end) {
        setError(error, "The table database has trailing payload data.");
        return false;
    }
    tables_ = std::move(parsed);
    return true;
}

void TableDatabase::clear() {
    tables_.clear();
}

const TableData* TableDatabase::find(
    std::int32_t table_number) const {
    for (const TableData& table : tables_) {
        if (table.number() == table_number) {
            return &table;
        }
    }
    return nullptr;
}

const std::vector<TableData>& TableDatabase::tables() const {
    return tables_;
}

}  // namespace osf
