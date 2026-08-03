#ifndef OPENSHADOWFLARE_GAPI_BIT_MASK_IMAGE_HPP
#define OPENSHADOWFLARE_GAPI_BIT_MASK_IMAGE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf::gapi {

class BitMaskImage {
public:
    bool create(
        std::int32_t width,
        std::int32_t height,
        bool fill = false);
    void clear();
    void fillRectangle(
        std::int32_t x,
        std::int32_t y,
        std::int32_t width,
        std::int32_t height,
        bool value);

    bool value(std::int32_t x, std::int32_t y) const;
    std::int32_t width() const;
    std::int32_t height() const;
    std::size_t strideBytes() const;
    const std::vector<std::uint8_t>& bytes() const;
    std::uint64_t memoryUsageBytes() const;

private:
    std::int32_t width_ = 0;
    std::int32_t height_ = 0;
    std::size_t stride_bytes_ = 0;
    std::vector<std::uint8_t> bytes_;
};

}  // namespace osf::gapi

#endif
