#ifndef OPENSHADOWFLARE_RETAIL_SAVE_WORLD_STATE_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_WORLD_STATE_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

struct RetailSaveWorldState {
    bool running = false;
    std::int32_t scenario_id = 0;
    std::int32_t entry_value = 0;
};

bool restoreRetailWorldState(
    const std::vector<std::uint8_t>& payload,
    std::size_t mine_end,
    RetailSaveWorldState& state,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

bool replaceRetailWorldState(
    std::vector<std::uint8_t>& payload,
    std::size_t mine_end,
    const RetailSaveWorldState& state,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

}  // namespace osf

#endif
