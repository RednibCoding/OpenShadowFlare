#ifndef OPENSHADOWFLARE_COMPANION_STATUS_MESSAGE_HPP
#define OPENSHADOWFLARE_COMPANION_STATUS_MESSAGE_HPP

#include <cstdint>
#include <string>

namespace osf {

class PlayerData;
class TableDatabase;

bool buildRetailCompanionStatusMessage(
    const TableDatabase& tables,
    const PlayerData& player,
    std::int32_t companion_type,
    std::string& message,
    std::string* error = nullptr);

}  // namespace osf

#endif
