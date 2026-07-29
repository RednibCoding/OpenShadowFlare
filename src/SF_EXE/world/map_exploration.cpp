#include "map_exploration.hpp"

#include <cstdint>
#include <limits>

namespace osf {
namespace {

constexpr std::int32_t kRevealWidth = 68;
constexpr std::int32_t kRevealHeight = 46;

}  // namespace

bool MapExploration::initialize(const GroundMap& ground) {
    clear();
    const std::int64_t width =
        static_cast<std::int64_t>(ground.width()) *
        ground.chipWidth() / 10;
    const std::int64_t height =
        static_cast<std::int64_t>(ground.height()) *
        ground.chipHeight() / 10;
    if (width <= 0 || height <= 0 ||
        width > std::numeric_limits<std::int32_t>::max() ||
        height > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }
    return mask_.create(
        static_cast<std::int32_t>(width),
        static_cast<std::int32_t>(height),
        {0, 0, 0, 255});
}

void MapExploration::clear() {
    mask_.clear();
}

void MapExploration::reveal(WorldPosition position) {
    const ScreenPosition real = calculateRealPosition(position);
    mask_.fillRectangle(
        real.x / 10 - kRevealWidth / 2,
        real.y / 10 - kRevealHeight / 2,
        kRevealWidth,
        kRevealHeight,
        {255, 255, 255, 0});
}

const gapi::BitmapImage& MapExploration::mask() const {
    return mask_;
}

bool MapExploration::explored(
    std::int32_t map_x,
    std::int32_t map_y) const {
    if (map_x < 0 || map_y < 0 ||
        map_x >= mask_.width() ||
        map_y >= mask_.height()) {
        return false;
    }
    return mask_.pixels()[
               static_cast<std::size_t>(map_y) *
                   static_cast<std::size_t>(mask_.width()) +
               static_cast<std::size_t>(map_x)]
               .alpha == 0;
}

}  // namespace osf
