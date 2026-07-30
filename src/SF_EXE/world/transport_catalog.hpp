#ifndef OPENSHADOWFLARE_TRANSPORT_CATALOG_HPP
#define OPENSHADOWFLARE_TRANSPORT_CATALOG_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class TableDatabase;

struct TransportDestination {
    std::int32_t row = -1;
    std::string name;
    std::int32_t scenario = -1;
    std::int32_t entry = -1;
};

class TransportCatalog {
public:
    bool load(
        const TableDatabase& tables,
        std::string* error = nullptr);
    void clear();

    const std::vector<TransportDestination>& destinations() const;
    const TransportDestination* find(std::int32_t row) const;
    bool enabled(std::int32_t row) const;
    std::vector<std::int32_t> enabledRows() const;
    void setEnabled(std::int32_t row, bool enabled);
    const std::vector<std::int32_t>& enabledFlags() const;
    bool restoreEnabledFlags(
        const std::vector<std::int32_t>& flags);

private:
    std::vector<TransportDestination> destinations_;
    std::vector<std::int32_t> enabled_flags_;
};

}  // namespace osf

#endif
