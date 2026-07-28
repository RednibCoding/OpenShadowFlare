#include "gameplay_renderer.hpp"

#include "gapi/caf.hpp"
#include "gapi/gapi.hpp"
#include "gapi/njp.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {
namespace {

constexpr std::int32_t kScreenWidth = 640;
constexpr std::int32_t kScreenHeight = 480;
constexpr std::int32_t kRetailBaseX = 15;
constexpr std::int32_t kRetailBaseY = 10;

std::int32_t toScreenX(
    std::int32_t world_x,
    std::int32_t world_y) {
    return static_cast<std::int32_t>(
        (static_cast<std::int64_t>(world_x - world_y) *
         kRetailBaseX) /
        100);
}

std::int32_t toScreenY(
    std::int32_t world_x,
    std::int32_t world_y) {
    return static_cast<std::int32_t>(
        (static_cast<std::int64_t>(world_x + world_y) *
         kRetailBaseY) /
        100);
}

void renderPlayer(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t animation_frame) {
    if (!world.hasPlayer() ||
        world.playerAnimation().charts().empty()) {
        return;
    }
    const gapi::CafChart& chart =
        world.playerAnimation().charts().front();
    const gapi::CafDirection& direction =
        chart.directions[0];
    if (direction.frame_count <= 0 ||
        direction.parts.empty()) {
        return;
    }
    if ((chart.status & 1) != 0) {
        animation_frame %= direction.frame_count;
    }
    if (animation_frame < 0 ||
        animation_frame >= direction.frame_count) {
        animation_frame = 0;
    }

    std::vector<const gapi::CafCell*> ordered(
        direction.parts.size(), nullptr);
    for (const std::vector<gapi::CafCell>& part :
         direction.parts) {
        if (static_cast<std::size_t>(animation_frame) >=
            part.size()) {
            continue;
        }
        const gapi::CafCell& cell =
            part[static_cast<std::size_t>(animation_frame)];
        if (cell.priority >= 0 &&
            static_cast<std::size_t>(cell.priority) <
                ordered.size()) {
            ordered[
                static_cast<std::size_t>(cell.priority)] = &cell;
        }
    }

    for (std::size_t priority = ordered.size();
         priority != 0;
         --priority) {
        const gapi::CafCell* cell = ordered[priority - 1];
        if (!cell || cell->pattern_index < 0) {
            continue;
        }
        renderer.drawPattern(
            world.playerPatterns(),
            static_cast<std::size_t>(cell->pattern_index),
            {kScreenWidth / 2,
             kScreenHeight / 2,
             1000,
             1000,
             std::clamp<std::int32_t>(
                 cell->transparency, 0, 1000)});
    }
}

}  // namespace

void renderInitialLoadingScreen(
    gapi::Backend& renderer,
    const gapi::NjpImage& waiting,
    std::int32_t counter,
    bool ready_to_continue) {
    renderer.drawPattern(
        waiting,
        0,
        {0, 0});
    if (!ready_to_continue) {
        renderer.drawPattern(
            waiting,
            3,
            {572, 443});
        return;
    }

    const std::int32_t arrow_offset =
        std::max(counter, 0) % 16;
    renderer.drawPattern(
        waiting,
        2,
        {592 + arrow_offset, 450});
}

void renderWorld(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t animation_frame) {
    const GroundMap& ground = world.ground();
    if (ground.width() <= 0 || ground.height() <= 0) {
        return;
    }

    const std::int32_t camera_x =
        toScreenX(
            world.playerWorldX(),
            world.playerWorldY()) -
        kScreenWidth / 2;
    const std::int32_t camera_y =
        toScreenY(
            world.playerWorldX(),
            world.playerWorldY()) -
        kScreenHeight / 2;
    const std::int32_t start_x =
        std::max(camera_x / ground.chipWidth(), 0);
    const std::int32_t start_y =
        std::max(camera_y / ground.chipHeight(), 0);
    const std::int32_t end_x = std::min(
        (camera_x + kScreenWidth) / ground.chipWidth(),
        ground.width() - 1);
    const std::int32_t end_y = std::min(
        (camera_y + kScreenHeight) / ground.chipHeight(),
        ground.height() - 1);

    const auto& patterns = world.groundPatterns();
    for (std::int32_t y = start_y; y <= end_y; ++y) {
        for (std::int32_t x = start_x; x <= end_x; ++x) {
            const GroundCell* cell = ground.cell(x, y);
            if (!cell || cell->pattern_set < 0 ||
                static_cast<std::size_t>(cell->pattern_set) >=
                    patterns.size() ||
                !patterns[
                    static_cast<std::size_t>(
                        cell->pattern_set)] ||
                cell->pattern < 0) {
                continue;
            }
            renderer.drawPattern(
                *patterns[
                    static_cast<std::size_t>(
                        cell->pattern_set)],
                static_cast<std::size_t>(cell->pattern),
                {ground.chipWidth() * x - camera_x,
                 ground.chipHeight() * y - camera_y});
        }
    }

    renderPlayer(renderer, world, animation_frame);
}

}  // namespace osf
