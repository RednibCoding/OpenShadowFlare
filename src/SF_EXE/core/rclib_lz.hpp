#ifndef OPENSHADOWFLARE_RCLIB_LZ_HPP
#define OPENSHADOWFLARE_RCLIB_LZ_HPP

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
