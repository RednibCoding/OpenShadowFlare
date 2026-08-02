#include "runtime/platform/memory_usage.hpp"

namespace osf::runtime {

std::optional<std::uint64_t> residentMemoryUsageBytes() {
    return std::nullopt;
}

}  // namespace osf::runtime
