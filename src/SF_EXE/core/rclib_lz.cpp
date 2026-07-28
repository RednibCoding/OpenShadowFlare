#include "rclib_lz.hpp"

#include <array>
#include <cstring>

namespace osf {

bool decodeRclibLz(
    const std::uint8_t* bytes,
    std::size_t size,
    std::size_t expected_size,
    std::vector<std::uint8_t>& output) {
    if (!bytes || size < 16 ||
        std::memcmp(bytes, "RCLIB-L", 7) != 0) {
        return false;
    }

    const std::uint32_t output_size =
        static_cast<std::uint32_t>(bytes[8]) |
        (static_cast<std::uint32_t>(bytes[9]) << 8u) |
        (static_cast<std::uint32_t>(bytes[10]) << 16u) |
        (static_cast<std::uint32_t>(bytes[11]) << 24u);
    if (output_size != expected_size) {
        return false;
    }
    output.assign(output_size, 0);

    std::array<std::uint8_t, 4096> window{};
    std::size_t source = 16;
    std::size_t destination = 0;
    std::size_t window_position = 0xfee;
    while (source < size && destination < output.size()) {
        const std::uint8_t flags = bytes[source++];
        for (std::uint8_t mask = 0x80;
             mask != 0 && destination < output.size();
             mask >>= 1u) {
            if ((flags & mask) != 0) {
                if (source + 2 > size) {
                    return false;
                }
                const std::uint8_t first = bytes[source++];
                const std::uint8_t second = bytes[source++];
                const std::size_t offset =
                    first |
                    (static_cast<std::size_t>(
                         second & 0xf0u)
                     << 4u);
                const std::size_t length =
                    static_cast<std::size_t>(
                        second & 0x0fu) +
                    3u;
                for (std::size_t index = 0;
                     index < length &&
                     destination < output.size();
                     ++index) {
                    const std::uint8_t value =
                        window[(offset + index) & 0xfffu];
                    output[destination++] = value;
                    window[window_position] = value;
                    window_position =
                        (window_position + 1u) & 0xfffu;
                }
            } else {
                if (source >= size) {
                    return false;
                }
                const std::uint8_t value = bytes[source++];
                output[destination++] = value;
                window[window_position] = value;
                window_position =
                    (window_position + 1u) & 0xfffu;
            }
        }
    }
    return destination == output.size();
}

}  // namespace osf
