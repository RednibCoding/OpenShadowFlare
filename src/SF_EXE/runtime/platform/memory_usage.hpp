#ifndef OPENSHADOWFLARE_RUNTIME_PLATFORM_MEMORY_USAGE_HPP
#define OPENSHADOWFLARE_RUNTIME_PLATFORM_MEMORY_USAGE_HPP

#include <cstdint>
#include <optional>

namespace osf::runtime {

std::optional<std::uint64_t> residentMemoryUsageBytes();

}  // namespace osf::runtime

#endif
