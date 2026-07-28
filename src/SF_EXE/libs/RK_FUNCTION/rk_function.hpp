#ifndef OPENSHADOWFLARE_LIBS_RK_FUNCTION_HPP
#define OPENSHADOWFLARE_LIBS_RK_FUNCTION_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {

bool decodeRclibLz(
    const std::uint8_t* bytes,
    std::size_t size,
    std::size_t expected_size,
    std::vector<std::uint8_t>& output);

}  // namespace osf

#endif
