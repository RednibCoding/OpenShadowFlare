#ifndef OPENSHADOWFLARE_RETAIL_SAVE_PROGRESS_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_PROGRESS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

struct RetailSaveProgress {
    // These are the executable's operand-type 12, 10, and 11 arrays, in the
    // same order in which FUN_0044b580 writes them.
    std::vector<std::int32_t> scenario_flags;
    std::vector<std::int32_t> transport_flags;
    std::vector<std::int32_t> quest_flags;
    bool running = false;
};

bool restoreRetailProgress(
    const std::vector<std::uint8_t>& payload,
    std::size_t owned_items_end,
    RetailSaveProgress& progress,
    std::string* error = nullptr);

bool replaceRetailProgress(
    std::vector<std::uint8_t>& payload,
    std::size_t owned_items_end,
    const RetailSaveProgress& progress,
    std::string* error = nullptr);

bool restoreRetailTransportFlags(
    const std::vector<std::uint8_t>& payload,
    std::size_t owned_items_end,
    std::vector<std::int32_t>& flags,
    std::string* error = nullptr);

}  // namespace osf

#endif
