#include "transport_catalog.hpp"

#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <utility>

namespace osf {
namespace {

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

}  // namespace

bool TransportCatalog::load(
    const TableDatabase& tables,
    std::string* error) {
    clear();
    const TableData* table = tables.find(40);
    if (!table || table->columnCount() < 3 ||
        table->rowCount() <= 0) {
        setError(
            error,
            "The retail transport table is missing.");
        return false;
    }
    destinations_.reserve(
        static_cast<std::size_t>(table->rowCount()));
    for (std::int32_t row = 0;
         row < table->rowCount();
         ++row) {
        destinations_.push_back({
            row,
            std::string(table->text(row, 0)),
            table->value(row, 1),
            table->value(row, 2),
        });
    }
    enabled_flags_.assign(destinations_.size(), 0);
    // FUN_00440f70 enables Remote Town for every new player.
    enabled_flags_.front() = 1;
    if (error) {
        error->clear();
    }
    return true;
}

void TransportCatalog::clear() {
    destinations_.clear();
    enabled_flags_.clear();
}

const std::vector<TransportDestination>&
TransportCatalog::destinations() const {
    return destinations_;
}

const TransportDestination* TransportCatalog::find(
    std::int32_t row) const {
    return row >= 0 &&
                   static_cast<std::size_t>(row) <
                       destinations_.size()
               ? &destinations_[static_cast<std::size_t>(row)]
               : nullptr;
}

bool TransportCatalog::enabled(std::int32_t row) const {
    return row >= 0 &&
           static_cast<std::size_t>(row) <
               enabled_flags_.size() &&
           enabled_flags_[static_cast<std::size_t>(row)] != 0;
}

std::vector<std::int32_t>
TransportCatalog::enabledRows() const {
    std::vector<std::int32_t> rows;
    rows.reserve(destinations_.size());
    for (const TransportDestination& destination :
         destinations_) {
        if (enabled(destination.row)) {
            rows.push_back(destination.row);
        }
    }
    return rows;
}

void TransportCatalog::setEnabled(
    std::int32_t row,
    bool enabled_value) {
    if (row < 0 ||
        static_cast<std::size_t>(row) >=
            enabled_flags_.size()) {
        return;
    }
    enabled_flags_[static_cast<std::size_t>(row)] =
        enabled_value ? 1 : 0;
}

const std::vector<std::int32_t>&
TransportCatalog::enabledFlags() const {
    return enabled_flags_;
}

bool TransportCatalog::restoreEnabledFlags(
    const std::vector<std::int32_t>& flags) {
    if (flags.size() != enabled_flags_.size()) {
        return false;
    }
    enabled_flags_ = flags;
    return true;
}

}  // namespace osf
