#ifndef OPENSHADOWFLARE_SAVE_SLOT_HPP
#define OPENSHADOWFLARE_SAVE_SLOT_HPP

#include <functional>
#include <string>
#include <string_view>

namespace osf {

struct RetailSavePath {
    std::string path;
    bool available = false;
};

RetailSavePath findNextRetailSavePath(
    const std::function<bool(std::string_view)>& file_exists);

}  // namespace osf

#endif
