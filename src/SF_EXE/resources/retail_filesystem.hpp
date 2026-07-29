#ifndef OPENSHADOWFLARE_RETAIL_FILESYSTEM_HPP
#define OPENSHADOWFLARE_RETAIL_FILESYSTEM_HPP

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace osf {

std::filesystem::path resolveRetailPath(
    const std::filesystem::path& root,
    std::string_view retail_path);
bool retailFileExists(
    const std::filesystem::path& root,
    std::string_view retail_path);
std::int32_t countRetailSaves(
    const std::filesystem::path& root);
bool deleteRetailSave(
    const std::filesystem::path& root,
    std::int32_t logical_index);

}  // namespace osf

#endif
