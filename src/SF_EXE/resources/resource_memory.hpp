#ifndef OPENSHADOWFLARE_RESOURCES_RESOURCE_MEMORY_HPP
#define OPENSHADOWFLARE_RESOURCES_RESOURCE_MEMORY_HPP

#include <cstdint>

namespace osf::gapi {

class BitmapImage;
class CafAnimation;
class NjpImage;

}  // namespace osf::gapi

namespace osf {

std::uint64_t decodedMemoryUsageBytes(
    const gapi::BitmapImage& image);
std::uint64_t decodedMemoryUsageBytes(
    const gapi::CafAnimation& animation);
std::uint64_t decodedMemoryUsageBytes(
    const gapi::NjpImage& image);

}  // namespace osf

#endif
