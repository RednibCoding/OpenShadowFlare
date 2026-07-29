#ifndef OPENSHADOWFLARE_SAVE_CATALOG_HPP
#define OPENSHADOWFLARE_SAVE_CATALOG_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf {

struct RetailSaveSummary {
    std::int32_t slot = -1;
    std::filesystem::path save_path;
    std::filesystem::path preview_path;
    std::string name;
    std::int32_t gender = 0;
    std::int32_t job = 0;
    std::int32_t level = 0;
};

std::vector<RetailSaveSummary> loadRetailSaveCatalog(
    const std::filesystem::path& game_root);

}  // namespace osf

#endif
