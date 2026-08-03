#include "resource_memory.hpp"

#include "libs/RKC_DIB/rkc_dib.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include <cstdint>

namespace osf {

std::uint64_t decodedMemoryUsageBytes(
    const gapi::BitmapImage& image) {
    return static_cast<std::uint64_t>(image.pixels().capacity()) *
        sizeof(gapi::Color);
}

std::uint64_t decodedMemoryUsageBytes(
    const gapi::CafAnimation& animation) {
    std::uint64_t bytes =
        static_cast<std::uint64_t>(animation.charts().capacity()) *
        sizeof(gapi::CafChart);
    for (const gapi::CafChart& chart : animation.charts()) {
        for (const gapi::CafDirection& direction : chart.directions) {
            bytes += static_cast<std::uint64_t>(
                         direction.parts.capacity()) *
                sizeof(std::vector<gapi::CafCell>);
            for (const std::vector<gapi::CafCell>& part :
                 direction.parts) {
                bytes += static_cast<std::uint64_t>(part.capacity()) *
                    sizeof(gapi::CafCell);
            }
        }
    }
    return bytes;
}

std::uint64_t decodedMemoryUsageBytes(
    const gapi::NjpImage& image) {
    std::uint64_t bytes =
        static_cast<std::uint64_t>(image.parts().capacity()) *
            sizeof(gapi::NjpPart) +
        static_cast<std::uint64_t>(image.patterns().capacity()) *
            sizeof(gapi::NjpPattern) +
        static_cast<std::uint64_t>(image.palettes().capacity()) *
            sizeof(gapi::NjpPalette) +
        static_cast<std::uint64_t>(
            image.decodedPatternFlags().capacity());
    for (const gapi::NjpPart& part : image.parts()) {
        bytes += part.pixels.capacity();
    }
    for (const gapi::NjpPattern& pattern : image.patterns()) {
        bytes += static_cast<std::uint64_t>(pattern.parts.capacity()) *
            sizeof(gapi::NjpPatternPart);
    }
    return bytes;
}

}  // namespace osf
