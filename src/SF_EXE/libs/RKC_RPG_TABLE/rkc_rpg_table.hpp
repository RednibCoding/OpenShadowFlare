#ifndef OPENSHADOWFLARE_LIBS_RKC_RPG_TABLE_HPP
#define OPENSHADOWFLARE_LIBS_RKC_RPG_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace osf {

class TableData {
public:
    std::int32_t number() const;
    std::int32_t rowCount() const;
    std::int32_t columnCount() const;
    bool contains(std::int32_t row, std::int32_t column) const;
    std::int32_t value(
        std::int32_t row,
        std::int32_t column) const;
    std::string_view text(
        std::int32_t row,
        std::int32_t column) const;

private:
    friend class TableDatabase;

    std::size_t cellIndex(
        std::int32_t row,
        std::int32_t column) const;

    std::int32_t number_ = 0;
    std::int32_t row_count_ = 0;
    std::int32_t column_count_ = 0;
    std::vector<std::int32_t> values_;
    std::vector<std::string> strings_;
};

class TableDatabase {
public:
    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);
    bool decode(
        const std::uint8_t* bytes,
        std::size_t size,
        std::string* error = nullptr);
    void clear();

    const TableData* find(std::int32_t table_number) const;
    const std::vector<TableData>& tables() const;

private:
    std::vector<TableData> tables_;
};

}  // namespace osf

#endif
