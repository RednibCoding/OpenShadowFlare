#ifndef OPENSHADOWFLARE_RETAIL_SAVE_PROGRESS_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_PROGRESS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

struct RetailSaveProgress {
    // FUN_0044b580 writes the executable's operand-type 12, 10, and 11
    // arrays in this order. Type 12 is the quest state consumed by the
    // Mission List; type 11 retains broader script progression such as
    // Ostare's completed opening conversation.
    std::vector<std::int32_t> quest_flags;
    std::vector<std::int32_t> transport_flags;
    std::vector<std::int32_t> script_state_flags;
};

bool restoreRetailProgress(
    const std::vector<std::uint8_t>& payload,
    std::size_t owned_items_end,
    RetailSaveProgress& progress,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

bool replaceRetailProgress(
    std::vector<std::uint8_t>& payload,
    std::size_t owned_items_end,
    const RetailSaveProgress& progress,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

bool restoreRetailTransportFlags(
    const std::vector<std::uint8_t>& payload,
    std::size_t owned_items_end,
    std::vector<std::int32_t>& flags,
    std::string* error = nullptr);

}  // namespace osf

#endif
