#include "player_giant_warehouse.hpp"

#include <algorithm>

namespace osf {

void PlayerGiantWarehouse::initializeNew() {
    for (PlayerSpecialItems& storage : pages_) {
        storage.clear();
    }
    enabled_flags_.fill(0);
    enabled_flags_[0] = 1;
    selected_page_ = 0;
}

void PlayerGiantWarehouse::restoreEnabledFlags(
    const EnabledFlags& flags) {
    enabled_flags_ = flags;
    const auto first_enabled = std::find_if(
        enabled_flags_.begin(),
        enabled_flags_.end(),
        [](std::int32_t value) { return value != 0; });
    selected_page_ = first_enabled == enabled_flags_.end()
        ? 0
        : static_cast<std::size_t>(
              first_enabled - enabled_flags_.begin());
}

bool PlayerGiantWarehouse::pageEnabled(
    std::size_t page_index) const {
    return page_index < page_count &&
           enabled_flags_[page_index] != 0;
}

const PlayerGiantWarehouse::EnabledFlags&
PlayerGiantWarehouse::enabledFlags() const {
    return enabled_flags_;
}

std::size_t PlayerGiantWarehouse::selectedPage() const {
    return selected_page_;
}

bool PlayerGiantWarehouse::selectPage(
    std::size_t page_index) {
    if (!pageEnabled(page_index)) {
        return false;
    }
    selected_page_ = page_index;
    return true;
}

PlayerSpecialItems& PlayerGiantWarehouse::page(
    std::size_t page_index) {
    return pages_.at(page_index);
}

const PlayerSpecialItems& PlayerGiantWarehouse::page(
    std::size_t page_index) const {
    return pages_.at(page_index);
}

}  // namespace osf
