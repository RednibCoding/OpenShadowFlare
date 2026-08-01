#ifndef OPENSHADOWFLARE_RETAIL_INTEGER_HPP
#define OPENSHADOWFLARE_RETAIL_INTEGER_HPP

#include <cstdint>
#include <limits>

namespace osf {

inline std::int32_t retailSignedWord(
    std::uint32_t value) {
    constexpr std::uint32_t kSignedMaximum =
        static_cast<std::uint32_t>(
            std::numeric_limits<std::int32_t>::max());
    if (value <= kSignedMaximum) {
        return static_cast<std::int32_t>(value);
    }
    return -1 -
           static_cast<std::int32_t>(
               std::numeric_limits<std::uint32_t>::max() -
               value);
}

inline std::int32_t retailMultiply(
    std::int32_t left,
    std::int32_t right) {
    return retailSignedWord(
        static_cast<std::uint32_t>(left) *
        static_cast<std::uint32_t>(right));
}

inline std::int32_t retailAdd(
    std::int32_t left,
    std::int32_t right) {
    return retailSignedWord(
        static_cast<std::uint32_t>(left) +
        static_cast<std::uint32_t>(right));
}

inline std::int32_t retailSubtract(
    std::int32_t left,
    std::int32_t right) {
    return retailSignedWord(
        static_cast<std::uint32_t>(left) -
        static_cast<std::uint32_t>(right));
}

}  // namespace osf

#endif
