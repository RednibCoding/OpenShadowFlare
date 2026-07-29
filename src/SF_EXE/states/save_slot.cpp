#include "save_slot.hpp"

#include <iomanip>
#include <sstream>

namespace osf {

RetailSavePath findNextRetailSavePath(
    const std::function<bool(std::string_view)>& file_exists) {
    RetailSavePath result;
    for (int index = 0; index < 6; ++index) {
        std::ostringstream path;
        path << "Save\\" << std::setfill('0') << std::setw(4)
             << index << ".Ssv";
        result.path = path.str();
        if (!file_exists || !file_exists(result.path)) {
            result.available = true;
            return result;
        }
    }
    return result;
}

}  // namespace osf
