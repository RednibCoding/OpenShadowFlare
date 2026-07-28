#include "title_renderer.hpp"

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {
namespace {

struct Point {
    std::int32_t x;
    std::int32_t y;
};

constexpr std::array<Point, 10> kSmokePositions{{
    {222, 282},
    {535, 208},
    {122, 472},
    {562, 60},
    {62, 54},
    {547, 350},
    {53, 377},
    {566, 294},
    {113, 473},
    {420, 463},
}};

void renderSmokeFrame(
    gapi::Backend& renderer,
    const TitleSmokeAsset& asset,
    std::int32_t frame,
    Point position,
    std::int32_t brightness) {
    if (!asset.patterns || !asset.animation ||
        asset.animation->charts().empty() || frame < 0) {
        return;
    }

    const gapi::CafChart& chart =
        asset.animation->charts().front();
    const gapi::CafDirection& direction =
        chart.directions[8];
    if (direction.frame_count <= 0 ||
        direction.parts.empty()) {
        return;
    }
    if ((chart.status & 1) != 0) {
        frame %= direction.frame_count;
    }
    if (frame < 0 || frame >= direction.frame_count) {
        return;
    }

    std::vector<const gapi::CafCell*> ordered(
        direction.parts.size(), nullptr);
    for (const std::vector<gapi::CafCell>& part :
         direction.parts) {
        if (static_cast<std::size_t>(frame) >= part.size()) {
            continue;
        }
        const gapi::CafCell& cell =
            part[static_cast<std::size_t>(frame)];
        if (cell.priority >= 0 &&
            static_cast<std::size_t>(cell.priority) <
                ordered.size()) {
            ordered[static_cast<std::size_t>(cell.priority)] =
                &cell;
        }
    }

    for (std::size_t index = ordered.size();
         index != 0;
         --index) {
        const gapi::CafCell* cell = ordered[index - 1];
        if (!cell || cell->pattern_index < 0) {
            continue;
        }
        const std::int32_t cellBrightness =
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(brightness) *
                std::clamp<std::int32_t>(
                    cell->transparency, 0, 1000) /
                1000);
        renderer.drawPattern(
            *asset.patterns,
            static_cast<std::size_t>(cell->pattern_index),
            {position.x, position.y, 1000, 1000,
             cellBrightness});
    }
}

}  // namespace

void renderTitle(
    gapi::Backend& renderer,
    const gapi::NjpImage& title,
    const std::array<TitleSmokeAsset, 10>& smoke,
    const TitleFrameResult& frame) {
    renderer.drawPattern(
        title,
        0,
        {0, 0, 1000, 1000, frame.scene_brightness});
    for (std::size_t item = 0;
         item < frame.menu_visible.size();
         ++item) {
        if (!frame.menu_visible[item]) {
            continue;
        }
        renderer.drawPattern(
            title,
            item + 1,
            {0, 0, 1000, 1000,
             frame.menu_brightness[item]});
    }

    for (std::size_t index = 0; index < smoke.size(); ++index) {
        renderSmokeFrame(
            renderer,
            smoke[index],
            frame.smoke_frames[index],
            kSmokePositions[index],
            frame.scene_brightness);
    }
}

}  // namespace osf
