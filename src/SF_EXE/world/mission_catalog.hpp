#ifndef OPENSHADOWFLARE_MISSION_CATALOG_HPP
#define OPENSHADOWFLARE_MISSION_CATALOG_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class TableDatabase;

struct MissionDefinition {
    std::int32_t id = -1;
    std::string title;
    std::vector<std::string> description;
};

class MissionCatalog {
public:
    bool load(
        const TableDatabase& tables,
        std::string* error = nullptr);
    void clear();

    const MissionDefinition* find(std::int32_t mission_id) const;
    const std::vector<MissionDefinition>& missions() const;

private:
    std::vector<MissionDefinition> missions_;
};

}  // namespace osf

#endif
