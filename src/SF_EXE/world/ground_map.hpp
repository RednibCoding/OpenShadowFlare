#ifndef OPENSHADOWFLARE_GROUND_MAP_HPP
#define OPENSHADOWFLARE_GROUND_MAP_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace osf {

struct GroundCell {
    std::int16_t status = 0;
    std::int16_t pattern_set = -1;
    std::int16_t pattern = -1;
};

class GroundMap {
public:
    bool decode(
        const std::vector<std::uint8_t>& bytes,
        std::string* error = nullptr);
    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);
    void clear();

    std::int32_t width() const;
    std::int32_t height() const;
    std::int32_t chipWidth() const;
    std::int32_t chipHeight() const;
    std::int32_t baseMagnificationX() const;
    std::int32_t baseMagnificationY() const;
    const GroundCell* cell(
        std::int32_t x,
        std::int32_t y) const;

private:
    std::int32_t width_ = 0;
    std::int32_t height_ = 0;
    std::int32_t chip_width_ = 0;
    std::int32_t chip_height_ = 0;
    std::int32_t base_magnification_x_ = 0;
    std::int32_t base_magnification_y_ = 0;
    std::vector<GroundCell> cells_;
};

}  // namespace osf

#endif
