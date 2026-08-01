#ifndef OPENSHADOWFLARE_WORLD_POINTER_HPP
#define OPENSHADOWFLARE_WORLD_POINTER_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace osf {

enum class WorldPointerTargetKind {
    none,
    scenario_object,
    npc,
    enemy,
    ground_item,
};

struct WorldPointerTarget {
    WorldPointerTargetKind kind =
        WorldPointerTargetKind::none;
    std::int32_t id = -1;
};

struct WorldPointerConfiguration {
    std::int32_t range = 2;
    bool range_enabled = true;
    std::array<std::int32_t, 5> click_priority{{
        4, 2, 3, 1, 0,
    }};
};

struct WorldPointerCandidate {
    WorldPointerTarget target;
    DisplayOrderEntry display;
    std::int32_t retail_type = -1;
    bool exact_hit = false;
    std::int64_t pointer_distance_squared = 0;
};

std::int32_t worldPointerHalfSize(
    const WorldPointerConfiguration& configuration);

class WorldPointer {
public:
    void configure(
        const WorldPointerConfiguration& configuration);
    void reset();
    void clearSelection();
    void update(
        std::int32_t screen_x,
        std::int32_t screen_y,
        std::vector<WorldPointerCandidate> candidates);

    std::int32_t screenX() const;
    std::int32_t screenY() const;
    bool active() const;
    const WorldPointerTarget& target() const;
    const WorldPointerConfiguration& configuration() const;

private:
    WorldPointerConfiguration configuration_;
    WorldPointerTarget target_;
    std::int32_t screen_x_ = 0;
    std::int32_t screen_y_ = 0;
    bool active_ = false;
};

}  // namespace osf

#endif
