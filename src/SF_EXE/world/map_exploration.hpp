#ifndef OPENSHADOWFLARE_MAP_EXPLORATION_HPP
#define OPENSHADOWFLARE_MAP_EXPLORATION_HPP

#include "libs/RKC_DIB/rkc_dib.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>

namespace osf {

class MapExploration {
public:
    bool initialize(const GroundMap& ground);
    void clear();
    void reveal(WorldPosition position);

    const gapi::BitmapImage& mask() const;
    bool explored(
        std::int32_t map_x,
        std::int32_t map_y) const;

private:
    gapi::BitmapImage mask_;
};

}  // namespace osf

#endif
